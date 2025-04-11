#pragma once

#include "Logging.hpp"

#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

#include <memory>
#include <vector>
#include <iostream>

// Describes how byte buffers get packed into uint32_ts for interchange
// with the PIO system. When using a buffer with a size not divisible by 4, 
// there is special behavior to account for partially-filled registers.
enum class PioBufferPacking
{
  MsbToLsb,    // Index increases from MSB to LSB, for left shifting
  LsbToMsb,    // Index increases from LSB to MSB, for right shifting
};

struct PioByteBuffer
{
  uint8_t* data = nullptr;
  size_t size = 0;
  PioBufferPacking pack = PioBufferPacking::MsbToLsb;
};

struct PioWordBuffer
{
  uint32_t* data;
  size_t size;
};

class PioMachine
{
  class PioProgram
  {
  public:
    PioProgram(const PIO& pio, const pio_program* prog)
    {
      // Load the PIO program
      pio_ = pio;
      prog_ = prog;
      offset_ = pio_add_program(pio_, prog_);
      loaded_ = true;
      DEBUG_LOG("Loaded program on PIO" << PIO_NUM(pio));
      DEBUG_LOG("    Offset: " << offset_);
    }
  
    ~PioProgram()
    {
      if (loaded_)
      {
        pio_remove_program(pio_, prog_, offset_);
      }
      loaded_ = false;
    }
  
    PIO pio() const
    {
      return pio_;
    }
  
    uint offset() const
    {
      return offset_;
    }

    const pio_program* program() const
    {
      return prog_;
    }
  
    // Cannot copy or move
    PioProgram(const PioProgram& o) = delete;
    PioProgram(PioProgram&& other) = delete;
    PioProgram& operator=(PioProgram&& other) = delete;
  
  private:
    const pio_program* prog_;
    PIO pio_;
    uint offset_;
    bool loaded_ = false;
  };

  // Static collection of all loaded program instances
  static std::vector<std::weak_ptr<PioProgram>> cachedPrograms;

  static void cleanupUnloadedPrograms()
  {
    // get rid of any dead weak ptrs
    cachedPrograms.erase(std::remove_if(cachedPrograms.begin(), cachedPrograms.end(), [](std::weak_ptr<PioProgram> prog) 
    {
      return prog.expired();
    }), cachedPrograms.end());
  }

  static bool pioHasFreeStateMachine(PIO pio)
  {
    uint sm = pio_claim_unused_sm(pio, false);
    pio_sm_unclaim(pio, sm);
    return sm >= 0;
  }

  static bool pioInstanceHasProgramLoaded(PIO pio, const pio_program* prog)
  {
    for (const auto weakPioProgPtr : cachedPrograms)
    {
      auto pioProgPtr = weakPioProgPtr.lock();
      if (pioProgPtr->program() == prog && pioProgPtr->pio() == pio)
      {
        return true;
      }
    }
    return false;
  }

  // Load a program on to a PIO core that doesn't already have a copy
  static std::shared_ptr<PioProgram> loadProgram(const pio_program* prog)
  {
    for (int i = 0; i < NUM_PIOS; ++i)
    {
      PIO pio = PIO_INSTANCE(i);
      if (pioHasFreeStateMachine(pio) && !pioInstanceHasProgramLoaded(pio, prog))
      {
        auto pm = std::make_shared<PioProgram>(pio, prog);
        cachedPrograms.push_back(pm);
        return pm;
      }
    }
    return nullptr;
  }

protected:  
  PioMachine(const pio_program* prog)
  {
    cleanupUnloadedPrograms();

    // Look in the static collection and try claiming a state machine
    // on an existing PIO core
    for (const auto weakPioProgPtr : cachedPrograms)
    {
      auto pioProgPtr = weakPioProgPtr.lock();
      if (pioProgPtr->program() == prog && pioHasFreeStateMachine(pioProgPtr->pio()))
      {
        prog_ = pioProgPtr;
        pio_ = prog_->pio();
        sm_ = pio_claim_unused_sm(pio_, true);
        loaded_ = true;
        break;
      }
    }

    // If no free state machines were found, try loading it on any core
    // that doesn't already have it
    if (!loaded_)
    {
      prog_ = loadProgram(prog);
      if (prog_)
      {
        pio_ = prog_->pio();
        sm_ = pio_claim_unused_sm(pio_, true);
        loaded_ = true;
      }
    }
  }

public:
  PioMachine(const PioMachine& o) = delete;

