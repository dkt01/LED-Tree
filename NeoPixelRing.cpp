#include "NeoPixelRing.h"

NeoPixelRing::NeoPixelRing(uint8_t pinNum,
                           neoPixelType npType,
                           uint8_t nRings,
                           const uint8_t* rings)
{
  numRings = nRings;
  ringSizes = new uint8_t [numRings];
  for(uint8_t i = 0; i < numRings; i++)
  {
    ringSizes[i] = rings[i];
  }
  uint8_t totalLength = 0;
  for(uint8_t i = 0; i < numRings; i++)
  {
    totalLength += ringSizes[i];
  }
  ringPatterns = new Pattern [numRings];
  ringColors = new uint32_t [numRings];
  ringPeriods = new uint16_t [numRings];
  ringParams = new uint8_t [numRings];

  if(totalLength > 7)
  {
    numFlash = totalLength >> 3; // Divide by 8
  }
  else
  {
    numFlash = 1;
  }
  flashPixels = new uint8_t [numFlash];

  strip = Adafruit_NeoPixel(totalLength, pinNum, npType);

  // Gamma correction for x is:
  // ( x / MAXVAL )^2.5 * MAXVAL
  for(int i = 0; i < 256; i++)
  {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    GAMMA[i] = x;
  }
}

NeoPixelRing::~NeoPixelRing()
{
  if(ringSizes!= NULL)
    delete [] ringSizes;
  if(ringPatterns!= NULL)
    delete [] ringPatterns;
  if(ringColors!= NULL)
    delete [] ringColors;
  if(ringPeriods!= NULL)
    delete [] ringPeriods;
  if(ringParams!= NULL)
    delete [] ringParams;
  if(flashPixels!= NULL)
    delete [] flashPixels;
}

void NeoPixelRing::begin()
{
  strip.begin();
}

void NeoPixelRing::update()
{
  now = millis();
  uint8_t start = 0;
  for(uint8_t i = 0; i < numRings; i++)
  {
    uint8_t end = start + ringSizes[i] - 1;
    switch(ringPatterns[i])
    {
      case SOLID:
        SetSolid(start, end, ringColors[i]);
        break;
      case PULSE:
        SetPulse(start, end, ringColors[i], ringPeriods[i]);
        break;
      case PROGRESS:
        SetProgress(start, end, ringColors[i], ringPeriods[i], ringParams[i]);
        break;
      case SPIN:
        SetSpin(start, end, ringColors[i], ringPeriods[i]);
        break;
      case RAINBOW:
        SetRainbow(start, end, ringPeriods[i]);
        break;
      default:
        SetSolid(start, end, 0);
    }
    start += ringSizes[i];
  }

  SetFlash();

  strip.show();
}

void NeoPixelRing::setBrightness(uint8_t b)
{
  strip.setBrightness(b);
}

void NeoPixelRing::setPattern(uint8_t ringNum,
                              Pattern p,
                              uint32_t color,
                              uint16_t period,
                              uint8_t param)
{
  if(ringNum < numRings)
  {
    ringPatterns[ringNum] = p;
    ringColors[ringNum] = color;
    ringPeriods[ringNum] = period;
    ringParams[ringNum] = param;
  }
}

void NeoPixelRing::setColor(uint8_t ringNum, uint32_t color)
{
  if(ringNum < numRings)
  {
    ringColors[ringNum] = color;
  }
}

void NeoPixelRing::setPeriod(uint8_t ringNum, uint16_t period)
{
  if(ringNum < numRings)
  {
    ringPeriods[ringNum] = period;
  }
}

void NeoPixelRing::setParam(uint8_t ringNum, uint8_t param)
{
  if(ringNum < numRings)
  {
    ringParams[ringNum] = param;
  }
}

void NeoPixelRing::SetSolid(uint8_t startPixel,
                            uint8_t endPixel,
                            uint32_t color)
{
  for(uint8_t i = startPixel; i <= endPixel; i++)
  {
    strip.setPixelColor(i, GammaColor(color));
  }
}

void NeoPixelRing::SetPulse(uint8_t startPixel, uint8_t endPixel,
                            uint32_t color, uint16_t period)
{
  uint32_t pulseColor = CalcPulseColor(color, period, true);
  for(uint8_t i = startPixel; i <= endPixel; i++)
  {
    strip.setPixelColor(i, GammaColor(pulseColor));
  }
}

void NeoPixelRing::SetProgress(uint8_t startPixel, uint8_t endPixel,
                              uint32_t color, uint16_t period, uint8_t param)
{
  uint8_t length = endPixel - startPixel + 1;
  uint16_t numProgress = length * param;
  numProgress /= 100;
  numProgress += startPixel;
  uint8_t minRed = ((color >> 16) & 0xFF) / 4;
  uint8_t minGreen = ((color >> 8) & 0xFF) / 4;
  uint8_t minBlue = (color & 0xFF) / 4;
  uint32_t minColor = strip.Color(minRed, minGreen, minBlue);
  uint32_t pulseColor = CalcPulseColor(color, period, true, minColor);
  for(uint8_t i = startPixel; i < numProgress; i++)
  {
    strip.setPixelColor(i, GammaColor(pulseColor));
  }
  for(uint8_t i = numProgress; i <= endPixel; i++)
  {
    strip.setPixelColor(i, GammaColor(minColor));
  }
}

