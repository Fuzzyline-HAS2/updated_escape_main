void TimerInit(){
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);
    gameTimerId = GameTimer.setInterval(500,GameTimerFunc);
    GameTimer.disable(gameTimerId);
}

void WifiIntervalFunc(){
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
