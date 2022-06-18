/*
 * Copyright (c) 2017 Jari Saukkonen, Samuli Vuorinen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <EEPROM.h>
#include <TimerOne.h>
#include "Config.h"
#include "DRV8825.h"
#include "moonlite.h"

#define DRIVER_DISCONNECTED_MILLIS (30 * 1000)
#define EEPROM_MARKER 'F'
#define INTERRUPT_RATE_US 500

/* Stepper definition */
DRV8825 stepper(PIN_DIR, PIN_STEP, PIN_ENABLE, PIN_M0, PIN_M1, PIN_M2);

/* Microstepping level. Original moonlite is full/half, we have different settings from 
   MICROSTEPS_FULL and MICROSTEPS_HALF. */
bool useFineMicroSteps = true;

/* Shall we keep focuser power on after the move has ended? */
bool holdEnabled = false;

/* Timestamp (micros()) of last received serial traffic */
unsigned long lastSerialReceived = 0;

/* Settings data in EEPROM */
struct Settings {
	uint8_t marker;
	uint16_t currentPosition;
	uint8_t useFineMicroSteps;
	uint8_t delayMultiplier;
	uint8_t holdEnabled;
} settings;

/* Serial commands are stored in this buffer for parsing. */
#define SERIAL_BUFFER_LENGTH 8
char serialBuffer[SERIAL_BUFFER_LENGTH];

/* Current focuser position in steps. */
volatile uint16_t currentPosition;

/*
newPosition is the position given with SP command.
Focuser will go to this position when given FG command.
*/
volatile uint16_t newPosition;

/*
Delay length between step, given by SD command.
Only the following values are accepted in the spec:
2 -> 4 ms,
4 -> 8 ms,
8 -> 16 ms,
16 -> 32 ms,
32 -> 64 ms.

But we take some liberty in interpreting this as we use heavy microstepping.
The actual delay is delayMultiplier * 500us.
*/
volatile uint8_t delayMultiplier = 2;

/* Is the focuser currently moving? */
volatile bool isMoving = false;

/* Counter for calculating when to disengage focuser power after move */
volatile int disengageCounter = 0;

/* Interrupt counter for slower slew rates */
volatile int interruptCounter = 0;

/* Flag for requesting settings to be saved; used in interrupt handler */
volatile bool needToSaveSettings = false;

void motorInterrupt();

void loadSettings() {
	uint8_t* dst = (uint8_t*)&settings;
	for (unsigned int i = 0; i < sizeof(Settings); i++) {
		dst[i] = EEPROM.read(i);
	}

	if (settings.marker == EEPROM_MARKER) {
		currentPosition = settings.currentPosition;
		delayMultiplier = settings.delayMultiplier;
		useFineMicroSteps = settings.useFineMicroSteps;
		holdEnabled = settings.holdEnabled;
	} else {
		settings.marker = EEPROM_MARKER;
		settings.currentPosition = currentPosition = 0;
		settings.delayMultiplier = delayMultiplier = 2;
		settings.useFineMicroSteps = useFineMicroSteps = true;
		settings.holdEnabled = holdEnabled = false;
	}
}

void saveSettings() {
	settings.currentPosition = currentPosition;
	settings.delayMultiplier = delayMultiplier;
	settings.useFineMicroSteps = useFineMicroSteps;
	settings.holdEnabled = holdEnabled;

	uint8_t* dst = (uint8_t*)&settings;
	for (unsigned int i = 0; i < sizeof(Settings); i++) {
		if (EEPROM.read(i) != dst[i]) {
			EEPROM.write(i, dst[i]);
		}
	}
}

void setup() {
	loadSettings();

	newPosition = currentPosition;
	clearBuffer(serialBuffer, 8);
	Serial.begin(9600);
	while(!Serial) {
		delay(10);
	}

	stepper.setMicrostepping(useFineMicroSteps ? MICROSTEPS_HALF : MICROSTEPS_FULL);
	if (holdEnabled) {
		stepper.enable();
	} else {
		stepper.disable();
	}

	Timer1.initialize(INTERRUPT_RATE_US);
	Timer1.attachInterrupt(motorInterrupt);
}

void disableStepperWithDelay() {
	disengageCounter = INTERRUPT_RATE_US*2;
}

void handleDelayedDisable() {
	if (!isMoving && disengageCounter > 0) {
		disengageCounter--;
		if (disengageCounter == 0 && !holdEnabled) {
			stepper.disable();
		}
	} else {
		disengageCounter = 0;
	}
}

