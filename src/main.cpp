#include <DRV8825.h>
#include "moonlite.h"

/* Stepper pins */
#define PIN_M0 6
#define PIN_M1 5
#define PIN_M2 4
#define PIN_DIR 2
#define PIN_STEP 3
#define PIN_ENABLE 7
#define PIN_LED 13

#define MICROSTEPS_HALF 32
#define MICROSTEPS_FULL 8
#define RPM 200

/* Stepper definition */

DRV8825 stepper(200, PIN_DIR, PIN_STEP, PIN_ENABLE, PIN_M0, PIN_M1, PIN_M2);
bool useFineMicroSteps = true;

/* Serial commands are stored in this buffer for parsing. */
#define SERIAL_BUFFER_LENGTH 8
char serialBuffer[SERIAL_BUFFER_LENGTH];

/* Current focuser position in steps. */
uint16_t currentPosition;

/*
newPosition is the position given with SP command.
Focuser will go to this position when given FG command.
*/
uint16_t newPosition;

/*
Delay length between step, given by SD command.
Only the following values are accepted in the spec:
2 -> 4 ms,
4 -> 8 ms,
8 -> 16 ms,
16 -> 32 ms,
32 -> 64 ms.
*/
uint8_t delayMultiplier = 2;

/* Is the focuser currently moving? */
bool isMoving = false;

void setup() {
	currentPosition = 32768;
	newPosition = currentPosition;
	clearBuffer(serialBuffer, 8);
	Serial.begin(9600);
	while(!Serial) {
		delay(10);
	}

	stepper.begin(RPM, MICROSTEPS_HALF);
	stepper.disable();
}

void loop() {
	if (isMoving) {
		/* Move stepper and update currentPosition */
		if (currentPosition < newPosition) {
			if (currentPosition < 65535) {
				stepper.enable();
				delay(1);
				stepper.move(1);
				currentPosition += 1;
			} else {
				isMoving = false;
			}
		} else if (currentPosition > newPosition) {
			if (currentPosition > 0) {
				stepper.enable();
				delay(1);
				stepper.move(-1);
				currentPosition -= 1;
			} else {
				isMoving = false;
			}
		}
		/* Set stepping delay based on speed commands */
		delayMicroseconds(((unsigned int)delayMultiplier) * 200);

		if (currentPosition == newPosition) {
			isMoving = false;
		}

		/* Release motor if stopping conditions reached */
		if (!isMoving) {
			stepper.disable();
		}
	}

	/* Check for serial communications and act accordingly. */
	if (Serial.available()) {
		if (Serial.read() == ':') {
			clearBuffer(serialBuffer, SERIAL_BUFFER_LENGTH);
			Serial.readBytesUntil('#', serialBuffer, 8);
			MOONLITE_COMMAND command = moonliteStringToEnum(serialBuffer);
			switch (command) {
				case stop:
					/* Stop moving */
					isMoving = false;
					stepper.disable();
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
					stepper.setMicrostep(MICROSTEPS_FULL);
					useFineMicroSteps = false;
					break;
				case set_half_step:
					/* Set half-step mode */
					if (isMoving) break;
					stepper.setMicrostep(MICROSTEPS_HALF);
					useFineMicroSteps = true;
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
					break;
				case get_temperature:
					/* TODO: Get temperature */
					Serial.print("0000#");
					break;
				case unrecognized:
					/* TODO: React to unrecognized command */
					break;
				default:
					break;
			}

			Serial.flush();
		}
	}
}

