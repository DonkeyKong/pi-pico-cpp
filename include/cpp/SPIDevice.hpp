#pragma once

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <hardware/gpio.h>

#include <vector>

class SPIDevice
{
public:
  SPIDevice(spi_inst_t* spiInstance,
            uint misoPin,
            uint mosiPin,
            uint clockPin,
            uint chipSelectPin,
            uint32_t maxBusSpeedHz = 488000,
            uint32_t maxTransferSizeBytes = 4096
          )
    : spiInstance{spiInstance}
    , misoPin{misoPin}
    , mosiPin{mosiPin}
    , clockPin{clockPin}
    , chipSelectPin{chipSelectPin}
    , maxBusSpeedHz{maxBusSpeedHz}
    , maxTransferSizeBytes{maxTransferSizeBytes}
  {
    spi_init((spi_inst_t*)spiInstance, maxBusSpeedHz);
    gpio_set_function(misoPin, GPIO_FUNC_SPI);
    gpio_set_function(clockPin, GPIO_FUNC_SPI);
    gpio_set_function(mosiPin, GPIO_FUNC_SPI);
    gpio_set_function(chipSelectPin, GPIO_FUNC_SPI);
  }

  ~SPIDevice()
  {
    spi_deinit((spi_inst_t*)spiInstance);
  }

  int write(const std::vector<uint8_t> &buf)
  {
    return write(buf.data(), buf.size());
  }

  int read(std::vector<uint8_t> &buf)
  {
    return read(buf.data(), buf.size());
  }

  int write(const uint8_t* buf, size_t len)
  {
    int ret = 0;
    uint32_t blocks = len / maxTransferSizeBytes + ((len % maxTransferSizeBytes != 0) ? 1 : 0);
    for (int i=0; i < blocks; i++)
    {
      ret += spi_write_blocking(spiInstance, buf + i*maxTransferSizeBytes, std::min(maxTransferSizeBytes, len - i*maxTransferSizeBytes));
    }

    return ret;
  }

  int read(uint8_t* buf, size_t len)
  {
    int ret = 0;
    uint32_t blocks = len / maxTransferSizeBytes + ((len % maxTransferSizeBytes != 0) ? 1 : 0);
    for (int i=0; i < blocks; i++)
    {
      ret += spi_read_blocking(spiInstance, repeatedTxData, buf + i*maxTransferSizeBytes, std::min(maxTransferSizeBytes, len - i*maxTransferSizeBytes));
    }

    return ret;
  }

  uint8_t repeatedTxData = 0;

  private:
    spi_inst_t* spiInstance;
    uint misoPin;
    uint mosiPin;
    uint clockPin;
    uint chipSelectPin;
    uint32_t maxBusSpeedHz;
    uint32_t maxTransferSizeBytes;
};