void NeoPixelRing::SetSpin(uint8_t startPixel, uint8_t endPixel,
                           uint32_t color, uint16_t period)
{
  uint8_t length = endPixel - startPixel + 1;
  uint16_t offsetPerPixel = period / length;
  for(uint8_t i = startPixel; i <= endPixel; i++)
  {
    uint16_t offset = offsetPerPixel * (i - startPixel);
    uint32_t pixelColor = CalcPulseColor(color, period, true, 0, offset);
    strip.setPixelColor(i, GammaColor(pixelColor));
  }
}

void NeoPixelRing::SetRainbow(uint8_t startPixel, uint8_t endPixel,
                              uint16_t period)
{
  period /= PERIODDIVISOR;
  unsigned long condensedNow = now / PERIODDIVISOR;
  uint16_t curPhase = condensedNow % period;
  uint32_t pos = 255 * curPhase;
  pos /= period;
  for(uint8_t i = startPixel; i <= endPixel; i++)
  {
    strip.setPixelColor(i, GammaColor(Wheel(i+pos)));
  }
}

void NeoPixelRing::SetFlash()
{
  static bool first = true;
  static uint8_t lastUpdate = 0;
  if(first)
  {
    // On first update, generate all flashing pixels
    GenFlash();
    first = false;
  }
  if((now - flashStart) >= FLASH_PERIOD)
  {
    // Start of period may have been in the past
    flashStart = now - (now % FLASH_PERIOD);
    GenFlash(0);
    lastUpdate = numFlash;
  }

  uint16_t offsetPerPixel = FLASH_PERIOD/numFlash;

  for(int16_t i = numFlash-1; i >= 0; i--)
  {
    uint16_t curOffset = offsetPerPixel * i;

    if( (now-flashStart) > (FLASH_PERIOD - curOffset) &&
        lastUpdate == (uint8_t)(i + 1) )
    {
      GenFlash(i);
      lastUpdate = i;
    }

    uint32_t flashColor = CalcPulseColor(GammaColor(FLASH_COLOR),
                                         FLASH_PERIOD/2,
                                         true,
                                         strip.getPixelColor(flashPixels[i]),
                                         curOffset);

    strip.setPixelColor(flashPixels[i], flashColor);
  }
}

void NeoPixelRing::GenFlash(uint8_t index)
{
  int curIndex = 0;
  uint8_t flashCheck = 0;
  uint8_t numPixels = strip.numPixels();
  if(index < numFlash)
  {
    curIndex = index;
  }
  while(curIndex < numFlash &&
        curIndex <= index)
  {
    // Don't flash the last pixel (tip of cone)
    if(numPixels > 1)
    {
      flashCheck = random(numPixels-1);
    }
    else
    {
      flashCheck = 0;
    }

    bool repeat = false;
    // Check if pixel has already been selected
    // If regenerating all, only check up to current index
    // If regenerating one pixel, check all values
    int checkIndex = curIndex;
    if(index < numFlash)
    {
      checkIndex = numFlash;
    }
    for(int i = 0; i < checkIndex; i++)
    {
      if(flashPixels[i] == flashCheck &&
         i != index)
      {
        repeat = true;
        break;
      }
    }

    // If repeated, get a new pixel index
    if(repeat)
    {
      continue;
    }

    // Save pixel index
    flashPixels[curIndex] = flashCheck;
    curIndex++;
  }
}

uint32_t NeoPixelRing::CalcPulseColor(uint32_t color, uint16_t period, bool smooth,
                                      uint32_t offColor, uint16_t offset)
{
  bool invert = false;
  period /= PERIODDIVISOR;
  offset /= PERIODDIVISOR;
  unsigned long condensedNow = now / PERIODDIVISOR;
  if(smooth)
  {
    invert = ((condensedNow + offset) % (2 * period)) !=
             ((condensedNow + offset) % period);
  }
  uint16_t curPhase = ((condensedNow + offset) % period);
  if(invert)
  {
    curPhase = period - curPhase;
  }
  uint16_t redRange = ((color >> 16) & 0xFF) - ((offColor >> 16) & 0xFF);
  uint32_t red = curPhase * redRange;
  red /= period;
  red += ((offColor >> 16) & 0xFF);
  uint16_t greenRange = ((color >> 8) & 0xFF) - ((offColor >> 8) & 0xFF);
  uint32_t green = curPhase * greenRange;
  green /= period;
  green += ((offColor >> 8) & 0xFF);
  uint16_t blueRange = (color & 0xFF) - (offColor & 0xFF);
  uint32_t blue = curPhase * blueRange;
  blue /= period;
  blue += (offColor & 0xFF);
  return strip.Color(red,green,blue);
}

uint32_t NeoPixelRing::Wheel(uint8_t WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t NeoPixelRing::GammaColor(uint32_t color)
{
  uint8_t red = (color >> 16) & 0xFF;
  red = GAMMA[red];
  uint8_t green = (color >> 8) & 0xFF;
  green = GAMMA[green];
  uint8_t blue = color & 0xFF;
  blue = GAMMA[blue];
  uint32_t outColor = ((uint32_t)red   << 16) |
                      ((uint32_t)green << 8)  |
                      ((uint32_t)blue);
  return outColor;
}