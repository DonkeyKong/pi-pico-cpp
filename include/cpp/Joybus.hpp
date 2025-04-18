#pragma once

#include "Logging.hpp"
#include "Math.hpp"
#include "Pio.hpp"

#include "joybus_host.pio.h"
#include "joybus_client.pio.h"

#include <iostream>
#include <iomanip>
#include <array>

enum class JoybusCommand : uint8_t
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

struct JoybusBuffer : public PioBuffer
{
  uint8_t* data;
  JoybusBuffer(uint8_t* data, size_t size) : PioBuffer(size), data(data) {}

  virtual void pack(uint32_t& dst, size_t i) const override
  {
    // Data being sent is inverted, and left shifted 24 places
    // Only one byte is sent per Tx buffer
    dst = (~data[i]) << 24;
  }

  virtual void unpack(const uint32_t& src, size_t i) override
  {
    // Data received is not inverted, and just needs to be truncated
    // Only one byte is received per Rx buffer
    if (i < size) data[i] = (uint8_t)(src);
  }
};

struct JoybusBuffer16 : public PioBuffer
{
  uint16_t* data;
  JoybusBuffer16(uint16_t* data, size_t size) : PioBuffer(size*2), data(data) {}

  virtual void pack(uint32_t& dst, size_t i) const override
  {
    // Data being sent is inverted, and left shifted 24 places
    // Only one byte is sent per Tx buffer
    if (i % 2 == 0)
    {
      dst = ~((uint32_t)data[i/2] << 16);
    }
    else
    {
      dst = ~((uint32_t)data[i/2] << 24);
    }
  }

  virtual void unpack(const uint32_t& src, size_t i) override
  {
    if (i % 2 == 0)
    {
      data[i/2] = (data[i/2] & 0x00FF) | ((src << 8) & 0xFF00);
    }
    else
    {
      data[i/2] = (src & 0x00FF) | (data[i/2] & 0xFF00);
    }
  }
};

class JoybusHost : PioMachine
{
  static constexpr uint64_t host_readTimeoutUs = 5000;
  static constexpr uint64_t host_writeTimeoutUs = 5000;
  static constexpr uint64_t host_commandIntervalUs = 150;
  static constexpr uint64_t host_writeAccessoryIntervalUs = 500;

  absolute_time_t commandAllowedTime;
public:
  JoybusHost(uint pin) : PioMachine(&joybus_host_program)
  {
    config_ = joybus_host_program_get_default_config(prog_->offset());

    // Configure IOs for bidirectional Joybus comms
    gpio_set_pulls(pin, false, false);
    gpio_set_input_enabled(pin, true);

    // Map the state machine's i/o pin groups to one pin, namely the `pin`
    sm_config_set_in_pins(&config_, pin);
    sm_config_set_out_pins(&config_, pin, 1);
    sm_config_set_set_pins(&config_, pin, 1);
    sm_config_set_jmp_pin(&config_, pin);

    sm_config_set_in_shift(&config_, false, true, 8);
    sm_config_set_out_shift(&config_, false, true, 8);

    // Set this pin's GPIO function (connect PIO to the pad)
    pio_gpio_init(pio_, pin);

    // Set the pin direction to input at the PIO
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin, 1, false);
    sm_config_set_clkdiv(&config_, 15.625f);

    // Load our configuration, and jump to the start of the program
    pio_sm_init(pio_, sm_, prog_->offset(), &config_);

    // Set the state machine running
    pio_sm_set_enabled(pio_, sm_, true);

