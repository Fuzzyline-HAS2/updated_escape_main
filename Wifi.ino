void DataChanged()
{
  static StaticJsonDocument<2048> cur;
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
    else if((String)(const char*)my["game_state"] == "escape"){
        EscapeClose();
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
    }
  }
  if (my["brightness"].as<int>() != cur["brightness"].as<int>())
    UpdateBrightness();
  if((String)(const char*)my["device_state"] != (String)(const char*)cur["device_state"]){
    if((String)(const char*)my["device_state"] == "player_win"){
        AllNeoOn(BLUE);
        EscapeClose();
        ptrCurrentMode = WaitFunc;
        GameTimer.disable(gameTimerId);
    }
    else if((String)(const char*)my["device_state"] == "fake"){
        FakeDeviceFunc(8);
    }
    else if((String)(const char*)my["device_state"] == "tagger"){
        FakeDeviceFunc(9);
    }
    else if((String)(const char*)my["device_state"] == "github"){
        Serial.println("[OTA] OTA 업데이트 요청 수신");
        ota.check();
    }
  }
  cur = my;
}

void WaitFunc(){
}

void SettingFunc(void)
{
    Serial.println("SETTING");
    digitalWrite(RELAY_PIN, HIGH);
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
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(RED);
    EscapeClose();
    ptrCurrentMode = WaitFunc;
}

// device_state == "fake"(가짜 탈출장치) / "tagger"(봉쇄된 탈출장치) 공용 진입 처리
// 두 상태는 UI/UX(보라색 LED, 닫힘)와 동작이 완전히 동일하고, 태그 시 재생되는
// 안내 오디오 트랙 번호(folder 1의 8번/9번)만 다르다.
void FakeDeviceFunc(int tagTrack){
    Serial.println("FAKE/TAGGER DEVICE (track " + String(tagTrack) + ")");
    AllNeoOn(PURPLE);
    EscapeClose();
    fakeTagTrack = tagTrack;
    fakeTagPresent = false; // 진입 시점 기준으로 엣지 재계산 (이미 태그가 올라와 있으면 즉시 1회 재생)
    ptrCurrentMode = FakeTagCheck;
    GameTimer.enable(gameTimerId);
}

// PN532 리더(T1/T2/T3) 중 하나라도 태그가 물리적으로 감지되면(서버 role 조회 없이)
// 안내 오디오를 1회만 재생한다. 태그를 계속 올려둬도 재생은 반복되지 않고,
// 태그를 뗐다가 다시 올리면 재생된다 (엣지 트리거).
void FakeTagCheck(){
    bool tagNow = (tag1.length() >= 4 && tag1[3] != '0') ||
                  (tag2.length() >= 4 && tag2[3] != '0') ||
                  (tag3.length() >= 4 && tag3[3] != '0');
    if (tagNow && !fakeTagPresent) {
        Serial.println("[FAKE/TAGGER] Tag detected -> playLargeFolder(1, " + String(fakeTagTrack) + ")");
        myDFPlayer.playLargeFolder(1, fakeTagTrack);
        AllNeoBlink(PURPLE, 3, 300); // 안내 오디오 재생 중 보라색 점멸 (사용 불가 알림)
        AllNeoOn(PURPLE);           // 점멸 종료 후 평소 상태(보라색 고정)로 복귀
    }
    fakeTagPresent = tagNow;
}
