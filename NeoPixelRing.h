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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Updates all LEDs in accordance with the set parameters for each
    ///        ring.  All patterns are time-based, so the rate at which update()
    ///        is called will not change the speed of patterns.
    ////////////////////////////////////////////////////////////////////////////
    void update();
    void setBrightness(uint8_t b);

    // Pattern control

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Updates the displayed pattern for a given ring.  These changes
    ///        will be reflected on the next call to update().
    /// @param ringNum Index of the ring to update parameters.  If index is
    ///                invalid, no update will occur.
    /// @param p Pattern to display on the ring.
    /// @param color Color to use when generating pattern.  This may not be
    ///              used for all patterns.
    /// @param period Length of time for one complete pattern display loop in
    ///               milliseconds.  This may not be used for all patterns.
    /// @param param Miscellaneous parameter for pattern.  This may not be used
    ///              for all patterns.
    ////////////////////////////////////////////////////////////////////////////
    void setPattern(uint8_t ringNum,
                    Pattern p,
                    uint32_t color = 0,
                    uint16_t period = 2000, // 0.5 Hz
                    uint8_t param = 0);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Changes only the color of one ring.  These changes will be
    ///        reflected on the next call to update().
    /// @param ringNum Index of the ring to update parameters.  If index is
    ///                invalid, no update will occur.
    /// @param color Color to use when generating pattern.  This may not be
    ///              used for all patterns.
    ////////////////////////////////////////////////////////////////////////////
    void setColor(uint8_t ringNum,
                  uint32_t color);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Changes only the period of one ring.  These changes will be
    ///        reflected on the next call to update().
    /// @param ringNum Index of the ring to update parameters.  If index is
    ///                invalid, no update will occur.
    /// @param period Length of time for one complete pattern display loop in
    ///               milliseconds.  This may not be used for all patterns.
    ////////////////////////////////////////////////////////////////////////////
    void setPeriod(uint8_t ringNum,
                   uint16_t period);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Changes only the miscellaneous parameter for one ring.  These
    ///        changes will be reflected on the next call to update().
    /// @param ringNum Index of the ring to update parameters.  If index is
    ///                invalid, no update will occur.
    /// @param param Miscellaneous parameter for pattern.  This may not be used
    ///              for all patterns.
    ////////////////////////////////////////////////////////////////////////////
    void setParam(uint8_t ringNum,
                  uint8_t param);

  private:
    ////////////////////////////////////////////////////////////////////////////
    /// @brief Sets a set of LEDs to a constant color.
    /// @param startPixel The first pixel index to set.
    /// @param endPixel The last pixel index to set.
    /// @param color Color to set all pixels to.  This value will be gamma
    ///              corrected before displaying.
    ////////////////////////////////////////////////////////////////////////////
    void SetSolid(uint8_t startPixel, uint8_t endPixel,
                  uint32_t color);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Sets a set of LEDs to fade in to a given color and out to off.
    /// @param startPixel The first pixel index to set.
    /// @param endPixel The last pixel index to set.
    /// @param color Color to fade all pixels to.  This value will be gamma
    ///              corrected before displaying.
    /// @param period Milliseconds from off to the given color.  Full pulse
    ///               period is actually double this value.
    ////////////////////////////////////////////////////////////////////////////
    void SetPulse(uint8_t startPixel, uint8_t endPixel,
                  uint32_t color, uint16_t period);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Pulses a percentage of the LEDs while maintaining the remaining
    ///        LEDs at a dim value.
    /// @param startPixel The first pixel index to set.
    /// @param endPixel The last pixel index to set.
    /// @param color Color to fade all pixels to.  This value will be gamma
    ///              corrected before displaying.
    /// @param period Milliseconds from off to the given color.  Full pulse
    ///               period is actually double this value.
    /// @param param Percentage of LEDs to pulse.  Range is 0-100.  LEDs are
    ///              illuminated starting at startPixel index.
    ////////////////////////////////////////////////////////////////////////////
    void SetProgress(uint8_t startPixel, uint8_t endPixel,
                     uint32_t color, uint16_t period, uint8_t param);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Sets a set of LEDs to fade in to a given color and out to off.
    ///        LEDs in the range have a time offset from each other creating a
    ///        chase pattern around the ring.
    /// @param startPixel The first pixel index to set.
    /// @param endPixel The last pixel index to set.
    /// @param color Color to fade all pixels to.  This value will be gamma
    ///              corrected before displaying.
    /// @param period Milliseconds from off to the given color.  Full pulse
    ///               period is actually double this value.
    ///               The time for the pattern to propagate around the ring is
    ///               the period value.
    ////////////////////////////////////////////////////////////////////////////
    void SetSpin(uint8_t startPixel, uint8_t endPixel,
                 uint32_t color, uint16_t period);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Sets a set of LEDs to cycle through max saturation rainbow colors.
    ///        The Colors have an offset to create a chase pattern around the
    ///        ring.
    /// @param startPixel The first pixel index to set.
    /// @param endPixel The last pixel index to set.
    /// @param period Milliseconds to complete one rainbow cycle.
    ////////////////////////////////////////////////////////////////////////////
    void SetRainbow(uint8_t startPixel, uint8_t endPixel,
                    uint16_t period);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Calculate a color to use in the pulse sequence.  The pulse may
    ///        be a linear fade between any two colors in the RGB color space.
    /// @param color Color used as the 'on' color in the fade.
    /// @param period Milliseconds from 'on' to 'off' color.  Full sequence may
    ///               be double this value if smooth is set to true.
    /// @param smooth When true, there is first a linear fade from 'off' to 'on'
    ///               then a linear fade from 'on' to 'off'.  When false, there
    ///               is a linear fade from 'off' to 'on', then this repeats
    ///               without a transition.
    /// @param offset Time offset in milliseconds for the pattern.  Each pattern
    ///               starts when
    ///               ((ms since startup) + offset) % period == 0
    /// @return Color value matching parameters and current system time
    ////////////////////////////////////////////////////////////////////////////
    uint32_t CalcPulseColor(uint32_t color, uint16_t period, bool smooth = false,
                            uint32_t offColor = 0, uint16_t offset = 0);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Flashes random LEDs according to FLASH_COLOR and FLASH_PERIOD.
    ///        This produces a 'twinkling' effect on top of other patterns.
    ///        Calls GenFlash() as necessary to update flashing pixels.
    ////////////////////////////////////////////////////////////////////////////
    void SetFlash();

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Updates flashPixels to contain new random pixels to flash.
    ///        When selecting pixels, no two values will be identical
    ///        (i.e. flashPixels is a unique set of indices)
    /// @param index Index of flashPixels to update.  If set to 255, update all
    ///              values.
    ////////////////////////////////////////////////////////////////////////////
    void GenFlash(uint8_t index = 255);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Generates a value in a maximum saturation color wheel.  Adapted
    ///        from Adafruit NeoPixel library example code.
    /// @param pos Position in color wheel.  0 and 255 are adjacent, creating
    ///            a continuous wheel.
    ////////////////////////////////////////////////////////////////////////////
    uint32_t Wheel(uint8_t pos);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Converts a raw color to a gamma-corrected color.  This
    ///        adjusts for human perception of brightness.
    /// @param color Raw RGB color for conversion
    /// @return Converted color in 8-bit RGB.
    ////////////////////////////////////////////////////////////////////////////
    uint32_t GammaColor(uint32_t color);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Converts a gamma-corrected color to a raw color.  Using this in
    ///        conjunction with GammaColor() is lossy, but should yield visually
    ///        acceptable results.
    /// @param color Gamma-corrected RGB color for conversion
    /// @return Raw color in 8-bit RGB.
    ////////////////////////////////////////////////////////////////////////////
    uint32_t GammaInvColor(uint32_t color);

    Adafruit_NeoPixel strip;
    uint8_t numRings;
    uint8_t* ringSizes;
    Pattern* ringPatterns;
    uint32_t* ringColors;
    uint16_t* ringPeriods;
    uint8_t* ringParams;

    const uint16_t FLASH_PERIOD = 4000;
    const uint32_t FLASH_COLOR = 0xFFFF00;
    uint8_t numFlash;
    uint8_t* flashPixels;
    unsigned long flashStart;
    unsigned long now;

    uint8_t GAMMA[256];     ///< This is used to transform raw colors to gamma-corrected colors
                            ///  to compensate for brightness perception
    uint8_t GAMMA_INV[256]; ///< This is used to transform gamma-corrected colors to raw colors
};
