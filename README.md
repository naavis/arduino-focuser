Arduino-based Telescope Focuser
===============================

This sketch implements telescope focuser controller logic for a stepper motor and a Pololu DRV8825 stepper driver board. The serial protocol is compatible with Moonlite focusers, so their ASCOM drivers and supporting software can be used.

Main features:
- Interrupt-driven motor control for fast and accurate operation (up to 1000 steps/s)
- Microstepping up to 1/32 steps
- Persistent settings and position. Position data is saved to EEPROM after every move

To use this controller, you should first connect your Arduino to the DRV8825 board as described in the Pololu's [DRV8825 product page](https://www.pololu.com/product/2133). If you want to use microstepping or have the driver shut down focuser power when not in use then connect also the remaining pins (ENABLE, M0, M1, M2). Finally, edit `Config.h` to set your pin mapping.

![DRV8825 wiring diagram](https://a.pololu-files.com/picture/0J4232.600.png)

Note: For the Moonlite ASCOM driver to work, you must add a 100uF capacitor to your Arduino between RST and GND to prevent the Arduino from rebooting when the serial connection is opened. Otherwise the ASCOM driver will time out and not detect your controller.

Differences to the Moonlite controller
--------------------------------------

The Moonlite driver has the option for selecting the focuser speed from 250 steps/s to 16 steps/s. In order to achieve the high speed of up to 1000 steps/s these values are multiplied by four by the controller.

The driver also has a selection for setting the microstepping mode to "full" or "half". The controller supports 1/1, 1/2, 1/4, 1/8, 1/16 and 1/32 step microstepping modes. Please edit the `Config.h` to select the microstepping modes that the Moonlite "full" and "half" commands should correspond to.

Focuser hold power
------------------

The sketch supports an extension to the Moonlite protocol to enable keeping the focuser continously powered. The hold power can be enabled with the Moonlite Single Channel Controller application that allows you to send arbitrary serial commands to the controller. To enable the hold power send the `HE` command. To disable it, use `HD`. The setting is saved in EEPROM so you need to do this only once.

When the hold power is enabled, the controller keeps the focuser continuously powered. The power is turned off only after no serial traffic is detected for 30 seconds. In this case the ASCOM driver is not connected so we deduce that the controller is not being used.

When the hold power is disabled, the focuser is powered before starting a move and turned off
slightly after a move has ended.
