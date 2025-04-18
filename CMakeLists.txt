cmake_minimum_required(VERSION 3.18)

# Welcome to the pi pico cpp sample project! 
#
# This file assumes the following:
#   1. Visual Studio Code is your IDE
#   2. Docker and the Dev Containers vscode extension are installed
#   3. pico-sdk is cloned next to this workspace folder (at ../pico-sdk)
#   4. vscode has attached to the container "RPi Pico Build" 
#      (vscode will suggest this after opening this folder)
#
# If you don't like any of these assumptions, feel free to read the pi pico docs 
# and pick apart this file for just the parts you want.

# Set the name of your project
set(PROJECT_NAME "ppc_examples")

# Choose between "pico" and "picow" for your target board
set(PICO_BOARD "pico")

# We assume the pico-sdk is located at /pico-sdk because
# this is where we mount it in the docker
set(PICO_SDK_PATH "/pico-sdk")

# This framework does not use RTTI or C++ exceptions.
# I don't recommend turning them on but you can
set(PICO_CXX_ENABLE_EXCEPTIONS 0)
set(PICO_CXX_ENABLE_RTTI 0)

# Pull in pico SDK (must be before project)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Create the project and specify our C and C++ versions
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add pi-pico-cpp include directory
include_directories(include)

# Build all the examples
add_subdirectory(examples/bootsel_button)
add_subdirectory(examples/logging)
add_subdirectory(examples/flash)
add_subdirectory(examples/fan_controller)
add_subdirectory(examples/n64_controller_input)
add_subdirectory(examples/led_strip)