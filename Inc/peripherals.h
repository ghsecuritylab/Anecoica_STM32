/*
 * peripherals.h
 *
 *  Created on: 28 ene. 2019
 *      Author: charlas
 */

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include <stdint.h>

void init_peripherials();
float pot_value();
void select_axis(uint8_t axis);
void enable_axis(uint8_t axis);
void release_brake(uint8_t axis);
void push_brakes();
uint8_t clock_limits();
uint8_t clock_limit(uint8_t axis);
uint8_t counter_limits();
uint8_t counter_limit(uint8_t axis);
void limits_controller(void const * argument);
float synchro_position();
uint8_t synchro_BIT();
uint8_t synchro_CB();
void synchro_INH(uint8_t enable);
void fire_TTL();



#endif /* PERIPHERALS_H_ */
