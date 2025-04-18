#pragma once

#include "Joybus.hpp"

#include <cstring>

namespace N64Status
{
  enum Flag : uint8_t
  {
    None = 0x00,
    PakInserted = 0x01 << 0,    // A controller pak is inserted
    PakRemoved = 0x01 << 1,     // A controller pak was removed since last status
    AddressCrcError = 0x01 << 2 // The last read or write command contained an address with a bad CRC
  };
}

namespace N64Buttons
{
  enum Flag : uint16_t
  {
    None = 0x00,
  
    A = (0x01 << 15),
    B = (0x01 << 14),
    Z = (0x01 << 13),
    Start = (0x01 << 12),
    PadUp = (0x01 << 11),
    PadDown = (0x01 << 10),
    PadLeft = (0x01 << 9),
    PadRight = (0x01 << 8),
  
    Reset = (0x01 << 7),
    Reserved = (0x01 << 6),
    L = (0x01 << 5),
    R = (0x01 << 4),
    CUp = (0x01 << 3),
    CDown = (0x01 << 2),
    CLeft = (0x01 << 1),
    CRight = (0x01 << 0),
  };
}


struct N64ControllerInfo : public PioBuffer
{
  uint8_t header1 = 0x05;
  uint8_t header2 = 0x00;
  N64Status::Flag status = N64Status::None;

  N64ControllerInfo() : PioBuffer(3) {}

  bool getStatusFlag(N64Status::Flag flag) const
  {
    return (status & flag) != N64Status::None;
  }

  void setStatusFlag(N64Status::Flag flag, bool value)
  {
    if (value)
    {
      status = (N64Status::Flag)(status | flag);
    }
    else
    {
      status = (N64Status::Flag)(status & ~flag);
    }
  }

  virtual void pack(uint32_t& dst, size_t i) const override
  {
    switch (i)
    {
      case 0:
        dst = (uint32_t)header1 << 24;
        break;
      case 1:
        dst = (uint32_t)header2 << 24;
        break;
      case 2:
        dst = (uint32_t)status << 24;
        break;
    }
  }

  virtual void unpack(const uint32_t& src, size_t i) override
  {
    switch (i)
    {
      case 0:
        header1 = (uint8_t)src;
        break;
      case 1:
        header2 = (uint8_t)src;
        break;
      case 2:
        status = (N64Status::Flag)src;
        break;
    }
  }
};

struct N64ControllerButtonState : public PioBuffer
{
  N64Buttons::Flag buttons = N64Buttons::None;
  int8_t xAxis = 0;
  int8_t yAxis = 0;

  N64ControllerButtonState() : PioBuffer(4) {}

  Vect2f getStick()
  {
    return {(float)xAxis, (float)yAxis};
  }

  void setStick(const Vect2f &pos)
  {
    xAxis = (int8_t)pos.x;
    yAxis = (int8_t)pos.y;
  }

  bool getButton(N64Buttons::Flag button) const
  {
    return (button & buttons) != N64Buttons::None;
  }

  void setButton(N64Buttons::Flag button, bool value)
  {
    if (value)
    {
      buttons = (N64Buttons::Flag)(buttons | button);
    }
    else
    {
      buttons = (N64Buttons::Flag)(buttons & ~button);
    }
  }

  // Implementation for PioMachine to use this as as a PioBuffer


  virtual void pack(uint32_t& dst, size_t i) const override
  {
    switch (i)
    {
      case 0:
        dst = (uint32_t)buttons << 16;
        break;
      case 1:
        dst = (uint32_t)buttons << 24;
        break;
      case 2:
        dst = (uint32_t)xAxis << 24;
        break;
      case 3:
        dst = (uint32_t)yAxis << 24;
        break;
    }
  }

