#include <EtherCard.h>
#include <IPAddress.h>

#include <Adafruit_NeoPixel.h>
#include "NeoPixelRing.h"
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 9
#define STANDALONE 1 // set to 0 to listen to external Jenkins monitor

#define DEBUG 0

#if STANDALONE
  #include <EEPROM.h>
  #include "config_html.h"

  #define DNS_RETRY_INTERVAL_MS 5000
  #define SERVER_POLL_INTERVAL_MS 10000

  #define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)
  #define BUFFERSIZE 900

  #define HTTP_SERVER_PORT 80 // Port to respond to HTTP config page requests on

#else
  #define STATIC 1
  #define BUFFERSIZE 500
#endif

uint8_t buildStatus;

#define PERSISTENT_MEMORY_VERSION 2
#define PERSISTENT_MEMORY_DOMAIN_ADDRESS 2
#define PERSISTENT_MEMORY_PORT_ADDRESS 4
#define PERSISTENT_MEMORY_ENDPOINT_ADDRESS 6
#define PERSISTENT_MEMORY_DATA_START 8

#define BUILDSTATUS_SUCCESS    0x01
#define BUILDSTATUS_UNSTABLE   0x02
#define BUILDSTATUS_FAILURE    0x04
#define BUILDSTATUS_OTHER      0x08
#define BUILDSTATUS_UNKNOWN    0x10
#define BUILDSTATUS_BUILDING   0x80

// IP Address configuration
#if STATIC
static byte myIP[] = {192,168,1,253};
static byte gwIP[] = {192,168,1,254}; // Not used
#endif

// MAC Address
static byte myMAC[] = {0x6,0x5,0x4,0x3,0x2,0x1};

// Send/receive buffer
byte Ethernet::buffer[BUFFERSIZE];

// Colors
#define COLOR_RED    0x00FF0000
#define COLOR_GREEN  0x0000FF00
#define COLOR_YELLOW 0x00FFBF00

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

#if STANDALONE == 0
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
#else

////////////////////////////////////////////////////////////////////////////////
///////////////////////////// HTTP Response Headers ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n";

const char http_BadRequest[] PROGMEM =
"HTTP/1.0 400 Bad Request\r\n"
"Content-Type: text/html\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1>";

////////////////////////////////////////////////////////////////////////////////
///////////////////////////// API Query Components /////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const char http_Get_Prefix[] PROGMEM =
"GET ";

const char http_Get_Middle[] PROGMEM =
" HTTP/1.0\r\nHost: ";

const char http_Get_Suffix[] PROGMEM =
"\r\nAccept: text/html\r\n\r\n";


////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// EEPROM Access ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

uint16_t GetEEPROMWord(uint16_t address)
{
  uint16_t retval = (EEPROM.read(address) & 0xFF);
  retval |= ((EEPROM.read(address + 1) & 0xFF) << 8);
  return retval;
}

void SetEEPROMWord(uint16_t address, uint16_t val)
{
  EEPROM.write(address, val & 0xFF);
  EEPROM.write(address + 1, (val >> 8) & 0xFF);
}

bool HaveURL()
{
  return GetEEPROMWord(PERSISTENT_MEMORY_DOMAIN_ADDRESS) >= PERSISTENT_MEMORY_DATA_START;
}

