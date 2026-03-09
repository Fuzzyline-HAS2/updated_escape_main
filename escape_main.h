#ifndef _DONE_ESCAPE_MAIN_CODE_
#define _DONE_ESCAPE_MAIN_CODE_

#include "Library_and_pin.h"
#include "QC/QC_Engine.h"
#include "QC/QC_Rules.h"
//****************************************WIFI****************************************************************
HAS2_Wifi has2wifi("http://172.30.1.43");
void DataChanged();
void SettingFunc(void);
void ActivateFunc(void);
void ReadyFunc(void);
void WaitFunc();
void WifiIntervalLoop(unsigned long intervalValue);
unsigned long wifiInterval = 0;
//****************************************Pointer
//System****************************************************************
void (*ptrCurrentMode)(); // 현재모드 저장용 포인터 함수

//****************************************Serial
//Communication*********************************************************
void CommnunicationBeetle();
bool PlayerDetector(String playerNum);
HardwareSerial toSubSerial(1); //
String tag1;
String tag2;
String tag3;
bool tagState[3] = {false, false, false};
//****************************************Game
//System****************************************************************
int tagCnt = 0;
unsigned long lastBeetleMs = 0; // Beetle 마지막 수신 시각
bool motorCloseTimeout = false;  // HW_MOTOR_02: EscapeClose() 타임아웃 플래그
bool motorOpenTimeout = false;   // HW_MOTOR_02: EscapeOpen() 타임아웃 플래그
String lastBeetleRawPacket = ""; // LOGIC_SERIAL_02: 마지막 수신 T 패킷 원문
int invalidCmdCount = 0;         // LOGIC_SERIAL_03: 허용되지 않은 명령 수신 횟수
int packetFormatErrorCount = 0;  // LOGIC_SERIAL_02: T 패킷 포맷 오류 누적
int tagParseErrorCount = 0;      // LOGIC_TAG_02: 태그 파싱 실패 누적
uint8_t beetleBadEventStreak = 0;   // 연속 bad-event 사이클 수 (silence 제외)
uint8_t beetleRecoverAttempts = 0;  // Beetle UART 복구 시도 횟수
bool systemFaultLatched = false;    // 모터/기구 고장 래치 (수동 복구 필요)
String systemFaultReason = "";      // 고장 원인 메시지
//****************************************Recovery
//Functions****************************************************************
void LatchSystemFault(const String& reason);
void ClearSystemFault();
void HandleRuntimeRecovery();
void RecoverBeetleConnection();
void ResetBeetleErrorCounters();
bool SendDeviceStateWithRetry(const String& value, uint8_t retries = 3);
//****************************************Step
//Motor****************************************************************
void StepMotorInit();
void EscapeClose();
void EscapeOpen();
const int stepsPerRevolution = 100; // 기본세팅 200 AE탈장만 100으로 설정함
//****************************************SimpleTimer
//SETUP****************************************************************
SimpleTimer GameTimer;
SimpleTimer WifiTimer;
void TimerInit();
void WifiIntervalFunc();
void GameTimerFunc();
int wifiTimerId;
int gameTimerId;

//****************************************DFPlayer
//SETUP****************************************************************
HardwareSerial MP3Serial(2);
DFRobotDFPlayerMini myDFPlayer;
void Mp3_Setup();
enum { VE1 = 1, VE2, VE3, VE4, VE5 };
//****************************************Neopixel
//SETUP****************************************************************
void NeopixelInit();
void NeoBlink(int neo, int neoColor, int cnt, int blinkTime);
const int NumPixels[3] = {16, 60, 10};
const int NeopixelNum = 3;
enum { LINE = 0, ROUND, ONBOARD };
enum {
  WHITE = 0,
  RED,
  YELLOW,
  GREEN,
  BLUE,
  PURPLE,
  BLACK,
  BLUE0,
  BLUE1,
  BLUE2,
  BLUE3
};
// Neopixel 색상정보
int color[11][3] = {{20, 20, 20}, // WHITE
                    {40, 0, 0},   // RED
                    {40, 40, 0},  // YELLOW
                    {0, 40, 0},   // GREEN
                    {0, 0, 40},   // BLUE
                    {40, 0, 40},  // PURPLE
                    {0, 0, 0},    // BLACK
                    {0, 0, 20},   // ENCODERBLUE0
                    {0, 0, 40},   // ENCODERBLUE1
                    {0, 0, 60},   // ENCODERBLUE2
                    {0, 0, 80}};  // ENCODERBLUE3

const int neopixel_num = 3; // 설치된 네오픽셀의 개수

Adafruit_NeoPixel pixels[NeopixelNum] = {
    Adafruit_NeoPixel(NumPixels[LINE], LINE_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[ROUND], ROUND_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[ONBOARD], ONBOARD_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800)};

#endif
