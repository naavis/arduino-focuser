#include <DRV8825.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include "moonlite.h"

/* Stepper pins */
#define PIN_M0 6
#define PIN_M1 5
#define PIN_M2 4
#define PIN_DIR 2
#define PIN_STEP 3
#define PIN_ENABLE 7
#define PIN_LED 13

#define MICROSTEPS_HALF 16
#define MICROSTEPS_FULL 8
#define RPM 200
#define DELAY_MULTIPLIER 300

#define EEPROM_MARKER 'F'

/* Stepper definition */
DRV8825 stepper(200, PIN_DIR, PIN_STEP, PIN_ENABLE, PIN_M0, PIN_M1, PIN_M2);
bool useFineMicroSteps = true;
bool stepperEnabled = false;

struct Settings {
	uint8_t marker;
	uint16_t currentPosition;
	uint8_t useFineMicroSteps;
	uint8_t delayMultiplier;
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

But we take some liberty in interpreting this as we use heavy microstepping..
*/
volatile uint8_t delayMultiplier = 2;

/* Is the focuser currently moving? */
volatile bool isMoving = false;

void setStepInterval(long microseconds);
void motorInterrupt();

void loadSettings() {
	uint8_t* dst = (uint8_t*)&settings;
	for (int i = 0; i < sizeof(Settings); i++) {
		dst[i] = EEPROM.read(i);
	}

	if (settings.marker == EEPROM_MARKER) {
		currentPosition = settings.currentPosition;
		delayMultiplier = settings.delayMultiplier;
		useFineMicroSteps = settings.useFineMicroSteps;
	} else {
		settings.marker = EEPROM_MARKER;
		settings.currentPosition = currentPosition = 0;
		settings.delayMultiplier = delayMultiplier = 2;
		settings.useFineMicroSteps = useFineMicroSteps = true;
	}
}

void saveSettings() {
	settings.currentPosition = currentPosition;
	settings.delayMultiplier = delayMultiplier;
	settings.useFineMicroSteps = useFineMicroSteps;

	uint8_t* dst = (uint8_t*)&settings;
	for (int i = 0; i < sizeof(Settings); i++) {
		if (EEPROM.read(i) != dst[i]) {
			EEPROM.write(i, dst[i]);
		}
	}
}
	
void setup() {
	loadSettings();

	currentPosition = settings.currentPosition;
	delayMultiplier = settings.delayMultiplier;
	newPosition = currentPosition;
	clearBuffer(serialBuffer, 8);
	Serial.begin(9600);
	while(!Serial) {
		delay(10);
	}

	stepper.begin(RPM, MICROSTEPS_HALF);
	stepper.disable();
	setStepInterval(((unsigned int)delayMultiplier) * DELAY_MULTIPLIER);
	Timer1.attachInterrupt(motorInterrupt);
}

void motorInterrupt() {
	if (!isMoving) {
		return;
	}

	/* Move stepper and update currentPosition */
	if (currentPosition < newPosition) {
		if (currentPosition < 65535) {
			if (!stepperEnabled) {
				stepper.enable();
				delay(1);
				stepperEnabled = true;
			}
			stepper.move(1);
			currentPosition += 1;
		} else {
			isMoving = false;
		}
	} else if (currentPosition > newPosition) {
		if (currentPosition > 0) {
			if (!stepperEnabled) {
				stepper.enable();
				delay(1);
				stepperEnabled = true;
			}
			stepper.move(-1);
			currentPosition -= 1;
		} else {
			isMoving = false;
		}
	}
	
	if (currentPosition == newPosition) {
		isMoving = false;
		saveSettings();
	}

	/* Release motor if stopping conditions reached */
	if (!isMoving) {
		stepper.disable();
		stepperEnabled = false;
	}
}

void setStepInterval(long microseconds) {
	Timer1.initialize(microseconds);
}

void loop() {
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
					stepper.setMicrostep(MICROSTEPS_FULL);
					useFineMicroSteps = false;
					saveSettings();
					break;
				case set_half_step:
					/* Set half-step mode */
					if (isMoving) break;
					stepper.setMicrostep(MICROSTEPS_HALF);
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
					setStepInterval(((unsigned int)delayMultiplier) * DELAY_MULTIPLIER);
					saveSettings();
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

