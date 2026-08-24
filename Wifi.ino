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
        FakeDeviceFunc(7);
    }
    else if((String)(const char*)my["device_state"] == "tagger"){
        FakeDeviceFunc(8);
    }
    else if((String)(const char*)my["device_state"] == "activate"){
        ActivateFunc(); // fake/tagger 상태 해제 후 정상 게임(탈출장치 열림)으로 복귀
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

// PN532 리더(T1/T2/T3) 중 하나라도 태그가 물리적으로 올라와 있는지 여부
bool AnyTagPresent(){
    return (tag1.length() >= 4 && tag1[3] != '0') ||
           (tag2.length() >= 4 && tag2[3] != '0') ||
           (tag3.length() >= 4 && tag3[3] != '0');
}

// device_state == "fake"(가짜 탈출장치) / "tagger"(봉쇄된 탈출장치) 공용 진입 처리
// 두 상태는 UI/UX(보라색 LED)와 동작이 완전히 동일하고, 태그 시 재생되는
// 안내 오디오 트랙 번호(folder 1의 7번/8번)만 다르다.
// 이 시점에는 이미 탈출장치가 닫혀 있는 상태이므로 모터는 움직이지 않고
// 네오픽셀만 보라색으로 전환한다.
void FakeDeviceFunc(int tagTrack){
    Serial.println("FAKE/TAGGER DEVICE (track " + String(tagTrack) + ")");
    AllNeoOn(PURPLE);
    fakeTagTrack = tagTrack;
    // 진입 시점에 이미 올라와 있는 태그는 새로 태그한 것으로 치지 않는다
    // (엣지 기준선을 현재 상태로 맞춰, 상태 전환 "후"의 새 태그만 오디오를 재생시킨다).
    fakeTagPresent = AnyTagPresent();
    ptrCurrentMode = FakeTagCheck;
    GameTimer.enable(gameTimerId);
}

// PN532 리더(T1/T2/T3) 중 하나라도 태그가 물리적으로 감지되면(서버 role 조회 없이)
// 안내 오디오를 1회만 재생한다. 태그를 계속 올려둬도 재생은 반복되지 않고,
// 태그를 뗐다가 다시 올리면 재생된다 (엣지 트리거).
void FakeTagCheck(){
    bool tagNow = AnyTagPresent();
    if (tagNow && !fakeTagPresent) {
        Serial.println("[FAKE/TAGGER] Tag detected -> playLargeFolder(1, " + String(fakeTagTrack) + ")");
        myDFPlayer.playLargeFolder(1, fakeTagTrack);
        AllNeoBlink(PURPLE, 3, 300); // 안내 오디오 재생 중 보라색 점멸 (사용 불가 알림)
        AllNeoOn(PURPLE);           // 점멸 종료 후 평소 상태(보라색 고정)로 복귀
    }
    fakeTagPresent = tagNow;
}
