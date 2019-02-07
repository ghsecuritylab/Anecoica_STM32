/*
 * state_machine.c
 *
 *  Created on: 8 oct. 2018
 *      Author: charlas
 */

#include <modes.h>

#include <math.h>
#include <string.h>

#include "cmsis_os.h"
#include "stm32f7xx_hal.h"

#include "global_preferences.h"
#include "utils.h"
#include "motor_driver.h"
#include "tcp.h"
#include "adc_driver.h"
#include "peripherals.h"

#define SPEED_WINDOW 0.01
#define SLOW_SPEED 0.1
#define FAST_SPEED 1
#define EXP_SPEED 2

void position_mode();
void register_mode();
void manual_mode();
uint8_t dist_to_speed_cond(direction_t dir, float position, float end_speed);
uint8_t arrived_to_position(direction_t dir, float position);

run_mode_t mode = IDLE;

position_params_t position_params;
register_params_t register_params;
manual_params_t manual_params;

void run_modes(void const * argument) {

	while (1) {
		switch (mode) {
		case IDLE:
			send_state(mode);
			break;

		case POSITION_MODE:
			send_state(mode);
			position_mode();
			mode = IDLE;
			send_state(mode);
			break;

		case REGISTER_MODE:
			send_state(mode);
			register_mode();
			mode = IDLE;
			send_state(mode);
			break;

		case MANUAL_MODE:
			send_state(mode);
			manual_mode();
			mode = IDLE;
			send_state(mode);
			break;

		default:
			break;
		}
		osDelay(100);
	}

	osThreadTerminate(NULL);
}

uint16_t run_position_mode(char* buf) {
	if (mode == IDLE) {
		memcpy(&position_params.packet, buf, sizeof(position_params.packet));
		mode = POSITION_MODE;
	}
	return sizeof(position_params.packet);
}

uint16_t run_register_mode(char* buf) {
	if (mode == IDLE) {
		memcpy(&register_params.packet, buf, sizeof(register_params.packet));
		mode = REGISTER_MODE;
	}
	return sizeof(register_params.packet);
}

uint16_t run_manual_mode(char* buf) {
	if (mode == IDLE) {
		memcpy(&manual_params.packet, buf, sizeof(manual_params.packet));
		mode = MANUAL_MODE;
	}
	return sizeof(manual_params.packet);
}

uint16_t stop_mode() {
	mode = IDLE;
	return 0;
}

void position_mode() {

	// TODO: BRAKES; STOP; SPEED
	if (fabs(shortDistance(get_position(), position_params.bf.position)) <= positionWindow()) {
		return;
	}

	direction_t dir;

	if (position_params.bf.direction == SHORT_DIR) {

		if (axisType() == LINEAR) {
			dir = position_params.bf.position >= get_position() ? DIRECT_DIR : REVERSE_DIR;
		} else if (axisType() == ANGULAR) {
			dir = frontDistance(get_position(), position_params.bf.position) <= behindDistance(get_position(), position_params.bf.position) ?
					DIRECT_DIR : REVERSE_DIR;
		}

	} else {

		dir = position_params.bf.direction;

	}

	float speed = dir == DIRECT_DIR ? position_params.bf.speed : -position_params.bf.speed;

	speed_params_t speed_params = { HAL_GetTick(), get_speed(), speed };


	set_speed_params(speed_params);

	while (dist_to_speed_cond(dir, position_params.bf.position, 0.0) == 0 && mode != IDLE) {

		//change_speed();
		osDelay(10);

	}

	stop_speed();

	speed_params.startTime = HAL_GetTick();
	speed_params.startSpeed = get_speed();
	speed_params.targetSpeed = 0.0;

	set_speed_params(speed_params);

	while (fabs(shortDistance(get_position(), position_params.bf.position)) >= positionWindow() && get_speed_state() != SPEED_STOP) {

		//change_speed();
		osDelay(10);

	}
	stop_speed();

	set_speed(0.0);
	// TODO: BRAKES;

}

void register_mode() {

	// TODO: BRAKES; STOP;
	float pos_speed = register_params.bf.direction == DIRECT_DIR ? register_params.bf.pos_speed : -register_params.bf.pos_speed;
	float reg_speed = register_params.bf.direction == DIRECT_DIR ? register_params.bf.reg_speed : -register_params.bf.reg_speed;
	direction_t dir = register_params.bf.direction;
	float from = correctPosition(dir == DIRECT_DIR ? register_params.bf.from - 1 : register_params.bf.from + 1);

	if (dist_to_speed_cond(dir, from + register_params.bf.direction == DIRECT_DIR ? -1 : 1, reg_speed)) {
		return;
	}

	speed_params_t speed_params = { HAL_GetTick(), get_speed(), pos_speed };

	enable_axis(axisSelected());
	set_speed_params(speed_params);

	while (dist_to_speed_cond(dir, from, reg_speed) == 0 && mode != IDLE) {

		//change_speed();
		osDelay(1);
	}

	stop_speed();

	speed_params.startTime = HAL_GetTick();
	speed_params.startSpeed = get_speed();
	speed_params.targetSpeed = reg_speed;

	set_speed_params(speed_params);

	float nextPos = register_params.bf.from;

	uint16_t checkpoints = 0;

	while (1 && mode != IDLE) {

		if (arrived_to_position(dir, nextPos)) {

			checkpoints++;
			fire_TTL();
			if (checkpoints >= register_params.bf.checkpoints) {
				break;
			}

			nextPos = correctPosition(dir == DIRECT_DIR ? nextPos + register_params.bf.increment : nextPos - register_params.bf.increment);
		}
		osDelay(1);
	}

	stop_speed();

	speed_params.startTime = HAL_GetTick();
	speed_params.startSpeed = get_speed();
	speed_params.targetSpeed = 0.0;

	set_speed_params(speed_params);

	while (fabs(get_speed()) >= SPEED_WINDOW) {

		//change_speed();
		//osDelay(1);

	}
	stop_speed();

	enable_axis(0);
	set_speed(0.0);
	// TODO: BRAKES;
}

void manual_mode() {
	float speed = 0.0;
	float pot;
	enable_axis(axisSelected());
	release_brake(axisSelected());
	while (mode != IDLE) {
		pot = pot_value();
		switch (manual_params.bf.type) {
		case SLOW_TYPE:
			speed = pot * SLOW_SPEED;
			break;
		case FAST_TYPE:
			speed = pot * FAST_SPEED;
			break;
		case EXP_TYPE:
			speed = pot > 0 ? pow(EXP_SPEED, pot) : -pow(EXP_SPEED, -pot);
			speed -= 1;
			break;
		default:
			break;
		}

		set_speed(speed);
		osDelay(1);
	}

	enable_axis(0);
	set_speed(0.0);

	push_brakes();
}

uint8_t dist_to_speed_cond(direction_t dir, float position, float end_speed) {
	float dist_to_pos = dir == DIRECT_DIR ? frontDistance(get_position(), position) : behindDistance(get_position(), position);
	float speed = get_speed();
	float dist_to_speed = fabs(end_speed) <= fabs(speed) ? decelerationDistance(speed, end_speed) : accelerationDistance(speed, end_speed);
	return dist_to_pos <= dist_to_speed;
}

uint8_t arrived_to_position(direction_t dir, float position) {
	float front = frontDistance(get_position(), position);
	float behind = behindDistance(get_position(), position);
	if (dir == DIRECT_DIR) {
		return front >= behind || fabs(get_position() - position) <= positionWindow();
	} else {
		return behind >= front || fabs(get_position() - position) <= positionWindow();
	}
}
