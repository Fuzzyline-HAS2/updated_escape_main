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
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "escape");
        EscapeClose();
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
        delay(5000);
        myDFPlayer.playLargeFolder(1, VE5);
    }

    for(int  i = 0; i < 3; i++)
        tagState[i] = false;
}