/*
 * adc_driver.c
 *
 *  Created on: 20 dic. 2018
 *      Author: charlas
 */

#include "adc.h"

#define VOLT 3.3
#define RESISTOR 1000

float multiplier = 0.0;

void adc_init()
{
	HAL_ADC_Start(&hadc1);
	multiplier = (3300 * 2)/(4095 * VOLT * RESISTOR);
}

float adc_value()
{
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	return ((float) HAL_ADC_GetValue(&hadc1)) * multiplier - 1;
}