bool SaveURL(char* urlString)
{
  #if DEBUG
  Serial.println(F("Saving URL..."));
  #endif

  // Indices within urlString where newline delimiters are
  uint16_t divs[2];

  // Ensure delimiter at end of domain exists and domain string is longer than
  // four characters.  a.co style should be shortest domain.
  divs[0] = strcspn(urlString, "\n");
  if('\n' != urlString[divs[0]]
     || divs[0] < 4)
  {
    #if DEBUG
    Serial.println(F("Could not find first newline"));
    #endif
    return false;
  }

  // Ensure delimiter at end of port exists
  divs[1] = strcspn(urlString + divs[0] + 1, "\n");
  divs[1] += (divs[0] + 1);
  if('\n' != urlString[divs[1]])
  {
    #if DEBUG
    Serial.println(F("Could not find second newline"));
    Serial.print(F("First newline at "));
    Serial.println(divs[0],DEC);
    Serial.print(F("Second newline at "));
    Serial.println(divs[1],DEC);
    #endif
    return false;
  }

  // Ensure port string is at least one digit
  if(divs[1] - divs[0] < 2)
  {
    #if DEBUG
    Serial.println(F("Port not present"));
    #endif
    return false;
  }

  // Ensure an API endpoint string exists
  if('\0' == urlString[divs[1] + 1])
  {
    #if DEBUG
    Serial.println(F("API endpoint not present"));
    #endif
    return false;
  }

  // Extract and validate port
  uint16_t port = 0;
  uint8_t charVal = 0;
  for(uint16_t i = divs[0] + 1; i < divs[1]; i++)
  {
    // Validate character in range '0'-'9'
    charVal = urlString[i] - 48;
    if(charVal > 9)
    {
      #if DEBUG
      Serial.print(F("Invalid character "));
      Serial.println(charVal, DEC);
      #endif
      return false;
    }
    // Port is decimal ascii
    port *= 10;
    port += charVal;
    ether.hisport = port;
  }

  // Write pointer to domain string
  uint16_t eepromAddress = PERSISTENT_MEMORY_DATA_START;
  SetEEPROMWord(PERSISTENT_MEMORY_DOMAIN_ADDRESS, PERSISTENT_MEMORY_DATA_START);

  // Write domain string
  for(uint16_t i = 0; i < divs[0]; i++)
  {
    EEPROM.write(eepromAddress, urlString[i]);
    eepromAddress++;
  }
  // Write null stop character
  EEPROM.write(eepromAddress, '\0');
  eepromAddress++;

  // Write pointer to port
  SetEEPROMWord(PERSISTENT_MEMORY_PORT_ADDRESS, eepromAddress);

  // Write port
  SetEEPROMWord(eepromAddress, port);
  eepromAddress += 2;

  // Write pointer to API endpoint URL
  SetEEPROMWord(PERSISTENT_MEMORY_ENDPOINT_ADDRESS, eepromAddress);

  // Write API endpoint URL
  uint16_t i = divs[1] + 1;
  do
  {
    EEPROM.write(eepromAddress, urlString[i]);
    eepromAddress++;
    i++;
  } while(urlString[i] != '\0');
  // Write null stop character
  EEPROM.write(eepromAddress, '\0');

  // Indicate successful write
  return true;
}

uint16_t LoadURL(char* urlString)
{
  #if DEBUG
  Serial.println(F("Loading URL..."));
  #endif
  uint16_t totalLength = 0;

  // Check that a URL has been written
  if(!HaveURL())
  {
    #if DEBUG
    Serial.println(F("Don't have URL"));
    #endif
    urlString[0] = '\0';
    return 0;
  }

  // Output full formatted API URL string
  totalLength += LoadDomain(urlString);
  urlString[totalLength] = ':';
  totalLength++;
  totalLength += LoadPort(urlString + totalLength);
  totalLength += LoadEndpoint(urlString + totalLength);

  return totalLength;
}

uint16_t LoadDomain(char* destString)
{
  uint16_t domainSize = 0;
  uint8_t eepromByte = '\0';
  // Get start address for domain string
  uint16_t eepromAddress = GetEEPROMWord(PERSISTENT_MEMORY_DOMAIN_ADDRESS);

  // If start address is invalid, return empty string
  if(!HaveURL())
  {
    destString[0] = '\0';
    return domainSize;
  }

  // Load null-terminated string from EEPROM
  do
  {
    eepromByte = EEPROM.read(eepromAddress);
    destString[domainSize] = eepromByte;
    eepromAddress++;
    domainSize++;
  } while(eepromByte != '\0');

  // Return size not including null stop character
  return domainSize - 1;
}

