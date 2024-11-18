/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "app_event.h"
#include "app_uart.h"
#include "uart_api.h"


/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define UART_THREAD_SIZE            (2048U)
#define UART_THREAD_PRIORITY        (10U)

/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
static void uart_thread(void *unused_0, void *unused_1, void *unused_2);
static int msg_to_code(uint8_t *msg, uint16_t len);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */
static const char *msg_str[] = {
    [MSG_BOOT_DONE]                       = "BOOT",
    [MSG_START_MEASUREMENT]               = "START",
    [MSG_START_MEASUREMENT_ACK]           = "START",
    [MSG_STOP_MEASUREMENT]                = "STOP",
    [MSG_STOP_MEASUREMENT_ACK]            = "STOP",
    [MSG_STORE_MEASUREMENT]               = "STORE",
    [MSG_STORE_MEASUREMENT_ACK]           = "STORE",
    [MSG_POWERDOWN]                       = "POWERDOWN",
};

static K_THREAD_DEFINE(uart_thread_id, UART_THREAD_SIZE, uart_thread, NULL, NULL, NULL,
                       UART_THREAD_PRIORITY, 0U, 0U);
LOG_MODULE_REGISTER(APP_UART, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int app_uart_write(uint8_t *msg, uint16_t len)
{
    uart_print(msg, len);
    return 0;
}
__NO_RETURN static void uart_thread(void *unused_0, void *unused_1, void *unused_2)
{
    uint8_t msg[256] = {0};
    app_event_t event;
    uart_msg_t msg_to_send;
    uart_init();
    while (1)
    {
        if (0 == uart_wait_for_msg(msg, sizeof(msg), K_FOREVER));

        LOG_DBG("MSG: %s", msg);
        switch (msg_to_code(msg, sizeof(msg)))
        {
            case MSG_BOOT_DONE:
                LOG_DBG("MSG BOOT DONE");
                msg_to_send.msg_type = MSG_BOOT_DONE;
                event.event = APP_EVENT_UART;
                event.data = &msg_to_send;
                app_event_submit(event);
                break;

            case MSG_START_MEASUREMENT:
                LOG_DBG("MSG START MEASUREMENT");
                msg_to_send.msg_type = MSG_START_MEASUREMENT;
                event.event = APP_EVENT_UART;
                memcpy(msg_to_send.payload, msg + sizeof("START"), sizeof("OK"));
                event.data = &msg_to_send;
                app_event_submit(event);
                break;

            case MSG_START_MEASUREMENT_ACK:
                LOG_DBG("MSG START MEASUREMENT ACK");
                break;

            case MSG_STOP_MEASUREMENT:
                LOG_DBG("MSG STOP MEASUREMENT");
                break;

            case MSG_STOP_MEASUREMENT_ACK:
                LOG_DBG("MSG STOP MEASUREMENT ACK");
                break;

            case MSG_STORE_MEASUREMENT:
                LOG_DBG("MSG STORE MEASUREMENT");
                break;

            case MSG_STORE_MEASUREMENT_ACK:
                LOG_DBG("MSG STORE MEASUREMENT ACK");
                break;

            case MSG_POWERDOWN:
                LOG_DBG("MSG POWERDOWN");
                break;

            default:
                LOG_WRN("Unhandled message code: %d", msg_to_code(msg, sizeof(msg)));
        
            }
    }
}

/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
static int msg_to_code(uint8_t *msg, uint16_t len)
{
    char *next = NULL;
    uint16_t code_len = 0;
    if (!msg)
        return -1;
    next = memchr(msg, ',', len);
    if (next)
        code_len = next - (char*)msg;
    else
        code_len = strlen(msg) - 1;

    for (int i = 0; i < MSG_NUM; i++)
        if (0 == strncmp(msg, msg_str[i], code_len))
            return i;
    
}