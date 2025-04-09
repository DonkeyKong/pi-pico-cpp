// Sample code for FlashStorage<T>
// Usage:
//    Load this on a Pico
//    Connect to a PC with USB
//    Open a serial terminal app
//    Send "get\n" and see "default"
//    Send "set hello\n"
//    Send "get\n" and see "hello"
//    Power cycle pico and reconnect
//    Send "get\n" and see "hello" persisted!

// pi-pico-cpp headers
#include <cpp/FlashStorage.hpp>
#include <cpp/CommandParser.hpp>

// Pico SDK headers
#include <pico/stdlib.h>
#include <pico/stdio.h>

// std headers
#include <iostream>
#include <string>

struct Settings
{
  char persistentString[256] = "default";
};

int main()
{
  // Configure stdio for the command parser
  stdio_init_all();

  FlashStorage<Settings> settings;
  settings.readFromFlash();

  CommandParser parser;
  parser.addCommand("set", "string", "Set the persistent string", [&](std::string str)
  {
    if (str.size() < 256)
    {
      strcpy(settings.data.persistentString, str.c_str());
      settings.writeToFlash();
    }
  });

  parser.addCommand("get", "", "Get the persistent string", [&]()
  {
    std::cout << settings.data.persistentString << "\n";
  });

  while(true)
  {
    parser.processStdIo();
  }
  return 0;
}