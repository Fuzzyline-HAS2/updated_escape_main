#ifndef _DONE_ESCAPE_MAIN_CODE_
#define _DONE_ESCAPE_MAIN_CODE_

#include "Library_and_pin.h"
#include "QC/QC_Engine.h"
#include "QC/QC_Rules.h"
//****************************************WIFI****************************************************************
HAS2_Wifi has2wifi("http://172.30.1.44");
SecureOTA ota(
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_escape_main/main/update.bin",
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_escape_main/main/version.txt",
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/updated_escape_main/main/update.sig",
    HMAC_SECRET,
    FIRMWARE_VER
);
void DataChanged();
void SettingFunc(void);
void ActivateFunc(void);
void ReadyFunc(void);
void WaitFunc();
void WifiIntervalLoop(unsigned long intervalValue);
unsigned long wifiInterval = 0;
//****************************************Fake/Tagger
//Device****************************************************************
void FakeDeviceFunc(int tagTrack); // "fake"(가짜 탈출장치) / "tagger"(봉쇄된 탈출장치) 진입 공용 처리
void FakeTagCheck();               // PN532 태그 물리 감지 시 안내 오디오 1회 재생 (엣지 트리거)
int fakeTagTrack = 8;              // 8 = fake 안내, 9 = tagger 안내
bool fakeTagPresent = false;       // 직전 폴링에서 태그가 있었는지 (엣지 감지용)
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
String lastBeetleRawPacket = ""; // LOGIC_SERIAL_02: 마지막 수신 T 패킷 원문
int invalidCmdCount = 0;         // LOGIC_SERIAL_03: 허용되지 않은 명령 수신 횟수
int packetFormatErrorCount = 0;  // LOGIC_SERIAL_02: T 패킷 포맷 오류 누적
int tagParseErrorCount = 0;      // LOGIC_TAG_02: 태그 파싱 실패 누적
uint8_t beetleBadEventStreak = 0;   // 연속 bad-event 사이클 수 (silence 제외)
uint8_t beetleRecoverAttempts = 0;  // Beetle UART 복구 시도 횟수
//****************************************Recovery
//Functions****************************************************************
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
bool dfPlayerOk = false;
enum { VE1 = 1, VE2, VE3, VE4, VE5 };
//****************************************Neopixel
//SETUP****************************************************************
void NeopixelInit();
void NeoBlink(int neo, int neoColor, int cnt, int blinkTime);
void AllNeoOn(int neoColor);
void UpdateBrightness();
#define DEFAULT_BRIGHTNESS 50
int ledBrightness = DEFAULT_BRIGHTNESS;
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
int currentNeoColor = WHITE;
const int NumPixels[3] = {16, 60, 10};
const int NeopixelNum = 3;
// Neopixel 색상정보
int color[11][3] = {{255, 255, 255}, // WHITE
                    {255, 0,   0  }, // RED
                    {255, 255, 0  }, // YELLOW
                    {0,   255, 0  }, // GREEN
                    {0,   0,   255}, // BLUE
                    {255, 0,   255}, // PURPLE
                    {0,   0,   0  }, // BLACK
                    {0,   0,   64 }, // ENCODERBLUE0
                    {0,   0,   128}, // ENCODERBLUE1
                    {0,   0,   192}, // ENCODERBLUE2
                    {0,   0,   255}}; // ENCODERBLUE3

const int neopixel_num = 3; // 설치된 네오픽셀의 개수

Adafruit_NeoPixel pixels[NeopixelNum] = {
    Adafruit_NeoPixel(NumPixels[LINE], LINE_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[ROUND], ROUND_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[ONBOARD], ONBOARD_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800)};

#endif