    commandAllowedTime = get_absolute_time();
  }

  // Send a command with no payload, then read the response into a single buffer
  bool command(JoybusCommand cmd, PioBuffer& responseBuffer)
  {
    JoybusBuffer commandBuffer((uint8_t*)(&cmd), 1);

    size_t expectedWordsWritten = 2 + commandBuffer.size;
    size_t sendPayloadBits = (commandBuffer.size) * 8 - 1;
    size_t responsePayloadBits = (responseBuffer.size) * 8 - 1;

    sleep_until(commandAllowedTime);
    size_t wordsWritten = 0;
    wordsWritten = write(sendPayloadBits, host_writeTimeoutUs);
    wordsWritten += write(commandBuffer, host_writeTimeoutUs);
    wordsWritten += write(responsePayloadBits, host_writeTimeoutUs);
    if (wordsWritten != expectedWordsWritten)
    {
      DEBUG_LOG("Wrote " << wordsWritten << " but expected " << expectedWordsWritten << " words, resetting...");
      reset();
      commandAllowedTime = make_timeout_time_us(host_commandIntervalUs);
      return false;
    }

    // Read the response, however long that is
    int wordsRead = read(responseBuffer, host_readTimeoutUs);
    if (wordsRead != responseBuffer.size)
    {
      DEBUG_LOG("Read " << wordsRead << " but expected " << responseBuffer.size << " words, resetting...");
      reset();
      commandAllowedTime = make_timeout_time_us(host_commandIntervalUs);
      return false;
    }

    commandAllowedTime = make_timeout_time_us(host_commandIntervalUs);
    return true;
  }

  bool command(JoybusCommand cmd, uint16_t address, PioBuffer& responseBuffer, uint8_t& crc)
  {
    JoybusBuffer commandBuffer((uint8_t*)(&cmd), 1);
    JoybusBuffer16 addressBuffer(&address, 1);
    JoybusBuffer crcBuffer((uint8_t*)(&crc), 1);

    size_t sendWords = commandBuffer.size + addressBuffer.size;
    size_t recWords = responseBuffer.size + crcBuffer.size;
    size_t expectedWordsWritten = 2 + sendWords;
    size_t sendPayloadBits = (sendWords) * 8 - 1;
    size_t responsePayloadBits = (recWords) * 8 - 1;

    sleep_until(commandAllowedTime);
    size_t wordsWritten = 0;
    wordsWritten += write(sendPayloadBits, host_writeTimeoutUs);
    wordsWritten += write(commandBuffer, host_writeTimeoutUs);
    wordsWritten += write(addressBuffer, host_writeTimeoutUs);
    wordsWritten += write(responsePayloadBits, host_writeTimeoutUs);

    if (wordsWritten != expectedWordsWritten)
    {
      DEBUG_LOG("Wrote " << wordsWritten << " but expected " << expectedWordsWritten << " words, resetting...");
      reset();
      commandAllowedTime = make_timeout_time_us(host_writeAccessoryIntervalUs);
      return false;
    }

    // Read the response, however long that is
    int wordsRead = 0;
    wordsRead += read(responseBuffer, host_readTimeoutUs);
    wordsRead += read(crcBuffer, host_readTimeoutUs);

    if (wordsRead != recWords)
    {
      DEBUG_LOG("Read " << wordsRead << " but expected " << recWords << " words, resetting...");
      reset();
      commandAllowedTime = make_timeout_time_us(host_writeAccessoryIntervalUs);
      return false;
    }

    commandAllowedTime = make_timeout_time_us(host_writeAccessoryIntervalUs);
    return true;
  }

  bool command(JoybusCommand cmd, uint16_t address, const PioBuffer& sendBuffer, uint8_t& crc)
  {
    JoybusBuffer commandBuffer((uint8_t*)(&cmd), 1);
    JoybusBuffer16 addressBuffer(&address, 1);
    JoybusBuffer crcBuffer((uint8_t*)(&crc), 1);

    size_t sendWords = commandBuffer.size + addressBuffer.size + sendBuffer.size;
    size_t recWords = crcBuffer.size;
    size_t expectedWordsWritten = 2 + sendWords;
    size_t sendPayloadBits = (sendWords) * 8 - 1;
    size_t responsePayloadBits = (recWords) * 8 - 1;

    sleep_until(commandAllowedTime);
    size_t wordsWritten = 0;
    wordsWritten += write(sendPayloadBits, host_writeTimeoutUs);
    wordsWritten += write(commandBuffer, host_writeTimeoutUs);
    wordsWritten += write(addressBuffer, host_writeTimeoutUs);
    wordsWritten += write(sendBuffer, host_writeTimeoutUs);
    wordsWritten += write(responsePayloadBits, host_writeTimeoutUs);

    if (wordsWritten != expectedWordsWritten)
    {
      DEBUG_LOG("Wrote " << wordsWritten << " but expected " << expectedWordsWritten << " words, resetting...");
      reset();
      commandAllowedTime = make_timeout_time_us(host_writeAccessoryIntervalUs);
      return false;
    }

    // Read the response, however long that is
    int wordsRead = 0;
    wordsRead += read(crcBuffer, host_readTimeoutUs);

    if (wordsRead != recWords)
    {
      DEBUG_LOG("Read " << wordsRead << " but expected " << recWords << " words, resetting...");
      reset();
      commandAllowedTime = make_timeout_time_us(host_writeAccessoryIntervalUs);
      return false;
    }

    commandAllowedTime = make_timeout_time_us(host_writeAccessoryIntervalUs);
    return true;
  }
};

class JoybusClient : PioMachine
{
  static constexpr uint client_readTimeoutUs = 5000;
  static constexpr uint client_writeTimeoutMs = 5000;
  static std::vector<JoybusClient*> instances;

  enum class ClientState
  {
    WaitingForCommand,
    ReceivingRequest,
    SendingReply,
    Stopping
  };

