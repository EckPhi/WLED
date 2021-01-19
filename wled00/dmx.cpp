#include "wled.h"

/*
 * Support for DMX via MAX485.
 * Change the output pin in src/dependencies/ESPDMX.cpp if needed.
 * Library from:
 * https://github.com/Rickgg/ESP-Dmx
 */

#ifdef WLED_ENABLE_DMX
#ifndef ESP8266
static uint8_t dmxbuffer[DMX_MAX_FRAME];
static bool changed = true;
#endif // ESP32

static void applyColorsToDMX(void)
{
#ifdef ESP8266
  dmx.update(); // update the DMX bus
#else
  if (changed)
  {
    xSemaphoreTake(ESP32DMX.lxDataLock, portMAX_DELAY);
    for (int i = 1; i < DMX_MAX_FRAME; i++)
    {
      ESP32DMX.setSlot(i, dmxbuffer[i]);
    }
    changed = false;
    xSemaphoreGive(ESP32DMX.lxDataLock);
  }
#endif
  DEBUG_PRINTLN("Sent colors via DMX");
}

static void setDMX(uint16_t DMXAddr, byte value)
{
#ifdef ESP8266
  dmx.write(DMXAddr, value);
#else
  if (DMXAddr < DMX_MAX_FRAME && dmxbuffer[DMXAddr] != value)
  {
    dmxbuffer[DMXAddr] = value;
    changed = true;
  }
#endif
}

void handleDMX()
{
  // don't act, when in DMX Proxy mode
  if (e131ProxyUniverse != 0)
    return;

  uint8_t brightness = strip.getBrightness();
  static bool calcBrightness = true;

  // check if no shutter channel is set
  for (byte i = 0; i < DMXChannels; i++)
  {
    if (DMXFixtureMap[i] == 5)
      calcBrightness = false;
  }

  for (int i = DMXStartLED; i < ledCount; i++)
  { // uses the amount of LEDs as fixture count

    uint32_t in = strip.getPixelColor(i); // time to get the colors for the individual fixtures as suggested by AirCookie at issue #462
    byte w = in >> 24 & 0xFF;
    byte r = in >> 16 & 0xFF;
    byte g = in >> 8 & 0xFF;
    byte b = in & 0xFF;

    int DMXFixtureStart = DMXStart + (DMXGap * (i - DMXStartLED));
    for (int j = 0; j < DMXChannels; j++)
    {
      int DMXAddr = DMXFixtureStart + j;
      switch (DMXFixtureMap[j])
      {
      case 0: // Set this channel to 0. Good way to tell strobe- and fade-functions to fuck right off.
        setDMX(DMXAddr, 0);
        break;
      case 1: // Red
        setDMX(DMXAddr, calc_brightness ? (r * brightness) / 255 : r);
        break;
      case 2: // Green
        setDMX(DMXAddr, calc_brightness ? (g * brightness) / 255 : g);
        break;
      case 3: // Blue
        setDMX(DMXAddr, calc_brightness ? (b * brightness) / 255 : b);
        break;
      case 4: // White
        setDMX(DMXAddr, calc_brightness ? (w * brightness) / 255 : w);
        break;
      case 5: // Shutter channel. Controls the brightness.
        setDMX(DMXAddr, brightness);
        break;
      case 6: // Sets this channel to 255. Like 0, but more wholesome.
        setDMX(DMXAddr, 255);
        break;
      }
    }
  }

  applyColorsToDMX(); // apply new colors to the DMX bus
}

void initDMX()
{
#ifdef ESP8266
  dmx.init(512); // initialize with bus length
#else
  pinMode(DMXDirectionPin, OUTPUT);
  pinMode(DMXSerialOutputPin, OUTPUT);

  ESP32DMX.setDirectionPin(DMXDirectionPin);
  ESP32DMX.startOutput(DMXSerialOutputPin);
#endif
  DEBUG_PRINT("Initialized DMX output");
}

#if (LEDPIN == 2)
#pragma message "Pin conflict compiling with DMX and LEDs on pin 2. Please set a different LEDPIN."
#endif

#else
void handleDMX() {}
void initDMX() {}
#endif
