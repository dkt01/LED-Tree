#include <EtherCard.h>
#include <IPAddress.h>

#include <Adafruit_NeoPixel.h>
#include "NeoPixelRing.h"
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 9

// IP Address configuration
static byte myIP[] = {192,168,1,253};
static byte gwIP[] = {192,168,1,254}; // Not used

// MAC Address
static byte myMAC[] = {0x6,0x5,0x4,0x3,0x2,0x1};

// Send/receive buffer
byte Ethernet::buffer[500];

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
const uint8_t ringsArray[6] = { 32, 24, 16, 12, 8, 1 };
NeoPixelRing tree = NeoPixelRing(PIN, NEO_GRB + NEO_KHZ800, 6, ringsArray);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

//callback that prints received packets to the serial port
void udpDataReceived(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len){
  IPAddress src(src_ip[0],src_ip[1],src_ip[2],src_ip[3]);

  // Serial.print("dest_port: ");
  // Serial.println(dest_port);
  // Serial.print("src_port: ");
  // Serial.println(src_port);

  // Serial.print("src_ip: ");
  // ether.printIp(src_ip);

  uint8_t red;
  uint8_t green;
  uint8_t blue;
  NeoPixelRing::Pattern pattern;
  uint8_t param;

  // 0x0 : red value
  // 0x1 : green value
  // 0x2 : blue value
  // 0x3 : pattern
  // 0x4 : param
  if(len != 6)
  {
    // Serial.write("\nData not equal to 6 bytes. Received:");
    // Serial.println(data);
    // Serial.write("\nLength:");
    // Serial.print(len);
  }
  for(uint8_t i = 0; i < len; i++)
  {
    switch (i)
    {
        case 0:
          red = data[i];
          // Serial.write("\nRed:");
          // Serial.print(red);
          break;
        case 1:
          green = data[i];
          // Serial.write("\nGreen:");
          // Serial.print(green);
          break;
        case 2:
          blue = data[i];
          // Serial.write("\nBlue:");
          // Serial.print(blue);
          break;
        case 3:
          pattern = data[i];
          // Serial.write("\nPattern:");
          // Serial.print(pattern);
          break;
        case 4:
          param = data[i];
          // Serial.write("\nParam:");
          // Serial.print(param);
          break;
        default:
          break;
    }
  }

  uint32_t color = ((uint32_t)red)   << 16 |
                   ((uint32_t)green) << 8  |
                   ((uint32_t)blue)  << 0;

  for(uint8_t i = 0; i < (sizeof(ringsArray) - 1); i++)
  {
    tree.setPattern(i,
                    pattern,
                    color,
                    2000,
                    param);
  }
}

void setup()
{
  Serial.begin(9600);

  if(ether.begin(sizeof Ethernet::buffer, myMAC, 10) == 0)
  {
    Serial.println(F("Failed to access Ethernet controller"));
  }
  ether.staticSetup(myIP);

  ether.printIp("IP: ", ether.myip);
  ether.printIp("GW: ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  ether.udpServerListenOnPort(&udpDataReceived, 8733);

  tree.begin();
  tree.setBrightness(75);
  tree.update(); // Initialize all pixels to 'off'
  tree.setPattern(0,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(1,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(2,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(3,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(4,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(5,
                  NeoPixelRing::RAINBOW,
                  0x00FF00,
                  4000,
                  0);
}

void loop()
{
  // static uint8_t progress = 0;
  // static uint8_t loopCount = 0;
  ether.packetLoop(ether.packetReceive());
  tree.update();
  // if(loopCount % 30 == 0)
  // {
  //   Serial.write("\nHI");
  //   Serial.print(loopCount);
  // }
  // loopCount++;
}
