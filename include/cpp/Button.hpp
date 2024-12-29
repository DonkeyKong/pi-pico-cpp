#pragma once

#include <pico/stdlib.h>
#include <hardware/gpio.h>

class Button
{
public:
  virtual ~Button() = default;

  // Get if the button is currently pressed
  bool operator()() const
  {
    return state_;
  }

  // Get if the button is currently pressed
  bool pressed() const
  {
    return state_;
  }

  // Get the number of ms the button has been pressed for or 0 if it's not being pressed
  uint32_t heldTimeMs() const
  {
    if (!state_) return 0;
    return to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(stateTime_);
  }

  // Get the number of ms the button has been released for or 0 if it's being pressed
  uint32_t releasedTimeMs() const
  {
    if (state_) return 0;
    return to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(stateTime_);
  }

  // Returns true after an update in which the button transitioned from being held for fewer
  // than 1000ms to being held for more than 1000 ms. Returns false if update is called again,
  // even if the button remains held.
  bool heldActivate() const
  {
    return holdActivate_;
  }

  bool buttonDown() const
  {
    return state_ && !lastState_;
  }
  
  bool buttonUp() const
  {
    return !state_ && lastState_;
  }

  void update()
  {
    lastState_ = state_;
    state_ = getButtonState();
    if (lastState_ != state_)
    {
      stateTime_ = get_absolute_time();
    }

    if (enableHoldAction_)
    {
      if (buttonDown())
      {
        holdActivationTime_ = make_timeout_time_ms(holdActivationMs_);
      }

      if (buttonUp() && holdSuppressButtonUp_)
      {
        lastState_ = state_;
        holdSuppressButtonUp_ = false;
        holdSuppressRepeat_ = false;
      }

      if (state_ && !holdSuppressRepeat_ && to_ms_since_boot(holdActivationTime_) <= to_ms_since_boot(get_absolute_time()))
      {
        holdActivate_ = true;
        holdActivationTime_ = make_timeout_time_ms(holdActivationRepeatMs_);
        holdSuppressButtonUp_ = true;
        holdSuppressRepeat_ = holdActivationRepeatMs_ < 0;
      }
      else
      {
        holdActivate_ = false;
      }
    }
  }

  void holdActivationtMs(int val)
  {
    holdActivationMs_ = val;
  }

  void holdActivationRepeatMs(int val)
  {
    holdActivationRepeatMs_ = val;
  }

protected:
  Button(bool enableHoldAction = false) : enableHoldAction_(enableHoldAction) {}
  virtual bool getButtonState() = 0;
  bool state_;
  bool lastState_;
  absolute_time_t stateTime_;

  bool enableHoldAction_ = false;
  int holdActivationMs_ = 1000;
  int holdActivationRepeatMs_ = 0;
  absolute_time_t holdActivationTime_;
  bool holdActivate_ = false;
  bool holdSuppressButtonUp_ = false;
  bool holdSuppressRepeat_ = false;
};

class GPIOButton : public Button
{
public:
  GPIOButton(uint32_t pin, bool enableHoldAction = false, bool pullUp = true, bool pullDown = false, bool invert = true):
    Button(enableHoldAction),
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

    update();
    lastState_ = state_;
    stateTime_ = get_absolute_time();
  }

  virtual bool getButtonState() override
  {
    return gpio_get(pin_);
  }

  ~GPIOButton()
  {
    gpio_deinit(pin_);
  }

private:
  uint32_t pin_;
};

class RegisterButton : public Button
{
public:
  RegisterButton(uint8_t& reg, int bit, int highState = 1, bool enableHoldAction = false)
    : Button(enableHoldAction)
    , reg_(reg)
    , bit_(bit)
    , highState_(highState)
  {
    update();
    lastState_ = state_;
    stateTime_ = get_absolute_time();
  }

  virtual bool getButtonState() override
  {
    return ((reg_ >> bit_) & 0x1) == highState_;
  }
private:
  uint8_t& reg_;
  int bit_;
  int highState_;
};