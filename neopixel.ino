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
    for(int i = 0; i < cnt; i++){                          //0.5*10=5초동안 점멸
        pixels[neo].lightColor(color[BLACK]); //전체 off
        delay(blinkTime);            
        pixels[neo].lightColor(color[neoColor]); //전체 적색on
        delay(blinkTime);                   //전체 적색on
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