uint16_t LoadPort()
{
  // Check if URL data is set
  if(!HaveURL())
  {
    return -1;
  }

  uint16_t port = GetEEPROMWord(GetEEPROMWord(PERSISTENT_MEMORY_PORT_ADDRESS));
  SetPort(port);
  return port;
}

uint16_t LoadPort(char* destString)
{
  // Check if URL data is set
  if(!HaveURL())
  {
    return -1;
  }

  uint16_t port = GetEEPROMWord(GetEEPROMWord(PERSISTENT_MEMORY_PORT_ADDRESS));
  return sprintf(destString, "%d", port);
}

uint16_t LoadEndpoint(char* destString)
{
  uint16_t endpointSize = 0;
  uint8_t eepromByte = '\0';
  // Get start address for endpoint string
  uint16_t eepromAddress = GetEEPROMWord(PERSISTENT_MEMORY_ENDPOINT_ADDRESS);

  // If start address is invalid, return empty string
  if(!HaveURL())
  {
    destString[0] = '\0';
    return endpointSize;
  }

  // Load null-terminated string from EEPROM
  do
  {
    eepromByte = EEPROM.read(eepromAddress);
    destString[endpointSize] = eepromByte;
    eepromAddress++;
    endpointSize++;
  } while(eepromByte != '\0');

  // Return size not including null stop character
  return endpointSize - 1;
}


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// API Query Helpers //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static uint16_t FillServerQuery(uint8_t sessionID)
{
  // Create API query in payload
  uint8_t* startPos = EtherCard::tcpOffset();
  uint16_t len = sizeof(http_Get_Prefix) - 1;
  memcpy_P(startPos, http_Get_Prefix, len);
  len += LoadEndpoint(startPos+len);
  memcpy_P(startPos + len, http_Get_Middle, sizeof(http_Get_Middle));
  len += sizeof(http_Get_Middle) - 1;
  len += LoadDomain(startPos + len);
  memcpy_P(startPos + len, http_Get_Suffix, sizeof(http_Get_Suffix));
  len += sizeof(http_Get_Suffix) - 1;
  return len;
}

