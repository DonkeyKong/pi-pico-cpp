// pi-pico-cpp headers
#include <cpp/LedStripWs2812b.hpp>

// Pico SDK headers
#include <pico/stdlib.h>

int main()
{
  // The data pins of 4 Ws2812b strips are connected to
  // GP22, 26, 27, and 28
  LedStripWs2812b strip1(22);
  LedStripWs2812b strip2(26);
  LedStripWs2812b strip3(27);
  LedStripWs2812b strip4(28);

  // Most Ws2812b strips have very poor linearity, brightness to
  // RGB value specified. Apply a gamma to correct that.
  strip1.gamma(2.5f);
  strip2.gamma(2.5f);
  strip3.gamma(2.5f);
  strip4.gamma(2.5f);

  // Create a buffer of 8 colors for the LED strips
  LEDBuffer colors
  {
    {255, 0, 0},    // red
    {255, 255, 0},  // yellow
    {0, 255, 0},    // green
    {0, 255, 255},  // cyan
    {0, 0, 255},    // blue
    {255, 0, 255},  // magenta
    {255, 255, 255},// white
    {0, 0, 0},      // black
  };

  // Send the buffer to each strip with a different global brightness.
  strip1.writeColors(colors, 1.00f);
  strip2.writeColors(colors, 0.75f);
  strip3.writeColors(colors, 0.50f);
  strip4.writeColors(colors, 0.25f);

  return 0;
}