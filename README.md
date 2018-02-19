# Tab Stomper (V-USB)

Firmware for AVR microcontrollers (based on Objective Development's [V-USB](https://www.obdev.at/products/vusb/)) to automatically press `PageDown` and `UpArrow` keys.


## Devices

The firmware is designed for use with an ATmega328, but it should work with other microcontrollers as well.
There are two Makefile templates:

* `Makefile.arduino`: This should work with an Arduino Uno without any modifications, except maybe the port.
* `Makefile.usbtiny`: This is for programming a standalone chip with a USBtiny. You may need to change the programmer, microcontroller, frequency, and fuse settings.


## Compiling

1. Set up a Makefile.
1. `make all`
1. `make flash`

If you're running a standalone chip (not Arduino), you should also `make fuse`.
Please review the fuse settings first and make sure you won't be bricking your chip.


## Hookup

For the USB connection to the host, you'll need something compatible with V-USB.
For example, a [dedicated board with a USB connector](https://d.i10o.ca/projects/vusb-dev-board/).
If what you're using doesn't have a pull-up pin (so the pull-up is hard-wired), you should change the firmware (see `USB_CFG_PULLUP_BIT`).

For the foot button, you can get creative.
The only limitations are that it should be floating when not pressed (and be pulled high by the AVR internal pull-up) and short to ground when pressed (to pull the pin low).


### Arduino Uno

On an Arduino Uno, you'll need to make the following connections by default:

* digital pin 2: USB `D+`
* digital pin 3: USB pull-up
* digital pin 4: USB `D-`
* digital pin 5: button
* `5V`: USB `VBUS`
* `GND`: USB `GND`, button

The device will be powered through the USB connection, so you shouldn't have the built-in Arduino USB or barrel connectors plugged in.


### Standalone

If you're using a standalone chip, you'll need to make sure that it has an appropriate crystal hooked up.
You'll also need the following connections by default:

* `PD2`: USB `D+`
* `PD3`: USB pull-up
* `PD4`: USB `D-`
* `PD5`: button
* `VCC`: USB `VBUS`
* `GND`: USB `GND`, button


## Usage

Plug it in to a host via USB, and you're good to go.
If the host is running Linux, you should expect something like the following in the `dmesg` output:
```
usb 3-2: new low-speed USB device number 15 using xhci_hcd
input: d.i10o.ca Tab Stomper as /devices/pci0000:00/0000:00:14.0/usb3/3-2/3-2:1.0/0003:16C0:27DB.0229/input/input288
hid-generic 0003:16C0:27DB.0229: input,hidraw4: USB HID v1.01 Keyboard [d.i10o.ca Tab Stomper] on usb-0000:00:14.0-2/input0
```


## License

Provided under the terms of the GPL, version 3.
See `NOTICE` and `LICENSE` for more information.

This software is distributed with V-USB, found in `usbdrv/`, which is available under the terms of the GPL, version 2 or 3.