static uint8_t ReceiveServerResponse( uint8_t sessionID,
                                      uint8_t flags,
                                      uint16_t offset,
                                      uint16_t length )
{
  // Reset port to enable http server
  SetPort(HTTP_SERVER_PORT);

  char* responseStart = (char*)(Ethernet::buffer + offset);
  Ethernet::buffer[offset+length] = '\0';
  #if DEBUG
  Serial.println(F("Request callback:"));
  #endif

  // Serial.println(F("Content:"));
  // Serial.println(responseStart);

  // Find beginning of JSON
  size_t jsonStartIdx = strcspn(responseStart, "{");
  responseStart += jsonStartIdx;
  if(*responseStart != '{')
  {
    #if DEBUG
    Serial.println(F("JSON not found!"));
    #endif
    return 0;
  }

  char* key = NULL;
  uint8_t keyLen = 0;
  char* value = NULL;
  uint8_t valueLen = 0;

  while(getKVPair(responseStart, key, keyLen, value, valueLen))
  {
    if(keyLen > 0 && valueLen > 0 && 0 == strncmp(key,"building",keyLen))
    {
      if(0 == strncmp(value,"true",valueLen))
      {
        // Set building flag
        buildStatus |= BUILDSTATUS_BUILDING;
        #if DEBUG
        Serial.println(F("Building!"));
        #endif
      }
      else
      {
        // Clear building flag
        uint8_t statusMask = ~BUILDSTATUS_BUILDING;
        buildStatus &= statusMask;
        #if DEBUG
        Serial.println(F("Not Building!"));
        #endif
      }
    }
    else if(keyLen > 0 && valueLen > 0 && 0 == strncmp(key,"result",keyLen))
    {
      if(0 == strncmp(value,"\"SUCCESS\"",valueLen))
      {
        // Clear all status flags except success and building
        uint8_t statusMask = BUILDSTATUS_BUILDING | BUILDSTATUS_SUCCESS;
        buildStatus &= statusMask;
        // Set success flag
        buildStatus |= BUILDSTATUS_SUCCESS;
        #if DEBUG
        Serial.println(F("Build: SUCCESS"));
        Serial.print(buildStatus);
        #endif
      }
      else if(0 == strncmp(value,"\"FAILURE\"",valueLen))
      {
        // Clear all status flags except failure and building
        uint8_t statusMask = BUILDSTATUS_BUILDING | BUILDSTATUS_FAILURE;
        buildStatus &= statusMask;
        // Set failure flag
        buildStatus |= BUILDSTATUS_FAILURE;
        #if DEBUG
        Serial.println(F("Build: FAILURE"));
        Serial.print(buildStatus);
        #endif
      }
      else if(0 == strncmp(value,"\"NOT_BUILT\"",valueLen))
      {
        // Clear all status flags except other and building
        uint8_t statusMask = BUILDSTATUS_BUILDING | BUILDSTATUS_OTHER;
        buildStatus &= statusMask;
        // Set other flag
        buildStatus |= BUILDSTATUS_OTHER;
        #if DEBUG
        Serial.println(F("Build: NOT_BUILT"));
        Serial.print(buildStatus);
        #endif
      }
      else if(0 == strncmp(value,"\"ABORTED\"",valueLen))
      {
        // Clear all status flags except other and building
        uint8_t statusMask = BUILDSTATUS_BUILDING | BUILDSTATUS_OTHER;
        buildStatus &= statusMask;
        // Set other flag
        buildStatus |= BUILDSTATUS_OTHER;
        #if DEBUG
        Serial.println(F("Build: ABORTED"));
        Serial.print(buildStatus);
        #endif
      }
      else if(0 == strncmp(value,"\"UNSTABLE\"",valueLen))
      {
        // Clear all status flags except failure and building
        uint8_t statusMask = BUILDSTATUS_BUILDING | BUILDSTATUS_UNSTABLE;
        buildStatus &= statusMask;
        // Set unstable flag
        buildStatus |= BUILDSTATUS_UNSTABLE;
        #if DEBUG
        Serial.println(F("Build: UNSTABLE"));
        Serial.print(buildStatus);
        #endif
      }
      else
      {
        // Do nothing and maintain previous status
        #if DEBUG
        Serial.println(F("Build: OTHER"));
        Serial.print(buildStatus);
        #endif
      }
    }
    responseStart = value + valueLen + 1;
  }
  return 0; // Not used for anything
}

bool getKVPair(char* responseStart, char*& key, uint8_t& keyLen, char*& value, uint8_t& valueLen)
{
  // Every key should start with a double quote and end with a double quote
  size_t findOffset = strcspn(responseStart, "\"");
  if(responseStart[findOffset] != '\"')
  {
    // No key found
    return false;
  }
  // Key is always a string, so don't include the first double quote
  key = responseStart + findOffset + 1;
  responseStart = key;

  findOffset = strcspn(responseStart, "\"");
  if(responseStart[findOffset] != '\"')
  {
    // No key end found
    return false;
  }
  keyLen = findOffset;
  responseStart += findOffset;

  // A colon will always appear immediately before the value
  findOffset = strcspn(responseStart, ":");
  if(responseStart[findOffset] != ':')
  {
    // No KV pair separator found
    return false;
  }
  value = responseStart + findOffset + 1;
  responseStart = value;

  // Comma separates keys, end brace indicates end of selection
  findOffset = strcspn(responseStart, ",}");
  if(responseStart[findOffset] != ',' &&
     responseStart[findOffset] != '}')
  {
    // No value end found
    return false;
  }
  valueLen = findOffset;
  return true;
}

