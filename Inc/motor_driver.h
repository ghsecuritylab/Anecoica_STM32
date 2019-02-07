/*
 * motor_driver.h
 *
 *  Created on: 8 oct. 2018
 *      Author: charlas
 */

#ifndef _MOTOR_DRIVER_H_
#define _MOTOR_DRIVER_H_

#include <stdint.h>

typedef struct {
	uint32_t startTime;
	float startSpeed;
	float targetSpeed;
} speed_params_t;

typedef enum {
	SPEED_STOP = 0, SPEED_RUNNING
} speed_state_t;

void position_updater(void const * argument);
void update_position();
float get_position();
float get_speed();
void set_speed(float speed);
void select_motor(uint8_t select);
void set_speed_params(speed_params_t params);
void stop_speed();
speed_state_t get_speed_state();
void speed_updater(void const * argument);

#endif /* _MOTOR_DRIVER_H_ */
