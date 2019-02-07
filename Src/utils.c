/*
 * utils.c
 *
 *  Created on: 14 oct. 2018
 *      Author: charlas
 */

#include "global_preferences.h"

#include <math.h>
#include <string.h>
#include <limits.h>

#include "stm32f7xx_hal.h"

#include "stm32f7xx.h"

float frontDistance(float from, float to) {
	if (from <= to) {
		return to - from;
	} else {
		return (upperRange() - lowerRange()) - (from - to);
	}
}

float behindDistance(float from, float to) {
	if (to <= from) {
		return from - to;
	} else {
		return (upperRange() - lowerRange()) - (to - from);
	}
}

float shortDistance(float from, float to) {

	float front = frontDistance(from, to);
	float behind = behindDistance(from, to);
	if (front <= behind) {
		return front;
	} else {
		return -behind;
	}

}

float accelerationDistance(float from, float to) {
	return fabs(pow(to, 2) - pow(from, 2)) / (acceleration() * 2);
}

float decelerationDistance(float from, float to) {
	return fabs(pow(to, 2) - pow(from, 2)) / (deceleration() * 2);
}

float correctPosition(float pos) {
	if (pos < lowerRange()) {
		return pos + (upperRange() - lowerRange());
	} else if (pos > upperRange()) {
		return pos - (upperRange() - lowerRange());
	} else {
		return pos;
	}

}

float getFloat(char* buf) {
	float number;
	//float number = *(buf) << 24 | *(buf + 1) << 16 | *(buf + 2) << 8 | *(buf + 3) << 0;
	memcpy(&number, buf, sizeof(number));
	return number;
}

void DWT_Init(void) {

	DWT->LAR = 0xC5ACCE55;
	DWT->CTRL |= 1;
	DWT->CYCCNT = 0;
}

void DWT_Delay(uint32_t us) // microseconds
{
	int32_t targetTick = DWT->CYCCNT + us * (SystemCoreClock / 1000000);
	do {
	} while (DWT->CYCCNT <= targetTick);
}

