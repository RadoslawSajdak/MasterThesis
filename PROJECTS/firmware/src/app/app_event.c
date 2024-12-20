/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "app_uart.h"
#include "app_event.h"

#include "rtc_api.h"


/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define EVENT_THREAD_SIZE            (1024U)
#define EVENT_THREAD_PRIORITY        (10U)

typedef void (*app_event_cb_t)(void * data);
/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
static void event_thread(void *unused_0, void *unused_1, void *unused_2);
static void uart_msg_cb(void *data);
static void uart_epoch_update(void *data);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */

static app_event_cb_t app_event[APP_EVENT_NUM] = {
    [APP_EVENT_UART] = uart_msg_cb,
    [APP_EVENT_EPOCH]= uart_epoch_update,
};


static K_THREAD_DEFINE(evt_thread_id, EVENT_THREAD_SIZE, event_thread, NULL, NULL, NULL,
                       EVENT_THREAD_PRIORITY, 0U, 0U);
K_MSGQ_DEFINE(app_event_q, sizeof(app_event_t), 5, 2);

LOG_MODULE_REGISTER(APP_EVENT, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */


int app_event_submit(app_event_t event)
{
    return k_msgq_put(&app_event_q, &event, K_NO_WAIT);
}

/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
__NO_RETURN static void event_thread(void *unused_0, void *unused_1, void *unused_2)
{
    app_event_t event;
    while (1)
    {
        k_msgq_get(&app_event_q, &event, K_FOREVER);

        if (event.event >= APP_EVENT_NUM)
        {
            LOG_ERR("Event %d overflows the array!", event.event);
            continue;
        }
        if (app_event[event.event])
            app_event[event.event](event.data);
    }
}

static void uart_msg_cb(void *data)
{
    enum uart_msg uart_msg = *(enum uart_msg *)data; 
    LOG_DBG("Got msg %d", uart_msg);
    switch (uart_msg)
    {
    case MSG_BOOT_DONE:
        break;
    
    case MSG_START_MEASUREMENT:
        LOG_DBG("Measurement result %d", uart_msg);
        break;
    case MSG_EPOCH_SYNC:
        break;
    
    default:
        break;
    }
}

static void uart_epoch_update(void *data)
{
    if (!data)
        return -EINVAL;

    rtc_epoch_update(*(uint32_t *)data);
}