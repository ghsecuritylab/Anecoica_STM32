/*
 * global_preferences.h
 *
 *  Created on: 14 oct. 2018
 *      Author: charlas
 */

#ifndef GLOBAL_PREFERENCES_H_
#define GLOBAL_PREFERENCES_H_

#include <stdint.h>

typedef enum {
	LINEAR = 0,
	ANGULAR
} axis_type_t;

typedef union {
    struct  __attribute__((packed)){
    	float upperLimit;
    	float lowerLimit;
    	float offset;
    	float convFactor;
    	float maxSpeed;
    	float minSpeed;
    	float limitSpeed;
    	float posWindow;
    	uint32_t limited;
    	float slowSpeed;
    	float fastSpeed;
    	float expSpeed;
    	float acceleration;
    	float deceleration;
    }bf;
    unsigned char packet[56];
} basic_config_t;

typedef union {
    struct  __attribute__((packed)){
    	float upperRange;
    	float lowerRange;
    	uint32_t invLimits;
    	float maxVolt;
    	float minVolt;
    	float accelDist;
    	float deccelDist;
    	axis_type_t axisType;
    }bf;
    unsigned char packet[32];
} advanced_config_t;

typedef union {
    struct  __attribute__((packed)){
    	uint32_t address;
    	uint32_t ttlFlank;
    }bf;
    unsigned char packet[8];
} system_config_t;

uint16_t set_basic_config(char* buf);
uint16_t set_advanced_config(char* buf);
uint16_t set_system_config(char* buf);
uint16_t set_axis(char* buf);

axis_type_t axisType();
float upperRange();
float lowerRange();
float acceleration();
float deceleration();
float positionWindow();
float minSpeed();
float maxSpeed();
float maxVolt();
uint8_t limited();
uint8_t axisSelected();
uint32_t TTL_flank();

#endif /* GLOBAL_PREFERENCES_H_ */
