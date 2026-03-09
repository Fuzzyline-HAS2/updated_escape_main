"""
시나리오 5: 런타임 복구 및 안정 상태 관리 테스트
error_recovery.ino의 핵심 복구 로직을 Python 상태머신으로 검증합니다.

테스트 클래스:
  TestBadEventRecovery          - bad event 3회 → UART recovery 실행
  TestStableSnapshotCapture     - 정상 전이 성공 후 stable snapshot 갱신
  TestTransitionFailurePreservesStable - 전이 실패 시 stable snapshot 유지
  TestFaultLatchedBlocksActivate - fault latched 시 activate 차단
  TestRecoveryRestoresOutputs   - recovery 후 stable state 기준 출력 복원
  TestMotorFaultSafeStop        - motor fault → safe stop, stable restore 미실행
"""

import pytest
from state_machine import EscapeMainSM


# ─────────────────────────────────────────────────────────────────────────────
# [1] bad event 3회 누적 → UART recovery 실행
#     Beetle silence가 아닌 이벤트 기반 복구의 핵심 검증.
# ─────────────────────────────────────────────────────────────────────────────
class TestBadEventRecovery:
    def test_uart_recovery_triggered_after_3_bad_event_cycles(self):
        """bad event가 3 사이클에 걸쳐 누적되면 UART 재초기화가 실행된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")

        sm.receive_bad_event("invalid_cmd")
        sm.handle_runtime_recovery()   # streak=1

        sm.receive_bad_event("format_error")
        sm.handle_runtime_recovery()   # streak=2

        sm.receive_bad_event("tag_parse")
        sm.handle_runtime_recovery()   # streak=3 → UART recovery

        assert sm.beetle_recover_attempts >= 1

    def test_uart_recovery_event_logged(self):
        """UART 재초기화 시 이벤트 로그가 남는다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")

        for kind in ("invalid_cmd", "format_error", "tag_parse"):
            sm.receive_bad_event(kind)
            sm.handle_runtime_recovery()

        assert any("[RECOVERY] UART reset" in e for e in sm.get_events())

    def test_uart_recovery_resets_error_counters(self):
        """UART 재초기화 후 오류 카운터가 초기화된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")

        for kind in ("invalid_cmd", "format_error", "tag_parse"):
            sm.receive_bad_event(kind)
            sm.handle_runtime_recovery()

        assert sm.invalid_cmd_count == 0
        assert sm.packet_format_error_count == 0
        assert sm.tag_parse_error_count == 0
        assert sm.beetle_bad_event_streak == 0

    def test_silence_does_not_trigger_recovery(self):
        """Beetle silence만으로는 UART recovery가 실행되지 않는다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")

        for _ in range(10):
            sm.handle_runtime_recovery()  # 아무 bad event 없이 반복

        assert sm.beetle_recover_attempts == 0

    def test_single_bad_event_does_not_trigger_recovery(self):
        """bad event가 2회 이하이면 UART recovery가 실행되지 않는다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")

        sm.receive_bad_event("invalid_cmd")
        sm.handle_runtime_recovery()   # streak=1
        sm.receive_bad_event("format_error")
        sm.handle_runtime_recovery()   # streak=2

        assert sm.beetle_recover_attempts == 0


# ─────────────────────────────────────────────────────────────────────────────
# [2] 정상 전이 성공 후 stable snapshot 갱신
#     복구 설계의 중심: 올바른 상태가 캡처되는지 검증.
# ─────────────────────────────────────────────────────────────────────────────
class TestStableSnapshotCapture:
    def test_stable_valid_after_setting(self):
        """setting 전이 성공 후 stable_valid가 True가 된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        assert sm.stable_valid is True

    def test_stable_captures_setting_state(self):
        """setting 성공 후 stable 스냅샷이 올바르게 저장된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        assert sm.stable_game_state == "setting"
        assert sm.stable_door_target == "closed"
        assert sm.stable_relay_high is True
        assert sm.stable_timer_enabled is False
        assert sm.stable_mode == "wait"

    def test_stable_captures_activate_state(self):
        """activate 성공 후 stable 스냅샷이 올바르게 저장된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        sm.data_changed(game_state="activate")
        assert sm.stable_game_state == "activate"
        assert sm.stable_door_target == "open"
        assert sm.stable_relay_high is False
        assert sm.stable_timer_enabled is True
        assert sm.stable_mode == "tagcount"

    def test_stable_captures_ready_state(self):
        """ready 성공 후 stable 스냅샷이 올바르게 저장된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        sm.data_changed(game_state="activate")
        sm.data_changed(game_state="ready")
        assert sm.stable_game_state == "ready"
        assert sm.stable_door_target == "closed"
        assert sm.stable_relay_high is True
        assert sm.stable_mode == "wait"

    def test_stable_updated_on_each_successful_transition(self):
        """전이할 때마다 stable이 갱신된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        assert sm.stable_game_state == "setting"
        sm.data_changed(game_state="ready")
        assert sm.stable_game_state == "ready"
        sm.data_changed(game_state="activate")
        assert sm.stable_game_state == "activate"

    def test_stable_captures_escape_state_after_3_tags(self):
        """3태그 escape 성공 후 stable이 activate+escape+door_closed로 갱신된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm.set_tags(True, True, True)
        sm.tick_game_timer()
        assert sm.stable_game_state == "activate"
        assert sm.stable_device_state == "escape"
        assert sm.stable_door_target == "closed"


# ─────────────────────────────────────────────────────────────────────────────
# [3] 전이 실패 시 stable snapshot 유지
#     이게 없으면 잘못된 상태를 stable로 저장하는 치명적 버그 발생.
# ─────────────────────────────────────────────────────────────────────────────
class TestTransitionFailurePreservesStable:
    def test_motor_timeout_on_ready_does_not_update_stable(self):
        """ready 전환 중 모터 timeout 시 stable이 이전 setting 상태를 유지한다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        assert sm.stable_game_state == "setting"

        sm._motor_close_timeout = True
        sm.data_changed(game_state="ready")

        assert sm.stable_game_state == "setting"  # 이전 stable 유지

    def test_motor_timeout_on_activate_does_not_update_stable(self):
        """activate 전환 중 모터 timeout 시 stable이 이전 setting 상태를 유지한다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        assert sm.stable_game_state == "setting"

        sm._motor_open_timeout = True
        sm.data_changed(game_state="activate")

        assert sm.stable_game_state == "setting"

    def test_transition_failed_event_recorded(self):
        """전이 실패 시 '[TRANSITION] Failed' 이벤트가 기록된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        sm._motor_close_timeout = True
        sm.clear_events()
        sm.data_changed(game_state="ready")
        assert any("Failed" in e for e in sm.get_events())

    def test_stable_not_set_when_never_transitioned_successfully(self):
        """한 번도 성공적인 전이 없으면 stable_valid가 False를 유지한다."""
        sm = EscapeMainSM()
        sm._motor_close_timeout = True
        sm.data_changed(game_state="setting")
        assert sm.stable_valid is False

    def test_stable_preserves_previous_after_two_consecutive_failures(self):
        """연속 두 번 실패해도 이전 성공 stable이 유지된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")       # stable = setting

        sm._motor_close_timeout = True
        sm.data_changed(game_state="ready")          # fail
        sm.data_changed(game_state="setting")        # same state, no retrigger
        sm._motor_open_timeout = True
        sm.data_changed(game_state="activate")       # fail (motor open)

        assert sm.stable_game_state == "setting"


# ─────────────────────────────────────────────────────────────────────────────
# [4] fault latched 시 activate 차단
#     위험 동작(모터 전원 ON) 방지 핵심.
# ─────────────────────────────────────────────────────────────────────────────
class TestFaultLatchedBlocksActivate:
    def _latch_via_motor_timeout(self, sm: EscapeMainSM):
        """모터 timeout으로 systemFaultLatched를 True로 만드는 헬퍼."""
        sm.data_changed(game_state="setting")
        sm._motor_close_timeout = True
        sm.data_changed(game_state="ready")   # MarkTransitionFailed
        sm.handle_runtime_recovery()           # LatchSystemFault

    def test_fault_latched_blocks_activate(self):
        """systemFaultLatched=True 시 ActivateFunc가 차단되고 blocked 로그가 남는다."""
        sm = EscapeMainSM()
        self._latch_via_motor_timeout(sm)
        assert sm.system_fault_latched is True

        sm.clear_events()
        sm.data_changed(game_state="activate")
        assert any("blocked" in e.lower() for e in sm.get_events())

    def test_relay_stays_safe_when_activate_blocked(self):
        """차단된 activate에서 릴레이가 HIGH(안전) 상태를 유지한다."""
        sm = EscapeMainSM()
        self._latch_via_motor_timeout(sm)
        sm.data_changed(game_state="activate")
        assert sm.relay_high is True

    def test_mode_stays_wait_when_activate_blocked(self):
        """차단된 activate에서 mode가 tagcount로 변경되지 않는다."""
        sm = EscapeMainSM()
        self._latch_via_motor_timeout(sm)
        sm.data_changed(game_state="activate")
        assert sm._mode == "wait"

    def test_timer_stays_disabled_when_activate_blocked(self):
        """차단된 activate에서 GameTimer가 활성화되지 않는다."""
        sm = EscapeMainSM()
        self._latch_via_motor_timeout(sm)
        sm.data_changed(game_state="activate")
        assert sm._timer_enabled is False

    def test_stable_not_updated_when_activate_blocked(self):
        """차단된 activate에서 stable이 갱신되지 않는다."""
        sm = EscapeMainSM()
        self._latch_via_motor_timeout(sm)
        prev_stable = sm.stable_game_state

        sm.data_changed(game_state="activate")
        assert sm.stable_game_state == prev_stable


# ─────────────────────────────────────────────────────────────────────────────
# [5] recovery 후 마지막 stable state 기준 출력 복원
#     단순 recovery 함수 호출만이 아닌 relay/timer/mode/LED 실제 복원 검증.
# ─────────────────────────────────────────────────────────────────────────────
class TestRecoveryRestoresOutputs:
    def test_recovery_restores_relay_for_activate(self):
        """activate stable에서 논리 꼬임 후 recovery 시 relay가 LOW로 복원된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        # 논리 꼬임 시뮬레이션
        sm.relay_high = True
        sm._mode = "wait"
        sm._timer_enabled = False

        result = sm.recover_to_last_stable_state("logic mismatch")
        assert result is True
        assert sm.relay_high is False

    def test_recovery_restores_timer_for_activate(self):
        """activate stable에서 recovery 시 GameTimer가 재활성화된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._timer_enabled = False  # 강제 꼬임

        sm.recover_to_last_stable_state("test")
        assert sm._timer_enabled is True

    def test_recovery_restores_mode_for_activate(self):
        """activate stable에서 recovery 시 mode가 tagcount로 복원된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._mode = "wait"  # 강제 꼬임

        sm.recover_to_last_stable_state("test")
        assert sm._mode == "tagcount"

    def test_recovery_restores_relay_for_setting(self):
        """setting stable에서 relay가 HIGH(OFF)로 복원된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="setting")
        sm.relay_high = False  # 강제 꼬임

        sm.recover_to_last_stable_state("test")
        assert sm.relay_high is True

    def test_recovery_resends_escape_device_state(self):
        """stable device_state=escape이면 recovery 시 device_state=escape를 재전송한다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm.set_tags(True, True, True)
        sm.tick_game_timer()  # escape 성공 → stable device=escape
        sm.clear_sent()

        sm.recover_to_last_stable_state("wifi reconnected")
        sent = sm.get_sent()
        assert any(s["value"] == "escape" for s in sent)

    def test_recovery_returns_false_without_stable_state(self):
        """stable state가 없으면 recover_to_last_stable_state가 False를 반환한다."""
        sm = EscapeMainSM()  # stable_valid=False
        result = sm.recover_to_last_stable_state("test")
        assert result is False

    def test_uart_recovery_then_full_output_restored(self):
        """UART bad event 복구 후 activate stable 기준으로 전체 출력이 복원된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")   # stable = activate

        # 논리 꼬임 (WiFi 재연결 등으로 상태 불일치 발생)
        sm.relay_high = True
        sm._mode = "wait"
        sm._timer_enabled = False

        # bad event 3사이클 → UART recovery → stable 복원
        for kind in ("invalid_cmd", "format_error", "tag_parse"):
            sm.receive_bad_event(kind)
            sm.handle_runtime_recovery()

        assert sm.relay_high is False
        assert sm._mode == "tagcount"
        assert sm._timer_enabled is True

    def test_recovery_escape_stable_restores_wait_mode(self):
        """escape stable(activate+escape)에서 recovery 시 wait 모드로 복원된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm.set_tags(True, True, True)
        sm.tick_game_timer()             # stable = activate+escape, mode=wait
        sm._mode = "tagcount"            # 강제 꼬임

        sm.recover_to_last_stable_state("test")
        assert sm._mode == "wait"
        assert sm.relay_high is True


