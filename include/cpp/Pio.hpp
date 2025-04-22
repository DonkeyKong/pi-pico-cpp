#pragma once

#include "Logging.hpp"
#include "RttiCache.hpp"

#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

#include <memory>
#include <vector>
#include <iostream>
#include <type_traits>

enum class PioIrqType
{
  Interrupt,
  TxFifoNotFull,
  RxFifoNotEmpty
};

struct PioIrqHandler
{
  PIO pio;
  uint irqn;
  irq_handler_t handler;

  PioIrqHandler(PIO pio, uint irqn, irq_handler_t handler)
    : pio(pio), irqn(irqn), handler(handler)
  {
    irq_add_shared_handler(pio_get_irq_num(pio, irqn), handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(pio_get_irq_num(pio, irqn), true);
  }

  ~PioIrqHandler()
  {
    irq_remove_handler(pio_get_irq_num(pio, irqn), handler);
    irq_set_enabled(pio_get_irq_num(pio, irqn), false);
  }
};

struct PioIrqEventConnection
{
  PIO pio;
  uint irqn;
  pio_interrupt_source_t source;

  PioIrqEventConnection(PIO pio, uint irqn, pio_interrupt_source_t source)
    : pio(pio), irqn(irqn), source(source)
  {
    pio_set_irqn_source_enabled(pio, irqn, source, true);
  }

  ~PioIrqEventConnection()
  {
    pio_set_irqn_source_enabled(pio, irqn, source, false);
  }
};

// A helper class that takes a chunk of data and turns it to/from a set of
// uint32 values for use with PIO programs.
// Size and index (i) are in number of uint32_t sized transfers
struct PioBuffer
{
  // Size in number of PIO transfers
  size_t size;

  // Pack the bytes i to i+3 from the internal data rep into dst
  virtual void pack(uint32_t& dst, size_t i) const = 0;

  // Unpack the bytes from src to the internal data rep (bytes i to i+3)
  virtual void unpack(const uint32_t& src, size_t i) = 0;

protected:
  // Hide these to prevent construction or slicing
  PioBuffer(size_t size) : size(size) {}
  PioBuffer(const PioBuffer&) = default;
  PioBuffer& operator=(const PioBuffer&) = default;
  PioBuffer(PioBuffer&&) = delete;
};

// Unused, untested
// struct PioPackedByteBufferLeft : public PioBuffer
// {
//   size_t sizeInBytes;
//   uint8_t* data;
//   PioPackedByteBufferLeft(uint8_t* data, size_t sizeInBytes) : PioBuffer((sizeInBytes + 3)/4), sizeInBytes(sizeInBytes), data(data) {}

//   inline uint32_t getShiftedByte(size_t byteIndex, int lshift) const
//   {
//     return (byteIndex < sizeInBytes) ? ((uint32_t)data[byteIndex]) << lshift : 0ul;
//   }

//   virtual void pack(uint32_t& dst, size_t i) const override
//   {
//     dst = 0;
//     dst |= getShiftedByte(i*4+0, 24);
//     dst |= getShiftedByte(i*4+1, 16);
//     dst |= getShiftedByte(i*4+2, 8);
//     dst |= getShiftedByte(i*4+3, 0);
//   }

//   virtual void unpack(const uint32_t& src, size_t i) override
//   {
//     if (i*4+0 < sizeInBytes) data[i*4+0] = (uint8_t)(src >> 24);
//     if (i*4+1 < sizeInBytes) data[i*4+1] = (uint8_t)(src >> 16);
//     if (i*4+2 < sizeInBytes) data[i*4+2] = (uint8_t)(src >> 8);
//     if (i*4+3 < sizeInBytes) data[i*4+3] = (uint8_t)(src >> 0);
//   }
// };

// Unused, tested
// struct PioSparseByteBufferLeft : public PioBuffer
// {
//   uint8_t* data;
//   PioSparseByteBufferLeft(uint8_t* data, size_t size) : PioBuffer(size), data(data) {}

//   virtual void pack(uint32_t& dst, size_t i) const override
//   {
//     dst = data[i] << 24;
//   }

//   virtual void unpack(const uint32_t& src, size_t i) override
//   {
//     if (i < size) data[i] = (uint8_t)(src >> 24);
//   }
// };

// Unused, untested
// struct PioWordBuffer : public PioBuffer
// {
//   uint32_t* data;
//   PioWordBuffer(uint32_t* data, size_t size) : PioBuffer(size), data(data) {}

//   virtual void pack(uint32_t& dst, size_t i) const override
//   {
//     dst = (i < size) ? data[i] : 0ul;
//   }

//   virtual void unpack(const uint32_t& src, size_t i) override
//   {
//     if (i < size) data[i] = src;
//   }
// };

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

  static bool pioHasFreeStateMachine(PIO pio)
  {
    uint sm = pio_claim_unused_sm(pio, false);
    pio_sm_unclaim(pio, sm);
    return sm >= 0;
  }

