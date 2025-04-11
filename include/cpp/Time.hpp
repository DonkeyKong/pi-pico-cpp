#pragma once

#include <pico/time.h>
#include <hardware/timer.h>

enum class TimingStrategy
{
  Absolute,       // Nth iteration scheduled for [first iteration time] + N * intervalUs
  RelativeStart,  // Next interation scheduled for intervalUs after the start of current iteration (default)
  RelativeEnd     // Next interation scheduled for intervalUs after the end of current iteration 
};

// Run a function every X microseconds, forever
// Default interval is 16667 us or 60 Hz
// TimingStrategy usually doesn't matter and RelativeStart is almost always what you want
template <typename Callable>
void intervalLoop(Callable loopFunc, uint64_t intervalUs = 16667, TimingStrategy strategy = TimingStrategy::RelativeStart)
{
    absolute_time_t nextUpdateTime = get_absolute_time();
    while(true)
    {
      absolute_time_t loopStartTime = get_absolute_time();
      sleep_until(nextUpdateTime);

      loopFunc();

      if (strategy == TimingStrategy::Absolute)
      {
        nextUpdateTime = delayed_by_us(nextUpdateTime, intervalUs);
      }
      else if (strategy == TimingStrategy::RelativeStart)
      {
        nextUpdateTime = delayed_by_us(loopStartTime, intervalUs);
      }
      else if (strategy == TimingStrategy::RelativeEnd)
      {
        nextUpdateTime = make_timeout_time_us(intervalUs);
      }
    }
}