#ifndef __APP_UART_H__
#define __APP_UART_H__

/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <stdint.h>


/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
enum uart_msg{
    MSG_BOOT_DONE,
    MSG_START_MEASUREMENT,
    MSG_STOP_MEASUREMENT,
    MSG_STORE_MEASUREMENT,
    MSG_POWERDOWN,
    MSG_EPOCH_SYNC,
    MSG_NUM
};

/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int app_uart_write(uint8_t *msg, uint16_t len);
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
#endif