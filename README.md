# Pi Pico C++
*C++ wrappers, drivers, and utils for pi pico*

Jump right into making complex C++ projects with the pi pico, without spending forever reading docs or writing boilerplate.

This repo is a work in progress. Some components are still being generalized. None of this code uses exceptions.

Each header will require particular pieces of the pico SDK to actually compile. I'm working on making this automatic.

## Debug Logging
```c++
#include <cpp/Logging.hpp>
#define LOGGING_ENABLED

DEBUG_LOG("Plain message");
DEBUG_LOG("Message will time out in " << timeoutMs << " ms");
DEBUG_LOG_IF((returnCode != 0), "Something went wrong!");
```

If you don't define `LOGGING_ENABLED`, logging is bypassed without any performance penalty. The strings aren't even compiled into your app.

### About Standard Out on pico
If you're new to the pico and you're wondering where these logs print: you choose UART or USB in your cmake file. 

```cmake
pico_enable_stdio_usb(${PROJECT_NAME} 1)  # swap for uart
pico_enable_stdio_uart(${PROJECT_NAME} 0)
```

You'll also need to init stdio at the top of your main function.

```c++
int main()
{
  // Configure stdio
  stdio_init_all();

  // If debug logging is on, wait so USB terminals can
  // reconnect and catch any startup messages.
  #ifdef LOGGING_ENABLED
  sleep_ms(2000);
  #endif
  
  // ...
}
```

I like USB, as it presents the pico as a USB serial adapter to your PC. Use any terminal emulator to connect to the serial device.

## Command Parser
This module lets you use stdin as a simple command processor.

```c++
#include <cpp/CommandParser.hpp>

CommandParser parser;
parser.addCommand("release", [&]()
{
  servoX.release();
  servoY.release();
}, "", "Power down all servos");

// Main loop
while(true)
{
  parser.processStdIo();
}
```

Commands can take arguments, so long as all the argument types can be assigned using `operator>>` from an istream. You can also return true or false to indicate if your command ran ok.

```c++
parser.addCommand("mt", [&](double x, double y)
{
  if (between(x, 0.0, 1.0) && between(y, 0.0, 1.0))
  {
    servoX.posT(x);
    servoY.posT(y);
    return true; // returning true prints "ok"
  }
  return false; // returning false prints "err"
}, "[x] [y]", "Move to (x, y)");
```

This command is invoked from a connected terminal like this:
>mt 0.43 1.0

Typing `help` lists all commands.

```
help

Command Listing:

    release              Power down all servos
    mt [x] [y]           Move to (x, y)
```

This is not a full terminal emulator but it contains just enough features to act as a debug/programming interface and get a project going.

## Flash Storage
Saving settings or other data to flash memory on the pi pico is harder than it should be. This module lets you take a C++ data structure, then read or write it to flash, as a way of saving settings.

```c++
#include <cpp/FlashStorage.hpp>

struct Settings
{
  float center = 0.0f;
  bool autoShutoff = true;
};

FlashStorage<Settings> flashStorage;
flashStorage.readFromFlash();
flashStorage.data.autoShutoff = false;
flashStorage.writeToFlash();
```

See the `FlashStorage.hpp` header for more technical details.

> Note: Frequent writes to flash memory may reduce the lifespan of the pi pico. Write to flash memory infrequently (i.e.: a few times per day, not 100 times per second)

## Button
```c++
#include <cpp/Button.hpp>

// Button connects pin 2 to ground when pressed
// Press and hold detection enabled
GPIOButton button(2, true);

// main loop
while (1)
{
  button.update();
  DEBUG_LOG_IF(button.buttonDown(), "button pressed");
  DEBUG_LOG_IF(button.buttonUp(), "button released");
  DEBUG_LOG_IF(button.heldActivate(), "button held (repeating)");
}
```

> GPIO is only one type of Button. Anything with a fetchable boolean state can be a "Button". Make your own classes that inherit from Button.

## DiscreteOut.hpp
```c++
#include <cpp/DiscreteOut.hpp>

// Pin 1, pulls down by default
DiscreteOut out1(1);
out1.set(true);
```
> This is a light wrap of pico C API and mostly useful to skip looking up documentation.

## Nintendo Peripherals
### Nunchuck
Wire a Wii Nunchuck to one of the i2c busses and go!

```c++
#include <cpp/Nunchuck.hpp>

Nunchuck nunchuck(i2c1, 14, 15); // device, data, clock

// Main loop
while (true)
{
  nunchuck.requestControllerState();
  // Nunchuck will take ~5ms to be ready for fetch
  // so do stuff here if you want. Fetch will block
  // if it needs to.
  if (nunchuck.fetchControllerState())
  {
    if (nunchuck.c())
    {
      DEBUG_LOG("C button pressed");
    }
  }
}

> The C and Z buttons are actually the `Button` class from this library, so they have all those features.
```

## Color
RGB and HSV color data structures with conversion and some basic processing functions like blend, gamma, multiply, add, and more. Great for calibrationg LED lights.

## WiFi
Very simple class to bring up the wifi interface and connect it to a specified network. Hard coded to WPA2-PSK security. Needs more work to become useful and general purpose.

## NTPSync.hpp
Synchronize the pi pico system clock with an internet time server. Based on pi pico examples.

### PioProgram.hpp
Manages instantiation and resources of pio programs and machines. Light wrapper of C API so you don't have to look up as much stuff.

> Needs work and documentation to be useful general purpose. Mostly used as a base for other classes that support hardware via PIO

## I2C Interface

## Motors and Motion
### Servo.hpp
Provides an interface for servos connected directly to a GPIO pin. Properly sets up PWM for best resolution. Assumes 60 Hz update rate. Construct by specifying a pin and movement range. Set position using a float param 0-1, or degrees.
### Stepper.hpp
Provides a virtual base class for stepper motors. TBD: support with generic H bridges.
### DCMotor.hpp
Provides a virtual base class for dc motors. TBD: support with generic H bridge.
### MotorKit.hpp
Support for the adafruit Motorkit Hat. Communicates with motor controller via i2c. Construct and control 2 Stepper or 4 DCMotor objects.
### MotionXY.hpp
Base class for a generic 2D motion stage. Implementations provided for a standard XY gantry and a Core XY belt driven stage. Provides basic homing and movement. Stages require 2 steppers and 2 buttons to construct.

## Individually Addressable LED (NeoPixel)
Control a Ws2812b, NeoPixel, or other compatible chain of individually addressable LEDs. Just provide an I/O pin for the data line to set it up. Write an array of RGBColor stuctures to it. Brightness, color balance, and gamma correction are all supported to ensure uniform light output with the same input colors across different makes and models of LEDs

