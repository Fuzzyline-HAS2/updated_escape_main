#ifndef QC_RULES_H
#define QC_RULES_H

#include "../Library_and_pin.h"
#include "QC_Types.h"
#include <WiFi.h>
#include <esp_system.h>

// 'my' 는 HAS2_Wifi.h 에서 선언된 서버 데이터 JSON 객체

// ---------------------------------------------------------
// [SYS_MEM_01] 남은 힙 메모리 체크
// ---------------------------------------------------------
// 20KB 미만이면 WARN, 10KB 미만이면 FAIL.
// 메모리 누수나 과도한 String 연산이 반복되면 ESP32가 재시작될 수 있습니다.
class QCRule_HeapMemory : public IQCRule {
public:
  String getId() const override { return "SYS_MEM_01"; }
  String getName() const override { return "Free Heap Check"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    uint32_t freeHeap = ESP.getFreeHeap();
    String val = String(freeHeap) + " bytes";
    if (freeHeap < 10000) {
      return QCResult(QCLevel::FAIL, getId(), "Free Heap", ">=10000", val,
                      "메모리 누수 점검 또는 정적 할당 줄이기");
    } else if (freeHeap < 20000) {
      return QCResult(QCLevel::WARN, getId(), "Free Heap", ">=20000", val,
                      "메모리 사용량을 지속 모니터링 필요");
    }
    return QCResult();
  }
};

// ---------------------------------------------------------
// [NET_WIFI_00] WiFi 연결 상태 체크 (Fast)
// ---------------------------------------------------------
// WiFi.status()가 WL_CONNECTED가 아니면 즉시 FAIL.
// 매 루프마다 실행되어 연결 끊김을 즉시 감지합니다.
class QCRule_WifiConnection : public IQCRule {
public:
  String getId() const override { return "NET_WIFI_00"; }
  String getName() const override { return "WiFi Connection Status"; }
  bool isFastCheck() const override { return true; }

  QCResult check() override {
    if (WiFi.status() != WL_CONNECTED) {
      return QCResult(QCLevel::FAIL, getId(), "WiFi Status",
                      "Connected", "Disconnected",
                      "공유기/비밀번호 확인 또는 ESP32 재시작");
    }
    return QCResult();
  }
};

// ---------------------------------------------------------
// [NET_WIFI_01] WiFi 신호 세기(RSSI) 체크
// ---------------------------------------------------------
// -75dBm 미만이면 WARN, -85dBm 미만이면 FAIL.
// 신호가 약하면 서버 통신이 불안정해져 게임 진행에 지장이 생깁니다.
class QCRule_WifiSignal : public IQCRule {
public:
  String getId() const override { return "NET_WIFI_01"; }
  String getName() const override { return "WiFi Signal Strength"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    if (WiFi.status() != WL_CONNECTED) return QCResult();
    int32_t rssi = WiFi.RSSI();
    String val = String(rssi) + " dBm";
    if (rssi < -85) {
      return QCResult(QCLevel::FAIL, getId(), "WiFi RSSI", ">=-85dBm", val,
                      "공유기와 거리를 줄이거나 안테나 점검");
    } else if (rssi < -75) {
      return QCResult(QCLevel::WARN, getId(), "WiFi RSSI", ">=-75dBm", val,
                      "신호 약함, 패킷 손실 가능성 있음");
    }
    return QCResult();
  }
};

// ---------------------------------------------------------
// [SYS_RST_01] 마지막 리셋 원인 체크
// ---------------------------------------------------------
// 부팅 후 한 번만 실행됩니다.
// Brownout(전압 부족)은 FAIL, WDT/Panic(크래시)은 WARN.
// 릴레이/모터 서지 노이즈나 공유 GND 불량이 주요 원인입니다.
class QCRule_ResetReason : public IQCRule {
public:
  String getId() const override { return "SYS_RST_01"; }
  String getName() const override { return "Reset Reason Check"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    static bool reported = false;
    if (reported) return QCResult();

    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_BROWNOUT) {
      reported = true;
      return QCResult(QCLevel::FAIL, getId(), "Last Reset Reason",
                      "No brownout", "ESP_RST_BROWNOUT",
                      "5V/3.3V 전원 확인, 모터 서지 노이즈, 공유 GND 점검");
    }
    bool isWdt = (reason == ESP_RST_WDT || reason == ESP_RST_INT_WDT);
#ifdef ESP_RST_TASK_WDT
    isWdt = isWdt || (reason == ESP_RST_TASK_WDT);
#endif
    if (isWdt) {
      reported = true;
      return QCResult(QCLevel::WARN, getId(), "Last Reset Reason",
                      "No WDT reset", String((int)reason),
                      "루프 블로킹 코드 확인 (EscapeClose while 루프 등)");
    }
    if (reason == ESP_RST_PANIC) {
      reported = true;
      return QCResult(QCLevel::WARN, getId(), "Last Reset Reason",
                      "No panic", "ESP_RST_PANIC",
                      "널 포인터 / 메모리 오염 확인");
    }
    return QCResult();
  }
};

