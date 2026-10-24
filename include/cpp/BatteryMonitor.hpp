#pragma once

#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include "hardware/adc.h"

#include <cpp/Math.hpp>

class BatteryMonitor
{
public:
  struct BatteryReading
  {
    uint16_t rawValue;
  };

  BatteryMonitor(uint32_t apin = 26, float voltageDivider = 1.27f, float empty = 2.25f, float full = 4.2f) :
    pin_(apin), voltageDivider_(voltageDivider), empty_(empty), full_(full)
  {
    adc_init();
    adc_gpio_init(pin_);
    adc_select_input(0);
  }

  BatteryReading read()
  {
    return {adc_read()};
  }

  float readingToVoltage(BatteryReading reading)
  {
    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const float conversion_factor = 3.3f / (1 << 12);
    return reading.rawValue * conversion_factor * voltageDivider_;
  }

  float readingToPercentage(BatteryReading reading)
  {
    return remapClamp(readingToVoltage(reading), empty_, full_, 0.0f, 100.0f);
  }

  ~BatteryMonitor()
  {
    gpio_deinit(pin_);
  }
private:
  uint32_t pin_;
  float voltageDivider_;
  float empty_;
  float full_;
};