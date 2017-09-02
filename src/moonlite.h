#ifndef _MOONLITE_H_
#define _MOONLITE_H_

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

uint16_t two_chars_to_uint8(char* buffer) {
	long int result = strtol(buffer, NULL, 16);
	return (uint8_t)(result & 0xFF);
}

void clearBuffer(char* buffer, uint16_t length) {
	for (unsigned int i = 0; i < length; ++i) {
		buffer[i] = 0;
	}
}

#endif
