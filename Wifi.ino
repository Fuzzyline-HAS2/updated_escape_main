void DataChanged()
{
  static StaticJsonDocument<2048> cur;
  String gameState = (String)(const char*)my["game_state"];
  String deviceState = (String)(const char*)my["device_state"];
  // 서버는 game_state와 device_state를 항상 같이 보낸다. device_state가
  // fake/tagger면 game_state가 "activate"로 같이 와도 실제로 문을 열면 안 되므로
  // (닫힘 상태는 아래 device_state 블록의 FakeDeviceFunc()가 책임진다) 여기서 막는다.
  bool isFakeOrTagger = (deviceState == "fake" || deviceState == "tagger");

  if (gameState != (String)(const char*)cur["game_state"]){
    if (gameState == "setting"){
        SettingFunc();
    }
    else if (gameState == "ready"){
        ReadyFunc();
    }
    else if (gameState == "activate"){
        if (!isFakeOrTagger){
            ActivateFunc();
        }
    }
    else if (gameState == "escape"){
        EscapeClose();
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
    }
  }
  if (my["brightness"].as<int>() != cur["brightness"].as<int>())
    UpdateBrightness();
  if (deviceState != (String)(const char*)cur["device_state"]){
    if (deviceState == "player_win"){
        AllNeoOn(BLUE);
        EscapeClose();
        ptrCurrentMode = WaitFunc;
        GameTimer.disable(gameTimerId);
    }
    else if (deviceState == "fake"){
        FakeDeviceFunc(7);
    }
    else if (deviceState == "tagger"){
        FakeDeviceFunc(8);
    }
    else if (deviceState == "activate"){
        // game_state가 실제로 activate인 경우에만 복귀 (third_store 브랜치의 검증된 이중 안전장치와 동일)
        if (gameState == "activate"){
            ActivateFunc(); // fake/tagger 상태 해제 후 정상 게임(탈출장치 열림)으로 복귀
        }
    }
    else if (deviceState == "github"){
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
// 두 상태는 UI/UX(보라색 LED, 닫힘)와 동작이 완전히 동일하고, 태그 시 재생되는
// 안내 오디오 트랙 번호(folder 1의 7번/8번)만 다르다.
// EscapeClose()는 이미 닫혀 있으면(리밋 스위치 기준) 즉시 반환되어 아무 움직임도
// 없고, 열려 있던 경우(예: 이전 activate 상태에서 전환)에만 실제로 닫는다.
void FakeDeviceFunc(int tagTrack){
    Serial.println("FAKE/TAGGER DEVICE (track " + String(tagTrack) + ")");
    AllNeoOn(PURPLE);
    EscapeClose();
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
