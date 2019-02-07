/*
 * utils.h
 *
 *  Created on: 14 oct. 2018
 *      Author: charlas
 */

#ifndef UTILS_H_
#define UTILS_H_

float frontDistance(float from, float to);
float behindDistance(float from, float to);
float shortDistance(float from, float to);
float accelerationDistance(float from, float to);
float decelerationDistance(float from, float to);
float correctPosition(float pos);

float getFloat(char* buf);
void HAL_Delay_Microseconds(uint32_t uSec);

void DWT_Init(void);
void DWT_Delay(uint32_t us);

#endif /* UTILS_H_ */