  PioMachine(PioMachine&& other):
    sm_(other.sm_),
    config_(other.config_),
    loaded_(other.loaded_),
    prog_(other.prog_),
    pio_(other.pio_)
  {
    other.prog_.reset();
    other.loaded_ = false;
  }

  ~PioMachine()
  {
    if (loaded_)
    {
      pio_sm_unclaim(pio_, sm_);
    }
    loaded_ = false;
  }

  PioMachine& operator=(PioMachine&& other)
  {
    if (loaded_)
    {
      pio_sm_unclaim(pio_, sm_);
    }
    sm_ = other.sm_;
    config_ = other.config_;
    loaded_ = other.loaded_;
    prog_ = other.prog_;
    pio_ = other.pio_;
    other.prog_.reset();
    other.loaded_ = false;
    return *this;
  }

  // Write a word out to PIO. Return 1 if written, 0 if timed out.
  // No endian or byte order conversion will occur.
  inline size_t write(const uint32_t& data, int timeoutMs = -1)
  {
    // Wait for a tx buffer spot to open up and exit
    // if we time out waiting for it
    auto start = to_ms_since_boot(get_absolute_time());
    while (pio_sm_is_tx_fifo_full(pio_, sm_))
    {
      auto now = to_ms_since_boot(get_absolute_time());
      if (timeoutMs >= 0 && (now - start) > timeoutMs)
      {
        return 0;
      }
    }
    pio_sm_put(pio_, sm_, data);
    return 1;
  }

  // Write a word buffer out to PIO. Return the number of words written
  // No endian or byte order conversion will occur.
  size_t write(const PioWordBuffer& buf, int timeoutMs = -1)
  {
    auto start = to_ms_since_boot(get_absolute_time());
    for (size_t i = 0; i < buf.size; ++i)
    {
      // Wait for a tx buffer spot to open up and exit
      // if we time out waiting for it
      while (pio_sm_is_tx_fifo_full(pio_, sm_))
      {
        auto now = to_ms_since_boot(get_absolute_time());
        if (timeoutMs >= 0 && (now - start) > timeoutMs)
        {
          return i;
        }
      }

      pio_sm_put(pio_, sm_, buf.data[i]);
    }
    return buf.size;
  }

  // Write a byte buffer out to PIO. Return the number of bytes written.
  size_t write(const PioByteBuffer& buf, int timeoutMs = -1)
  {
    auto start = to_ms_since_boot(get_absolute_time());
    for (size_t i = 0; i < buf.size; i+=4)
    {
      // Wait for a tx buffer spot to open up and exit with false
      // if one does not
      while (pio_sm_is_tx_fifo_full(pio_, sm_))
      {
        auto now = to_ms_since_boot(get_absolute_time());
        if (timeoutMs >= 0 && (now - start) > timeoutMs)
        {
          return i;
        }
      }

      if (buf.pack == PioBufferPacking::MsbToLsb)
      {
        uint32_t scratch = buf.data[i] << 24;
        if (i + 1 < buf.size) scratch |= (uint32_t)buf.data[i+1] << 16;
        if (i + 2 < buf.size) scratch |= (uint32_t)buf.data[i+2] << 8;
        if (i + 3 < buf.size) scratch |= (uint32_t)buf.data[i+3] << 0;
        pio_sm_put(pio_, sm_, scratch);
      }
      else //if (buf.pack == PioBufferPacking::LsbToMsb)
      {
        uint32_t scratch = buf.data[i] << 0;
        if (i + 1 < buf.size) scratch |= (uint32_t)buf.data[i+1] << 8;
        if (i + 2 < buf.size) scratch |= (uint32_t)buf.data[i+2] << 16;
        if (i + 3 < buf.size) scratch |= (uint32_t)buf.data[i+3] << 24;
        pio_sm_put(pio_, sm_, scratch);
      }
    }
    return buf.size;
  }

