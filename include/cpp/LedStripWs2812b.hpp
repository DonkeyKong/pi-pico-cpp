#pragma once

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

  static LedStripWs2812b create(uint pin)
  {
    std::shared_ptr<PioProgram> prog = prog_.lock();
    if (!prog)
    {
      // TBD: support autoselection of pio0 and pio1
      prog = std::make_shared<PioProgram>(pio0, &ws2812b_program);
      prog_ = prog;
    }
    return LedStripWs2812b(prog, pin);
  }

  inline void writeColors(const LEDBuffer& buffer, float brightness = 1.0f)
  {
    RGBColor calibrated;
    uint32_t data;
    // Send the colors
    for (int i=0; i < buffer.size(); ++i)
    {
      // Format the color for I/O
      calibrated = buffer[i] * colorBalance_ * brightness;
      calibrated.applyGamma(gamma_);
      data = calibrated.G << 16 | calibrated.R << 8 | calibrated.B;
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
  static std::weak_ptr<PioProgram> prog_;
  Vec3f colorBalance_ {1.0f, 1.0f, 1.0f};
  float gamma_ {1.0f};
  LedStripWs2812b(std::shared_ptr<PioProgram>& prog, uint pin) :
    PioMachine(prog, ws2812b_program_init, pin, 5.0) {}
};

std::weak_ptr<PioProgram> LedStripWs2812b::prog_;