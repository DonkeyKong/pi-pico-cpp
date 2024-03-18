#pragma once

#include "hardware/pwm.h"
#include "Logging.hpp"

class Servo
{

  static constexpr uint64_t pwmFreqHz = 60;

  // Determine the best integer value clock divider given our desired pwmFreq
  // and the 16 bit limit of the pwm wrap register
  static constexpr uint64_t clockFreqHz = 125000000;
  static constexpr uint16_t pwmWrapMax = 65535;
  static constexpr uint8_t pwmClockDiv = clockFreqHz / pwmWrapMax / pwmFreqHz + 1;

  // With the clock divider found, calculate the pwm wrap counter value
  // that gets us closest to our requested frequency
  static constexpr uint64_t nsPerPwmTick = 1000000000 / clockFreqHz * pwmClockDiv;
  static constexpr uint64_t nsPerPwmCycleDesired = 1000000000 / pwmFreqHz;
  static constexpr uint16_t pwmWrap = nsPerPwmCycleDesired / nsPerPwmTick;

  // Verify our chosen int clock div and wrap get us close to our requested pwmFreq
  static constexpr double actualPwmFreqHz = 1000000000.0 / (double)(pwmWrap * nsPerPwmTick);

public:
  Servo(int pin, uint16_t pwmMinUs = 1000, uint16_t pwmMaxUs = 2000, float minDeg = 0.0f, float maxDeg = 180.0f)
    : pin_{pin}
    , minDeg_{minDeg}
    , maxDeg_{maxDeg}
    , pwmMinUs_{pwmMinUs}
    , pwmMaxUs_{pwmMaxUs}
    , pwmRangeUs_{(uint16_t)(pwmMaxUs_ - pwmMinUs_)}
    , grabbed_{false}
  {
    // Allocate the requested pin to a PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Find out which PWM slice the pin is using
    slice_ = pwm_gpio_to_slice_num(0);

    // Set the wrap and clock divider
    pwm_set_clkdiv_int_frac(slice_, pwmClockDiv, 0); 	
    pwm_set_wrap(slice_, pwmWrap);

    // Just in case...
    bool pullUp = true; bool pullDown = false; bool invert = false;
    if (pullUp)
    {
      gpio_pull_up(pin_);
    }
    if (pullDown)
    {
      gpio_pull_down(pin_);
    }
    if (invert)
    {
      gpio_set_outover(pin_, GPIO_OVERRIDE_INVERT);
    }
    else
    {
      gpio_set_outover(pin_, GPIO_OVERRIDE_NORMAL);
    }

    // Enable PWM with a duty cycle of 0
    pwm_set_gpio_level(pin_, 0);
    pwm_set_enabled(slice_, true);
  }

  ~Servo() = default;
  Servo(const Servo&) = delete;
  Servo(Servo&&) = delete;

  // Set the servo position as a float 0 to 1
  void posT(double t)
  {
    if (t < 0.0f || t > 1.0f)
    {
      DEBUG_LOG("Requested servo pos out of range, t=" << t);
      return;
    }
    
    uint16_t cyclesOn = (uint16_t)(((double)pwmRangeUs_ * t + (double)pwmMinUs_) * 1000.0 / (double)nsPerPwmTick);
    pwm_set_gpio_level(pin_, cyclesOn);    
  }

  void posDeg(double deg)
  {
    if (deg < minDeg_ || deg > maxDeg_)
    {
      return;
    }
    posT((deg - minDeg_) / (maxDeg_ - minDeg_));
  }

  void release()
  {
    pwm_set_gpio_level(pin_, 0);
    grabbed_ = false;
  }

private:
  int pin_;
  float minDeg_;
  float maxDeg_;
  uint16_t pwmMinUs_;
  uint16_t pwmMaxUs_;
  uint16_t pwmRangeUs_;
  uint slice_;
  bool grabbed_;
};