// ---------------------------------------------------------
// [HW_PIN_01] 핀 번호 충돌 체크 (Fast)
// ---------------------------------------------------------
// Library_and_pin.h에 정의된 핀들이 서로 겹치지 않는지 확인합니다.
// Neopixel 3개, 스테퍼 모터, 릴레이, DFPlayer, 서브 시리얼 핀 간 충돌 검사.
class QCRule_PinConflict : public IQCRule {
public:
  String getId() const override { return "HW_PIN_01"; }
  String getName() const override { return "Pin Conflict Check"; }
  bool isFastCheck() const override { return true; }

  QCResult check() override {
    // 1. Neopixel 3개 핀이 서로 겹치지 않는지
    if (LINE_NEOPIXEL_PIN == ROUND_NEOPIXEL_PIN ||
        LINE_NEOPIXEL_PIN == ONBOARD_NEOPIXEL_PIN ||
        ROUND_NEOPIXEL_PIN == ONBOARD_NEOPIXEL_PIN) {
      String val = "LINE:" + String(LINE_NEOPIXEL_PIN) +
                   " ROUND:" + String(ROUND_NEOPIXEL_PIN) +
                   " ONBOARD:" + String(ONBOARD_NEOPIXEL_PIN);
      return QCResult(QCLevel::FAIL, getId(), "Neopixel Pin Conflict",
                      "핀 중복 없음", val,
                      "각 Neopixel 스트립에 서로 다른 GPIO 할당");
    }

    // 2. 스테퍼 모터 핀(STEP, DIR, EN)이 Neopixel 핀과 겹치지 않는지
    const int neoPins[3] = {LINE_NEOPIXEL_PIN, ROUND_NEOPIXEL_PIN, ONBOARD_NEOPIXEL_PIN};
    const int motorPins[3] = {STEP_PIN, DIR_PIN, EN_PIN};
    for (int m = 0; m < 3; m++) {
      for (int n = 0; n < 3; n++) {
        if (motorPins[m] == neoPins[n]) {
          return QCResult(QCLevel::FAIL, getId(), "Stepper/Neopixel Pin Conflict",
                          "핀 중복 없음",
                          "STEP/DIR/EN=" + String(motorPins[m]),
                          "스테퍼 핀을 Neopixel 핀과 분리");
        }
      }
    }

    // 3. RELAY_PIN이 모터/Neopixel 핀과 겹치지 않는지
    for (int i = 0; i < 3; i++) {
      if (RELAY_PIN == neoPins[i] || RELAY_PIN == motorPins[i]) {
        return QCResult(QCLevel::FAIL, getId(), "Relay Pin Conflict",
                        "핀 중복 없음", "RELAY=" + String(RELAY_PIN),
                        "릴레이 핀을 다른 GPIO로 변경");
      }
    }

    // 4. DFPlayer TX 핀이 Neopixel / 모터 핀과 겹치지 않는지
    for (int i = 0; i < 3; i++) {
      if (DFPLAYER_TX_PIN == neoPins[i] || DFPLAYER_TX_PIN == motorPins[i]) {
        return QCResult(QCLevel::FAIL, getId(), "DFPlayer TX Pin Conflict",
                        "핀 중복 없음", "DFPLAYER_TX=" + String(DFPLAYER_TX_PIN),
                        "DFPlayer TX 핀을 다른 GPIO로 변경");
      }
    }

    // 5. 서브 시리얼(Beetle) TX/RX가 DFPlayer TX/RX와 겹치지 않는지
    if (HWSERIAL_TX == DFPLAYER_TX_PIN || HWSERIAL_TX == DFPLAYER_RX_PIN ||
        HWSERIAL_RX == DFPLAYER_TX_PIN || HWSERIAL_RX == DFPLAYER_RX_PIN) {
      String val = "SubTX:" + String(HWSERIAL_TX) +
                   " SubRX:" + String(HWSERIAL_RX) +
                   " MP3TX:" + String(DFPLAYER_TX_PIN) +
                   " MP3RX:" + String(DFPLAYER_RX_PIN);
      return QCResult(QCLevel::FAIL, getId(), "Serial Pin Conflict",
                      "UART 핀 중복 없음", val,
                      "Beetle UART와 DFPlayer UART 핀 분리");
    }

    // 6. SW_PIN(리미트 스위치)이 모터/Neopixel 핀과 겹치지 않는지
    for (int i = 0; i < 3; i++) {
      if (SW_PIN == neoPins[i] || SW_PIN == motorPins[i]) {
        return QCResult(QCLevel::FAIL, getId(), "SW Pin Conflict",
                        "핀 중복 없음", "SW=" + String(SW_PIN),
                        "리미트 스위치 핀을 다른 GPIO로 변경");
      }
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [HW_GPIO_01] GPIO 입출력 기능 적합성 체크 (Fast)
// ---------------------------------------------------------
// ESP32의 GPIO34~39는 입력 전용(Input-Only)입니다.
// 출력이 필요한 핀(Neopixel, Motor, Relay, DFPlayer TX, Serial TX)이
// 34~39에 할당되어 있으면 FAIL을 반환합니다.
// DFPLAYER_RX_PIN=39, HWSERIAL_RX=18은 입력이므로 GPIO39도 허용됩니다.
class QCRule_GpioCapability : public IQCRule {
public:
  String getId() const override { return "HW_GPIO_01"; }
  String getName() const override { return "GPIO Capability Check"; }
  bool isFastCheck() const override { return true; }

  QCResult check() override {
    auto isInputOnly = [](int pin) { return pin >= 34 && pin <= 39; };

    // 출력이 필요한 핀 목록
    const int outputPins[] = {
        LINE_NEOPIXEL_PIN, ROUND_NEOPIXEL_PIN, ONBOARD_NEOPIXEL_PIN,
        STEP_PIN, DIR_PIN, EN_PIN,
        RELAY_PIN,
        DFPLAYER_TX_PIN,
        HWSERIAL_TX
    };
    const char *outputNames[] = {
        "LINE_NEO", "ROUND_NEO", "ONBOARD_NEO",
        "STEP", "DIR", "EN",
        "RELAY",
        "DFPLAYER_TX",
        "HWSERIAL_TX"
    };
    const int outputCount = sizeof(outputPins) / sizeof(outputPins[0]);

    for (int i = 0; i < outputCount; i++) {
      if (isInputOnly(outputPins[i])) {
        return QCResult(QCLevel::FAIL, getId(),
                        String(outputNames[i]) + " on Input-Only GPIO",
                        "GPIO 0~33", String(outputPins[i]),
                        String(outputNames[i]) + " 핀을 GPIO34~39 범위 밖으로 이동");
      }
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [LOGIC_STATE_01] 게임 상태값 유효성 체크
// ---------------------------------------------------------
// 서버에서 받아온 my JSON의 game_state, device_state가
// escape_main에서 허용된 값인지 확인합니다.
// 알 수 없는 상태값이 내려오면 서버 설정 오류 또는 로직 버그입니다.
class QCRule_GameState : public IQCRule {
public:
  String getId() const override { return "LOGIC_STATE_01"; }
  String getName() const override { return "Game State Validity"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    if (my.size() == 0) return QCResult(); // 아직 서버 데이터 없음

    auto safeStr = [](JsonVariantConst v) -> String {
      const char *p = v.as<const char *>();
      return p ? String(p) : String("");
    };

    // game_state 허용값: setting / ready / activate / ""
    if (my.containsKey("game_state")) {
      String gState = safeStr(my["game_state"]);
      bool gValid = (gState == "setting" || gState == "ready" ||
                     gState == "activate" || gState == "");
      if (!gValid) {
        return QCResult(QCLevel::WARN, getId(), "game_state",
                        "setting/ready/activate", gState,
                        "서버 game_state 설정 확인");
      }
    }

    // device_state 허용값: setting / activate / ready / escape / player_win / ""
    if (my.containsKey("device_state")) {
      String dState = safeStr(my["device_state"]);
      bool dValid = (dState == "setting" || dState == "activate" ||
                     dState == "ready" || dState == "escape" ||
                     dState == "player_win" || dState == "");
      if (!dValid) {
        return QCResult(QCLevel::WARN, getId(), "device_state",
                        "setting/activate/ready/escape/player_win", dState,
                        "서버 device_state 설정 확인");
      }
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [LOGIC_TAG_01] 태그 카운트 범위 초과 방지 체크 (Fast)
// ---------------------------------------------------------
// TagCount()에서 집계되는 tagCnt가 0~3 범위를 벗어나면 이상 동작입니다.
// tagCnt가 3을 초과하면 탈출 트리거 로직이 오동작할 수 있습니다.
class QCRule_TagCntBounds : public IQCRule {
public:
  String getId() const override { return "LOGIC_TAG_01"; }
  String getName() const override { return "Tag Count Bounds Check"; }
  bool isFastCheck() const override { return true; }

  QCResult check() override {
    extern int tagCnt;

    if (tagCnt > 3) {
      return QCResult(QCLevel::FAIL, getId(), "tagCnt", "0~3",
                      String(tagCnt),
                      "TagCount() 로직 확인, tagState[] 초기화 누락 의심");
    }
    if (tagCnt < 0) {
      return QCResult(QCLevel::FAIL, getId(), "tagCnt", ">=0",
                      String(tagCnt),
                      "tagCnt 음수 감소 경로 확인");
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [HW_STEPPER_01] 스테퍼 모터 설정 유효성 체크
// ---------------------------------------------------------
// stepsPerRevolution이 0이면 모터가 움직이지 않습니다.
// 너무 작으면(< 10) 회전이 부족하고, 너무 크면(> 500) loop 블로킹이 심합니다.
// EscapeOpen()은 stepsPerRevolution*10 스텝을 blocking으로 실행하므로 주의.
class QCRule_StepperConfig : public IQCRule {
public:
  String getId() const override { return "HW_STEPPER_01"; }
  String getName() const override { return "Stepper Motor Config Check"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    extern const int stepsPerRevolution;

    if (stepsPerRevolution <= 0) {
      return QCResult(QCLevel::FAIL, getId(), "stepsPerRevolution",
                      ">0", String(stepsPerRevolution),
                      "stepsPerRevolution을 양수로 설정 (기본값: 100 또는 200)");
    }
    if (stepsPerRevolution < 10) {
      return QCResult(QCLevel::WARN, getId(), "stepsPerRevolution",
                      ">=10", String(stepsPerRevolution),
                      "회전량이 너무 작아 탈출 장치가 열리지 않을 수 있음");
    }
    if (stepsPerRevolution > 500) {
      return QCResult(QCLevel::WARN, getId(), "stepsPerRevolution",
                      "<=500", String(stepsPerRevolution),
                      "EscapeOpen() 실행 시 loop() 블로킹 시간 과다 (최대 " +
                      String((stepsPerRevolution * 10 * 4) / 1000) + "초)");
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [HW_SW_01] 리미트 스위치 상태 일관성 체크
// ---------------------------------------------------------
// SW_PIN은 NC(normally-closed) 스위치 + INPUT_PULLUP 구성입니다.
// 모터가 닫힌 위치(EscapeClose 완료) = 스위치 개방 = SW_PIN HIGH.
// setting/ready 상태에서 SW_PIN이 LOW이면 Close가 완료되지 않은 것입니다.
// → 스위치 배선 불량, 모터 탈조, 또는 EscapeClose while 루프 비정상 종료 의심.
class QCRule_LimitSwitch : public IQCRule {
public:
  String getId() const override { return "HW_SW_01"; }
  String getName() const override { return "Limit Switch State Consistency"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    if (my.size() == 0) return QCResult();

    auto safeStr = [](JsonVariantConst v) -> String {
      const char *p = v.as<const char *>();
      return p ? String(p) : String("");
    };

    String gState = safeStr(my["game_state"]);

    // setting/ready = EscapeClose() 완료 상태 = SW_PIN HIGH 이어야 정상
    if ((gState == "setting" || gState == "ready") &&
        digitalRead(SW_PIN) == LOW) {
      return QCResult(QCLevel::WARN, getId(),
                      "SW_PIN (GPIO " + String(SW_PIN) + ")",
                      "HIGH (closed)", "LOW",
                      "리미트 스위치 배선 점검, 모터 탈조 또는 EscapeClose() 미완료 확인");
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [LOGIC_SERIAL_01] Beetle 시리얼 통신 타임아웃 체크
// ---------------------------------------------------------
// activate 상태에서 Beetle로부터 30초 이상 데이터가 없으면 WARN을 반환합니다.
// lastBeetleMs는 escape_main.h에 선언, serial_Communication.ino에서 갱신됩니다.
// 최초 1회 이상 수신 후에만 타임아웃을 판단합니다(부팅 직후 오탐 방지).
class QCRule_BeetleTimeout : public IQCRule {
public:
  String getId() const override { return "LOGIC_SERIAL_01"; }
  String getName() const override { return "Beetle Serial Timeout"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    if (my.size() == 0) return QCResult();

    auto safeStr = [](JsonVariantConst v) -> String {
      const char *p = v.as<const char *>();
      return p ? String(p) : String("");
    };

    String gState = safeStr(my["game_state"]);
    if (gState != "activate") return QCResult();

    extern unsigned long lastBeetleMs;

    // 한 번도 수신 안 된 경우는 스킵 (부팅 직후 오탐 방지)
    if (lastBeetleMs == 0) return QCResult();

    const unsigned long TIMEOUT_MS = 30000UL;
    unsigned long elapsed = millis() - lastBeetleMs;
    if (elapsed > TIMEOUT_MS) {
      return QCResult(QCLevel::WARN, getId(), "Beetle Last Recv",
                      "< 30s ago", String(elapsed / 1000) + "s ago",
                      "Beetle 전원/배선 점검, TX(" + String(HWSERIAL_TX) +
                      ")/RX(" + String(HWSERIAL_RX) + ") 확인");
    }

    return QCResult();
  }
};

// ---------------------------------------------------------
// [HW_RELAY_01] 릴레이 상태 vs game_state 일관성 체크
// ---------------------------------------------------------
// EscapeOpen()은 RELAY_PIN을 LOW(ON)로, EscapeClose()는 HIGH(OFF)로 설정합니다.
// activate 상태 = 모터 열림 = RELAY_PIN LOW 이어야 정상.
// setting/ready 상태 = 모터 닫힘 = RELAY_PIN HIGH 이어야 정상.
// 불일치는 ActivateFunc/SettingFunc/ReadyFunc 내 EscapeOpen/Close 호출 누락 의심.
class QCRule_RelayState : public IQCRule {
public:
  String getId() const override { return "HW_RELAY_01"; }
  String getName() const override { return "Relay State Consistency"; }
  bool isFastCheck() const override { return false; }

  QCResult check() override {
    if (my.size() == 0) return QCResult();

    auto safeStr = [](JsonVariantConst v) -> String {
      const char *p = v.as<const char *>();
      return p ? String(p) : String("");
    };

    String gState = safeStr(my["game_state"]);
    int relayPin = digitalRead(RELAY_PIN);
    String val = (relayPin == LOW) ? "LOW (ON)" : "HIGH (OFF)";

    // activate = EscapeOpen() 실행 → RELAY_PIN LOW 이어야 정상
    if (gState == "activate" && relayPin == HIGH) {
      return QCResult(QCLevel::WARN, getId(), "RELAY_PIN", "LOW (모터 전원 ON)",
                      val, "ActivateFunc() 에서 EscapeOpen() 호출 여부 확인");
    }

    // setting/ready = EscapeClose() 실행 → RELAY_PIN HIGH 이어야 정상
    if ((gState == "setting" || gState == "ready") && relayPin == LOW) {
      return QCResult(QCLevel::WARN, getId(), "RELAY_PIN", "HIGH (모터 전원 OFF)",
                      val, "SettingFunc/ReadyFunc 에서 RELAY_PIN HIGH 또는 EscapeClose() 확인");
    }

    return QCResult();
  }
};

#endif // QC_RULES_H
