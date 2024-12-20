#ifndef __APP_EVENT_H__
#define __APP_EVENT_H__

/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <stdint.h>

/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
enum app_event{
    APP_EVENT_UART,
    APP_EVENT_EPOCH,

    APP_EVENT_NUM
};

typedef struct
{
    enum app_event event;
    void *data;
}app_event_t;
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int app_event_submit(app_event_t event);
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
#endif