/*
 * dac.c
 *
 *  Created on: 3 feb. 2019
 *      Author: charlas
 */

#include "dac.h"

#include "stm32f7xx_hal.h"
#include "spi.h"

#include "utils.h"

#define NSS_PORT GPIOB
#define NSS_PIN GPIO_PIN_12

#define clear_volt(packet, cv) packet[1] = packet[1] | ((0x0003 & cv) << 1)
#define over_range(packet, ov) packet[1] = packet[1] | ((0x0001 & ov))
#define bipolar_range(packet, bi) packet[2] = packet[2] | ((0x0001 & bi) << 7)
#define thermal_alert(packet, et) packet[2] = packet[2] | ((0x0001 & et) << 6)
#define internal_ref(packet, ir) packet[2] = packet[2] | ((0x0001 & ir) << 5)
#define power_volt(packet, pv) packet[2] = packet[2] | ((0x0003 & pv) << 3)
#define out_range(packet, ra) packet[2] = packet[2] | ((0x0007 & ra))
#define daisy(packet, da) packet[2] = packet[2] | ((0x0001 & da))

typedef union {
	struct  __attribute__((packed)){
		unsigned d_care : 23;
		unsigned daisy : 1;
	} daisy;
	struct  __attribute__((packed)){
		unsigned d_care : 13;
		unsigned clear_volt : 2;
		unsigned over_range : 1;
		unsigned bipolar_range : 1;
		unsigned thermal_alert : 1;
		unsigned internal_ref : 1;
		unsigned power_volt : 2;
		unsigned out_range : 3;
	} control;
    struct  __attribute__((packed)){
        unsigned d_care : 4;
        unsigned address : 4;
        unsigned data : 16;
    } general;
    uint8_t packet[3];
} dac_command_t;

enum dac_commands {
	NOP = 0,
	INPUT_W,
	DAC_U,
	DAC_WU,
	CONTROL_W,
	RESET_S = 7,
	DAISY = 9,
	INPUT_R,
	DAC_R,
	CONTROL_R,
	RESET_F = 15,
};

void init_dac_command(dac_command_t *command);
void reorder_dac(dac_command_t *command);

void send_dac(dac_command_t *command);
void send_recv_dac(dac_command_t *recv);
void send_control_dac();
void read_control_dac();
void daisy_dac(uint8_t enabled);
void soft_reset_dac();
void reset_dac();

void init_dac() {
	HAL_SPI_Init(&hspi1);
	HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_SET);
	HAL_Delay(1);
	reset_dac();
	HAL_Delay(100);
	soft_reset_dac();
	HAL_Delay(10);
	//daisy_dac(1);
	send_control_dac();

}

void init_dac_command(dac_command_t *command) {
	for (int i = 0; i < 3; ++i) {
		command->packet[i] = 0;
	}
}

void reorder_dac(dac_command_t *command) {
	for (int i = 0; i < 2; ++i) {
		command->packet[i] = ((command->packet[i] & 0x0F) << 4) | ((command->packet[i] & 0xF0) >> 4);
	}
}

void send_dac(dac_command_t *command) {
	//reorder_dac(command);
	HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_RESET);
	DWT_Delay(1);
	HAL_SPI_Transmit(&hspi1,command->packet,3,1000);
	DWT_Delay(1);
	HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_SET);
	DWT_Delay(1);
}

void send_recv_dac(dac_command_t *recv) {
	//reorder_dac(recv);
	send_dac(recv);
	HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_RESET);
	DWT_Delay(1);
	uint8_t zeros[3] = { 0, 0, 0 };
	HAL_SPI_TransmitReceive(&hspi1, zeros,recv->packet, 3,1000);
	DWT_Delay(1);
	HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_SET);
	DWT_Delay(1);
}

void send_control_dac() {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = CONTROL_W;
	clear_volt(command.packet, 1);
	over_range(command.packet, 0);
	bipolar_range(command.packet, 0);
	thermal_alert(command.packet, 1);
	internal_ref(command.packet, 1);
	power_volt(command.packet, 1);
	out_range(command.packet, 0);
	send_dac(&command);
}

void set_volt_dac(uint16_t voltage) {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = DAC_WU;
	command.packet[1] = (voltage >> 8) & 0xFF;
	command.packet[2] = voltage & 0xFF;
	send_dac(&command);
}

void read_control_dac() {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = CONTROL_R;
	command.packet[1] = 0xFF;
	command.packet[2] = 0xFF;
	send_recv_dac(&command);
}

void daisy_dac(uint8_t enabled) {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = DAISY;
	daisy(command.packet, 1);
	send_dac(&command);
}

void soft_reset_dac() {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = RESET_S;
	send_dac(&command);
}

void reset_dac() {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = RESET_F;
	send_dac(&command);
}

void nop_dac() {
	dac_command_t command;
	init_dac_command(&command);
	command.packet[0] = NOP;
	send_dac(&command);
}

