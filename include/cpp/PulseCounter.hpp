#pragma once

#include "pulse_counter.pio.h"
#include "Pio.hpp"
#include "Logging.hpp"

#include <limits>
#include <algorithm>

// PulseCounter takes in a pin to listen on and a sample interval.
// It reports out how many pulses it has seen since on the pin since last update
// Up to 8 observations can be enqueued, at which point it stalls
// Pulses shorter than 5us (~200 kHz) may not be counted correctly
class PulseCounter : private PioMachine
{
  static constexpr uint64_t clockFreqHz = 125000000;
  static constexpr uint64_t nsPerPioDecrement = 1000000000 / clockFreqHz * 2;
  float sampleIntervalMs_;
public:
  PulseCounter(uint pin, bool pullup = true, float sampleIntervalMs = 16.6667f) 
    : PioMachine{&pulse_counter_program}
    , sampleIntervalMs_{sampleIntervalMs}
  {
    // If the program loaded sucessfully, then go ahead and initialize it
    if (loaded_)
    {
      uint offset = prog_->offset();
      config_ = pulse_counter_program_get_default_config(offset);

      // Map the state machine input and jump pins to 
      sm_config_set_in_pins(&config_, pin);
      sm_config_set_jmp_pin(&config_, pin);

      sm_config_set_in_shift(&config_, false, true, 32); // AUTOPUSH on
      sm_config_set_out_shift(&config_, false, false, 32); // AUTOPULL off
      
      // Set this pin's GPIO function (connect PIO to the pad)
      pio_gpio_init(pio_, pin);

      if (pullup)
      {
        gpio_pull_up(pin);
      }

      // Set the pin direction to input at the PIO
      pio_sm_set_consecutive_pindirs(pio_, sm_, pin, 1, false);
      sm_config_set_clkdiv(&config_, 1.0);
    
      // Load our configuration, and jump to the start of the program
      pio_sm_init(pio_, sm_, offset, &config_);
      
      // Set the state machine running
      pio_sm_set_enabled(pio_, sm_, true);
      
      uint64_t counter64 = (uint64_t)((double)sampleIntervalMs_ * 1000000.0 / (double)nsPerPioDecrement);
      uint32_t counter = (uint32_t)std::min(counter64, (uint64_t)std::numeric_limits<uint32_t>::max());

      DEBUG_LOG("PulseCounter setup with interval of " << sampleIntervalMs_ << "ms or " << counter << " pio counter decrements");

      write(counter);
    }
  }

  // Gets one measurement from the pulse counter
  // Returns false if there is no measurement to get
  // Best to call in a loop until it returns false
  bool pop(uint32_t& pulseCount)
  {
    if (pio_sm_is_rx_fifo_empty(pio_, sm_))
    {
      return false;
    }
    // The PIO state machine's counter is messed up because it starts
    // at zero and can only decrement. Just fix that here so the pulse counts
    // are normal.
    pulseCount = std::numeric_limits<uint32_t>::max() - pio_sm_get(pio_, sm_) + 1;
    return true;
  }

  float sampleIntervalMs()
  {
    return sampleIntervalMs_;
  }
};