  // Read a word buffer from PIO. Return 1 if read, 0 if timed out.
  inline size_t read(uint32_t& data, int timeoutMs = -1)
  {
    // Wait for a tx buffer spot to open up and exit
    // if we time out waiting for it
    auto start = to_ms_since_boot(get_absolute_time());
    while (pio_sm_is_rx_fifo_empty(pio_, sm_))
    {
      auto now = to_ms_since_boot(get_absolute_time());
      if (timeoutMs >= 0 && (now - start) > timeoutMs)
      {
        return 0;
      }
    }
    data = pio_sm_get(pio_, sm_);
    return 1;
  }

  // Read a word buffer from PIO. Return the number of words read
  size_t read(PioWordBuffer buf, int timeoutMs = -1)
  {
    auto start = to_ms_since_boot(get_absolute_time());
    for (size_t i = 0; i < buf.size; ++i)
    {
      // Wait for a tx buffer spot to open up and exit
      // if we time out waiting for it
      while (pio_sm_is_rx_fifo_empty(pio_, sm_))
      {
        auto now = to_ms_since_boot(get_absolute_time());
        if (timeoutMs >= 0 && (now - start) > timeoutMs)
        {
          return i;
        }
      }

      buf.data[i] = pio_sm_get(pio_, sm_);
    }
    return buf.size;
  }

  // Read a byte buffer from PIO. Return the number of bytes read.
  size_t read(PioByteBuffer buf, int timeoutMs = -1)
  {
    auto start = to_ms_since_boot(get_absolute_time());
    for (size_t i = 0; i < buf.size; i+=4)
    {
      // Wait for a tx buffer spot to open up and exit with false
      // if one does not
      while (pio_sm_is_rx_fifo_empty(pio_, sm_))
      {
        auto now = to_ms_since_boot(get_absolute_time());
        if (timeoutMs >= 0 && (now - start) > timeoutMs)
        {
          return i;
        }
      }

      if (buf.pack == PioBufferPacking::MsbToLsb)
      {
        uint32_t scratch = pio_sm_get(pio_, sm_);
        buf.data[i] = (uint8_t)(scratch >> 24);
        if (i + 1 < buf.size) buf.data[i+1] = (uint8_t)(scratch >> 16);
        if (i + 2 < buf.size) buf.data[i+2] = (uint8_t)(scratch >> 8);
        if (i + 3 < buf.size) buf.data[i+3] = (uint8_t)(scratch >> 0);
      }
      else //if (buf.pack == PioBufferPacking::LsbToMsb)
      {
        uint32_t scratch = pio_sm_get(pio_, sm_);
        buf.data[i] = (uint8_t)(scratch >> 0);
        if (i + 1 < buf.size) buf.data[i+1] = (uint8_t)(scratch >> 8);
        if (i + 2 < buf.size) buf.data[i+2] = (uint8_t)(scratch >> 16);
        if (i + 3 < buf.size) buf.data[i+3] = (uint8_t)(scratch >> 24);
      }
    }
    return buf.size;
  }

  void reset()
  {
    pio_sm_set_enabled(pio_, sm_, false);
    pio_sm_clear_fifos(pio_, sm_);
    pio_sm_restart(pio_, sm_);
    pio_sm_init(pio_, sm_, prog_->offset(), &config_);
    pio_sm_set_enabled(pio_, sm_, true);
  }

  bool loaded() const
  {
    return loaded_;
  }

protected:
  uint sm_;
  pio_sm_config config_;
  bool loaded_ = false;
  std::shared_ptr<PioProgram> prog_;
  PIO pio_;
};

std::vector<std::weak_ptr<PioMachine::PioProgram>> PioMachine::cachedPrograms;