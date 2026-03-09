void StepMotorInit(){
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(SW_PIN, INPUT_PULLUP);
}

// [HW_MOTOR_02] EscapeClose: SW_PIN HIGH 미감지 시 10초 후 강제 종료
void EscapeClose(){
    extern bool motorCloseTimeout;
    digitalWrite(DIR_PIN, LOW); // 모터 역방향
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Escapse Close");

    const unsigned long CLOSE_TIMEOUT_MS = 10000UL;
    unsigned long closeStart = millis();
    motorCloseTimeout = false;

    while(digitalRead(SW_PIN) == LOW)
    {
        if (millis() - closeStart > CLOSE_TIMEOUT_MS) {
            Serial.println("[HW_MOTOR_02] FAIL: EscapeClose timeout - SW_PIN never HIGH");
            motorCloseTimeout = true;
            break;
        }
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(2000);
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(2000);
    }
    if (!motorCloseTimeout) Serial.println("Close Finish");
}

// [HW_MOTOR_02] EscapeOpen: 총 스텝 수행 시간 10초 초과 시 강제 종료
void EscapeOpen(){
    extern bool motorOpenTimeout;
    digitalWrite(DIR_PIN, HIGH); // 모터 정방향
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Escapse Open");

    const unsigned long OPEN_TIMEOUT_MS = 10000UL;
    unsigned long openStart = millis();
    motorOpenTimeout = false;

    for(int x = 0; x < (stepsPerRevolution*10); x++)
    {
        if (millis() - openStart > OPEN_TIMEOUT_MS) {
            Serial.println("[HW_MOTOR_02] FAIL: EscapeOpen timeout");
            motorOpenTimeout = true;
            break;
        }
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(2000);
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(2000);
    }
    if (!motorOpenTimeout) Serial.println("Open Finish");
}
