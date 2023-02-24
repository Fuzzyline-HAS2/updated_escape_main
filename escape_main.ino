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

#include "escape_main.h""

void setup() {
    Serial.begin(115200);
    toSubSerial.begin(115200, SERIAL_8N1, HWSERIAL_RX, HWSERIAL_TX);
    has2wifi.Setup("badland");
    NeopixelInit();
    TimerInit();
    Mp3_Setup();
    StepMotorInit();
    pinMode(RELAY_PIN, OUTPUT);
<<<<<<< HEAD
    digitalWrite(RELAY_PIN, HIGH);
    // has2wifi.Setup("KT_GiGA_6C64","ed46zx1198");
    // has2wifi.Setup();
    has2wifi.Setup("badland");
=======
    // has2wifi.Setup("KT_GiGA_6C64","ed46zx1198");
    // has2wifi.Setup();
>>>>>>> d31204e303817acebfe47c1ea6e36b1905195a46
    DataChanged();
    toSubSerial.println("R"); 
    toSubSerial.println("R");
    toSubSerial.println("R");
}
void loop() {
    WifiTimer.run();
    GameTimer.run();
}
