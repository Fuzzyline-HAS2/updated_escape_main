void CommnunicationBeetle(){
  Serial.println("READ");
  if(toSubSerial.available() > 0){
    lastBeetleMs = millis();
    String command = toSubSerial.readStringUntil('\n');

    // 빈 문자열 방어
    if (command.length() == 0) return;

    char cmd = command[0];

    if(cmd == 'W'){
      Serial.println("Beetle Init Success");
      toSubSerial.println("W");
    }
    else if(cmd == 'R'){
      Serial.println("Beetle Reset Success");
    }
    else if(cmd == 'T'){
      // 원문 저장 (포맷 검증 전)
      lastBeetleRawPacket = command;

      // --- 포맷 검증: "T1:xxxx_T2:xxxx_T3:xxxx" (최소 길이 23, 구분자 위치 고정) ---
      bool fmtOk = (command.length() >= 23 &&
                    command[1] == '1' && command[2] == ':' &&
                    command[7] == '_' &&
                    command[8] == 'T' && command[9] == '2' && command[10] == ':' &&
                    command[15] == '_' &&
                    command[16] == 'T' && command[17] == '3' && command[18] == ':');

      if (!fmtOk) {
        packetFormatErrorCount++;
        Serial.println("[UART] WARN malformed T packet: " + command);
        return; // 파싱 금지 — 잘못된 substring 접근 방지
      }

      Serial.println(command);
      tag1 = command.substring(3, 7);
      tag2 = command.substring(11, 15);
      tag3 = command.substring(19, 23);

      Serial.println("TAG1 = " + tag1);
      Serial.println("TAG2 = " + tag2);
      Serial.println("TAG3 = " + tag3);

      tagState[0] = PlayerDetector(tag1);
      tagState[1] = PlayerDetector(tag2);
      tagState[2] = PlayerDetector(tag3);

      // 유효 패킷 처리 성공 → bad event 카운터 초기화
      ResetBeetleErrorCounters();
    }
    else if(cmd == 'B'){
      Serial.println(command);
    }
    else if(cmd == 'M'){
      ESP.restart();
    }
    else {
      // 허용되지 않은 명령 문자
      invalidCmdCount++;
      Serial.println("[UART] WARN unknown command '" + String(cmd) + "'");
    }
  }
  while(toSubSerial.available()){
    toSubSerial.read();
  }
}

bool PlayerDetector(String playerNum)
{
  // 길이 방어: playerNum[3] 접근 전 확인
  if (playerNum.length() < 4) {
    tagParseErrorCount++;
    Serial.println("[UART] WARN tag length < 4: '" + playerNum + "'");
    return false;
  }

  if(playerNum[3] == '0')
    return false;

  has2wifi.Receive(playerNum);
  String role = (String)(const char*)tag["role"];

  if (role == "player") {
    return true;
  } else if (role == "tagger") {
    return false;
  } else {
    // role 미해석: 서버 미등록 태그 또는 UART 노이즈
    tagParseErrorCount++;
    Serial.println("[UART] WARN tag role unresolved: '" + role + "' for " + playerNum);
    return false;
  }
}
