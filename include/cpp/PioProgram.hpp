#pragma once

#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

#include <memory>
#include <vector>
#include <iostream>

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

  inline bool write(uint32_t data, int timeoutMs = -1)
  {
    auto start = to_ms_since_boot(get_absolute_time());
    while (1) 
    {  
      auto now = to_ms_since_boot(get_absolute_time());
      if (!pio_sm_is_tx_fifo_full(pio_, sm_))
      {
        break;
      }
      if (timeoutMs >= 0 && (now - start) > timeoutMs)
      {
        return false;
      }
    }
    pio_sm_put(pio_, sm_, data);
    return true;
  }

  inline bool write(uint8_t data, int timeoutMs = -1)
  {
    return write((uint32_t)data << 24, timeoutMs);
  }

  inline bool read(uint32_t& data, int timeoutMs = -1)
  {
    auto start = to_ms_since_boot(get_absolute_time());
    while (1) 
    {  
      auto now = to_ms_since_boot(get_absolute_time());
      if (!pio_sm_is_rx_fifo_empty(pio_, sm_))
      {
        break;
      }
      if (timeoutMs >= 0 && (now - start) > timeoutMs)
      {
        return false;
      }
    }
    data = pio_sm_get(pio_, sm_);
    return true;
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