set(EXAMPLE_NAME "example_bootsel_button")

# Create the executable and add source files
add_executable(${EXAMPLE_NAME}
        main.cpp
)

# Choose which bits of the pico SDK are required
target_link_libraries(${EXAMPLE_NAME}
        pico_stdlib
        hardware_pio
)

# Create map/bin/hex file etc.
pico_add_extra_outputs(${EXAMPLE_NAME})