  ClientState state_ = ClientState::WaitingForCommand;
  PioBuffer* recBuf;
  PioBuffer* sendBuf;
  size_t recBufInd, sendBufInd;

  void tryBeginCommand()
  {
    if (state_ != ClientState::WaitingForCommand)
    {
      return;
    }

    // If this client has nothing in its rx buffer, skip
    if (!pio_sm_is_rx_fifo_empty(pio_, sm_))
    {
      uint32_t command = pio_sm_get(pio_, sm_);
      recBuf = onRecieveCommand((JoybusCommand)command);
      recBufInd = 0;
      state_ = ClientState::ReceivingRequest;
    }
  }

  void tryReceiveData()
  {
    if (state_ != ClientState::ReceivingRequest)
    {
      return;
    }

    while (!pio_sm_is_rx_fifo_empty(pio_, sm_) &&
            recBuf != nullptr &&
            recBufInd < recBuf->size)
    {
      recBuf->unpack(pio_sm_get(pio_, sm_), recBufInd);
      recBufInd += 4;
    }

    if (recBuf == nullptr || recBufInd >= recBuf->size)
    {
      sendBuf = onSendResult();
      sendBufInd = 0;
      state_ = ClientState::SendingReply;
    }
  }

  void tryPushData()
  {
    if (state_ != ClientState::SendingReply)
    {
      return;
    }

    while (!pio_sm_is_tx_fifo_full(pio_, sm_) &&
            sendBuf != nullptr &&
            sendBufInd < sendBuf->size)
    {
      uint32_t scratch;
      sendBuf->pack(scratch, sendBufInd);
      pio_sm_put(pio_, sm_, scratch);
      sendBufInd += 4;
    }

    if (sendBuf == nullptr || sendBufInd >= sendBuf->size)
    {
      state_ = ClientState::Stopping;
    }
  }

  void tryPushStop()
  {
    if (state_ != ClientState::Stopping)
    {
      return;
    }

    if (!pio_sm_is_tx_fifo_full(pio_, sm_))
    {
      pio_sm_put(pio_, sm_, 0);
      state_ = ClientState::WaitingForCommand;
    }
  }

  static void canPullData()
  {
    for (auto client : instances)
    {
      client->tryBeginCommand();
      client->tryReceiveData();
    }
  }

  static void canPushData()
  {
    for (auto client : instances)
    {
      client->tryPushData();
      client->tryPushStop();
    }
  }

protected:
  // A command was received. Return a byte buffer to fill with command data
  virtual PioBuffer* onRecieveCommand(JoybusCommand cmd) = 0;

  // Command data recieived. Finalize the result and hand over a buffer
  virtual PioBuffer* onSendResult() = 0;

public:
  JoybusClient(uint pin) : PioMachine(&joybus_client_program)
  {
    instances.push_back(this);

    config_ = joybus_client_program_get_default_config(prog_->offset());

    // On joybus the client is responsible for the pullup, but
    // this pullup needs to be 1k, not the 50k to 80k ohm of
    // the pi pico gpio system. So set all pulls false and
    // rely on a hardware resistor being installed.
    gpio_set_pulls(pin, false, false);
    gpio_set_input_enabled(pin, true);

    // Map the state machine's i/o pin groups to one pin, namely the `pin`
    sm_config_set_in_pins(&config_, pin);
    sm_config_set_out_pins(&config_, pin, 1);
    sm_config_set_set_pins(&config_, pin, 1);
    sm_config_set_jmp_pin(&config_, pin);

    // This machine uses autopull and autopush!
    sm_config_set_in_shift(&config_, false, true, 32);
    sm_config_set_out_shift(&config_, false, true, 32);

    // Set this pin's GPIO function (connect PIO to the pad)
    pio_gpio_init(pio_, pin);

    // Set the pin direction to input at the PIO
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin, 1, false);
    sm_config_set_clkdiv(&config_, 15.625f);

    // Load our configuration, and jump to the start of the program
    pio_sm_init(pio_, sm_, prog_->offset(), &config_);

    // Setup the IRQ callbacks for Rx and Tx state changes
    enableIrq(PioIrqType::RxFifoNotEmpty, 0, &JoybusClient::canPullData);
    enableIrq(PioIrqType::TxFifoNotFull, 1, &JoybusClient::canPushData);

    // Set the state machine running
    pio_sm_set_enabled(pio_, sm_, true);
  }

  ~JoybusClient()
  {
    // Erase self from instances list
    for (auto itr = instances.begin(); itr != instances.end(); ++itr)
    {
      if (*itr == this)
      {
        instances.erase(itr);
        break;
      }
    }
  }
};