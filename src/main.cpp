#include <Wire.h>
#include <Adafruit_MotorShield.h>

#define UNPOWERED_TEST

#define SERIAL_BUFFER_LENGTH 8
char serialBuffer[SERIAL_BUFFER_LENGTH];

Adafruit_MotorShield AFMS;
Adafruit_StepperMotor* motor;

uint16_t currentPosition;
uint16_t newPosition;
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

typedef enum {
  stop,
  get_current_position,
  set_current_position,
  get_new_position,
  set_new_position,
  go_to_new_position,
  check_if_half_step,
  set_full_step,
  set_half_step,
  check_if_moving,
  get_firmware_version,
  get_backlight_value,
  get_speed,
  set_speed,
  get_temperature,
  unrecognized
} MOONLITE_COMMAND;

#define NUM_MOONLITE_COMMANDS 15

const static struct {
  MOONLITE_COMMAND val;
  const char *str;
} moonlite_command_mapping [] = {
  { stop, "FQ" },
  { get_current_position, "GP" },
  { set_current_position, "SP" },
  { get_new_position, "GN" },
  { set_new_position, "SN" },
  { go_to_new_position, "FG" },
  { check_if_half_step, "GH" },
  { set_full_step, "SF" },
  { set_half_step, "SH" },
  { check_if_moving, "GI" },
  { get_firmware_version, "GV" },
  { get_backlight_value, "GB" },
  { get_speed, "GD" },
  { set_speed, "SD" },
  { get_temperature, "GT" }
};

MOONLITE_COMMAND moonliteStringToEnum(char* buffer) {
  for (int i = 0; i < NUM_MOONLITE_COMMANDS; ++i) {
    if (strncmp(buffer, moonlite_command_mapping[i].str, 2) == 0) {
      return moonlite_command_mapping[i].val;
    }
  }
  return MOONLITE_COMMAND::unrecognized;
}

uint16_t four_chars_to_uint16(char* buffer) {
  long int result = strtol(buffer, NULL, 16);
  return (uint16_t)(result & 0xFFFF);
}

void loop() {
  if (isMoving) {
    /* Move stepper and update currentPosition */
    if (currentPosition < newPosition) {
      if (currentPosition < 65535) {
#ifndef UNPOWERED_TEST
        motor->onestep(FORWARD, DOUBLE);
#endif
        currentPosition += 1;
      } else {
        isMoving = false;
      }
    } else if (currentPosition > newPosition) {
      if (currentPosition > 0) {
#ifndef UNPOWERED_TEST
        motor->onestep(BACKWARD, DOUBLE);
#endif
        currentPosition -= 1;
      } else {
        isMoving = false;
      }
    }
    /* TODO: Set stepping delay based on speed commands */
    delay(1000.0/250.0);

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
          Serial.print(currentPosition, HEX);
          Serial.print("#");
          break;
        case set_current_position:
          /* Set current position */
          if (isMoving) break;
          currentPosition = four_chars_to_uint16(serialBuffer + 2);
          break;
        case get_new_position:
          /* Get new position set by SN */
          Serial.print(newPosition, HEX);
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
          /* TODO: Check if half-stepping */
          Serial.print("00#");
          break;
        case set_full_step:
          /* TODO: Set full-step mode */
          if (isMoving) break;
          break;
        case set_half_step:
          /* TODO: Set half-step mode */
          if (isMoving) break;
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
          /* Get current RED Led Backlight value */
          Serial.print("00#");
          break;
        case get_speed:
          /* TODO: Get speed */
          Serial.print("04#");
          break;
        case set_speed:
          /* TODO: Set speed */
          if (isMoving) break;
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

