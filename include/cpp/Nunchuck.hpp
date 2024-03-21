#pragma once

#include "I2CInterface.hpp"
#include "Button.hpp"
#include "Logging.hpp"
#include "Math.hpp"
#include "Vector.hpp"

class Nunchuck
{
  static constexpr uint8_t Addr = 0x52;
  static constexpr uint32_t TxRxInterval = 5;
  static constexpr uint32_t ConnectInterval = 15;
  static constexpr uint32_t TxRxTimeout = 16;
  static constexpr uint32_t ReconnectInterval = 100;

  static constexpr uint8_t Init1[] {0xF0, 0x55};
  static constexpr uint8_t Init2[] {0xFB, 0x00};
  static constexpr uint8_t StateRegister[] {0x00};
  static constexpr uint8_t CalibRegister[] {0x20};

  static constexpr int I2CBaud = 100000;

public:
  Nunchuck(I2CInterface& i2c, bool autoconnect = true)
    : i2c_{i2c}
    , nextActionTime_{get_absolute_time()}
    , connected_{false}
    , autoconnect_{autoconnect}
    , c_{data_[5], 1, 0}
    , z_{data_[5], 0, 0}
    , c{c_}
    , z{z_}
  {
    this->autoconnect();
  }

  Nunchuck(i2c_inst_t* i2cInst, uint dataPin, uint clkPin, bool autoconnect = true)
    : ownedi2c_(std::make_unique<I2CInterface>(i2cInst, dataPin, clkPin, I2CBaud))
    , i2c_{*ownedi2c_.get()}
    , nextActionTime_{get_absolute_time()}
    , connected_{false}
    , autoconnect_{autoconnect}
    , c_{data_[5], 1, 0}
    , z_{data_[5], 0, 0}
    , c{c_}
    , z{z_}
  {
    this->autoconnect();
  }

  void autoconnect()
  {
    if (autoconnect_ && !connected_)
    {
      connect();
    }
  }

  bool connect()
  {
    sleep_until(nextActionTime_);
    bool ok = true;
    DEBUG_LOG("Nunchuck connecting");
    sleep_ms(TxRxInterval);
    ok &= (i2c_.write_blocking_until(Addr, Init1, 2, false, make_timeout_time_ms(TxRxTimeout)) == 2);
    sleep_ms(TxRxInterval);
    ok &= (i2c_.write_blocking_until(Addr, Init2, 2, false, make_timeout_time_ms(TxRxTimeout)) == 2);
    if (ok)
    {
      nextActionTime_ = make_timeout_time_ms(ConnectInterval);
      connected_ = true;
      readCalibration();
      DEBUG_LOG_IF(connected_, "Nunchuck connected");
    }
    else
    {
      disconnect();
    }
    
    return connected_;
  }

  void disconnect()
  {
    // This is mostly symbolic, just forces the next call to re-init
    connected_ = false;
    nextActionTime_ = make_timeout_time_ms(ReconnectInterval);
    DEBUG_LOG("Nunchuck disconnected");
  }

  bool ok()
  {
    return connected_;
  }

  bool readCalibration()
  {
    DEBUG_LOG("Reading nunchuck calibration...");
    autoconnect();
    writei2c(CalibRegister, 1);
    uint8_t calib[16];
    readi2c(calib, 16);
    if (connected_)
    {
      // Read calibration
      accelX0g_ = (((uint16_t)calib[0]) << 2) | ((calib[3] >> 0) & 0x3);
      accelY0g_ = (((uint16_t)calib[1]) << 2) | ((calib[3] >> 2) & 0x3);
      accelZ0g_ = (((uint16_t)calib[2]) << 2) | ((calib[3] >> 4) & 0x3);
      accelX1g_ = (((uint16_t)calib[4]) << 2) | ((calib[7] >> 0) & 0x3);
      accelY1g_ = (((uint16_t)calib[5]) << 2) | ((calib[7] >> 2) & 0x3);
      accelZ1g_ = (((uint16_t)calib[6]) << 2) | ((calib[7] >> 4) & 0x3);
      stickXMax_ = calib[8];
      stickXMin_ = calib[9];
      stickXCenter_ = calib[10];
      stickYMax_ = calib[11];
      stickYMin_ = calib[12];
      stickYCenter_ = calib[13];
      DEBUG_LOG("accelX0g " << (int)accelX0g_);
      DEBUG_LOG("accelY0g " << (int)accelY0g_);
      DEBUG_LOG("accelZ0g " << (int)accelZ0g_);
      DEBUG_LOG("accelX1g " << (int)accelX1g_);
      DEBUG_LOG("accelY1g " << (int)accelY1g_);
      DEBUG_LOG("accelZ1g " << (int)accelZ1g_);
      DEBUG_LOG("stickXMax " << (int)stickXMax_);
      DEBUG_LOG("stickXMin " << (int)stickXMin_);
      DEBUG_LOG("stickXCenter " << (int)stickXCenter_);
      DEBUG_LOG("stickYMax " << (int)stickYMax_);
      DEBUG_LOG("stickYMin " << (int)stickYMin_);
      DEBUG_LOG("stickYCenter " << (int)stickYCenter_);
    }
    return connected_;
  }

