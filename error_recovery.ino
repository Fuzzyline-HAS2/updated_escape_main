// error_recovery.ino
// 런타임 에러 감지 및 복구 로직.
// 설계 원칙: Beetle silence는 오류가 아님. bad event(unknown cmd / 포맷 오류 / 파싱 실패)
// 가 연속 3사이클 누적될 때만 UART 재초기화. 모터 timeout은 즉시 safe stop.
// 통신/논리 오류 → 마지막 stable state로 복구. 기구/모터 오류 → safe stop.

// ---------------------------------------------------------
// LatchSystemFault: 모터/기구 고장 시 safe stop 상태로 고정.
// RELAY OFF + GameTimer 정지 + WaitFunc 전환.
// 다음 상태 전이 시 ClearSystemFault()로 해제 후 재시도 가능.
// ---------------------------------------------------------
void LatchSystemFault(const String& reason) {
    if (systemFaultLatched) return;
    systemFaultLatched = true;
    systemFaultReason = reason;
    digitalWrite(RELAY_PIN, HIGH);
    GameTimer.disable(gameTimerId);
    ptrCurrentMode = WaitFunc;
    Serial.println("[SAFE] FAULT LATCHED: " + reason);
    Serial.println("[SAFE] 다음 상태 전이 명령으로 재시도 가능.");
}

// ---------------------------------------------------------
// ClearSystemFault: 전이 시작 직전 기존 fault를 해제.
// 재시도 중 또 timeout 나면 HandleRuntimeRecovery가 다시 래치.
// ---------------------------------------------------------
void ClearSystemFault() {
    if (!systemFaultLatched) return;
    systemFaultLatched = false;
    systemFaultReason = "";
    Serial.println("[SAFE] Fault cleared. Ready for retry.");
}

// ---------------------------------------------------------
// ResetBeetleErrorCounters: 유효 패킷 수신 또는 복구 후 오류 카운터 초기화.
// serial_Communication.ino의 정상 T 패킷 처리 성공 시 호출.
// RecoverBeetleConnection() 내부에서도 호출.
// ---------------------------------------------------------
void ResetBeetleErrorCounters() {
    invalidCmdCount = 0;
    packetFormatErrorCount = 0;
    tagParseErrorCount = 0;
    beetleBadEventStreak = 0;
}

// ---------------------------------------------------------
// RecoverBeetleConnection: Beetle UART 재초기화.
// silence가 아닌 bad event 3회 누적 시에만 호출.
// 재초기화 후 RecoverToLastStableState() 호출.
// ---------------------------------------------------------
void RecoverBeetleConnection() {
    beetleRecoverAttempts++;
    Serial.println("[RECOVERY] Beetle UART 재초기화 시도 #" + String(beetleRecoverAttempts));
    toSubSerial.end();
    delay(100);
    toSubSerial.begin(115200, SERIAL_8N1, HWSERIAL_RX, HWSERIAL_TX);
    for (int i = 0; i < 3; i++) {
        toSubSerial.println("R");
        delay(50);
    }
    toSubSerial.println("W");
    ResetBeetleErrorCounters();
    Serial.println("[RECOVERY] UART 재초기화 완료.");
}

