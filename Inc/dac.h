/*
 * dac.h
 *
 *  Created on: 3 feb. 2019
 *      Author: charlas
 */

#ifndef DAC_H_
#define DAC_H_

#include <stdint.h>

#define DAC_FLOAT_INT(v) ((uint16_t)((v+10.0)/20.0*(0x10000)))

void init_dac();
void reset_dac();
void set_volt_dac(uint16_t voltage);

#endif /* DAC_H_ */
