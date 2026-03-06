void CommnunicationBeetle(){
  Serial.println("READ");
  if(toSubSerial.available() > 0){
    lastBeetleMs = millis();
    String command = toSubSerial.readStringUntil('\n'); //추가 시리얼의 값을 수신하여 String으로 저장
    // Serial.println("INPUT: " + command);
    // Serial.println("received:" + String(command[0])); //기본 시리얼에 추가 시리얼 내용을 출력
    if(command[0] == 'W'){
      Serial.println("Beetle Init Success");
      toSubSerial.println("W");
    }
    else if(command[0] == 'R'){
      Serial.println("Beetle Reset Success");
    }
    else if(command[0] == 'T'){   // Packet ex.  "T1:GxP0_T2:GxP0_T3:GxP0"
      Serial.println(command);
      tag1 = command.substring(3,7);
      tag2 = command.substring(11,15);
      tag3 = command.substring(19,23);
      
      Serial.println("TAG1 = " + tag1);
      Serial.println("TAG2 = " + tag2);
      Serial.println("TAG3 = " + tag3);

      tagState[0] = PlayerDetector(tag1);
      tagState[1] = PlayerDetector(tag2);
      tagState[2] = PlayerDetector(tag3);
    }
    else if(command[0] == 'B'){
      Serial.println(command);
    }
    else if(command[0] == 'M'){
      ESP.restart();
    }
  }
  while(toSubSerial.available()){
    toSubSerial.read();
  }
}

bool PlayerDetector(String playerNum)
{
  if(playerNum[3] == '0')
    return false;
  else{
    has2wifi.Receive(playerNum);
    if((String)(const char*)tag["role"] == "player"){
      //Serial.println("PLAYER");
      return true;
    }
    else if((String)(const char*)tag["role"] == "tagger")
      return false;
  }
  
}