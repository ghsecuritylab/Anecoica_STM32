/*
 * tcp.c
 *
 *  Created on: 11 oct. 2018
 *      Author: charlas
 */

#include "cmsis_os.h"
#include "lwip/api.h"
#include "lwip/tcp.h"

#include <string.h>

#include "utils.h"
#include "modes.h"
#include "motor_driver.h"
#include "global_preferences.h"

const uint16_t magicNumber = 0x4d49; // MI
const uint16_t recvMagicMask = 0x5249; // RI
const uint16_t sendMagicMask = 0x414d; // AM

typedef enum {
	CMD_POSITION = 0, CMD_SPEED, CMD_POSITION_SPEED, CMD_STATE, CMD_CHECK, CMD_LIMITS, CMD_KEEPALIVE = 2999, CMD_ACK, CMD_MAX = 65535
} send_commands_t;

typedef struct {
	uint16_t magic;
	uint16_t length;
	uint16_t mask;
	send_commands_t command;
} tcp_header_t;

void conn_handler();

void send_keep_alive();
void send_ack();

uint16_t stop(char* buf);
uint16_t keepAlive(char* buf);

uint16_t (*recv_commands[])(char*) = {
	run_position_mode,
	run_register_mode,
	run_manual_mode,
	set_basic_config,
	set_advanced_config,
	set_system_config,
	set_axis,
	stop,
	keepAlive
};

struct netconn *connection;
float send_pos = 0;

osSemaphoreId write_sem;
uint8_t write = 1;

uint32_t lastSend = 0;

void tcp_server(void const * argument) {
	struct netconn *conn;
	err_t err, accept_err;

	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);

	if (conn == NULL) {
		NVIC_SystemReset();
	}
	/* Bind to port 80 (HTTP) with default IP address */
	err = netconn_bind(conn, NULL, 8000);

	if (err != ERR_OK) {
		NVIC_SystemReset();
	}
	/* Put the connection into LISTEN state */
	netconn_listen(conn);
	while (1) {
		/* accept any incoming connection */
		err = netconn_err(conn);
		if (err != ERR_OK) {
			NVIC_SystemReset();
		}
		accept_err = netconn_accept(conn, &connection);
		if (accept_err == ERR_OK) {
			conn_handler();
			stop_mode();
			NVIC_SystemReset();
		} else {
			NVIC_SystemReset();
		}
	}

	netconn_close(conn);
	netconn_delete(conn);

	NVIC_SystemReset();
	osThreadTerminate(NULL);
}

void conn_handler() {
	struct netbuf *inbuf;
	err_t conn_err;
	err_t recv_err;
	char* buf;
	uint16_t buflen;
	uint16_t pointer;
	tcp_header_t header;

	tcp_nagle_disable(connection->pcb.tcp);
	connection->recv_timeout = 1000;

	while (1) {
		conn_err = netconn_err(connection);
		if (conn_err != ERR_OK) {
			break;
		}

		recv_err = netconn_recv(connection, &inbuf);
		if (recv_err != ERR_OK) {
			break;
		}

		netbuf_data(inbuf, (void**) &buf, &buflen);

		pointer = 0;

		while (pointer < buflen) {
			memcpy(&header, buf, sizeof(tcp_header_t));
			if (magicNumber == header.magic) {
				uint16_t maskRecv = header.length + magicNumber;
				maskRecv &= recvMagicMask;
				if (maskRecv == header.mask) {
					pointer += sizeof(tcp_header_t) + recv_commands[(uint16_t) header.command](&buf[sizeof(tcp_header_t)]);
				}
			}
			send_ack();
		}
		netbuf_delete(inbuf);

		send_keep_alive();

		//osDelay(1);

	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(connection);
	netconn_delete(connection);

	/* Delete the buffer (netconn_recv gives us ownership,
	 so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
}

void tcp_writer(void const * argument) {
	osSemaphoreDef(WRITE_SEM);
	write_sem = osSemaphoreCreate(osSemaphore(WRITE_SEM), 1);

	char buf[11];
	tcp_header_t header = { magicNumber, 5, (5 + magicNumber) & sendMagicMask, 0 };

	while (1) {

		osSemaphoreWait(write_sem, osWaitForever);
		write = 0;
		if (netconn_err(connection) == ERR_OK) {
			memcpy(&buf[0], &header, sizeof(tcp_header_t));
			memcpy(&buf[7], &send_pos, sizeof(send_pos));
			netconn_write(connection, &buf, 11, NETCONN_NOCOPY);
		}
	}
}

void send_packet(void *data, uint32_t len, send_commands_t command) {
	char buf[sizeof(tcp_header_t) + len];
	uint16_t mask = magicNumber;
	mask += len;
	mask &= sendMagicMask;
	tcp_header_t header = { magicNumber, len, (len + magicNumber) & sendMagicMask, command };

	err_t err = netconn_err(connection);
	if (err == ERR_OK) {
		memcpy(&buf[0], &header, sizeof(tcp_header_t));
		memcpy(&buf[sizeof(tcp_header_t)], data, len);
		netconn_write(connection, &buf, sizeof(buf), NETCONN_COPY);
	} else {
		uint8_t hola = 0;
		hola++;
	}
}

void send_keep_alive() {
	uint8_t buf = 0;
	send_packet(&buf, sizeof(buf), CMD_KEEPALIVE);
}

void send_ack() {
	uint8_t buf = 0;
	send_packet(&buf, sizeof(buf), CMD_ACK);
}

void send_position_speed(float pos, float speed) {
	if (HAL_GetTick() - lastSend > 10) {
		uint8_t buf[sizeof(pos) * 2];
		memcpy(&buf[0], &pos, sizeof(pos));
		memcpy(&buf[sizeof(pos)], &speed, sizeof(speed));
		send_packet(buf, sizeof(buf), CMD_POSITION_SPEED);
		lastSend = HAL_GetTick();
	}
}

void send_position(float pos) {
	uint8_t buf[sizeof(float)];
	memcpy(&buf[0], &pos, sizeof(pos));
	send_packet(buf, sizeof(buf), CMD_POSITION);
}

void send_speed(float speed) {
	uint8_t buf[sizeof(float)];
	memcpy(&buf[0], &speed, sizeof(speed));
	send_packet(buf, sizeof(buf), CMD_SPEED);
}

void send_state(run_mode_t state) {
	uint8_t buf[sizeof(run_mode_t)];
	memcpy(&buf[0], &state, sizeof(state));
	send_packet(buf, sizeof(buf), CMD_STATE);
}

void send_checkpoint(float pos) {
	uint8_t buf[sizeof(pos)];
	memcpy(&buf[0], &pos, sizeof(pos));
	send_packet(buf, sizeof(buf), CMD_CHECK);
}

void send_limits(uint8_t clock_limits, uint8_t counter_limits) {
	uint8_t buf[2];
	buf[0] = clock_limits;
	buf[1] = counter_limits;
	send_packet(buf, sizeof(buf), CMD_LIMITS);
}

// TODO: LIMITS;

uint16_t stop(char* buf) {
	stop_mode();
	return 0;
}

uint16_t keepAlive(char* buf) {
	return 0;
}

// TODO: STOP

