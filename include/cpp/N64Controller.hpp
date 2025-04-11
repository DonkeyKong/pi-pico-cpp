#pragma once

#include "Logging.hpp"
#include "Math.hpp"
#include "Pio.hpp"

#include "joybus_host.pio.h"
#include "joybus_client.pio.h"

#include <iostream>
#include <iomanip>

enum class RequestCommand : uint8_t
{
  Info = 0x00,
  ControllerState = 0x01,
  ReadAccessory = 0x02,
  WriteAccessory = 0x03,
  ReadEEPROM = 0x04,
  WriteEEPROM = 0x05,
  ReadKeypress = 0x13, // For N64DD Randnet keyboard
  Reset = 0xFF,
};

enum class StatusFlag : uint8_t
{
  None = 0x00,
  PakInserted = 0x01 << 7,    // A controller pak is inserted
  PakRemoved = 0x01 << 6,     // A controller pak was removed since last status
  AddressCrcError = 0x01 << 5 // The last read or write command contained an address with a bad CRC
};

enum class ButtonFlag : uint16_t
{
  None = 0x00,

  A = (0x01 << 7),
  B = (0x01 << 6),
  Z = (0x01 << 5),
  Start = (0x01 << 4),
  PadUp = (0x01 << 3),
  PadDown = (0x01 << 2),
  PadLeft = (0x01 << 1),
  PadRight = (0x01 << 0),

  Reset = (0x01 << 15),
  Reserved = (0x01 << 14),
  L = (0x01 << 13),
  R = (0x01 << 12),
  CUp = (0x01 << 11),
  CDown = (0x01 << 10),
  CLeft = (0x01 << 9),
  CRight = (0x01 << 8),
};

struct __attribute__((packed)) N64ControllerState
{
  // In-memory rep on little endian is like
  //    reserved            status              header2              header1
  // 7 6 5 4 3 2 1 0    7 6 5 4 3 2 1 0    7 6 5 4 3 2 1 0       7 6 5 4 3 2 1 0
  //    yAxis               xAxis                       buttons
  // 7 6 5 4 3 2 1 0    7 6 5 4 3 2 1 0    15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

  uint8_t header1 = 0x05;
  uint8_t header2 = 0x00;
  StatusFlag status = StatusFlag::None;
  
  ButtonFlag buttons = ButtonFlag::None;
  int8_t xAxis = 0;
  int8_t yAxis = 0;
  
  PioByteBuffer infoBuffer()
  {
    return {(uint8_t*)&header1, 3, PioBufferPacking::MsbToLsb};
  }
  
  PioByteBuffer pollBuffer()
  {
    return {(uint8_t*)&buttons, 4, PioBufferPacking::MsbToLsb};
  }

  bool getStatusFlag(StatusFlag flag) const
  {
    return (StatusFlag)((uint8_t)status & (uint8_t)flag) != StatusFlag::None;
  }

  void setStatusFlag(StatusFlag flag, bool value)
  {
    if (value)
    {
      status = (StatusFlag)((uint8_t)status | (uint8_t)flag);
    }
    else
    {
      status = (StatusFlag)((uint8_t)status & ~((uint8_t)flag));
    }
  }

  Vect2f getStick()
  {
    return {(float)xAxis, (float)yAxis};
  }

  void setStick(const Vect2f &pos)
  {
    xAxis = (int8_t)pos.x;
    yAxis = (int8_t)pos.y;
  }

  bool getButton(ButtonFlag button) const
  {
    return (ButtonFlag)((uint16_t)button & (uint16_t)buttons) != ButtonFlag::None;
  }

  void setButton(ButtonFlag button, bool value)
  {
    if (value)
    {
      buttons = (ButtonFlag)((uint16_t)buttons | (uint16_t)button);
    }
    else
    {
      buttons = (ButtonFlag)((uint16_t)buttons & ~((uint16_t)button));
    }
  }
};

