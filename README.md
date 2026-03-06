# escape_main

탈출 장치(스테퍼 모터 + RFID 태그) ESP32 메인 컨트롤러 코드

---

## 프로젝트 구조

```
escape_main/
├── escape_main.ino         메인 setup / loop
├── escape_main.h           전역 변수 및 객체 선언
├── Library_and_pin.h       핀 번호 및 라이브러리 include
├── Wifi.ino                서버 통신 및 상태 전환 (DataChanged, SettingFunc 등)
├── Game_system.ino         태그 카운트 및 탈출 로직 (TagCount)
├── serial_Communication.ino Beetle 서브 컨트롤러 시리얼 통신
├── stepper_Motor.ino       스테퍼 모터 제어 (EscapeOpen / EscapeClose)
├── neopixel.ino            Neopixel LED 제어
├── dfplayer.ino            MP3 재생 (DFPlayer)
├── timer.ino               SimpleTimer 초기화
├── QC/                     하드웨어 / 소프트웨어 오류 자동 감지
└── tests/                  Python 소프트웨어 테스트 하네스
```

---

## QC 엔진 (QC/)

`setup()`에서 룰을 등록하고 `loop()`의 `QCEngine::getInstance().tick()`으로 자동 실행됩니다.
이상 감지 시 Serial 모니터에 `[FAIL]` / `[WARN]` 로그를 출력합니다.

### Fast 룰 (매 loop() 틱마다 실행)

| ID | 테스트 내용 |
|----|------------|
| `NET_WIFI_00` | WiFi 연결 끊김 감지 |
| `HW_PIN_01` | Neopixel · 모터 · 릴레이 · UART 핀 번호 중복 검사 |
| `HW_GPIO_01` | 출력 핀이 ESP32 입력전용 GPIO(34~39)에 할당됐는지 검사 |
| `LOGIC_TAG_01` | `tagCnt`가 0~3 범위를 벗어나는지 검사 |

### Slow 룰 (2초 주기 실행)

| ID | 테스트 내용 |
|----|------------|
| `SYS_MEM_01` | 힙 메모리 부족 (< 20KB WARN / < 10KB FAIL) |
| `NET_WIFI_01` | WiFi 신호 세기 (< -75dBm WARN / < -85dBm FAIL) |
| `SYS_RST_01` | 부팅 후 1회 — Brownout / WDT / Panic 재시작 원인 감지 |
| `LOGIC_STATE_01` | 서버에서 수신한 `game_state` / `device_state` 허용값 검사 |
| `HW_STEPPER_01` | `stepsPerRevolution` 범위 검사 (0 이하 FAIL / >500 WARN) |
| `HW_SW_01` | `setting` / `ready` 상태에서 리미트 스위치가 LOW면 Close 미완료 |
| `LOGIC_SERIAL_01` | `activate` 상태에서 Beetle로부터 30초 이상 무응답 감지 |
| `HW_RELAY_01` | 릴레이 핀 상태와 `game_state`가 불일치하는지 검사 |

---

## Python 테스트 하네스 (tests/)

하드웨어 없이 소프트웨어 로직만 검증합니다.
`state_machine.py`가 Arduino 로직을 Python으로 미러링하며, pytest로 자동 실행됩니다.

### 설치

```powershell
cd tests
py -m pip install -r requirements.txt
```

### 실행

```powershell
py -m pytest -v
```

### 테스트 목록

**test_state_transitions.py** — 상태 전환 검증

| 테스트 | 내용 |
|--------|------|
| `TestSettingState` | `setting` 전환 시 RELAY OFF, EscapeClose 호출, GameTimer 비활성화 확인 |
| `TestReadyState` | `ready` 전환 시 RELAY OFF, EscapeClose 호출 확인 |
| `TestActivateState` | `activate` 전환 시 RELAY ON, EscapeOpen 호출, GameTimer 활성화 확인 |
| `TestChangeDetection` | 동일 상태 재수신 시 함수 재실행 없음 (cur 비교 로직), `player_win` 처리 확인 |
| `TestFullGameFlow` | `setting → ready → activate` 전체 순서 및 순서 보장 검증 |

**test_tag_escape.py** — 태그 감지 → escape 전송 검증

| 테스트 | 내용 |
|--------|------|
| `TestTagCountLogic` | 태그 1 / 2개 시 escape 미전송, 3개 시 `device_state=escape` 전송 확인 |
| `TestTagStateReset` | TagCount 실행 후 `tagState[]` 초기화 확인 |
| `TestModeTransitionOnEscape` | escape 전송 후 `ptrCurrentMode = WaitFunc` 전환 및 추가 TagCount 차단 확인 |
| `TestTagCountAccumulation` | 1개 → 2개 → 3개 순차 감지 시 각 단계별 동작 확인 |

**test_edge_cases.py** — 경계 조건 검증

| 테스트 | 내용 |
|--------|------|
| `TestRapidStateChanges` | 빠른 연속 상태 전환 후 최종 상태 정합성 확인 |
| `TestGameTimerGating` | `setting` / `ready` 상태에서 TagCount가 절대 실행되지 않는지 확인 |
| `TestRelayStateConsistency` | 각 상태별 릴레이 ON/OFF 정합성 및 `activate → setting` 복귀 시 원복 확인 |
| `TestTagCntBounds` | `tagCnt`가 음수 또는 3 초과하지 않는지 확인 |
| `TestDeviceStateIndependent` | `game_state`와 `device_state` 독립 변경 및 알 수 없는 상태값 무시 확인 |