// ---------------------------------------------------------
// HandleRuntimeRecovery: 매 GameTimerFunc / WifiIntervalFunc 호출 시 실행.
// 1) 모터 timeout 플래그 → 즉시 LatchSystemFault (safe stop, 상태 복구 없음)
// 2) bad event 누적 → 3사이클 연속 시 RecoverBeetleConnection → stable 복구
// ※ Beetle silence(무수신)는 bad event로 간주하지 않음.
// ---------------------------------------------------------
void HandleRuntimeRecovery() {
    // --- 모터 timeout: 즉시 safe stop (stable restore 금지) ---
    if (motorCloseTimeout || motorOpenTimeout) {
        String reason = motorCloseTimeout
            ? "EscapeClose() timeout (SW_PIN never HIGH)"
            : "EscapeOpen() timeout";
        motorCloseTimeout = false;
        motorOpenTimeout = false;
        LatchSystemFault(reason);
        return;
    }

    // --- bad event 누적 감지 (delta 방식, silence 제외) ---
    static int prevCmd = 0, prevFmt = 0, prevTag = 0;

    // 외부에서 카운터가 감소한 경우(ResetBeetleErrorCounters 호출) prev 동기화
    if (invalidCmdCount < prevCmd) prevCmd = invalidCmdCount;
    if (packetFormatErrorCount < prevFmt) prevFmt = packetFormatErrorCount;
    if (tagParseErrorCount < prevTag) prevTag = tagParseErrorCount;

    bool newBadEvent = (invalidCmdCount > prevCmd ||
                        packetFormatErrorCount > prevFmt ||
                        tagParseErrorCount > prevTag);

    prevCmd = invalidCmdCount;
    prevFmt = packetFormatErrorCount;
    prevTag = tagParseErrorCount;

    if (newBadEvent) {
        beetleBadEventStreak++;
        Serial.println("[RECOVERY] bad event streak=" + String(beetleBadEventStreak) +
                       " (cmd=" + String(invalidCmdCount) +
                       " fmt=" + String(packetFormatErrorCount) +
                       " tag=" + String(tagParseErrorCount) + ")");
    }

    if (beetleBadEventStreak >= 3) {
        RecoverBeetleConnection();
        RecoverToLastStableState("beetle uart recovery");  // 통신 복구 후 stable 재적용
        prevCmd = 0; prevFmt = 0; prevTag = 0;
        if (beetleRecoverAttempts >= 3) {
            Serial.println("[RECOVERY] 최대 복구 시도 초과. 재시작합니다.");
            delay(500);
            ESP.restart();
        }
    }
}

// ---------------------------------------------------------
// SendDeviceStateWithRetry: device_state 전송 재시도 래퍼.
// WiFi 연결 상태 확인 후 retries 회 시도. 실패 시 로그만 남김.
// ---------------------------------------------------------
bool SendDeviceStateWithRetry(const String& value, uint8_t retries) {
    for (uint8_t i = 0; i < retries; i++) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WIFI] WARN: SendDeviceState '" + value +
                           "' attempt " + String(i + 1) + " skipped (no WiFi)");
            delay(200);
            continue;
        }
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", value);
        Serial.println("[WIFI] SendDeviceState '" + value +
                       "' sent (attempt " + String(i + 1) + ")");
        return true;
    }
    Serial.println("[WIFI] WARN: SendDeviceState '" + value + "' all retries failed.");
    return false;
}

// ===========================================================
// 마지막 안정 상태 스냅샷 관리 함수
// ===========================================================

// ---------------------------------------------------------
// CaptureStableState: 전이 성공 후 안정 상태 스냅샷 갱신.
// 반드시 전이가 완전히 성공했을 때만 호출할 것.
// ---------------------------------------------------------
void CaptureStableState(const String& gameState, const String& deviceState, DoorTarget door) {
    gStableState.stableGameState      = gameState;
    gStableState.stableDeviceState    = deviceState;
    gStableState.stableRelayHigh      = (digitalRead(RELAY_PIN) == HIGH);
    gStableState.stableGameTimerEnabled = (gameState == "activate");
    gStableState.stableMode           = (gameState == "activate") ? MODE_TAGCOUNT : MODE_WAIT;
    gStableState.stableDoorTarget     = door;
    gStableState.stableSinceMs        = millis();
    gStableState.stableValid          = true;

    String doorStr = (door == DOOR_OPEN) ? "OPEN"
                   : (door == DOOR_CLOSED) ? "CLOSED"
                   : "UNKNOWN";
    Serial.println("[STABLE] Captured: game=" + gameState +
                   " device=" + deviceState + " door=" + doorStr);
}