std::ostream &operator<<(std::ostream &os, const N64ControllerState &c)
{
  os << "{" << std::endl
     << "  \"Status\" : " << (int)c.status << std::endl
     << "  \"Controller ID\" : 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)c.header1 << std::setw(2) << (int)c.header2 << std::dec << std::endl
     << "  \"Pak Inserted\" : " << c.getStatusFlag(StatusFlag::PakInserted) << std::endl
     << "  \"Pak Removed\" : " << c.getStatusFlag(StatusFlag::PakRemoved) << std::endl
     << "  \"CRC Error\" : " << c.getStatusFlag(StatusFlag::AddressCrcError) << std::endl
     << "  \"PadRight\" : " << c.getButton(ButtonFlag::PadRight) << std::endl
     << "  \"PadLeft\" : " << c.getButton(ButtonFlag::PadLeft) << std::endl
     << "  \"PadDown\" : " << c.getButton(ButtonFlag::PadDown) << std::endl
     << "  \"PadUp\" : " << c.getButton(ButtonFlag::PadUp) << std::endl
     << "  \"Start\" : " << c.getButton(ButtonFlag::Start) << std::endl
     << "  \"Z\" : " << c.getButton(ButtonFlag::Z) << std::endl
     << "  \"B\" : " << c.getButton(ButtonFlag::B) << std::endl
     << "  \"A\" : " << c.getButton(ButtonFlag::A) << std::endl
     << "  \"CRight\" : " << c.getButton(ButtonFlag::CRight) << std::endl
     << "  \"CLeft\" : " << c.getButton(ButtonFlag::CLeft) << std::endl
     << "  \"CDown\" : " << c.getButton(ButtonFlag::CDown) << std::endl
     << "  \"CUp\" : " << c.getButton(ButtonFlag::CUp) << std::endl
     << "  \"R\" : " << c.getButton(ButtonFlag::R) << std::endl
     << "  \"L\" : " << c.getButton(ButtonFlag::L) << std::endl
     << "  \"Reserved\" : " << c.getButton(ButtonFlag::Reserved) << std::endl
     << "  \"Reset\" : " << c.getButton(ButtonFlag::Reset) << std::endl
     << "  \"StickX\" : " << (int)c.xAxis << std::endl
     << "  \"StickY\" : " << (int)c.yAxis << std::endl
     << "}";
  return os;
}

// Connect an N64 controller to a Pi Pico and read button
// presses, stick movements, and more.
// Copying to/from the controller pak isn't really supported
class N64ControllerIn : PioMachine
{
  static constexpr uint host_readTimeoutMs = 5;
  static constexpr uint host_writeTimeoutMs = 5;

  bool padOk_ = false;
  N64ControllerState state_;
public:
  N64ControllerIn(uint pin) : PioMachine(&joybus_host_program), padOk_{false}
  {
    config_ = joybus_host_program_get_default_config(prog_->offset());

    // Configure IOs for bidirectional Joybus comms
    gpio_set_pulls(pin, false, false);

    // Map the state machine's i/o pin groups to one pin, namely the `pin`
    sm_config_set_in_pins(&config_, pin);
    sm_config_set_out_pins(&config_, pin, 1);
    sm_config_set_set_pins(&config_, pin, 1);
    sm_config_set_jmp_pin(&config_, pin);
    

    sm_config_set_in_shift(&config_, false, false, 32);
    sm_config_set_out_shift(&config_, false, false, 32);
    
    // Set this pin's GPIO function (connect PIO to the pad)
    pio_gpio_init(pio_, pin);

    // Set the pin direction to input at the PIO
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin, 1, false);
    sm_config_set_clkdiv(&config_, 15.625f);

    // Load our configuration, and jump to the start of the program
    pio_sm_init(pio_, sm_, prog_->offset(), &config_);

