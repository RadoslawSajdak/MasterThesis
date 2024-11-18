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
static int get_epoch_from_msg(uint32_t *epoch, uint8_t *msg);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */
static const char *msg_str[] = {
    [MSG_BOOT_DONE]                       = "BOOT",
    [MSG_START_MEASUREMENT]               = "START",
    [MSG_STOP_MEASUREMENT]                = "STOP",
    [MSG_STORE_MEASUREMENT]               = "STORE",
    [MSG_POWERDOWN]                       = "POWERDOWN",
    [MSG_EPOCH_SYNC]                      = "EPOCH",
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
    uint32_t epoch;
    app_event_t event;
    enum uart_msg uart_msg_type;
    uart_init();
    while (1)
    {
        if (0 == uart_wait_for_msg(msg, sizeof(msg), K_FOREVER));
        LOG_DBG("MSG: %s", msg);
        uart_msg_type = msg_to_code(msg, sizeof(msg));
        if (uart_msg_type == MSG_EPOCH_SYNC)
        {
            event.event = APP_EVENT_EPOCH;
            get_epoch_from_msg(&epoch, msg);
            event.data = &epoch;
        }
        else
        {
            event.event = APP_EVENT_UART;
            event.data = &uart_msg_type;
        }
        app_event_submit(event);
    }
}

/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
static int get_epoch_from_msg(uint32_t *epoch, uint8_t *msg)
{
    char *next = NULL;
    char *end = NULL;
    if (!epoch || !msg)
        return -EINVAL;

    next = memchr(msg, ',', strlen(msg));
    if(!next)
        return -ENOTSUP;
    
    next++;
    end = memchr(next, '\n', strlen(next));
    *end = 0;

    *epoch = atoi(next);
    return 0;

}

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
    return MSG_NUM;
}