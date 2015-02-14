# STM8 Reverse Engineering Aid

This is a tool born out of my need to reverse engineer the B3603 power supply.
The intention is to have an aid to reverse engineer the board pins from the MCU
pins, for example by toggling the output of any pin and probe that pin with a
led or a logic analyzer to find where it gets controlled from.

When this firmware is loaded on the STM8 MCU it will communicate over serial (9600 8N1)
and will allow to toggle the pins such that only one pin is turned on at any single time.

It listens to the following commands:

* A, B, C, D -- toggle the port group
* 1, 2, 3, 4, 5, 6, 7, 8 -- select the pin in the port group
* Q -- Set CR1 for that pin to low, Open-Drain
* W -- Set CR1 for that pin to high, Push-Pull
* Z -- Set CR2 for that pin to low, 2MHz
* X -- Set CR2 for that pin to high, 10MHZ

# Author

Baruch Even <baruch@ev-en.org>
