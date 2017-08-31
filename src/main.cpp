#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "moonlite.h"

#define UNPOWERED_TEST

#define SERIAL_BUFFER_LENGTH 8
char serialBuffer[SERIAL_BUFFER_LENGTH];

Adafruit_MotorShield AFMS;
Adafruit_StepperMotor* motor;

uint16_t currentPosition;
uint16_t newPosition;
uint8_t stepMode = DOUBLE;
uint8_t delayMultiplier = 2;
bool isMoving = false;

void clearBuffer(char* buffer, uint16_t length) {
  for (unsigned int i = 0; i < length; ++i) {
    buffer[i] = 0;
  }
}

void setup() {
  currentPosition = 32768;
  newPosition = 32768;
  clearBuffer(serialBuffer, 8);
  Serial.begin(9600);
  while(!Serial) {
    delay(10);
  }

#ifndef UNPOWERED_TEST
  AFMS = Adafruit_MotorShield();
  motor = AFMS.getStepper(48, 2);
  motor->setSpeed(120);
#endif
}

void loop() {
  if (isMoving) {
    /* Move stepper and update currentPosition */
    if (currentPosition < newPosition) {
      if (currentPosition < 65535) {
#ifndef UNPOWERED_TEST
        motor->onestep(FORWARD, stepMode);
#endif
        currentPosition += 1;
      } else {
        isMoving = false;
      }
    } else if (currentPosition > newPosition) {
      if (currentPosition > 0) {
#ifndef UNPOWERED_TEST
        motor->onestep(BACKWARD, stepMode);
#endif
        currentPosition -= 1;
      } else {
        isMoving = false;
      }
    }
    /* Set stepping delay based on speed commands */
    delayMicroseconds(((unsigned int)delayMultiplier) * 2000);

    if (currentPosition == newPosition) {
      isMoving = false;
    }
  }

  if (Serial.available()) {
    if (Serial.read() == ':') {
      clearBuffer(serialBuffer, SERIAL_BUFFER_LENGTH);
      Serial.readBytesUntil('#', serialBuffer, 8);
      MOONLITE_COMMAND command = moonliteStringToEnum(serialBuffer);
      switch (command) {
        case stop:
          /* STOP EVERYTHING */
          isMoving = false;
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
          if (stepMode == INTERLEAVE) {
            Serial.print("FF#");
          } else {
            Serial.print("00#");
          }
          break;
        case set_full_step:
          /* Set full-step mode */
          if (isMoving) break;
          stepMode = DOUBLE;
          break;
        case set_half_step:
          /* Set half-step mode */
          if (isMoving) break;
          stepMode = INTERLEAVE;
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

