#pragma once

#include "Logging.hpp"
#include "Color.hpp"
#include "PioProgram.hpp"
#include "ws2812b.pio.h"

#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

#include <memory>
#include <vector>
#include <iostream>

using LEDBuffer = std::vector<RGBColor>;

class LedStripWs2812b : PioMachine
{
public:

  struct BufferMapping
  {
    LedStripWs2812b* output;
    int size;
    int offset;
    int index;
  };

  LedStripWs2812b(uint pin) : PioMachine(&ws2812b_program)
  {
    uint offset = prog_->offset();
    pio_sm_config c = ws2812b_program_get_default_config(offset);

    // Map the state machine's i/o pin groups to one pin, namely the `pin`
    sm_config_set_in_pins(&c, pin);
    sm_config_set_out_pins(&c, pin, 1);
    sm_config_set_set_pins(&c, pin, 1);
    sm_config_set_jmp_pin(&c, pin);

    sm_config_set_in_shift(&c, false, false, 32);
    sm_config_set_out_shift(&c, false, false, 32);
    
    // Set this pin's GPIO function (connect PIO to the pad)
    pio_gpio_init(pio_, pin);
    // Set the pin direction to input at the PIO
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin, 1, true);
    sm_config_set_clkdiv(&c, 5.0f);

    // Load our configuration, and jump to the start of the program
    pio_sm_init(pio_, sm_, offset, &c);
    // Set the state machine running
    pio_sm_set_enabled(pio_, sm_, true);
  }

  inline void writeColors(const LEDBuffer& buffer, float brightness = 1.0f)
  {
    RGBColor calibrated;
    uint32_t data = 0;
    // Send the colors
    for (int i=0; i < buffer.size(); ++i)
    {
      // Format the color for I/O
      calibrated = buffer[i] * colorBalance_ * brightness;
      calibrated.applyGamma(gamma_);
      data = (uint32_t)calibrated.G << 16 | (uint32_t)calibrated.R << 8 | (uint32_t)calibrated.B;
      pio_sm_put_blocking(pio_, sm_, data);
    }
    // Send a reset when done
    data = 0xFF << 24;
    pio_sm_put_blocking(pio_, sm_, data);
  }

  static inline void writeColorsParallel(const LEDBuffer& buffer, std::vector<BufferMapping>& mappings, float brightness = 1.0f)
  {
    for (BufferMapping& m : mappings)
    {
      m.index = 0;
    }

    while (true)
    {
      for (BufferMapping& m : mappings)
      {
        while (m.index < m.size && !pio_sm_is_tx_fifo_full(m.output->pio_, m.output->sm_))
        {
          int bufferIndex = std::clamp((m.index++) + m.offset, 0, (int)buffer.size()-1);
          RGBColor calibrated = buffer[bufferIndex] * m.output->colorBalance_ * brightness;
          calibrated.applyGamma(m.output->gamma_);
          uint32_t data = calibrated.G << 16 | calibrated.R << 8 | calibrated.B;
          pio_sm_put(m.output->pio_, m.output->sm_, data);
        }
      }

      int doneCount = 0;
      for (BufferMapping& m : mappings)
      {
        if (m.index == m.size) ++doneCount;
      }
      if (doneCount == mappings.size())
      {
        break;
      }
    }
  }

  inline void gamma(float gamma)
  {
    gamma_ = gamma;
  }

  inline void colorBalance(const Vec3f& colorBalance)
  {
    colorBalance_ = colorBalance;
  }

private:
  Vec3f colorBalance_ {1.0f, 1.0f, 1.0f};
  float gamma_ {1.0f};
};