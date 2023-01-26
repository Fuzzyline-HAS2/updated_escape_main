# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\escape_main.ino"
 ;/**

 * @file Done_Escape_Main_code.ino

 * @author 김병준 (you@domain.com)

 * @brief

 * @version 1.0

 * @date 2022-11-29

 *

 * @copyright Copyright (c) 2022

 *

 */
# 12 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\escape_main.ino"
# 13 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\escape_main.ino" 2

void setup() {
    Serial.begin(115200);
    toSubSerial.begin(115200, 0x800001c, 18, 23);
    NeopixelInit();
    TimerInit();
    Mp3_Setup();
    StepMotorInit();
    pinMode(14, 0x03);
    //has2wifi.Setup("KT_GiGA_6C64","ed46zx1198");
    has2wifi.Setup();
    DataChanged();
    toSubSerial.println("R");
    toSubSerial.println("R");
    toSubSerial.println("R");
}
void loop() {
    WifiTimer.run();
    GameTimer.run();
}
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\Game_system.ino"
void TagCount(){
    tagCnt = 0;
    for(int i = 0; i < 3; i++){
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

    for(int i = 0; i < 3; i++)
        tagState[i] = false;
}
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\Wifi.ino"
void DataChanged()
{
  static StaticJsonDocument<500> cur; //저장되어 있는 cur과 읽어온 my 값과 비교후 실행
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
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\dfplayer.ino"
//****************************************mp3_setup()****************************************************************
void Mp3_Setup(){
  //Serial.println();
  MP3Serial.begin(9600, 0x800001c, 39, 33);
  Serial.println("DFRobot DFPlayer Mini Demo");
  Serial.println("Initializing DFPlayer ... (May take 3~5 seconds)");
  myDFPlayer.setTimeOut(1000); //Set serial communictaion time out 1000 ms
  if (!myDFPlayer.begin(MP3Serial)) { //Use softwareSerial to communicate with mp3.
    Serial.println("Unable to begin:");
    Serial.println("1.Please recheck the connection!");
    Serial.println("2.Please insert the SD card!");
    // while(true);
  }
  Serial.println(((reinterpret_cast<const __FlashStringHelper *>(("DFPlayer Mini online.")))));
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  myDFPlayer.volume(30); //Set volume value (0~30).
  myDFPlayer.EQ(0);
  //myDFPlayer.enableDAC();  //Enable On-chip DAC
  myDFPlayer.outputDevice(2);

}//void MP3_SETUP
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\neopixel.ino"
void NeopixelInit()
{
  for (int i = 0; i < NeopixelNum; ++i)
  {
    pixels[i].begin();
  }
  for (int i = 0; i < NeopixelNum; ++i)
  {
    pixels[i].lightColor(color[WHITE]);
  }
}

void NeoBlink(int neo, int neoColor, int cnt, int blinkTime){
    for(int i = 0; i < cnt; i++){ //0.5*10=5초동안 점멸
        pixels[neo].lightColor(color[BLACK]); //전체 off
        delay(blinkTime);
        pixels[neo].lightColor(color[neoColor]); //전체 적색on
        delay(blinkTime); //전체 적색on
    }
}

void AllNeoOn(int neoColor){
    for (int i = 0; i < NeopixelNum; ++i)
        pixels[i].lightColor(color[neoColor]);
}

void LineNeoOn(int changeColr, int baseColor, int cnt){
  for(int i = 0; i < NumPixels[LINE]; i++)
    pixels[LINE].setPixelColor(i,pixels[LINE].Color(color[baseColor][0],color[baseColor][1],color[baseColor][2]));
  for(int i = 0; i < cnt; i++)
    pixels[LINE].setPixelColor(i,pixels[LINE].Color(color[changeColr][0],color[changeColr][1],color[changeColr][2]));
  pixels[LINE].show();
}
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\serial_Communication.ino"
void CommnunicationBeetle(){
  Serial.println("READ");
  if(toSubSerial.available() > 0){
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
    else if(command[0] == 'T'){ // Packet ex.  "T1:GxP0_T2:GxP0_T3:GxP0"
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
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\stepper_Motor.ino"
void StepMotorInit(){
    pinMode(13, 0x03);
    pinMode(15, 0x03);
    pinMode(4, 0x05);
}
void EscapeClose(){
    digitalWrite(15, 0x0);
    digitalWrite(14, 0x1);
    Serial.println("Escapse Close");
    while(digitalRead(4) == 0x0)
    {
        digitalWrite(13, 0x0);
        delayMicroseconds(2000);
        digitalWrite(13, 0x1);
        delayMicroseconds(2000);
    }
    Serial.println("Close Finish");
}
void EscapeOpen(){
    digitalWrite(15, 0x1);
    digitalWrite(14, 0x0);
    Serial.println("Escapse Open");
    for(int x = 0; x < (stepsPerRevolution*10); x++)
    {
        digitalWrite(13, 0x1);
        delayMicroseconds(2000);
        digitalWrite(13, 0x0);
        delayMicroseconds(2000);
    }
    Serial.println("Open Finish");
}
# 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\timer.ino"
void TimerInit(){
    wifiTimerId = WifiTimer.setInterval(2000,WifiIntervalFunc);
    gameTimerId = GameTimer.setInterval(500,GameTimerFunc);
    GameTimer.disable(gameTimerId);
}

void WifiIntervalFunc(){
    has2wifi.Loop(DataChanged);
    CommnunicationBeetle();
}
void GameTimerFunc(){
    CommnunicationBeetle();
    Serial.print("Tag Count:");
    Serial.println(tagCnt);
    ptrCurrentMode();
    while(toSubSerial.available()){
        toSubSerial.read();
    }
}
