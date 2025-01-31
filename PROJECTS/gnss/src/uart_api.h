#ifndef __UART_API_H__
#define __UART_API_H__

/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>


/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
extern struct k_msgq uart_rx_q;
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */


void uart_enable(void);
void uart_disable(void);
int uart_init(void);
int uart_wait_for_msg(uint8_t *buffer, uint16_t max_len, k_timeout_t timeout);
void uart_print(uint8_t *buf, uint16_t len);
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
#endif