# ─────────────────────────────────────────────────────────────────────────────
# [6] motor fault → safe stop, stable restore 미실행
#     통신 오류 복구와 기구 fault 처리를 명확히 분리.
# ─────────────────────────────────────────────────────────────────────────────
class TestMotorFaultSafeStop:
    def test_motor_close_timeout_triggers_safe_stop(self):
        """EscapeClose timeout 시 systemFaultLatched=True가 된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._motor_close_timeout = True
        sm.handle_runtime_recovery()
        assert sm.system_fault_latched is True

    def test_motor_open_timeout_triggers_safe_stop(self):
        """EscapeOpen timeout 시 systemFaultLatched=True가 된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._motor_open_timeout = True
        sm.handle_runtime_recovery()
        assert sm.system_fault_latched is True

    def test_motor_fault_sets_relay_safe(self):
        """motor fault 시 릴레이가 HIGH(모터 전원 OFF)로 설정된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        assert sm.relay_high is False  # activate 중 = LOW

        sm._motor_open_timeout = True
        sm.handle_runtime_recovery()
        assert sm.relay_high is True   # safe stop → HIGH

    def test_motor_fault_disables_game_timer(self):
        """motor fault 시 GameTimer가 비활성화된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        assert sm._timer_enabled is True

        sm._motor_open_timeout = True
        sm.handle_runtime_recovery()
        assert sm._timer_enabled is False

    def test_motor_fault_blocks_stable_recovery(self):
        """fault latched 상태에서 RecoverToLastStableState가 False를 반환한다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._motor_open_timeout = True
        sm.handle_runtime_recovery()

        result = sm.recover_to_last_stable_state("attempt after fault")
        assert result is False

    def test_motor_fault_does_not_call_uart_recovery(self):
        """motor fault는 Beetle UART 재초기화를 실행하지 않는다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._motor_open_timeout = True
        sm.handle_runtime_recovery()

        assert sm.beetle_recover_attempts == 0

    def test_motor_fault_reason_recorded(self):
        """fault 원인 메시지가 systemFaultReason에 기록된다."""
        sm = EscapeMainSM()
        sm.data_changed(game_state="activate")
        sm._motor_close_timeout = True
        sm.handle_runtime_recovery()
        assert sm.system_fault_reason != ""
        assert "timeout" in sm.system_fault_reason.lower()
