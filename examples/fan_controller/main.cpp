// Sample project making a fan controller for a 4 pin fan that targets a given RPM.
// Includes persistent settings.

// pi-pico-cpp headers
#include <cpp/FlashStorage.hpp>
#include <cpp/CommandParser.hpp>
#include <cpp/Fan.hpp>
#include <cpp/Time.hpp>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>

// std headers
#include <iostream>

struct Settings
{
  float targetRpm = 2000.0f;
  float deadZone = 100.0f;
  float adjustFactor = 0.01f;
};

int main()
{
  // Configure stdio for the command parser
  stdio_init_all();

  // Create the persistent setting object
  FlashStorage<Settings> settings;
  settings.readFromFlash();

  // Setup the command parser
  CommandParser parser;
  parser.addProperty("target_rpm", settings.data.targetRpm);
  parser.addProperty("dead_zone", settings.data.deadZone);
  parser.addProperty("adjust_factor", settings.data.adjustFactor);
  parser.addCommand("save", [&]()
  {
    settings.writeToFlash();
  });

  // Setup the fan hardware, with PWM out pin 0 and TACH in on pin 1
  // (you will likely need filtering capacitors of ~0.05uF between GPIO1 and GND)
  Fan fan(0, 1);

  intervalLoop([&]()
  {
    parser.processStdIo();
    fan.update();

    // Gently adjust fan power if it is outside of target range
    if (fan.getRpm() > (settings.data.targetRpm + settings.data.deadZone))
    {
      fan.setPower(fan.getPower() * (1.0f - settings.data.adjustFactor));
    }
    else if (fan.getRpm() < (settings.data.targetRpm - settings.data.deadZone))
    {
      fan.setPower(fan.getPower() * (1.0f + settings.data.adjustFactor));
    }
  });

  return 0;
}