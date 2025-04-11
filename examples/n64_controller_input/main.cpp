// pi-pico-cpp headers
#include <cpp/Time.hpp>
#include <cpp/N64Controller.hpp>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>

// std headers
#include <iostream>

int main()
{
  // Configure stdio for the command parser
  stdio_init_all();

  // If debug logging is on, wait so USB terminals can
  // reconnect and catch any startup messages.
  #ifdef LOGGING_ENABLED
  sleep_ms(2000);
  #endif

  N64ControllerIn controller(16);

  intervalLoop([&]()
  {
    controller.update();
    std::cout << controller.state() << std::endl;
  }, 100000);  // Update at 10 Hz (controller can go faster, cout not so much)

  return 0;
}