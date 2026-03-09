void DataChanged()
{
  static StaticJsonDocument<1000> cur;
  if((String)(const char*)my["game_state"] != (String)(const char*)cur["game_state"]){
    if((String)(const char*)my["game_state"] == "setting"){
        SettingFunc();
    }
    else if((String)(const char*)my["game_state"] == "ready"){
        ReadyFunc();
    }
    else if((String)(const char*)my["game_state"] == "activate"){
        ActivateFunc();
    }
  }
  if((String)(const char*)my["device_state"] != (String)(const char*)cur["device_state"]){
    if((String)(const char*)my["device_state"] == "player_win"){
        ClearSystemFault();  // 이전 fault 해제 — player_win은 항상 실행 가능
        AllNeoOn(BLUE);
        EscapeClose();
    }
  }
  cur = my;
}

void WaitFunc(){
}

void SettingFunc(void)
{
    ClearSystemFault();
    Serial.println("SETTING");
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(WHITE);
    EscapeClose();

    if (motorCloseTimeout) {
        return;
    }

    ptrCurrentMode = WaitFunc;
    GameTimer.disable(gameTimerId);
}

void ActivateFunc(void){
    ClearSystemFault();
    Serial.println("ACTIVATE");
    myDFPlayer.playLargeFolder(1, VE1);
    AllNeoOn(YELLOW);
    EscapeOpen();

    if (motorOpenTimeout) {
        return;
    }

    GameTimer.enable(gameTimerId);
    toSubSerial.flush();
    ptrCurrentMode = TagCount;
}

void ReadyFunc(void){
    ClearSystemFault();
    Serial.println("READY");
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(RED);
    EscapeClose();

    if (motorCloseTimeout) {
        return;
    }

    ptrCurrentMode = WaitFunc;
}