void motorInterrupt() {
	handleDelayedDisable();

	/* handle only every nth interrupt if a slower speed is selected */
	if (++interruptCounter < delayMultiplier) {
		return;
	}
	interruptCounter = 0;

	if (!isMoving) {
		return;
	}

	/* Enable stepper if it has been shut down */
	if (currentPosition != newPosition && !stepper.isEnabled()) {
		stepper.enable();
	}

	/* Move stepper and update currentPosition */
	if (currentPosition < newPosition && currentPosition < 65535) {
		stepper.step(1);
		currentPosition++;
	} else if (currentPosition > newPosition && currentPosition > 0) {
		stepper.step(-1);
		currentPosition--;
	}

	/* If we have reached desired position end the move and save new position to EEPROM */
	if (currentPosition == newPosition) {
		isMoving = false;
		needToSaveSettings = true;
		disableStepperWithDelay();
	}
}

void handleSerial() {
	/* Check for serial communications and act accordingly. */
	if (!Serial.available())
		return;

	if (Serial.read() == ':') {
		clearBuffer(serialBuffer, SERIAL_BUFFER_LENGTH);
		Serial.readBytesUntil('#', serialBuffer, 8);
		MOONLITE_COMMAND command = moonliteStringToEnum(serialBuffer);
		lastSerialReceived = millis();
		switch (command) {
			case stop:
				/* Stop moving */
				isMoving = false;
				newPosition = currentPosition;
				disableStepperWithDelay();
				break;
			case get_current_position:
				/* Get current position */
				char currentPositionString[4];
				sprintf(currentPositionString, "%04X", currentPosition);
				Serial.print(currentPositionString);
				Serial.print("#");
				break;
			case set_current_position:
				/* Set current position */
				if (isMoving) break;
				currentPosition = four_chars_to_uint16(serialBuffer + 2);
				saveSettings();
				break;
			case get_new_position:
				/* Get new position set by SN */
				char newPositionString[4];
				sprintf(newPositionString, "%04X", newPosition);
				Serial.print(newPositionString);
				Serial.print("#");
				break;
			case set_new_position:
				/* Set new position */
				if (isMoving) break;
				newPosition = four_chars_to_uint16(serialBuffer + 2);
				break;
			case go_to_new_position:
				/* Go to new position set by SN */
				isMoving = true;
				break;
			case check_if_half_step:
				/* Check if half-stepping */
				if (useFineMicroSteps) {
					Serial.print("FF#");
				} else {
					Serial.print("00#");
				}
				break;
			case set_full_step:
				/* Set full-step mode */
				if (isMoving) break;
				stepper.setMicrostepping(MICROSTEPS_FULL);
				useFineMicroSteps = false;
				saveSettings();
				break;
			case set_half_step:
				/* Set half-step mode */
				if (isMoving) break;
				stepper.setMicrostepping(MICROSTEPS_HALF);
				useFineMicroSteps = true;
				saveSettings();
				break;
			case check_if_moving:
				/* Check if moving */
				if (isMoving) {
					Serial.print("01#");
				} else {
					Serial.print("00#");
				}
				break;
			case get_firmware_version:
				/* Get firmware version */
				Serial.print("01#");
				break;
			case get_backlight_value:
				/* Get current Red LED Backlight value */
				Serial.print("00#");
				break;
			case get_speed:
				/* Get speed */
				char speedString[2];
				sprintf(speedString, "%02X", delayMultiplier);
				Serial.print(speedString);
				Serial.print("#");
				break;
			case set_speed:
				/* Set speed */
				if (isMoving) break;
				delayMultiplier = two_chars_to_uint8(serialBuffer + 2);
				saveSettings();
				break;
			case get_temperature:
				/* TODO: Get temperature */
				Serial.print("0000#");
				break;
			case set_hold_enabled:
				holdEnabled = true;
				stepper.enable();
				saveSettings();
				break;
			case set_hold_disabled:
				holdEnabled = false;
				stepper.disable();
				saveSettings();
				break;
			case unrecognized:
				/* We ignore other commands */
				break;
			default:
				break;
		}
	}
}

void disableStepperIfNoSerialTraffic() {
	/* timeout focuser power if no data is received for a while; driver is most likely disconnected */
	if (stepper.isEnabled() && (millis() - lastSerialReceived > DRIVER_DISCONNECTED_MILLIS)) {
		stepper.disable();
	}
}

void loop() {
	if (needToSaveSettings) {
		saveSettings();
		needToSaveSettings = false;
	}

	disableStepperIfNoSerialTraffic();
	handleSerial();
}
