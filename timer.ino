void TimerInit(){
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);
    gameTimerId = GameTimer.setInterval(500,GameTimerFunc);
    GameTimer.disable(gameTimerId);
}

void WifiIntervalFunc(){
    has2wifi.Loop(DataChanged);
    CommnunicationBeetle();
}
void GameTimerFunc(){
    CommnunicationBeetle();
    Serial.print("Tag Count:");
    Serial.println(tagCnt);
    ptrCurrentMode();
    while(toSubSerial.available()){
        toSubSerial.read();
    }
}