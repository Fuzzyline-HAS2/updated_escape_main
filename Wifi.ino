void DataChanged()
{
  static StaticJsonDocument<500> cur;   //저장되어 있는 cur과 읽어온 my 값과 비교후 실행
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
  cur = my; // cur 데이터 그룹에 현재 읽어온 데이터 저장
}
void WaitFunc(){

}
void SettingFunc(void)
{
    Serial.println("SETTING");
    AllNeoOn(WHITE);
    EscapeClose();
    ptrCurrentMode = WaitFunc;
    GameTimer.disable(gameTimerId);

}
void ActivateFunc(void){
    Serial.println("ACTIVATE");
    myDFPlayer.playLargeFolder(1, VE1);
    AllNeoOn(YELLOW);
    EscapeOpen();
    GameTimer.enable(gameTimerId);
    toSubSerial.flush();
    ptrCurrentMode = TagCount;

}
void ReadyFunc(void){
    Serial.println("READY");
    AllNeoOn(RED);
    EscapeClose();
    ptrCurrentMode = WaitFunc;
}