/*
 * tcp.h
 *
 *  Created on: 11 oct. 2018
 *      Author: charlas
 */

#ifndef TCP_H_
#define TCP_H_

#include "modes.h"

void tcp_server(void const * argument);
void tcp_writer(void const * argument);
void send_position_speed(float pos, float speed);
void send_position(float position);
void send_speed(float speed);
void send_state(run_mode_t state);
void send_limits(uint8_t clock_limits, uint8_t counter_limits);

#endif /* TCP_H_ */
