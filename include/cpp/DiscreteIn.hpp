#pragma once

#include <pico/stdlib.h>
#include <hardware/gpio.h>

class DiscreteIn
{
public:
  DiscreteIn(uint32_t pin, bool pullUp = false, bool pullDown = true, bool invert = false) :
    pin_(pin)
  {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);
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
      gpio_set_inover(pin_, GPIO_OVERRIDE_INVERT);
    }
    else
    {
      gpio_set_inover(pin_, GPIO_OVERRIDE_NORMAL);
    }
    
    // Give the gpio set operations a chance to settle!
    sleep_until(make_timeout_time_ms(1));
  }
  bool get()
  {
    return gpio_get(pin_);
  }

  bool waitFor(bool state, int timeoutMs)
  {
    int i = 0;
    while (get() != state)
    {
      sleep_ms(10);
      ++i;
      if (i*10 > timeoutMs)
      {
        return false;
      }
    }
    return true;
  }

  ~DiscreteIn()
  {
    gpio_deinit(pin_);
  }
private:
  uint32_t pin_;
};