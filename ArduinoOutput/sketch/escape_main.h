#line 1 "c:\\Users\\HAS1\\Desktop\\BBangJun\\Final_Code\\escape_main\\escape_main.h"
#ifndef _DONE_ESCAPE_MAIN_CODE_
#define _DONE_ESCAPE_MAIN_CODE_

#include "Library_and_pin.h"
//****************************************WIFI****************************************************************
HAS2_Wifi has2wifi;
void DataChanged();
void SettingFunc(void);
void ActivateFunc(void);
void ReadyFunc(void);
void WaitFunc();
void WifiIntervalLoop(unsigned long intervalValue);
unsigned long wifiInterval = 0;
//****************************************Pointer System****************************************************************
void (*ptrCurrentMode)();   //현재모드 저장용 포인터 함수

//****************************************Serial Communication*********************************************************
void CommnunicationBeetle();
bool PlayerDetector(String playerNum);
HardwareSerial toSubSerial(1); //
String tag1;
String tag2;
String tag3;
bool tagState[3] = {false,false,false};
//****************************************Game System****************************************************************
int tagCnt = 0;
//****************************************Step Motor****************************************************************
void EscapeClose();
void EscapeOpen();
const int stepsPerRevolution = 200;  
//****************************************SimpleTimer SETUP****************************************************************
SimpleTimer GameTimer;
SimpleTimer WifiTimer;
void TimerInit();
void WifiIntervalFunc();
void GameTimerFunc();
int wifiTimerId;
int gameTimerId;

//****************************************DFPlayer SETUP****************************************************************
HardwareSerial MP3Serial(2);
DFRobotDFPlayerMini myDFPlayer;
void Mp3_Setup();
enum{VE1 = 1, VE2, VE3, VE4, VE5};
//****************************************Neopixel SETUP****************************************************************
void NeopixelInit();
void NeoBlink(int neo, int neoColor, int cnt, int blinkTime);
const int NumPixels[3] = {16,60,10};
const int NeopixelNum = 3;
enum {LINE = 0, ROUND, ONBOARD};
enum {WHITE = 0, RED, YELLOW, GREEN, BLUE, PURPLE, BLACK, BLUE0, BLUE1, BLUE2, BLUE3};
// Neopixel 색상정보
int color[11][3] = {    {20, 20, 20},   //WHITE
                        {40, 0, 0},     //RED        
                        {40, 40, 0},    //YELLOW
                        {0, 40, 0},     //GREEN
                        {0, 0, 40},     //BLUE
                        {40, 0, 40},    //PURPLE
                        {0, 0, 0},      //BLACK
                        {0, 0, 20},     //ENCODERBLUE0
                        {0, 0, 40},     //ENCODERBLUE1
                        {0, 0, 60},     //ENCODERBLUE2
                        {0, 0, 80}};    //ENCODERBLUE3

const int neopixel_num = 3; // 설치된 네오픽셀의 개수

Adafruit_NeoPixel pixels[NeopixelNum] = {Adafruit_NeoPixel(NumPixels[LINE], LINE_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
                                         Adafruit_NeoPixel(NumPixels[ROUND], ROUND_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
                                         Adafruit_NeoPixel(NumPixels[ONBOARD], ONBOARD_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800)};
                                         
#endif