  virtual void unpack(const uint32_t& src, size_t i) override
  {
    switch (i)
    {
      case 0:
        buttons = (N64Buttons::Flag)((buttons & 0x00FF) | ((src << 8) & 0xFF00));
        break;
      case 1:
        buttons = (N64Buttons::Flag)((buttons & 0xFF00) | (src & 0x00FF));
        break;
      case 2:
        xAxis = (int8_t)src;
        break;
      case 3:
        yAxis = (int8_t)src;
        break;
    }
  }
};

// Connect an N64 controller to a Pi Pico and read button
// presses, stick movements, and more.
// Only button and stick state is supported at the moment
class N64ControllerIn : JoybusHost
{
  bool padOk_ = false;
public:
  N64ControllerIn(uint pin) : JoybusHost(pin), padOk_{false} { }

  N64ControllerInfo info;
  N64ControllerButtonState state;

  // Update the state of the N64 pad
  void update()
  {
    if (!padOk_)
    {
      padOk_ = command(JoybusCommand::Reset, info);
    }
    else
    {
      padOk_ = command(JoybusCommand::Info, info);
      padOk_ = command(JoybusCommand::ControllerState, state);
    }
  }

  // replace the lower 5 bits of the address with a checksum of the upper 11
  static uint16_t addressChecksum(uint16_t address)
  {
    constexpr uint8_t checksumTable[] = { 0x01, 0x1A, 0x0D, 0x1C, 0x0E, 0x07, 0x19, 0x16, 0x0B, 0x1F, 0x15 };
    uint8_t checksum = 0;
    for (int i=0; i < 11; ++i)
    {
      uint16_t bitmask = 1 << (15-i);
      if ((address & bitmask) != 0)
      {
        checksum ^= checksumTable[i];
      }
    }

    // Shove the checksum into the lower 5 bits
    return (address & 0xFFE0) | (checksum & 0x001F);
  }

  static uint8_t crc(uint8_t* data, size_t size) 
  {
    constexpr uint8_t polynomial = 0x85;
    uint8_t crc = 0x00;  // Initial value

    for (size_t i = 0; i < size; ++i) 
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) 
        {
            if (crc & 0x80) 
            {
                crc = (crc << 1) ^ polynomial;
            } 
            else 
            {
                crc <<= 1;
            }
        }
    }
    return crc;
  }

  bool readAccessory(uint16_t address, JoybusBuffer& readBuffer, bool checkCrc = true)
  {
    if (readBuffer.size != 32)
    {
      DEBUG_LOG("Buffer is wrong size!");
      return false;
    }

    uint8_t deviceCrc = 0;
    if (!command(JoybusCommand::ReadAccessory, addressChecksum(address), readBuffer, deviceCrc))
    {
      DEBUG_LOG("ReadAccessory comm failure!");
      return false;
    }

    if (checkCrc)
    {
      uint8_t hostCrc = crc(readBuffer.data, readBuffer.size);
      if (hostCrc != deviceCrc)
      {
        DEBUG_LOG("ReadAccessory crc mismatch!");
        return false;
      }
    }

    return true;
  }

  bool writeAccessory(uint16_t address, const JoybusBuffer& writeBuffer, bool checkCrc = true)
  {
    if (writeBuffer.size != 32)
    {
      DEBUG_LOG("Buffer is wrong size!");
      return false;
    }

    uint8_t deviceCrc = 0;
    if (!command(JoybusCommand::WriteAccessory, addressChecksum(address), writeBuffer, deviceCrc))
    {
      DEBUG_LOG("WriteAccessory comm failure!");
      return false;
    }

    if (checkCrc)
    {
      uint8_t hostCrc = crc(writeBuffer.data, writeBuffer.size);
      if (hostCrc != deviceCrc)
      {
        DEBUG_LOG("WriteAccessory crc mismatch!");
        return false;
      }
    }

    return true;
  }

  void rumble(bool enabled)
  {
    uint8_t data[32] {0xFF};
    JoybusBuffer buf(&data[0], 32);
    memset(&data[0], enabled ? 0x01 : 0x00, 32);
    writeAccessory(0xC000, buf, false);
  }

  void initRumble()
  {
    // This init rumble procedure is 100% cargo cult, 
    // from https://github.com/DavidPagels/retro-pico-switch/blob/master/src/otherController/n64/N64Controller.cpp
    uint8_t data[32];
    JoybusBuffer buf(&data[0], 32);

    memset(&data[0], 0xEE, 32);
    writeAccessory(0x8000, buf, false);
    readAccessory(0x8000, buf, false);

    memset(&data[0], 0x80, 32);
    writeAccessory(0x8000, buf, false);
    readAccessory(0x8000, buf, false);

    rumble(false);
  }
};

