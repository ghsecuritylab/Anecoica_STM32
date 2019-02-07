/*
 * motor_driver.c
 *
 *  Created on: 8 oct. 2018
 *      Author: charlas
 */

#include "motor_driver.h"

#include <math.h>

#include "stm32f7xx_hal.h"
#include "cmsis_os.h"

#include "main.h"
#include "utils.h"
#include "tcp.h"
#include "global_preferences.h"
#include "peripherals.h"
#include "dac.h"

#define BUFFER_LENGTH 5

float position_buffer[BUFFER_LENGTH] = { 0 };
float position = 0.0;
float position_diff = 0.0;
uint32_t last_time = 0;
uint32_t last_diff = 0;
uint32_t buffer_pointer = 0;

float speed = 0;
speed_params_t speed_params;
speed_state_t speed_state = SPEED_STOP;

#ifndef HARDWARE
float h_speed = 0.0;
#endif

osSemaphoreId pos_sem;
osSemaphoreId speed_sem;

uint8_t delay = 1;

void update_speed(float pos);

void position_updater(void const * argument) {
	// Create semaphore
	osSemaphoreDef(POS_SEM);
	pos_sem = osSemaphoreCreate(osSemaphore(POS_SEM), 1);

	float temp_pos = 0;

#ifndef HARDWARE
	uint32_t time = HAL_GetTick();
#endif

	while (1) {
		if (!axisSelected()) {
			osDelay(10);
			continue;
		}
#ifdef HARDWARE
		// Wait for update_position() to release the semaphore
		osDelay(2);
		//osSemaphoreWait(pos_sem, osWaitForever);

		// Check if BIT and CB bits are low
		if (synchro_BIT() || synchro_CB()) {
			continue;
		}

		// Get position bits from synchro
		temp_pos = synchro_position();
#else
		osDelay(2);
		temp_pos += ((HAL_GetTick() - time) / 1000.0) * h_speed;
		temp_pos = correctPosition(temp_pos);
		time = HAL_GetTick();
#endif

		update_speed(temp_pos);
	}
}

void update_speed(float pos) {
	uint32_t diff = 0;

	if (buffer_pointer == 0) {
		last_time = HAL_GetTick();
	} else if (buffer_pointer == BUFFER_LENGTH - 1) {
		diff = HAL_GetTick() - last_time;
	}

	position_buffer[buffer_pointer] = pos;
	buffer_pointer++;

	if (buffer_pointer < BUFFER_LENGTH) {
		return;
	}

	buffer_pointer = 0;

	float sum = 0;

	for (uint8_t i = 1; i < BUFFER_LENGTH; i++) {
		sum += shortDistance(position_buffer[i - 1], position_buffer[i]);
	}

	sum /= BUFFER_LENGTH;
	sum += position_buffer[0];
	sum = correctPosition(sum);

	float temp_speed = shortDistance(position, sum);
	temp_speed /= (last_diff + diff) / 1000.0;

	position = correctPosition(sum);
	last_diff = diff;
	speed = temp_speed;

	send_position_speed(position, speed);
}

void update_position() {
	if (pos_sem && !delay) {
		osSemaphoreRelease(pos_sem);
	}
}

float get_position() {
	return position;
}

float get_speed() {
	return speed;
}

// TODO: Set speed to the motor
void set_speed(float speed) {
#ifdef HARDWARE
	speed = speed/maxSpeed()*maxVolt();
	set_volt_dac(DAC_FLOAT_INT(speed));
#else
	h_speed = speed;
#endif
}

void set_speed_params(speed_params_t params) {
	speed_params = params;
	if (speed_sem) {
		speed_state = SPEED_RUNNING;
		osSemaphoreRelease(speed_sem);
	}
}

void stop_speed() {
	speed_state = SPEED_STOP;
}

speed_state_t get_speed_state() {
	return speed_state;
}

void speed_updater(void const * argument) {

	osSemaphoreDef(SPEED_SEM);
	speed_sem = osSemaphoreCreate(osSemaphore(SPEED_SEM), 1);

	while (1) {
		osSemaphoreWait(speed_sem, osWaitForever);
		enable_axis(axisSelected());
		while (1) {
			if (speed_state == SPEED_STOP) {
				if(speed_params.targetSpeed == 0.0) {
					enable_axis(0);
					push_brakes();
				}
				break;
			}

			float diff = (HAL_GetTick() - speed_params.startTime) / 1000.0;

			float accel = fabs(speed_params.targetSpeed) >= fabs(speed_params.startSpeed) ? acceleration() : deceleration();
			accel = speed_params.targetSpeed >= speed_params.startSpeed ? accel : -accel;


			float speed = speed_params.startSpeed + diff * accel;
			if(speed > minSpeed()) {
				release_brake(axisSelected());
			}
			if (speed_params.targetSpeed >= speed_params.startSpeed) {
				if (speed >= speed_params.targetSpeed) {
					speed_state = SPEED_STOP;
					set_speed(speed_params.targetSpeed);
					continue;
				}
			} else {
				if (speed <= speed_params.targetSpeed) {
					speed_state = SPEED_STOP;
					set_speed(speed_params.targetSpeed);
					continue;
				}
			}

			set_speed(speed);
			osDelay(1);
		}

	}

}
