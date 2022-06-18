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
	set_hold_enabled,
	set_hold_disabled,
	unrecognized
} MOONLITE_COMMAND;

#define NUM_MOONLITE_COMMANDS 17

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
	{ get_temperature, "GT" },
	{ set_hold_enabled, "HE" },
	{ set_hold_disabled, "HD" }
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

uint8_t two_chars_to_uint8(char* buffer) {
	long int result = strtol(buffer, NULL, 16);
	return (uint8_t)(result & 0xFF);
}

void clearBuffer(char* buffer, uint16_t length) {
	for (unsigned int i = 0; i < length; ++i) {
		buffer[i] = 0;
	}
}

#endif