  // Static collection of all loaded program instances
  static RttiCache<PioProgram, const PIO&, const pio_program*> cachedPrograms;
  static RttiCache<PioIrqHandler, PIO, uint, irq_handler_t> cachedHandlers;
  static RttiCache<PioIrqEventConnection, PIO, uint, pio_interrupt_source_t> cachedIrqConnections;

protected:  
  PioMachine(const pio_program* prog)
  {
    // Check if an existing pio with the program loaded has a free state machine,
    // use that.
    for (int i = 0; i < NUM_PIOS; ++i)
    {
      PIO pio = PIO_INSTANCE(i);
      auto pioProgPtr = cachedPrograms.get(pio, prog);
      if (pioProgPtr && pioHasFreeStateMachine(pio))
      {
        prog_ = pioProgPtr;
        pio_ = prog_->pio();
        sm_ = pio_claim_unused_sm(pio_, true);
        loaded_ = true;
        return;
      }
    }

    // If no free state machines were found, try loading it on any core
    // that doesn't already have it
    for (int i = 0; i < NUM_PIOS; ++i)
    {
      PIO pio = PIO_INSTANCE(i);
      if (!cachedPrograms.contains(pio, prog) && pioHasFreeStateMachine(pio))
      {
        prog_ = cachedPrograms.getOrCreate(pio, prog);
        if (prog_)
        {
          pio_ = prog_->pio();
          sm_ = pio_claim_unused_sm(pio_, true);
          loaded_ = true;
          return;
        }
      }
    }
    // Hopefully it's loaded. You can always check loaded()
    // after construction to see if it worked. In general, apps
    // should know if the init will succeed though.
  }

  pio_interrupt_source_t getInterruptSource(PioIrqType eventType)
  {
    switch (eventType)
    {
      case PioIrqType::RxFifoNotEmpty:
        return pio_get_rx_fifo_not_empty_interrupt_source(sm_);
      case PioIrqType::TxFifoNotFull:
        return pio_get_tx_fifo_not_full_interrupt_source(sm_);
      default:
        return (pio_interrupt_source_t)(pis_interrupt0 + sm_);
    }
  }

  void enableIrq(PioIrqType eventType, uint irqn, irq_handler_t handler)
  {
    irq_ = cachedHandlers.getOrCreate(pio_, irqn, handler);
    eventConnections_.push_back(cachedIrqConnections.getOrCreate(pio_, irqn, getInterruptSource(eventType)));
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
    eventConnections_.clear();
    irq_.reset();
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

  // Waits until the recieve buffer gets some data or endTime is in the past.
  // true if rx buffer opened up, false if time is up
  // Note that even if end time is in the past, if rx buffer
  // is free on entry, this function will return true
  inline bool waitForRxBufferUntil(const absolute_time_t& endTime)
  {
    while(pio_sm_is_rx_fifo_empty(pio_, sm_))
    {
      if (time_reached(endTime))
      {
        return false;
      }
    }
    return true;
  }

  // Waits until the transmit buffer has free space or endTime is in the past.
  // true if tx buffer opened up, false if time is up
  // Note that even if end time is in the past, if buffer
  // is free on entry, this function will return true
  inline bool waitForTxBufferUntil(const absolute_time_t& endTime)
  {
    while(pio_sm_is_tx_fifo_full(pio_, sm_))
    {
      if (time_reached(endTime))
      {
        return false;
      }
    }
    return true;
  }

  // Write a buffer out to PIO. Return the number of words written.
  size_t write(const PioBuffer& buf, uint64_t timeoutUs)
  {
    auto endTime = make_timeout_time_us(timeoutUs);
    for (size_t i = 0; i < buf.size; ++i)
    {
      if (!waitForTxBufferUntil(endTime)) return i;
      uint32_t scratch;
      buf.pack(scratch, i);
      pio_sm_put(pio_, sm_, scratch);
    }
    return buf.size;
  }

  // Write a word out to PIO. Return the number of words written (0 or 1).
  size_t write(uint32_t val, uint64_t timeoutUs)
  {
    auto endTime = make_timeout_time_us(timeoutUs);
    if (!waitForTxBufferUntil(endTime)) return 0;
    pio_sm_put(pio_, sm_, val);
    return 1;
  }

  // Read a buffer from PIO. Return the number of words read.
  size_t read(PioBuffer& buf, uint64_t timeoutUs)
  {
    auto endTime = make_timeout_time_us(timeoutUs);
    for (size_t i = 0; i < buf.size; ++i)
    {
      if (!waitForRxBufferUntil(endTime)) return i;
      uint32_t scratch = pio_sm_get(pio_, sm_);
      buf.unpack(scratch, i);
    }
    return buf.size;
  }

  // Read a buffer from PIO. Return the number of words read (0 or 1)
  size_t read(uint32_t& val, uint64_t timeoutUs)
  {
    auto endTime = make_timeout_time_us(timeoutUs);
    if (!waitForRxBufferUntil(endTime)) return 0;
    val = pio_sm_get(pio_, sm_);
    return 1;
  }
  
  // Write a buffer out to PIO, blocking until complete
  void write(const PioBuffer& buf)
  {
    for (size_t i = 0; i < buf.size; ++i)
    {
      uint32_t scratch;
      buf.pack(scratch, i);
      pio_sm_put_blocking(pio_, sm_, scratch);
    }
  }

  // Write a word out to PIO, blocking until complete
  void write(uint32_t val)
  {
    pio_sm_put_blocking(pio_, sm_, val);
  }

  // Read a buffer from PIO, blocking until complete
  void read(PioBuffer& buf)
  {
    for (size_t i = 0; i < buf.size; ++i)
    {
      uint32_t scratch = pio_sm_get_blocking(pio_, sm_);
      buf.unpack(scratch, i);
    }
  }

  // Read a buffer from PIO, blocking until complete
  void read(uint32_t& val)
  {
    val = pio_sm_get_blocking(pio_, sm_);
  }

  virtual void reset()
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
  std::shared_ptr<PioIrqHandler> irq_;
  std::vector<std::shared_ptr<PioIrqEventConnection>> eventConnections_;
  PIO pio_;
};

RttiCache<PioMachine::PioProgram, const PIO&, const pio_program*> PioMachine::cachedPrograms;
RttiCache<PioIrqHandler, PIO, uint, irq_handler_t> PioMachine::cachedHandlers;
RttiCache<PioIrqEventConnection, PIO, uint, pio_interrupt_source_t> PioMachine::cachedIrqConnections;