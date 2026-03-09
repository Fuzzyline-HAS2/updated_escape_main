"""
escape_main 소프트웨어 로직의 Python 시뮬레이션.
Wifi.ino(DataChanged, SettingFunc, ActivateFunc, ReadyFunc)와
Game_system.ino(TagCount)를 미러링합니다.
error_recovery.ino(RuntimeSnapshot, transition lifecycle, recovery) 포함.
하드웨어 없이 순수 소프트웨어 단위 테스트에 사용합니다.
"""


class EscapeMainSM:
    def __init__(self, device_name: str = "escape_main"):
        self.device_name = device_name
        self.game_state = ""
        self.device_state = ""
        self._cur_game = ""       # DataChanged()의 cur["game_state"]
        self._cur_device = ""     # DataChanged()의 cur["device_state"]
        self.tag_state = [False, False, False]
        self.tag_cnt = 0
        self.relay_high = True    # True=HIGH(OFF), False=LOW(ON)
        self._mode = "wait"       # "wait"=WaitFunc, "tagcount"=TagCount
        self._timer_enabled = False
        self._sent: list = []     # has2wifi.Send() 호출 기록
        self._events: list = []   # Serial.println() 등 동작 기록

        # ── Motor timeout simulation (stepper_Motor.ino watchdog 대응) ──────
        self._motor_close_timeout = False  # EscapeClose() 10초 초과 시 True
        self._motor_open_timeout = False   # EscapeOpen() 10초 초과 시 True

        # ── Beetle 통신 오류 카운터 (error_recovery.ino 대응) ───────────────
        self.invalid_cmd_count = 0          # 허용되지 않은 명령 수신 횟수
        self.packet_format_error_count = 0  # T 패킷 포맷 오류 누적
        self.tag_parse_error_count = 0      # 태그 파싱 실패 누적
        self.beetle_bad_event_streak = 0    # 연속 bad-event 사이클 수
        self.beetle_recover_attempts = 0    # UART 복구 시도 횟수

        # HandleRuntimeRecovery delta 추적 (static 변수 대응)
        self._prev_cmd = 0
        self._prev_fmt = 0
        self._prev_tag = 0

        # ── 하드웨어 fault 래치 ─────────────────────────────────────────────
        self.system_fault_latched = False
        self.system_fault_reason = ""

        # ── RuntimeSnapshot (last stable state) ────────────────────────────
        self.stable_valid = False
        self.stable_game_state = ""
        self.stable_device_state = ""
        self.stable_relay_high = True
        self.stable_timer_enabled = False
        self.stable_mode = "wait"
        self.stable_door_target = "unknown"  # "unknown" / "open" / "closed"

        # ── 전이 생명주기 ───────────────────────────────────────────────────
        self.transition_in_progress = False
        self.pending_game_state = ""
        self.pending_device_state = ""
        self.last_recovery_reason = ""

    # ── Public API ──────────────────────────────────────────────────────────

    def data_changed(self, game_state: str = None, device_state: str = None):
        """DataChanged() 시뮬레이션."""
        if game_state is not None:
            self.game_state = game_state
        if device_state is not None:
            self.device_state = device_state

        if self.game_state != self._cur_game:
            if self.game_state == "setting":
                self._setting_func()
            elif self.game_state == "ready":
                self._ready_func()
            elif self.game_state == "activate":
                self._activate_func()

        if self.device_state != self._cur_device:
            if self.device_state == "player_win":
                self._ev("AllNeoOn(BLUE)")
                self._ev("EscapeClose")

        self._cur_game = self.game_state
        self._cur_device = self.device_state

    def set_tags(self, t1: bool, t2: bool, t3: bool):
        """Beetle T패킷 수신 시뮬레이션."""
        self.tag_state = [t1, t2, t3]

    def tick_game_timer(self):
        """GameTimer tick 시뮬레이션 — ptrCurrentMode() 호출."""
        if self._mode == "tagcount":
            self._tag_count()

    def receive_bad_event(self, kind: str):
        """
        Beetle bad event 수신 시뮬레이션.
        kind: 'invalid_cmd' | 'format_error' | 'tag_parse'
        silence는 bad event가 아니므로 여기에 없음.
        """
        if kind == "invalid_cmd":
            self.invalid_cmd_count += 1
            self._ev("[UART] WARN unknown command")
        elif kind == "format_error":
            self.packet_format_error_count += 1
            self._ev("[UART] WARN malformed T packet")
        elif kind == "tag_parse":
            self.tag_parse_error_count += 1
            self._ev("[UART] WARN tag role unresolved")

    def handle_runtime_recovery(self):
        """
        HandleRuntimeRecovery() 시뮬레이션.
        timer.ino의 WifiIntervalFunc / GameTimerFunc에서 매 tick 호출됨.
        """
        # 1) 모터 timeout → 즉시 safe stop (stable restore 금지)
        if self._motor_close_timeout or self._motor_open_timeout:
            reason = ("EscapeClose() timeout" if self._motor_close_timeout
                      else "EscapeOpen() timeout")
            self._motor_close_timeout = False
            self._motor_open_timeout = False
            self._latch_system_fault(reason)
            return

        # 2) bad event delta 감지 (silence는 무시)
        if self.invalid_cmd_count < self._prev_cmd:
            self._prev_cmd = self.invalid_cmd_count
        if self.packet_format_error_count < self._prev_fmt:
            self._prev_fmt = self.packet_format_error_count
        if self.tag_parse_error_count < self._prev_tag:
            self._prev_tag = self.tag_parse_error_count

        new_bad = (self.invalid_cmd_count > self._prev_cmd or
                   self.packet_format_error_count > self._prev_fmt or
                   self.tag_parse_error_count > self._prev_tag)

        self._prev_cmd = self.invalid_cmd_count
        self._prev_fmt = self.packet_format_error_count
        self._prev_tag = self.tag_parse_error_count

        if new_bad:
            self.beetle_bad_event_streak += 1
            self._ev(f"[RECOVERY] bad event streak={self.beetle_bad_event_streak}")

        if self.beetle_bad_event_streak >= 3:
            self._recover_beetle_connection()
            self.recover_to_last_stable_state("beetle uart recovery")
            self._prev_cmd = 0
            self._prev_fmt = 0
            self._prev_tag = 0
            if self.beetle_recover_attempts >= 3:
                self._ev("[RECOVERY] Max attempts, restart")

    def recover_to_last_stable_state(self, reason: str) -> bool:
        """
        RecoverToLastStableState() 시뮬레이션.
        통신/논리 오류 복구 후 안정 상태로 복원.
        systemFaultLatched(기구 고장) 시 False 반환.
        """
        if self.system_fault_latched:
            self._ev(f"[RECOVERY] Blocked: {self.system_fault_reason}")
            return False
        if not self.stable_valid:
            self._ev("[RECOVERY] No stable state")
            return False
        self.last_recovery_reason = reason
        self._reconcile_outputs()
        # device_state=escape는 로컬 소유 → 재전송
        if self.stable_device_state == "escape":
            self._ev("SendDeviceStateWithRetry(escape)")
            self._sent.append({
                "key": self.device_name,
                "column": "device_state",
                "value": "escape",
            })
        self._ev(f"[RECOVERY] Restored to: {self.stable_game_state}")
        return True

    def capture_stable_state(self, game_state: str, device_state: str, door_target: str):
        """
        CaptureStableState() 시뮬레이션.
        전이가 완전히 성공한 직후에만 호출할 것.
        """
        self.stable_game_state = game_state
        self.stable_device_state = device_state
        self.stable_relay_high = self.relay_high
        self.stable_timer_enabled = self._timer_enabled
        self.stable_mode = self._mode
        self.stable_door_target = door_target
        self.stable_valid = True
        self._ev(
            f"[STABLE] Captured: game={game_state} device={device_state} door={door_target}"
        )

    def reset_beetle_error_counters(self):
        """ResetBeetleErrorCounters() 시뮬레이션."""
        self.invalid_cmd_count = 0
        self.packet_format_error_count = 0
        self.tag_parse_error_count = 0
        self.beetle_bad_event_streak = 0

    def mark_transition_start(self, game_state: str, device_state: str):
        self.transition_in_progress = True
        self.pending_game_state = game_state
        self.pending_device_state = device_state
        self._ev(f"[TRANSITION] Start: game={game_state} device={device_state}")

    def mark_transition_success(self):
        self.transition_in_progress = False
        self.pending_game_state = ""
        self.pending_device_state = ""
        self._ev("[TRANSITION] Success.")

    def mark_transition_failed(self):
        self.transition_in_progress = False
        self.pending_game_state = ""
        self.pending_device_state = ""
        self._ev("[TRANSITION] Failed — stable state unchanged.")

    # ── Private (Arduino 함수 대응) ─────────────────────────────────────────

    def _setting_func(self):
        """Wifi.ino SettingFunc() — transition lifecycle 포함"""
        self.mark_transition_start("setting", self.device_state)
        self._ev("SETTING")
        self.relay_high = True
        self._ev("AllNeoOn(WHITE)")
        self._ev("EscapeClose")
        if self._motor_close_timeout:
            self.mark_transition_failed()
            return
        self._mode = "wait"
        self._timer_enabled = False
        self.capture_stable_state("setting", self.device_state, "closed")
        self.mark_transition_success()

    def _ready_func(self):
        """Wifi.ino ReadyFunc() — transition lifecycle 포함"""
        self.mark_transition_start("ready", self.device_state)
        self._ev("READY")
        self.relay_high = True
        self._ev("AllNeoOn(RED)")
        self._ev("EscapeClose")
        if self._motor_close_timeout:
            self.mark_transition_failed()
            return
        self._mode = "wait"
        self.capture_stable_state("ready", self.device_state, "closed")
        self.mark_transition_success()

    def _activate_func(self):
        """Wifi.ino ActivateFunc() — fault guard + transition lifecycle 포함"""
        if self.system_fault_latched:
            self._ev(f"[SAFE] ActivateFunc blocked: {self.system_fault_reason}")
            return
        self.mark_transition_start("activate", self.device_state)
        self._ev("ACTIVATE")
        self._ev("MP3 VE1")
        self._ev("AllNeoOn(YELLOW)")
        self._ev("EscapeOpen")
        if self._motor_open_timeout:
            self.mark_transition_failed()
            return
        self.relay_high = False
        self._mode = "tagcount"
        self._timer_enabled = True
        self.capture_stable_state("activate", self.device_state, "open")
        self.mark_transition_success()

    def _tag_count(self):
        """Game_system.ino TagCount() — escape 후 stable 갱신 포함"""
        self.tag_cnt = sum(1 for t in self.tag_state if t)

        if self.tag_cnt == 1:
            self._ev("Tag 1 detected")
            self._ev("MP3 VE2")
        elif self.tag_cnt == 2:
            self._ev("Tag 2 detected")
            self._ev("MP3 VE3")
        elif self.tag_cnt >= 3:
            self._ev("Escape Activate")
            self._ev("MP3 VE4")
            self._sent.append({
                "key": self.device_name,
                "column": "device_state",
                "value": "escape",
            })
            self._ev("Send device_state=escape")
            self._ev("EscapeClose")
            self._mode = "wait"
            self._timer_enabled = False
            # game_state는 서버 기원이므로 "activate" 유지, device_state=escape 캡처
            if not self._motor_close_timeout:
                self.capture_stable_state("activate", "escape", "closed")
            self._ev("MP3 VE5")

        self.tag_state = [False, False, False]

    def _reconcile_outputs(self):
        """
        ReconcileOutputsToStableState() 시뮬레이션.
        stable 기준으로 relay/timer/mode/LED만 idempotent하게 재정렬.
        MP3 재생 없음. 문 재이동 없음.
        """
        gs = self.stable_game_state
        ds = self.stable_device_state
        self._ev(f"[RECOVERY] Reconciling outputs to: {gs}")
        if gs == "setting":
            self.relay_high = True
            self._timer_enabled = False
            self._mode = "wait"
            self._ev("AllNeoOn(WHITE)")
        elif gs == "ready":
            self.relay_high = True
            self._timer_enabled = False
            self._mode = "wait"
            self._ev("AllNeoOn(RED)")
        elif gs == "activate":
            if ds == "escape":
                # escape 완료 = 문 닫힘, 게임 종료 대기
                self.relay_high = True
                self._timer_enabled = False
                self._mode = "wait"
                self._ev("AllNeoOn(YELLOW)")
            else:
                # 일반 activate = 문 열림, 태그 감지 중
                self.relay_high = False
                self._timer_enabled = True
                self._mode = "tagcount"
                self._ev("AllNeoOn(YELLOW)")

    def _latch_system_fault(self, reason: str):
        """LatchSystemFault() 시뮬레이션."""
        if self.system_fault_latched:
            return
        self.system_fault_latched = True
        self.system_fault_reason = reason
        self.relay_high = True
        self._timer_enabled = False
        self._mode = "wait"
        self._ev(f"[SAFE] FAULT LATCHED: {reason}")

    def _recover_beetle_connection(self):
        """RecoverBeetleConnection() 시뮬레이션."""
        self.beetle_recover_attempts += 1
        self._ev(f"[RECOVERY] UART reset #{self.beetle_recover_attempts}")
        self.reset_beetle_error_counters()

    # ── Assertion helpers ────────────────────────────────────────────────────

    def _ev(self, msg: str):
        self._events.append(msg)

    def get_events(self) -> list:
        return list(self._events)

    def clear_events(self):
        self._events.clear()

    def get_sent(self) -> list:
        return list(self._sent)

    def clear_sent(self):
        self._sent.clear()