void updatePatterns(NeoPixelRing& lTree, uint8_t status)
{
  uint32_t illuminateColor = 0;
  if(status & BUILDSTATUS_SUCCESS)
  {
    illuminateColor = COLOR_GREEN;
  }
  else if(status & BUILDSTATUS_FAILURE)
  {
    illuminateColor = COLOR_RED;
  }
  else
  {
    illuminateColor = COLOR_YELLOW;
  }
  // While building, pulse light corresponding to previous build status
  if(status & BUILDSTATUS_BUILDING)
  {
    for(uint8_t i = 0; i < (lTree.getNumRings() - 1); i++)
    {
      lTree.setPattern(i,
                       NeoPixelRing::SPIN,
                       illuminateColor,
                       2000,
                       0);
    }
  }
  else
  {
    // Unknown flash all lights
    if(status & BUILDSTATUS_UNKNOWN)
    {
      for(uint8_t i = 0; i < (lTree.getNumRings() - 1); i++)
      {
        lTree.setPattern(i,
                         NeoPixelRing::PULSE,
                         illuminateColor,
                         2000,
                         0);
      }
    }
    // Solid corresponding to build status
    else
    {
      for(uint8_t i = 0; i < (lTree.getNumRings() - 1); i++)
      {
        lTree.setPattern(i,
                         NeoPixelRing::SOLID,
                         illuminateColor,
                         2000,
                         0);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Sets new TCP port for ethercard library.  This is the port used for
///        responses and requests
/// @param newPort New port to listen on/send to
////////////////////////////////////////////////////////////////////////////////
void SetPort(uint16_t newPort)
{
  ether.hisport = newPort;
}
#endif //STANDALONE

void setup()
{
  Serial.begin(9600);

  if(ether.begin(sizeof Ethernet::buffer, myMAC, 10) == 0)
  {
    #if DEBUG
    Serial.println(F("Failed to access Ethernet controller"));
    #endif
  }

#if STATIC
  ether.staticSetup(myIP);
#else
  if(false == ether.dhcpSetup())
  {
    #if DEBUG
    Serial.println(F("DHCP failed"));
    #endif
  }
#endif

  #if DEBUG
  ether.printIp("IP: ", ether.myip);
  ether.printIp("GW: ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
  #endif

#if STANDALONE == 0
  ether.udpServerListenOnPort(&udpDataReceived, 8733);
#endif

  tree.begin();
  tree.setBrightness(75);
  tree.update(); // Initialize all pixels to 'off'
  tree.setPattern(0,
                  NeoPixelRing::SPIN,
                  COLOR_GREEN,
                  2000,
                  0);
  tree.setPattern(1,
                  NeoPixelRing::SPIN,
                  COLOR_GREEN,
                  2000,
                  0);
  tree.setPattern(2,
                  NeoPixelRing::SPIN,
                  COLOR_YELLOW,
                  2000,
                  0);
  tree.setPattern(3,
                  NeoPixelRing::SPIN,
                  COLOR_YELLOW,
                  2000,
                  0);
  tree.setPattern(4,
                  NeoPixelRing::SPIN,
                  COLOR_RED,
                  2000,
                  0);
  tree.setPattern(5,
                  NeoPixelRing::RAINBOW,
                  COLOR_RED,
                  4000,
                  0);
}

void loop()
{
  word pos = ether.packetLoop(ether.packetReceive());

#if STANDALONE
  static bool pendingPut = false;
  bool generalMemCopy = true;

  static bool haveDNS = false;
  static long lastDNSLookup = 0;
  static long lastServerPoll = 0;

  if (pos)
  {
    uint16_t startPoint = 0; // Multi-packet response
    int sz = BUFFERSIZE - pos;
    char* data = (char*) Ethernet::buffer + pos;
    char* sendData = NULL;
    bool complete = false;

    #if DEBUG
    Serial.println(pos);
    #endif

    ether.httpServerReplyAck();

    if (strncmp("GET / ", data, 6) == 0)
    {
      pendingPut = false;

      #if DEBUG
      Serial.println(F("GET /:"));
      Serial.println(data);
      #endif
      sendData = const_cast<char*>(config_html);
      if(sizeof(config_html) < sz)
      {
        sz = sizeof(config_html);
      }
      else
      {
        do
        {
          memcpy_P(data, sendData + startPoint, sz); // Copy data from flash to RAM
          ether.httpServerReply_with_flags(sz, TCP_FLAGS_ACK_V);
          startPoint += sz;
          sz = BUFFERSIZE - pos;
          if(sizeof(config_html) - startPoint < sz)
          {
            sz = sizeof(config_html) - startPoint;
            complete = true;
          }
        } while (false == complete);
        sendData += startPoint;
      }
    }
    // else if (strncmp("GET /favicon.ico", data, 16) == 0)
    // {
    //   pendingPut = false;

    //   Serial.println(F("GET /favicon.ico:"));
    //   Serial.println(data);
    //   sendData = const_cast<char*>(favicon_ico);
    //   if(sizeof(favicon_ico) < sz)
    //   {
    //     sz = sizeof(favicon_ico);
    //     complete = true;
    //   }
    //   else
    //   {
    //     do
    //     {
    //       memcpy_P(data, sendData + startPoint, sz); // Copy data from flash to RAM
    //       ether.httpServerReply_with_flags(sz, TCP_FLAGS_ACK_V);
    //       startPoint += sz;
    //       sz = BUFFERSIZE - pos;
    //       if(sizeof(favicon_ico) - startPoint < sz)
    //       {
    //         sz = sizeof(favicon_ico) - startPoint;
    //         complete = true;
    //       }
    //     } while (false == complete);
    //     sendData += startPoint;
    //   }
    // }
    else if (strncmp("GET /apiURL", data, 10) == 0)
    {
      pendingPut = false;

      #if DEBUG
      Serial.println(F("GET /apiURL:"));
      Serial.println(data);
      #endif
      generalMemCopy = false; // Doing memcopy here to build response
      sz = sizeof(http_OK);
      memcpy_P(data, http_OK, sz);
      uint16_t urlSize = 0;
      urlSize = LoadURL(data+sz-1); // Start at null character from header string
      sz += (urlSize-1); // Don't send null characters
    }
    else if (strncmp("PUT /apiURL", data, 10) == 0)
    {
      pendingPut = false;

      #if DEBUG
      Serial.println(F("PUT /apiURL:"));
      Serial.println(data);
      #endif
      char* headerEnd = strstr(data, "\r\n\r\n");
      #if DEBUG
      Serial.print(F("After Header: "));
      Serial.println(headerEnd + 4);
      #endif
      if(SaveURL(headerEnd + 4))
      {
        sendData = const_cast<char*>(http_OK);
        sz = sizeof(http_OK);
        haveDNS = false;
        #if DEBUG
        Serial.println(F("Pending DNS"));
        #endif
        tree.setPattern(0,
                        NeoPixelRing::SOLID,
                        COLOR_GREEN,
                        1000,
                        0);
        tree.setPattern(1,
                        NeoPixelRing::SOLID,
                        COLOR_GREEN,
                        1000,
                        0);
        tree.setPattern(2,
                        NeoPixelRing::PULSE,
                        COLOR_YELLOW,
                        1000,
                        0);
        tree.setPattern(3,
                        NeoPixelRing::PULSE,
                        COLOR_YELLOW,
                        1000,
                        0);
        tree.setPattern(4,
                        NeoPixelRing::SOLID,
                        COLOR_RED,
                        1000,
                        0);
      }
      else if('\0' == *(headerEnd + 4))
      {
        // 2-part put message
        #if DEBUG
        Serial.println(F("Pending PUT"));
        #endif
        sendData = const_cast<char*>(http_OK);
        sz = sizeof(http_OK);
        pendingPut = true;
      }
      else
      {
        sendData = const_cast<char*>(http_BadRequest);
        sz = sizeof(http_BadRequest);
      }
    }
    else if(true == pendingPut)
    {
      pendingPut = false;

      #if DEBUG
      Serial.println(F("PUT pt2"));
      Serial.println(data);
      #endif
      if(SaveURL(data))
      {
        sendData = const_cast<char*>(http_OK);
        sz = sizeof(http_OK);
        haveDNS = false;
        #if DEBUG
        Serial.println(F("Pending DNS"));
        #endif
        tree.setPattern(0,
                        NeoPixelRing::SOLID,
                        COLOR_GREEN,
                        1000,
                        0);
        tree.setPattern(1,
                        NeoPixelRing::SOLID,
                        COLOR_GREEN,
                        1000,
                        0);
        tree.setPattern(2,
                        NeoPixelRing::PULSE,
                        COLOR_YELLOW,
                        1000,
                        0);
        tree.setPattern(3,
                        NeoPixelRing::PULSE,
                        COLOR_YELLOW,
                        1000,
                        0);
        tree.setPattern(4,
                        NeoPixelRing::SOLID,
                        COLOR_RED,
                        1000,
                        0);
      }
      else
      {
        #if DEBUG
        Serial.println(F("Not PUT pt2!"));
        #endif
        sendData = const_cast<char*>(http_Unauthorized);
        if(sizeof(http_Unauthorized) < sz)
        {
          sz = sizeof(http_Unauthorized);
        }
      }
    }
    else
    {
      // Page not found
      #if DEBUG
      Serial.println(F("???:"));
      Serial.println(data);
      #endif
      sendData = const_cast<char*>(http_Unauthorized);
      if(sizeof(http_Unauthorized) < sz)
      {
        sz = sizeof(http_Unauthorized);
      }
    }

    // Send http response
    if(generalMemCopy)
    {
      memcpy_P(data, sendData, sz); // Copy data from flash to RAM
    }
    // ether.httpServerReply(sz-1);
    ether.httpServerReply_with_flags(sz,TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V);
    ether.httpServerReply_with_flags(0,TCP_FLAGS_ACK_V|TCP_FLAGS_FIN_V);
    startPoint = 0;
  }

  if( !haveDNS
      && HaveURL()
      && (millis() - lastDNSLookup) > DNS_RETRY_INTERVAL_MS
      && ether.isLinkUp() )
  {
    lastDNSLookup = millis();
    LoadDomain(Ethernet::buffer+60);
    if(!ether.dnsLookup(Ethernet::buffer+60,true))
    {
      #if DEBUG
      Serial.println(F("DNS lookup failed"));
      #endif
    }
    else
    {
      #if DEBUG
      ether.printIp(F("Server: "), ether.hisip);
      #endif
      haveDNS = true;
      tree.setPattern(0,
                      NeoPixelRing::PULSE,
                      COLOR_GREEN,
                      1000,
                      0);
      tree.setPattern(1,
                      NeoPixelRing::PULSE,
                      COLOR_GREEN,
                      1000,
                      0);
      tree.setPattern(2,
                      NeoPixelRing::PULSE,
                      COLOR_YELLOW,
                      1000,
                      0);
      tree.setPattern(3,
                      NeoPixelRing::PULSE,
                      COLOR_YELLOW,
                      1000,
                      0);
      tree.setPattern(4,
                      NeoPixelRing::PULSE,
                      COLOR_RED,
                      1000,
                      0);
    }
  }

  if( haveDNS
      && HaveURL()
      && (millis() - lastServerPoll) > SERVER_POLL_INTERVAL_MS )
  {
    ether.clientTcpReq(ReceiveServerResponse, FillServerQuery, LoadPort());
    updatePatterns(tree, buildStatus);
    lastServerPoll = millis();
  }
#endif

  tree.update();
}
