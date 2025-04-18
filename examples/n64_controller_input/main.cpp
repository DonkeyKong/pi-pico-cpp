// pi-pico-cpp headers
#include <cpp/Time.hpp>
#include <cpp/N64Controller.hpp>
#include <cpp/BootSelButton.hpp>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>

// std headers
#include <iostream>

#define printButtonStateIfChanged(a, b, button) \
if (a.getButton(N64Buttons::button) != b.getButton(N64Buttons::button)) \
  std::cout << #button << " : " << b.getButton(N64Buttons::button) << std::endl

void printStatusFlags(const N64ControllerInfo a, const N64ControllerInfo b)
{
  if (b.getStatusFlag(N64Status::AddressCrcError)) 
    std::cout << "AddressCrcError" << std::endl;
  if (!a.getStatusFlag(N64Status::PakInserted) && b.getStatusFlag(N64Status::PakInserted))
    std::cout << "Pak Inserted" << std::endl;
  if (!a.getStatusFlag(N64Status::PakRemoved) && b.getStatusFlag(N64Status::PakRemoved))
    std::cout << "Pak Removed" << std::endl;
};

void printButtonDiff(const N64ControllerButtonState a, const N64ControllerButtonState b)
{
  printButtonStateIfChanged(a, b, PadRight);
  printButtonStateIfChanged(a, b, PadLeft);
  printButtonStateIfChanged(a, b, PadDown);
  printButtonStateIfChanged(a, b, PadUp);
  printButtonStateIfChanged(a, b, Start);
  printButtonStateIfChanged(a, b, Z);
  printButtonStateIfChanged(a, b, B);
  printButtonStateIfChanged(a, b, A);
  printButtonStateIfChanged(a, b, CRight);
  printButtonStateIfChanged(a, b, CLeft);
  printButtonStateIfChanged(a, b, CDown);
  printButtonStateIfChanged(a, b, CUp);
  printButtonStateIfChanged(a, b, R);
  printButtonStateIfChanged(a, b, L);
  printButtonStateIfChanged(a, b, Reserved);
  printButtonStateIfChanged(a, b, Reset);
  if (a.xAxis != b.xAxis) std::cout << "StickX: " << (int)b.xAxis << std::endl;
  if (a.yAxis != b.yAxis) std::cout << "StickY: " << (int)b.yAxis << std::endl;
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

  N64ControllerIn controller(28, /*autoInitRumblePak*/ true);

  // Used to track past controller state
  N64ControllerButtonState lastButtonState;
  N64ControllerInfo lastInfo;

  intervalLoop([&]()
  {
    // Update the controller's info and button state
    controller.update();

    // If a rumble pak is ready, rumble if A is held
    if (controller.rumblePakReady)
    {
      controller.rumble(controller.state.getButton(N64Buttons::A));
    }

    // Print out any flag or button state changes
    printStatusFlags(lastInfo, controller.info);
    printButtonDiff(lastButtonState, controller.state);
    lastInfo = controller.info;
    lastButtonState = controller.state;

  }, 16667);  // Update at 60 Hz
  return 0;
}