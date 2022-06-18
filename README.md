# Arduino-based Telescope Focuser

This sketch implements telescope focuser controller logic for a stepper motor and an Elecrow stepper motor driver shield. The serial protocol is compatible with Moonlite focusers, so their ASCOM drivers and supporting software can be used.

Main features:

- Interrupt-driven motor control for fast and accurate operation (up to 1000 steps/s)
- Microstepping
- Persistent settings and position. Position data is saved to EEPROM after every move

Note: For the Moonlite ASCOM driver to work, you must add a 100uF capacitor to your Arduino between RST and GND to prevent the Arduino from rebooting when the serial connection is opened. Otherwise the ASCOM driver will time out and not detect your controller.
