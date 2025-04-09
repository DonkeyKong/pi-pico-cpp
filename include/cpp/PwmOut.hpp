#pragma once

#include "Logging.hpp"

#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <algorithm>

class PwmOut
{
public:
  // clock freq is determined by hardware
  // pwmWrapMax is just uint16 max
  static constexpr uint64_t clockFreqHz = 125000000;
  static constexpr uint16_t pwmWrapMax = 65535;
  const uint8_t pwmClockDiv;
  const uint64_t nsPerPwmTick;
  const uint64_t nsPerPwmCycleDesired;
  const uint16_t pwmWrap;
  const double actualPwmFreqHz;

  PwmOut(uint pwmPin, uint64_t desiredPwmFreqHz)
    : pwmPin_{pwmPin}
    , grabbed_{false}
    , dc_{0.0f}
    // Determine the best integer value clock divider given our desired pwmFreq
    // and the 16 bit limit of the pwm wrap register
    , pwmClockDiv{(uint8_t)(clockFreqHz / pwmWrapMax / desiredPwmFreqHz + 1ull)}
    // With the clock divider found, calculate the pwm wrap counter value
    // that gets us closest to our requested frequency
    , nsPerPwmTick{1000000000 / clockFreqHz * pwmClockDiv}
    , nsPerPwmCycleDesired{1000000000 / desiredPwmFreqHz}
    , pwmWrap{(uint16_t)(nsPerPwmCycleDesired / nsPerPwmTick)}
    // Calculate the actual PWM frequency achieved
    , actualPwmFreqHz{1000000000.0 / (double)(pwmWrap * nsPerPwmTick)}
  {
    // Allocate the requested pin to a PWM
    gpio_set_function(pwmPin_, GPIO_FUNC_PWM);

    // Find out which PWM slice the pin is using
    slice_ = pwm_gpio_to_slice_num(pwmPin_);

    // Set the wrap and clock divider
    pwm_set_clkdiv_int_frac(slice_, pwmClockDiv, 0);
    pwm_set_wrap(slice_, pwmWrap);

    // Just in case...
    bool pullUp = false; bool pullDown = false; bool invert = false;
    if (pullUp)
    {
      gpio_pull_up(pwmPin_);
    }
    if (pullDown)
    {
      gpio_pull_down(pwmPin_);
    }
    if (invert)
    {
      gpio_set_outover(pwmPin_, GPIO_OVERRIDE_INVERT);
    }
    else
    {
      gpio_set_outover(pwmPin_, GPIO_OVERRIDE_NORMAL);
    }

    // Enable PWM with a duty cycle of 0
    pwm_set_gpio_level(pwmPin_, 0);
    pwm_set_enabled(slice_, true);

    // print debug info about the PWM setup
    DEBUG_LOG("Set up PWM output on GPIO pin " << pwmPin_);
    DEBUG_LOG("    Using PWM slice " << slice_);
    DEBUG_LOG("    Pull up: " << (pullUp ? "[on]" : "[off]") << " Pull down: " << (pullDown ? "[on]" : "[off]") << " Inv: " << (invert ? "[on]" : "[off]"));
    DEBUG_LOG("    Requested freq: " << desiredPwmFreqHz << " Actual freq: " << actualPwmFreqHz);
  }

  ~PwmOut() = default;
  PwmOut(const PwmOut&) = delete;
  PwmOut(PwmOut&&) = delete;

  // Set the PwmOut power as a float 0 to 1
  void setDutyCycle(float t)
  {
    t = std::clamp(t, 0.0f, 1.0f);

    uint16_t cyclesOn = (uint16_t)((float)pwmWrap * t);
    pwm_set_gpio_level(pwmPin_, cyclesOn);
    dc_ = t;
  }

  float getDutyCycle()
  {
    return dc_;
  }

  void release()
  {
    pwm_set_gpio_level(pwmPin_, 0);
    grabbed_ = false;
  }

private:
  uint pwmPin_;
  uint slice_;
  bool grabbed_;
  float dc_;
};