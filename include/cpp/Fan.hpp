#pragma once

#include "Logging.hpp"
#include "PwmOut.hpp"
#include "PulseCounter.hpp"

class Fan
{
  // PC fans want a nominal PWM frequency of 25kHz
  static constexpr uint64_t fanPwmFreqHz = 25000;
  static constexpr float tachSamplePeriodMs = 1000.0f;
  static constexpr float samplesPerSecond = 1000.0f / tachSamplePeriodMs;
  static constexpr int tachSampleCount = 3;
  static constexpr float convFactor = samplesPerSecond * 60.0f / 2.0f / (float)tachSampleCount;

  PwmOut pwm_;
  PulseCounter tach_;
  int tachSampleIndex_;
  float tachRpm_;
  float dc_;
  std::array<uint32_t, tachSampleCount> tachSamples_;

public:
  Fan(uint pwmPin, uint tachPin, float startingPower = 0.5f)
    : pwm_{pwmPin, fanPwmFreqHz}
    , tach_{tachPin, true, tachSamplePeriodMs}
    , tachSampleIndex_{0}
    , tachRpm_{0}
  {
    for (int i=0; i < tachSampleCount; ++i)
    {
      tachSamples_[i] = 0;
    }

    setPower(startingPower);
  }

  ~Fan() = default;
  Fan(const Fan&) = delete;
  Fan(Fan&&) = delete;

  // Set the fan power as a float 0 to 1
  void setPower(float t)
  {
    dc_ = t;
    pwm_.setDutyCycle(dc_);
  }

  float getPower()
  {
    return dc_;
  }

  float getRpm()
  {
    return tachRpm_;
  }

  // Release all hardware resources held by the fan
  void release()
  {
    pwm_.release();
  }

  void update()
  {
    // Update the tachometer samples
    uint32_t pulseCount = 0;
    bool tachUpdated = false;
    while (tach_.pop(pulseCount))
    {
      tachSamples_[tachSampleIndex_] = pulseCount;
      tachSampleIndex_ = (tachSampleIndex_ + 1) % tachSampleCount;
      tachUpdated = true;
      //DEBUG_LOG("Pulse count: " << pulseCount);
    }

    // Calculate RPM from the tach
    if (tachUpdated)
    {
      pulseCount = 0;
      for (int i=0; i < tachSampleCount; ++i)
      {
        pulseCount += tachSamples_[i];
      }
      tachRpm_ = (float)pulseCount * convFactor;
      //DEBUG_LOG("Fan RPM: " << tachRpm_);
    }
  }
};