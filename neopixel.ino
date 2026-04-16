void NeopixelInit()
{
  for (int i = 0; i < NeopixelNum; ++i) {
    pixels[i].begin();
    pixels[i].setBrightness(neoGlobalBrightness);
  }

  for (int i = 0; i < NeopixelNum; ++i) {
    for (int j = 0; j < NumPixels[i]; j++)
      pixels[i].setPixelColor(j, pixels[i].Color(color[WHITE][0], color[WHITE][1], color[WHITE][2]));
    pixels[i].show();
  }
}

void ApplyBrightness(int serverVal) {
    if (serverVal <= 0 || serverVal > 100) return; // 범위 벗어나면 디폴트 유지
    neoGlobalBrightness = (uint8_t)((serverVal * 255) / 100);
    for (int i = 0; i < NeopixelNum; ++i)
        pixels[i].setBrightness(neoGlobalBrightness);
    AllNeoOn(currentNeoColor); // 변경된 밝기로 현재 색상 재설정
}

void NeoBlink(int neo, int neoColor, int cnt, int blinkTime){
    for(int i = 0; i < cnt; i++){
        for(int j = 0; j < NumPixels[neo]; j++)
            pixels[neo].setPixelColor(j, pixels[neo].Color(color[BLACK][0], color[BLACK][1], color[BLACK][2]));
        pixels[neo].show();
        delay(blinkTime);
        for(int j = 0; j < NumPixels[neo]; j++)
            pixels[neo].setPixelColor(j, pixels[neo].Color(color[neoColor][0], color[neoColor][1], color[neoColor][2]));
        pixels[neo].show();
        delay(blinkTime);
    }
}

void AllNeoOn(int neoColor){
    currentNeoColor = neoColor;
    for (int i = 0; i < NeopixelNum; ++i) {
        for (int j = 0; j < NumPixels[i]; j++)
            pixels[i].setPixelColor(j, pixels[i].Color(color[neoColor][0], color[neoColor][1], color[neoColor][2]));
        pixels[i].show();
    }
}

void LineNeoOn(int changeColr, int baseColor, int cnt){
  for(int i = 0; i < NumPixels[LINE]; i++)
    pixels[LINE].setPixelColor(i,pixels[LINE].Color(color[baseColor][0],color[baseColor][1],color[baseColor][2]));
  for(int i = 0; i < cnt; i++)
    pixels[LINE].setPixelColor(i,pixels[LINE].Color(color[changeColr][0],color[changeColr][1],color[changeColr][2]));
  pixels[LINE].show();
}