class N64ControllerOut : JoybusClient
{
  JoybusCommand cmd_;
  N64ControllerInfo info_;
  N64ControllerButtonState state_;

public:
  N64ControllerOut(uint pin) 
    : JoybusClient(pin)
  {}

protected:
  virtual PioBuffer* onRecieveCommand(JoybusCommand cmd)
  {
    cmd_ = cmd;

    // We only handle reset, status, and button state commands, none of which have data attached
    return nullptr;
  }

  // Command data recieived. Finalize the result and hand over a buffer
  virtual PioBuffer* onSendResult()
  {
    switch (cmd_)
    {
      case JoybusCommand::Reset:
      case JoybusCommand::Info:
        return &info_;
      case JoybusCommand::ControllerState:
        return &state_;
      default:
        return nullptr;
    }
  }
};

std::ostream& operator<<(std::ostream &os, const N64ControllerButtonState &c)
{
  os << "{" << std::endl
    << "  \"PadRight\" : " << c.getButton(N64Buttons::PadRight) << std::endl
    << "  \"PadLeft\" : " << c.getButton(N64Buttons::PadLeft) << std::endl
    << "  \"PadDown\" : " << c.getButton(N64Buttons::PadDown) << std::endl
    << "  \"PadUp\" : " << c.getButton(N64Buttons::PadUp) << std::endl
    << "  \"Start\" : " << c.getButton(N64Buttons::Start) << std::endl
    << "  \"Z\" : " << c.getButton(N64Buttons::Z) << std::endl
    << "  \"B\" : " << c.getButton(N64Buttons::B) << std::endl
    << "  \"A\" : " << c.getButton(N64Buttons::A) << std::endl
    << "  \"CRight\" : " << c.getButton(N64Buttons::CRight) << std::endl
    << "  \"CLeft\" : " << c.getButton(N64Buttons::CLeft) << std::endl
    << "  \"CDown\" : " << c.getButton(N64Buttons::CDown) << std::endl
    << "  \"CUp\" : " << c.getButton(N64Buttons::CUp) << std::endl
    << "  \"R\" : " << c.getButton(N64Buttons::R) << std::endl
    << "  \"L\" : " << c.getButton(N64Buttons::L) << std::endl
    << "  \"Reserved\" : " << c.getButton(N64Buttons::Reserved) << std::endl
    << "  \"Reset\" : " << c.getButton(N64Buttons::Reset) << std::endl
    << "  \"StickX\" : " << (int)c.xAxis << std::endl
    << "  \"StickY\" : " << (int)c.yAxis << std::endl
    << "}";
  return os;
}

std::ostream& operator<<(std::ostream &os, const N64ControllerInfo &c)
{
  os << "{" << std::endl
    << "  \"Status\" : " << (int)c.status << std::endl
    << "  \"Controller ID\" : 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)c.header1 << std::setw(2) << (int)c.header2 << std::dec << std::endl
    << "  \"Pak Inserted\" : " << c.getStatusFlag(N64Status::PakInserted) << std::endl
    << "  \"Pak Removed\" : " << c.getStatusFlag(N64Status::PakRemoved) << std::endl
    << "  \"CRC Error\" : " << c.getStatusFlag(N64Status::AddressCrcError) << std::endl
    << "}";
  return os;
}