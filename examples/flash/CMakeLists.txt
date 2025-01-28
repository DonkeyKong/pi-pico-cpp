set(EXAMPLE_NAME "example_flash")

# Create the executable and add source files
add_executable(${EXAMPLE_NAME}
        main.cpp
)

# If using multicore, be certain to set this var to let FlashStorage know!
# Otherwise you'll corrupt the flash memory.
# target_compile_definitions(${EXAMPLE_NAME} PUBLIC ENABLE_PICO_MULTICORE)

# Enable USB serial port i/o
pico_enable_stdio_usb(${EXAMPLE_NAME} 1)
pico_enable_stdio_uart(${EXAMPLE_NAME} 0)

# Choose which bits of the pico SDK are required
target_link_libraries(${EXAMPLE_NAME}
        pico_stdio 
        pico_stdlib
        hardware_flash
)

# Create map/bin/hex file etc.
pico_add_extra_outputs(${EXAMPLE_NAME})