  bool requestControllerState()
  {
    autoconnect();
    writei2c(StateRegister, 1);
    return connected_;
  }

  bool fetchControllerState()
  {
    autoconnect();
    readi2c(data_, 6);
    if (connected_)
    {
      c_.update();
      z_.update();
    }
    return connected_;
  }

  uint8_t rawStickX()
  {
    return data_[0];
  }

  uint8_t rawStickY()
  {
    return data_[1];
  }

  float stickX()
  {
    return std::clamp(normalize3Point(rawStickX(), stickXMin_, stickXCenter_, stickXMax_), -1.0f, 1.0f);
  }

  float stickY()
  {
    return std::clamp(normalize3Point(rawStickY(), stickYMin_, stickYCenter_, stickYMax_), -1.0f, 1.0f);
  }
  
  uint16_t rawAccelX()
  {
    return (((uint16_t)data_[2]) << 2) | ((data_[5] >> 2) & 0x3);
  }

  uint16_t rawAccelY()
  {
    return (((uint16_t)data_[3]) << 2) | ((data_[5] >> 4) & 0x3);
  }

  uint16_t rawAccelZ()
  {
    return (((uint16_t)data_[4]) << 2) | ((data_[5] >> 6) & 0x3);
  }

  float accelX()
  {
    return remap(rawAccelX(), accelX0g_, accelX1g_, 0.0f, 1.0f);
  }

  float accelY()
  {
    return remap(rawAccelY(), accelY0g_, accelY1g_, 0.0f, 1.0f);
  }

  float accelZ()
  {
    return remap(rawAccelZ(), accelZ0g_, accelZ1g_, 0.0f, 1.0f);
  }

  // In degrees
  float pitch()
  {
    Vec3f d{accelX(),accelY(),accelZ()};
    d = d.normalize();
    return std::asin(-d.Y) * (180.0f / M_PI);
  }

  // In degrees
  float yaw()
  {
    Vec3f d{accelX(),accelY(),accelZ()};
    d = d.normalize();
    return std::atan2(d.X, d.Z) * (180.0f / M_PI);
  }
  
private:
  std::unique_ptr<I2CInterface> ownedi2c_;
  I2CInterface& i2c_;
  absolute_time_t nextActionTime_;
  bool connected_;
  bool autoconnect_;
  uint8_t data_[6];
  RegisterButton c_;
  RegisterButton z_;

  // Cal data
  uint16_t accelX0g_ = 0;
  uint16_t accelY0g_ = 0;
  uint16_t accelZ0g_ = 0;
  uint16_t accelX1g_ = 512;
  uint16_t accelY1g_ = 512;
  uint16_t accelZ1g_ = 512;
  uint8_t stickXMin_ = 0;
  uint8_t stickXCenter_ = 127;
  uint8_t stickXMax_ = 255;
  uint8_t stickYMin_ = 0;
  uint8_t stickYCenter_ = 127;
  uint8_t stickYMax_ = 255;

  void readi2c(uint8_t* data, size_t len)
  {
    if (connected_)
    {
      busy_wait_until(nextActionTime_);
      if (i2c_.read_blocking_until(Addr, data, len, false, make_timeout_time_ms(TxRxTimeout)) == len)
      {
        nextActionTime_ = make_timeout_time_ms(TxRxInterval);
      }
      else
      {
        DEBUG_LOG("Nunchuck read failed!");
        disconnect();
      }
    }
  }

  void writei2c(const uint8_t* data, size_t len)
  {
    if (connected_)
    {
      busy_wait_until(nextActionTime_);
      if (i2c_.write_blocking_until(Addr, data, len, false, make_timeout_time_ms(TxRxTimeout)) == len)
      {
        nextActionTime_ = make_timeout_time_ms(TxRxInterval);
      }
      else
      {
        DEBUG_LOG("Nunchuck write failed!");
        disconnect();
      }
    }
  }

public:
  const Button& c;
  const Button& z;
};
