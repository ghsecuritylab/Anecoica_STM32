/*
 * global_preferences.c
 *
 *  Created on: 14 oct. 2018
 *      Author: charlas
 */

#include "global_preferences.h"

#include <string.h>

#include "peripherals.h"

basic_config_t basic_config;
advanced_config_t advanced_config;
system_config_t system_config;

uint8_t axis_selected = 0;

uint16_t set_basic_config(char* buf) {
	memcpy(&basic_config.packet, buf, sizeof(basic_config.packet));
	return sizeof(basic_config.packet);
}

uint16_t set_advanced_config(char* buf) {
	memcpy(&advanced_config.packet, buf, sizeof(advanced_config.packet));
	return sizeof(advanced_config.packet);
}

uint16_t set_system_config(char* buf) {
	memcpy(&system_config.packet, buf, sizeof(system_config.packet));
	return sizeof(system_config.packet);
}

uint16_t set_axis(char* buf) {
	memcpy(&axis_selected, buf, sizeof(axis_selected));
	select_axis(axis_selected);
	return sizeof(axis_selected);
}



axis_type_t axisType() {
	return advanced_config.bf.axisType;
}

float upperRange() {
	return advanced_config.bf.upperRange;
}

float lowerRange() {
	return advanced_config.bf.lowerRange;
}

float acceleration() {
	return basic_config.bf.acceleration;
}

float deceleration() {
	return basic_config.bf.deceleration;
}

float positionWindow() {
	return basic_config.bf.posWindow;
}

float minSpeed() {
	return basic_config.bf.minSpeed;
}

float maxSpeed() {
	return basic_config.bf.maxSpeed;
}

float maxVolt() {
	return advanced_config.bf.maxVolt;
}

uint8_t limited() {
	return basic_config.bf.limited;
}

uint8_t axisSelected() {
	return axis_selected;
}

uint32_t TTL_flank() {
	return system_config.bf.ttlFlank;
}
