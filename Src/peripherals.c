/*
 * peripherals.c
 *
 *  Created on: 28 ene. 2019
 *      Author: charlas
 */

#include "peripherals.h"

#include "stm32f7xx_hal.h"
#include "cmsis_os.h"

#include "adc.h"
#include "spi.h"
#include "global_preferences.h"
#include "tcp.h"
#include "utils.h"

#define MIN_POSITION 0.005493164062 // 180 * 2^-15
#define VOLT 4.2
#define RESISTOR 10000

#define BIT_PORT GPIOE
#define BIT_PIN GPIO_PIN_6

#define CB_PORT GPIOG
#define CB_PIN GPIO_PIN_12

#define INH_PORT GPIOG
#define INH_PIN GPIO_PIN_9

#define TTL_PORT GPIOG
#define TTL_PIN GPIO_PIN_7

#define TTL_PERIOD 1

void init_pot();
void init_speed();
void init_TTL();
void stop_TTL(void const *argument);

GPIO_TypeDef* positionPorts[16] = { GPIOE, GPIOG, GPIOG, GPIOD, GPIOF, GPIOD,
GPIOF, GPIOF, GPIOE, GPIOE, GPIOE, GPIOE, GPIOG, GPIOD, GPIOD, GPIOD };

uint16_t positionPins[16] = { GPIO_PIN_1, GPIO_PIN_0, GPIO_PIN_1,
GPIO_PIN_0, GPIO_PIN_9, GPIO_PIN_1, GPIO_PIN_8, GPIO_PIN_2, GPIO_PIN_5,
GPIO_PIN_4, GPIO_PIN_3, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_6,
GPIO_PIN_5, GPIO_PIN_3 };

GPIO_TypeDef* selectPorts[3] = { GPIOG, GPIOG, GPIOG };
uint16_t selectPins[3] = { GPIO_PIN_4, GPIO_PIN_10, GPIO_PIN_15 };

GPIO_TypeDef* brakePorts[6] = { GPIOD, GPIOC, GPIOC, GPIOC, GPIOB, GPIOF };
uint16_t brakePins[6] = { GPIO_PIN_4, GPIO_PIN_3, GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_7, GPIO_PIN_7 };

GPIO_TypeDef* enablePorts[6] = { GPIOB, GPIOB, GPIOB, GPIOD, GPIOA, GPIOA };
uint16_t enablePins[6] = { GPIO_PIN_3, GPIO_PIN_5, GPIO_PIN_4, GPIO_PIN_13, GPIO_PIN_3, GPIO_PIN_10 };

GPIO_TypeDef* clockLimitsPorts[6] = { GPIOF, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC };
uint16_t clockLimitsPins[6] = { GPIO_PIN_6, GPIO_PIN_13, GPIO_PIN_2, GPIO_PIN_12, GPIO_PIN_10, GPIO_PIN_7 };

GPIO_TypeDef* counterLimitsPorts[6] = { GPIOA, GPIOA, GPIOC, GPIOC, GPIOD, GPIOB };
uint16_t counterLimitsPins[6] = { GPIO_PIN_15, GPIO_PIN_0, GPIO_PIN_0, GPIO_PIN_11, GPIO_PIN_2, GPIO_PIN_6 };

float pot_multiplier = 0.0;
osTimerId tim1;
uint8_t clock_limits_v = 0;
uint8_t counter_limits_v = 0;

void init_peripherials() {
	init_pot();
	init_TTL();
	synchro_INH(1);
	select_axis(0);
	enable_axis(0);
	push_brakes();
}

void init_pot() {
	HAL_ADC_Start(&hadc1);
	pot_multiplier = (5.0 / 4095.0) * (2.0 / VOLT);
	//pot_multiplier = (3300 * 2) / (4095 * VOLT * RESISTOR);
}

float pot_value() {
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	return ((float) HAL_ADC_GetValue(&hadc1)) * pot_multiplier - 1;
}

void select_axis(uint8_t axis) {
	for (uint8_t i = 0; i < 3; ++i) {
		HAL_GPIO_WritePin(selectPorts[i], selectPins[i], (axis >> i) & 1);
	}
}

void enable_axis(uint8_t axis) {
	axis = axis > 0 ? (1 << (axis - 1)) : 0;
	for (uint8_t i = 0; i < 6; ++i) {
		HAL_GPIO_WritePin(enablePorts[i], enablePins[i], !((axis >> i) & 1));
	}
}

void release_brake(uint8_t axis) {
	if (axis <= 0) {
		push_brakes();
		return;
	}
	axis = axis - 1;
	HAL_GPIO_WritePin(brakePorts[axis], brakePins[axis], 1);
}

void push_brakes() {
	for (uint8_t i = 0; i < 6; ++i) {
		HAL_GPIO_WritePin(brakePorts[i], brakePins[i], 0);
	}
}

uint8_t clock_limits() {
	uint8_t limit_bits = 0;
	for (uint8_t i = 0; i < 6; ++i) {
		limit_bits |= HAL_GPIO_ReadPin(clockLimitsPorts[i], clockLimitsPins[i]) << i;
	}
	clock_limits_v = limit_bits;
	return limit_bits;
}

uint8_t clock_limit(uint8_t axis) {
	return (clock_limits_v >> (axis - 1)) & 1;
}

uint8_t counter_limits() {
	uint8_t limit_bits = 0;
	for (uint8_t i = 0; i < 6; ++i) {
		limit_bits |= HAL_GPIO_ReadPin(counterLimitsPorts[i], counterLimitsPins[i]) << i;
	}
	counter_limits_v = limit_bits;
	return limit_bits;
}

uint8_t counter_limit(uint8_t axis) {
	return (counter_limits_v >> (axis - 1)) & 1;
}

void limits_controller(void const * argument) {
	while (1) {
		if (counter_limits() || clock_limits()) {
			send_limits(clock_limits_v, counter_limits_v);
		}
		osDelay(1);
	}

	osThreadTerminate(NULL);
}

float synchro_position() {
	uint16_t position_bits = 0;
	for (uint8_t i = 0; i < 16; ++i) {
		position_bits |= HAL_GPIO_ReadPin(positionPorts[i], positionPins[i]) << (15 - i);
	}

	return ((float) (position_bits)) * MIN_POSITION;
}

uint8_t synchro_BIT() {
	return HAL_GPIO_ReadPin(BIT_PORT, BIT_PIN);
}

uint8_t synchro_CB() {
	return HAL_GPIO_ReadPin(CB_PORT, CB_PIN);
}

void synchro_INH(uint8_t enable) {
	return HAL_GPIO_WritePin(INH_PORT, INH_PIN, enable);
}

void init_TTL() {
	osTimerDef(tim1, stop_TTL);
	osTimerCreate(osTimer(tim1), osTimerOnce, NULL);
	HAL_GPIO_WritePin(TTL_PORT, TTL_PIN, !TTL_flank());
}

void fire_TTL() {
	HAL_GPIO_WritePin(TTL_PORT, TTL_PIN, TTL_flank());
	DWT_Delay(TTL_PERIOD);
}

void stop_TTL(void const *argument) {
	HAL_GPIO_WritePin(TTL_PORT, TTL_PIN, !TTL_flank());
}

