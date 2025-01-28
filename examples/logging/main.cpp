// pi-pico-cpp headers
#include <cpp/Logging.hpp>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>

// std headers
#include <iostream>
#include <string>

#define LOGGING_ENABLED

int main()
{
  // Configure stdio
  stdio_init_all();

  // Wait for USB terminals to connect (otherwise you'll likely miss it!)
  #ifdef LOGGING_ENABLED
  sleep_ms(2000);
  #endif

  DEBUG_LOG("About to greet the earth...");
  std::cout << "Hello world!" << std::endl;

  // Pause here instead of exiting
  while (true)
  {
    tight_loop_contents();
  }
  return 0;
}