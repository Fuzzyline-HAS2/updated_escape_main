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
    MarkTransitionStart("setting", (String)(const char*)my["device_state"]);
    Serial.println("SETTING");
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(WHITE);
    EscapeClose();

    // 모터 timeout 발생 시 전이 실패 — HandleRuntimeRecovery가 LatchSystemFault 처리
    if (motorCloseTimeout) {
        MarkTransitionFailed();
        return;
    }

    ptrCurrentMode = WaitFunc;
    GameTimer.disable(gameTimerId);
    CaptureStableState("setting", (String)(const char*)my["device_state"], DOOR_CLOSED);
    MarkTransitionSuccess();
}

void ActivateFunc(void){
    // 기구 고장 래치 상태면 진입 차단
    if (systemFaultLatched) {
        Serial.println("[SAFE] ActivateFunc blocked: " + systemFaultReason);
        return;
    }

    MarkTransitionStart("activate", (String)(const char*)my["device_state"]);
    Serial.println("ACTIVATE");
    myDFPlayer.playLargeFolder(1, VE1);
    AllNeoOn(YELLOW);
    EscapeOpen();

    // 모터 timeout 발생 시 전이 실패
    if (motorOpenTimeout) {
        MarkTransitionFailed();
        return;
    }

    GameTimer.enable(gameTimerId);
    toSubSerial.flush();
    ptrCurrentMode = TagCount;
    CaptureStableState("activate", (String)(const char*)my["device_state"], DOOR_OPEN);
    MarkTransitionSuccess();
}

void ReadyFunc(void){
    MarkTransitionStart("ready", (String)(const char*)my["device_state"]);
    Serial.println("READY");
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(RED);
    EscapeClose();

    // 모터 timeout 발생 시 전이 실패
    if (motorCloseTimeout) {
        MarkTransitionFailed();
        return;
    }

    ptrCurrentMode = WaitFunc;
    CaptureStableState("ready", (String)(const char*)my["device_state"], DOOR_CLOSED);
    MarkTransitionSuccess();
}
