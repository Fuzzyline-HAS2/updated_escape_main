"""
escape_main 소프트웨어 로직의 Python 시뮬레이션.
Wifi.ino(DataChanged, SettingFunc, ActivateFunc, ReadyFunc)와
Game_system.ino(TagCount)를 미러링합니다.
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

    # ── Public API ──────────────────────────────────────────────────────────

    def data_changed(self, game_state: str = None, device_state: str = None):
        """
        DataChanged() 시뮬레이션.
        내부의 cur로 변경 감지 후 해당 Func 호출.
        """
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
        """
        Beetle T패킷 수신 시뮬레이션.
        serial_Communication.ino의 'T' 패킷 파싱 후 tagState 설정을 대응.
        """
        self.tag_state = [t1, t2, t3]

    def tick_game_timer(self):
        """
        GameTimer tick 시뮬레이션 - ptrCurrentMode() 호출.
        activate 상태일 때만 TagCount()가 실행됨.
        """
        if self._mode == "tagcount":
            self._tag_count()

    # ── Private (Arduino 함수 대응) ─────────────────────────────────────────

    def _setting_func(self):
        """Wifi.ino SettingFunc()"""
        self._ev("SETTING")
        self.relay_high = True
        self._ev("AllNeoOn(WHITE)")
        self._ev("EscapeClose")
        self._mode = "wait"
        self._timer_enabled = False

    def _ready_func(self):
        """Wifi.ino ReadyFunc()"""
        self._ev("READY")
        self.relay_high = True
        self._ev("AllNeoOn(RED)")
        self._ev("EscapeClose")
        self._mode = "wait"

    def _activate_func(self):
        """Wifi.ino ActivateFunc()"""
        self._ev("ACTIVATE")
        self._ev("MP3 VE1")
        self._ev("AllNeoOn(YELLOW)")
        self._ev("EscapeOpen")
        self.relay_high = False
        self._mode = "tagcount"
        self._timer_enabled = True

    def _tag_count(self):
        """Game_system.ino TagCount()"""
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
            self._ev("MP3 VE5")

        self.tag_state = [False, False, False]

    def _ev(self, msg: str):
        self._events.append(msg)

    # ── Assertion helpers ────────────────────────────────────────────────────

    def get_events(self) -> list:
        return list(self._events)

    def clear_events(self):
        self._events.clear()

    def get_sent(self) -> list:
        return list(self._sent)

    def clear_sent(self):
        self._sent.clear()
