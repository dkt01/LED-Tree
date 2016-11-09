#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PERIODDIVISOR 16

class NeoPixelRing
{
  public:
    enum Pattern
    {
      SOLID = 0,
      PULSE = 1,
      PROGRESS = 2,
      SPIN = 3,
      RAINBOW = 4
    };

    // Constructors/destructors
    NeoPixelRing(uint8_t pinNum, neoPixelType npType, uint8_t nRings, const uint8_t* rings);
    ~NeoPixelRing();

    // Strip control functions
    void begin();
    void update();
    void setBrightness(uint8_t b);

    // Pattern control
    void setPattern(uint8_t ringNum,
                    Pattern p,
                    uint32_t color = 0,
                    uint16_t period = 2000, // 0.5 Hz
                    uint8_t param = 0);

  private:
    void SetSolid(uint8_t startPixel, uint8_t endPixel,
                  uint32_t color);
    void SetPulse(uint8_t startPixel, uint8_t endPixel,
                  uint32_t color, uint16_t period);
    void SetProgress(uint8_t startPixel, uint8_t endPixel,
                     uint32_t color, uint16_t period, uint8_t param);
    void SetSpin(uint8_t startPixel, uint8_t endPixel,
                 uint32_t color, uint16_t period);
    void SetRainbow(uint8_t startPixel, uint8_t endPixel,
                    uint16_t period);
    uint32_t CalcPulseColor(uint32_t color, uint16_t period, bool smooth = false,
                            uint32_t offColor = 0, uint16_t offset = 0);
    void SetFlash();
    void GenFlash(uint8_t index = 255);
    uint32_t Wheel(uint8_t pos);
    uint32_t GammaColor(uint32_t color);

    Adafruit_NeoPixel strip;
    uint8_t numRings;
    uint8_t* ringSizes;
    Pattern* ringPatterns;
    uint32_t* ringColors;
    uint16_t* ringPeriods;
    uint8_t* ringParams;
    uint8_t brightness;

    const uint16_t FLASH_PERIOD = 4000;
    const uint32_t FLASH_COLOR = 0xFFFF00;
    uint8_t numFlash;
    uint8_t* flashPixels;
    unsigned long flashStart;
    unsigned long now;

    uint8_t GAMMA[256];
};