    // Set the state machine running
    pio_sm_set_enabled(pio_, sm_, true);
  }

  const N64ControllerState& state()
  {
    return state_;
  }

  inline bool hostCommand(RequestCommand cmd, PioByteBuffer responseBuffer)
  {
    // Invert commands because joybus sends with PINDIRS, where 1 = low
    uint8_t sendData = ~(uint8_t)cmd;
    size_t sendSize = 1;
    int bytesWritten = write({&sendData, sendSize, PioBufferPacking::MsbToLsb}, host_writeTimeoutMs);
    if (bytesWritten != 1)
    {
      DEBUG_LOG("Wrote " << bytesWritten << " but expected " << sendSize << " bytes, resetting...");
      reset();
      return false;
    }

    // Read the response, however long that is
    int bytesRead = read(responseBuffer, host_readTimeoutMs);
    if (bytesRead != responseBuffer.size)
    {
      DEBUG_LOG("Read " << bytesRead << " but expected " << responseBuffer.size << " bytes, resetting...");
      reset();
      return false;
    }

    return true;
  }

  // Update the state of the N64 pad
  void update()
  {
    if (!padOk_)
    {
      padOk_ = hostCommand(RequestCommand::Reset, state_.infoBuffer());
    }
    else
    {
      padOk_ = hostCommand(RequestCommand::ControllerState, state_.pollBuffer());
    }
  }
};

// Connect an pi pico to an N64 and have it emulate a controller
// Button presses and stick movements are supported.
// Pretending to have a controller pak isn't really supported
//
// Because the N64 queries for info whenever it feels like
// and will disconnect the controller if its does not respond
// promptly, we make use of an interrupt to send data
//
// IRQ 4 is hard coded at the moment
// class N64ControllerOut : PioMachine
// {
//   static constexpr uint client_readTimeoutMs = 1000;
//   static constexpr uint client_writeTimeoutMs = 0;
//   static constexpr uint client_errorWaitMs = 0;

//   N64ControllerState state_;
  
//   N64ControllerOut(uint pin) : PioMachine(&joybus_client_program)
//   {
//     config_ = joybus_client_program_get_default_config(prog_->offset());
    
//     // Configure IOs for bidirectional Joybus comms
//     gpio_set_pulls(pin, true, false);

//     // Map the state machine's i/o pin groups to one pin, namely the `pin`
//     // parameter to this function.
//     sm_config_set_in_pins(&config_, pin);
//     sm_config_set_out_pins(&config_, pin, 1);
//     sm_config_set_set_pins(&config_, pin, 1);

//     sm_config_set_in_shift(&config_, false, false, 32);
//     sm_config_set_out_shift(&config_, false, false, 32);
    
//     // Set this pin's GPIO function (connect PIO to the pad)
//     pio_gpio_init(pio_, pin);
//     // Set the pin direction to input at the PIO
//     pio_sm_set_consecutive_pindirs(pio_, sm_, pin, 1, false);
//     sm_config_set_clkdiv(&config_, 15.625f);

//     // Load our configuration, and jump to the start of the program
//     pio_sm_init(pio_, sm_, prog_->offset(), &config_);
//     // Set the state machine running
//     pio_sm_set_enabled(pio_, sm_, true);
//   }

//   inline bool writeLengthAndBytes(uint32_t data, uint32_t bytesToSend, int timeoutMs = -1)
//   {
//     if (!write((uint32_t)(bytesToSend*8-1), timeoutMs)) return false;
//     if (!write(data, timeoutMs)) return false;
//     return true;
//   }

//   void handleCommand()
//   {
//     // Wait for the N64 to request something
//     RequestCommand value;
//     if (read((uint32_t&)value), client_readTimeoutMs)
//     {
//       DEBUG_LOG("N64 command read timed out, resetting pio...");
//       reset();
//       if constexpr(client_errorWaitMs > 0) busy_wait_ms(client_errorWaitMs);
//       return;
//     }

//     if (value == RequestCommand::Info || value == RequestCommand::Reset)
//     {
//       if (!writeLengthAndBytes(state_.infoMessage(), 3, client_writeTimeoutMs))
//       {
//         DEBUG_LOG("Timed out responding to info cmd...");
//         reset();
//         if constexpr(client_errorWaitMs > 0) busy_wait_ms(client_errorWaitMs);
//         return;
//       }
//     }
//     else if (value == RequestCommand::ControllerState)
//     {
//       if (!writeLengthAndBytes(state_.pollMessage(), 4, client_writeTimeoutMs))
//       {
//         DEBUG_LOG("Timed out responding to poll cmd...");
//         reset();
//         if constexpr(client_errorWaitMs > 0) busy_wait_ms(client_errorWaitMs);
//         return;
//       }
//     }
//   }
//};