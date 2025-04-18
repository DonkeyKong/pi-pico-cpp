// pi-pico-cpp headers
#include <cpp/BootSelButton.hpp>

// Pico SDK headers
#include <pico/bootrom.h> // for reset_usb_boot()

int main()
{
  BootSelButton bootButton;

  while(true)
  {
    // Reboot into programming mode if the bootsel button is clicked
    bootButton.update();
    if (bootButton.buttonDown())
    {
      reset_usb_boot(0,0);
    }
  }
  return 0;
}