// ---------------------------------------------------------
// ReconcileOutputsToStableState: 안정 상태 기준으로 출력/모드를 idempotent하게 재정렬.
// MP3 재생 없음. 문 이동 없음. 안전한 출력만 재적용.
// ---------------------------------------------------------
void ReconcileOutputsToStableState() {
    if (!gStableState.stableValid) {
        Serial.println("[RECOVERY] No stable state to reconcile.");
        return;
    }
    String gs = gStableState.stableGameState;
    Serial.println("[RECOVERY] Reconciling outputs to stable state: " + gs);

    if (gs == "setting") {
        digitalWrite(RELAY_PIN, HIGH);
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
        AllNeoOn(WHITE);
    } else if (gs == "ready") {
        digitalWrite(RELAY_PIN, HIGH);
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
        AllNeoOn(RED);
    } else if (gs == "activate") {
        // device_state=escape 이면 문이 닫혀 있는 activate 완료 상태
        if (gStableState.stableDeviceState == "escape") {
            digitalWrite(RELAY_PIN, HIGH);
            GameTimer.disable(gameTimerId);
            ptrCurrentMode = WaitFunc;
            AllNeoOn(YELLOW);
        } else {
            digitalWrite(RELAY_PIN, LOW);
            GameTimer.enable(gameTimerId);
            ptrCurrentMode = TagCount;
            AllNeoOn(YELLOW);
        }
    }
}

// ---------------------------------------------------------
// RecoverToLastStableState: 통신/논리 오류 복구 후 안정 상태로 복원.
// systemFaultLatched(기구/모터 고장) 시 불가.
// 1차: ReconcileOutputsToStableState (LED/릴레이/타이머/모드 재정렬).
// device_state=escape 이면 재전송 시도.
// 문 재이동은 현재 구현에서 수행하지 않음 (안전 우선).
// ---------------------------------------------------------
bool RecoverToLastStableState(const String& reason) {
    if (systemFaultLatched) {
        Serial.println("[RECOVERY] Blocked by systemFaultLatched: " + systemFaultReason);
        return false;
    }
    if (!gStableState.stableValid) {
        Serial.println("[RECOVERY] No stable state available. Skipping.");
        return false;
    }

    lastRecoveryReason = reason;
    Serial.println("[RECOVERY] Restoring to last stable state: " +
                   gStableState.stableGameState + " (reason: " + reason + ")");

    // 1. 출력/모드 재정렬 (항상 실행)
    ReconcileOutputsToStableState();

    // 2. device_state=escape 는 로컬 소유 → 재전송
    if (gStableState.stableDeviceState == "escape") {
        SendDeviceStateWithRetry("escape");
    }

    Serial.println("[RECOVERY] Restored to: " + gStableState.stableGameState);
    return true;
}

// ---------------------------------------------------------
// MarkTransitionStart: 새 상태 전이 시작 시 pending 상태 기록.
// ---------------------------------------------------------
void MarkTransitionStart(const String& gameState, const String& deviceState) {
    transitionInProgress = true;
    pendingGameState     = gameState;
    pendingDeviceState   = deviceState;
    Serial.println("[TRANSITION] Start: game=" + gameState + " device=" + deviceState);
}

// ---------------------------------------------------------
// MarkTransitionSuccess: 전이 성공 확정. pending 초기화.
// CaptureStableState()를 먼저 호출한 뒤 이 함수를 호출할 것.
// ---------------------------------------------------------
void MarkTransitionSuccess() {
    transitionInProgress = false;
    pendingGameState     = "";
    pendingDeviceState   = "";
    Serial.println("[TRANSITION] Success.");
}

// ---------------------------------------------------------
// MarkTransitionFailed: 전이 실패. pending 폐기, stable 유지.
// ---------------------------------------------------------
void MarkTransitionFailed() {
    transitionInProgress = false;
    pendingGameState     = "";
    pendingDeviceState   = "";
    Serial.println("[TRANSITION] Failed — stable state unchanged.");
}
