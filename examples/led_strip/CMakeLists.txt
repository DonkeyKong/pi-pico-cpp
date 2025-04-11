set(EXAMPLE_NAME "example_led_strip")

# Create the executable and add source files
add_executable(${EXAMPLE_NAME}
        main.cpp
)

# Generate all PIO headers
file(GLOB pio_files "../../pio/*.pio")
foreach(pio_file ${pio_files})
  pico_generate_pio_header(${EXAMPLE_NAME} ${pio_file})
endforeach()

target_compile_definitions(${EXAMPLE_NAME} PUBLIC "LOGGING_ENABLED")

# Choose which bits of the pico SDK are required
target_link_libraries(${EXAMPLE_NAME}
        pico_stdlib
        hardware_pio
)

# Create map/bin/hex file etc.
pico_add_extra_outputs(${EXAMPLE_NAME})