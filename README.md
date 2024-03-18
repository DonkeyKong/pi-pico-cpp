# Pi Pico C++
*C++ wrappers, drivers, and utils for pi pico*

Do you love the Pi Pico but find yourself frustrated by the choices of C or Python? Are you saddened to buy hardware only to find it requires Arduino or Circuit Python to work? Are you spending hours reading docs and poring over examples to do anything?

With this collection of wrappers, drivers, and utilities, you'll be able to jump right into making complex C++ projects with the Pi Pico, without an enormous effort just to setup development scaffolding or talk to hardware.

This repo is a work in progress. Every component here originated in some pi pico project, but was found sufficiently useful to be reused again and again. Some components are missing and others are still in the process of being generalized. 

Eventually a better cmake package for this project will be created, but until then, just import it as a submodule, point cmake at the headers, and manaully add the source files needed. Each file will require particular pieces of the pico SDK to actually compile. I'm working on making this automatic.

# Project Framework

The following classes are based around creating the framework of a project: debugging, testing, persistant settings, and using pi pico built-in hardware features.

## Debug Logging
### Logging.hpp
Define the `LOGGING_ENABLED` macro to print logging to stdout, or clear it to have all logging message bypassed without any performance penalty.

Defines `DEBUG_LOG` and `DEBUG_LOG_IF` macros. Use them like this:

```c++
DEBUG_LOG("Plain message");

DEBUG_LOG("Message will time out in " << timeoutMs << " ms");

DEBUG_LOG_IF((returnCode != 0), "Something went wrong!");
```

You'll probably want stdio enabled on some interface in your cmake file. I like USB.

```cmake
pico_enable_stdio_usb(${PROJECT_NAME} 1)  # swap for uart
pico_enable_stdio_uart(${PROJECT_NAME} 0)
```

You'll also need to init stdio at the top of your main function. I like to also throw a little 2000 ms sleep in to give my terminal a chance to attach, to read debug messages printed by init code.

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

## Remote Command Terminal
### RemoteCommand.hpp (TBD)
This module lets you process stdin as a simple command processor. Most useful when stdin is setup and connected to USB or UART. Each command is a series of params separated by strings, and ending in a newline. Built-in commands include:

`prog` - Reboot into programming mode

`reboot` - Restart into normal mode

Use `setProgCallback(...)` and `setRebootCallback(...)` to add code before the reboots.

Use `addCommand(...)` to add a new custom command.

## Color
### Color.hpp
RGB and HSV color data structures with conversion and some basic processing functions like blend, gamma, multiply, add, and more. Great for calibrationg LED lights.

## Math and Vectors

## Settings and Flash
Saving settings or other data to flash memory on the pi pico is a whole lot harder than it seems like it should be. These classes let you take a C++ data structure, then read or write it to flash, as a way of saving settings.

> Note: Repeated writes to flash memory may reduce the lifespan of the pi pico. Write to flash memory infrequently (i.e.: a few times per day, not 100 times per second)

## WiFi
Very simple class to bring up the wifi interface and connect it to a specified network. Hard coded to WPA2-PSK security. Needs more work to become useful and general purpose.

### NTPSync.hpp
Synchronize the pi pico system clock with an internet time server. Based on pi pico examples.

# Peripheral Support

The following classes are designed to help you connect specific pieces of hardware to the pi pico.

## Input and Output
### Button.hpp
Create a polled button input from an input pin. Keeps track of state as well as transitions, double click, press and hold, and more. Really handy for making efficient use of limited physical buttons in projects.

### DiscreteOut.hpp
Create a single pin output. Set and get its level, configure pullup and pulldown simply. Light wrap of C API.

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

## Nintendo Peripherals
### NintendoJoybus.hpp
### NintendoI2C.hpp

## Individually Addressable LED (NeoPixel)
Control a Ws2812b, NeoPixel, or other compatible chain of individually addressable LEDs. Just provide an I/O pin for the data line to set it up. Write an array of RGBColor stuctures to it. Brightness, color balance, and gamma correction are all supported to ensure uniform light output with the same input colors across different makes and models of LEDs

