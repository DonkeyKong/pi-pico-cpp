// pi-pico-cpp headers
#include <cpp/Time.hpp>
#include <cpp/N64Controller.hpp>
#include <cpp/BootSelButton.hpp>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>
#include <pico/bootrom.h>

// std headers
#include <iostream>

void printInfoDiff(const N64ControllerInfo before, const N64ControllerInfo after)
{
  if (before.getStatusFlag(N64Status::AddressCrcError) != after.getStatusFlag(N64Status::AddressCrcError)) std::cout << "AddressCrcError: " << after.getStatusFlag(N64Status::AddressCrcError) << std::endl;
  if (!before.getStatusFlag(N64Status::PakInserted) && after.getStatusFlag(N64Status::PakInserted)) std::cout << "Pak Inserted" << std::endl;
  if (!before.getStatusFlag(N64Status::PakRemoved) && after.getStatusFlag(N64Status::PakRemoved)) std::cout << "Pak Removed" << std::endl;
};

void printButtonDiff(const N64ControllerButtonState before, const N64ControllerButtonState after)
{
  if (before.getButton(N64Buttons::PadRight) != after.getButton(N64Buttons::PadRight)) std::cout << "PadRight: " << after.getButton(N64Buttons::PadRight) << std::endl;
  if (before.getButton(N64Buttons::PadLeft) != after.getButton(N64Buttons::PadLeft)) std::cout << "PadLeft: " << after.getButton(N64Buttons::PadLeft) << std::endl;
  if (before.getButton(N64Buttons::PadDown) != after.getButton(N64Buttons::PadDown)) std::cout << "PadDown: " << after.getButton(N64Buttons::PadDown) << std::endl;
  if (before.getButton(N64Buttons::PadUp) != after.getButton(N64Buttons::PadUp)) std::cout << "PadUp: " << after.getButton(N64Buttons::PadUp) << std::endl;
  if (before.getButton(N64Buttons::Start) != after.getButton(N64Buttons::Start)) std::cout << "Start: " << after.getButton(N64Buttons::Start) << std::endl;
  if (before.getButton(N64Buttons::Z) != after.getButton(N64Buttons::Z)) std::cout << "Z: " << after.getButton(N64Buttons::Z) << std::endl;
  if (before.getButton(N64Buttons::B) != after.getButton(N64Buttons::B)) std::cout << "B: " << after.getButton(N64Buttons::B) << std::endl;
  if (before.getButton(N64Buttons::A) != after.getButton(N64Buttons::A)) std::cout << "A: " << after.getButton(N64Buttons::A) << std::endl;
  if (before.getButton(N64Buttons::CRight) != after.getButton(N64Buttons::CRight)) std::cout << "CRight: " << after.getButton(N64Buttons::CRight) << std::endl;
  if (before.getButton(N64Buttons::CLeft) != after.getButton(N64Buttons::CLeft)) std::cout << "CLeft: " << after.getButton(N64Buttons::CLeft) << std::endl;
  if (before.getButton(N64Buttons::CDown) != after.getButton(N64Buttons::CDown)) std::cout << "CDown: " << after.getButton(N64Buttons::CDown) << std::endl;
  if (before.getButton(N64Buttons::CUp) != after.getButton(N64Buttons::CUp)) std::cout << "CUp: " << after.getButton(N64Buttons::CUp) << std::endl;
  if (before.getButton(N64Buttons::R) != after.getButton(N64Buttons::R)) std::cout << "R: " << after.getButton(N64Buttons::R) << std::endl;
  if (before.getButton(N64Buttons::L) != after.getButton(N64Buttons::L)) std::cout << "L: " << after.getButton(N64Buttons::L) << std::endl;
  if (before.getButton(N64Buttons::Reserved) != after.getButton(N64Buttons::Reserved)) std::cout << "Reserved: " << after.getButton(N64Buttons::Reserved) << std::endl;
  if (before.getButton(N64Buttons::Reset) != after.getButton(N64Buttons::Reset)) std::cout << "Reset: " << after.getButton(N64Buttons::Reset) << std::endl;
  if (before.xAxis != after.xAxis) std::cout << "StickX: " << (int)after.xAxis << std::endl;
  if (before.yAxis != after.yAxis) std::cout << "StickY: " << (int)after.yAxis << std::endl;
};

int main()
{
  // Configure stdio for the command parser
  stdio_init_all();

  // If debug logging is on, wait so USB terminals can
  // reconnect and catch any startup messages.
  #ifdef LOGGING_ENABLED
  sleep_ms(2000);
  #endif

  N64ControllerIn controller(28);
  N64ControllerButtonState lastButtonState;
  N64ControllerInfo lastInfo;

  BootSelButton bootButton;

  controller.update();
  std::cout << controller.info << std::endl;
  sleep_ms(50);

  controller.initRumble();

  intervalLoop([&]()
  {
    // Reboot into programming mode if the bootsel button is clicked
    bootButton.update();
    if (bootButton.buttonDown())
    {
      reset_usb_boot(0,0);
    }

    controller.update();
    printInfoDiff(lastInfo, controller.info);
    lastInfo = controller.info;
    printButtonDiff(lastButtonState, controller.state);
    lastButtonState = controller.state;

    controller.rumble(controller.state.getButton(N64Buttons::A));

  }, 16667);  // Update at 60 Hz
  return 0;
}