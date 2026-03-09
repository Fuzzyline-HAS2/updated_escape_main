void TagCount(){
    tagCnt = 0;
    for(int  i = 0; i < 3; i++){
        if(tagState[i] == true)
            tagCnt++;
    }
    if(tagCnt == 1){
        Serial.println("Tag 1 detected");
        myDFPlayer.playLargeFolder(1, VE2);
        delay(1500);
    }
    else if(tagCnt == 2){
        Serial.println("Tag 2 detected");
        myDFPlayer.playLargeFolder(1, VE3);
        delay(1500);
    }
    else if(tagCnt >= 3){
        Serial.println("Escape Activate");
        myDFPlayer.playLargeFolder(1, VE4);
        SendDeviceStateWithRetry("escape");
        EscapeClose();
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
        // escape 성공 후 안정 상태 갱신 (device_state=escape, 문=CLOSED)
        // game_state는 서버 기원이므로 현재 서버 값 유지
        if (!motorCloseTimeout) {
            CaptureStableState("activate", "escape", DOOR_CLOSED);
        }
        delay(5000);
        myDFPlayer.playLargeFolder(1, VE5);
    }

    for(int  i = 0; i < 3; i++)
        tagState[i] = false;
}