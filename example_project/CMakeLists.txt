cmake_minimum_required(VERSION 3.12)

# Welcome to the pi pico cpp example project! 
#
# This file assumes the following:
#   1. Visual Studio Code is your IDE
#   2. Docker and the Dev Containers vscode extension are installed
#   3. You have installed the pico sdk at ../pico-sdk relative to this file
#   4. vscode has attached to the container "RPi Pico Build" 
#      (vscode will suggest this after opening this folder)
#
# If you don't like any of these assumptions, feel free to read the pi pico docs 
# and pick apart this file for just the parts you want.

# Set the name of your project
set(PROJECT_NAME "example_project")

# Choose between "pico" and "picow" for your target board
set(PICO_BOARD "pico")

# We assume the pico sdk is located at /pico-sdk because
# that's where we have told vscode to mount it in the docker
set(PICO_SDK_PATH "/pico-sdk")

# This framework does not use RTTI or C++ exceptions.
# I don't recommend turning them on.
#set(PICO_CXX_ENABLE_EXCEPTIONS 1)
#set(PICO_CXX_ENABLE_RTTI 1)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Find pi-pico-cpp
find_path (PI_PICO_CPP_DIR
          NAMES include/cpp/Logging.hpp pio/ws2812b.pio
          HINTS ${CMAKE_CURRENT_LIST_DIR}/pi-pico-cpp
                ${CMAKE_CURRENT_LIST_DIR}/../pi-pico-cpp
                ${CMAKE_CURRENT_LIST_DIR}/..
         )
message(STATUS "Found Pi Pico C++ at ${PI_PICO_CPP_DIR}")

# Create the project and specify our C and C++ versions
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Create the project executable and add source files
add_executable(${PROJECT_NAME}
        main.cpp
)

# Enable USB serial port output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Enable debug logging messages
target_compile_definitions(${PROJECT_NAME} PRIVATE LOGGING_ENABLED)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PI_PICO_CPP_DIR}/include
)

# Add any PIO sources required
pico_generate_pio_header(${PROJECT_NAME} ${PI_PICO_CPP_DIR}/pio/ws2812b.pio)

# Choose which bits of the pico SDK are required
# (along with any other libs)
target_link_libraries(${PROJECT_NAME}
        pico_stdio 
        pico_stdlib
)

# Create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
