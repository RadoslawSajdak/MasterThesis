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
    MSG_START_MEASUREMENT_ACK,
    MSG_STOP_MEASUREMENT,
    MSG_STOP_MEASUREMENT_ACK,
    MSG_STORE_MEASUREMENT,
    MSG_STORE_MEASUREMENT_ACK,
    MSG_POWERDOWN,
    MSG_NUM
};

typedef struct
{
    enum uart_msg msg_type;
    char payload[32];
}uart_msg_t;

/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int app_uart_write(uint8_t *msg, uint16_t len);
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
#endif