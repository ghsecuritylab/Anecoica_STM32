/*
 * state_machine.h
 *
 *  Created on: 8 oct. 2018
 *      Author: charlas
 */

#ifndef _MODES_H_
#define _MODES_H_

#include <stdint.h>

typedef enum
{
	IDLE = 0,
	POSITION_MODE,
	REGISTER_MODE,
	MANUAL_MODE,
	MODE_MAX = 65535
} run_mode_t;


typedef enum {
	DIRECT_DIR = 0,
	REVERSE_DIR,
	SHORT_DIR,
	DIR_MAX = 65535
} direction_t;

typedef union {
    struct  __attribute__((packed)){
        float position;
        float speed;
        direction_t direction;
    }bf;
    unsigned char packet[10];
} position_params_t;

typedef union {
    struct  __attribute__((packed)){
        float from;
        float to;
        float increment;
        float pos_speed;
        float reg_speed;
        direction_t direction;
        uint32_t checkpoints;
    }bf;
    unsigned char packet[26];
} register_params_t;

typedef enum
{
	SLOW_TYPE = 0,
	FAST_TYPE,
	EXP_TYPE,
	STOP_TYPE,
	MANUAL_MAX = 65535
} manual_type_t;

typedef union {
    struct  __attribute__((packed)){
    	manual_type_t type;
    }bf;
    unsigned char packet[2];
} manual_params_t;

uint16_t run_position_mode(char* buf);
uint16_t run_register_mode(char* buf);
uint16_t run_manual_mode(char* buf);
uint16_t stop_mode();

void run_modes(void const * argument);

#endif /* _MODES_H_ */
