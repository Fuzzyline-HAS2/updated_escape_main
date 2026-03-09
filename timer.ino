void TimerInit(){
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);
    gameTimerId = GameTimer.setInterval(500,GameTimerFunc);
    GameTimer.disable(gameTimerId);
}

void WifiIntervalFunc(){
    // WiFi 재연결 감지: 끊겼다가 다시 연결되면 stable state 재적용
    static bool lastWifiConnected = false;
    bool nowConnected = (WiFi.status() == WL_CONNECTED);
    if (!lastWifiConnected && nowConnected) {
        Serial.println("[WIFI] Reconnected.");
        RecoverToLastStableState("wifi reconnected");
    }
    lastWifiConnected = nowConnected;

    has2wifi.Loop(DataChanged);
    CommnunicationBeetle();
    HandleRuntimeRecovery(); // bad event 누적 + 모터 timeout 감시 (silence 제외)
}

void GameTimerFunc(){
    CommnunicationBeetle();
    Serial.print("Tag Count:");
    Serial.println(tagCnt);
    HandleRuntimeRecovery(); // bad event 누적 + 모터 timeout 감시 (silence 제외)
    ptrCurrentMode();
    while(toSubSerial.available()){
        toSubSerial.read();
    }
}
