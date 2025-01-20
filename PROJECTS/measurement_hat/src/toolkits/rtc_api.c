// /* ============================================================================================== */
// /*                                            INCLUDES                                            */
// #include <zephyr/kernel.h>
// #include <zephyr/logging/log.h>
// #include <nrfx_rtc.h>
// #include <hal/nrf_rtc.h>
#include "rtc_api.h"


// /* ============================================================================================== */

// /* ============================================================================================== */
// /*                                        DEFINES/TYPEDEFS                                        */
// #define RTC_SECONDS(s)                      (s * 8)
// #define RTC_ALARM_COMPARE                   (NRFX_RTC_INT_COMPARE0)
// #define RTC_EPOCH_SECONDS_COMPARE           (NRFX_RTC_INT_COMPARE1)


// /* ============================================================================================== */
// /*                                   PRIVATE FUNCTION DEFINITIONS                                 */
// static void rtc_handler(nrfx_rtc_int_type_t int_type);

// /* ============================================================================================== */
// /*                                        PRIVATE VARIABLES                                       */

// static const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);
static uint32_t epoch_time = 0U;
// static void (*alarm_cb)(void) = NULL;

// LOG_MODULE_REGISTER(RTC, LOG_LEVEL_DBG);
// /* ============================================================================================== */
// /*                                        PUBLIC FUNCTIONS                                        */

// int rtc_init(void)
// {
//     nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
//     config.prescaler = 4095; // 125ms per tick

//     nrfx_rtc_init(&rtc, &config, rtc_handler);

//     /* Rejestracja przerwań RTC2 */
//     IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_RTC2), 1, nrfx_isr, nrfx_rtc_2_irq_handler, 0);
//     irq_enable(NRFX_IRQ_NUMBER_GET(NRF_RTC2));

//     nrfx_rtc_tick_enable(&rtc, true);
//     nrfx_rtc_enable(&rtc);

//     nrfx_rtc_cc_set(&rtc, RTC_EPOCH_SECONDS_COMPARE, nrfx_rtc_counter_get(&rtc) + RTC_SECONDS(1), true);

//     LOG_INF("RTC initialized and started.");
//     return 0;
// }

// int rtc_alarm_cb_register( void (*cb)(void))
// {
//     if (!cb)
//         return -EINVAL;
//     alarm_cb = cb;
//     return 0;
// }

// int rtc_set_alarm_for(int seconds)
// {
//     return nrfx_rtc_cc_set(&rtc, RTC_ALARM_COMPARE, nrfx_rtc_counter_get(&rtc) + RTC_SECONDS(seconds), true);
// }

// int rtc_epoch_update(uint32_t new_epoch)
// {
//     static struct k_mutex epoch_mutex;
//     static bool initialized = false;
//     if (!initialized)
//         k_mutex_init(&epoch_mutex);

//     k_mutex_lock(&epoch_mutex, K_MSEC(1));

//     epoch_time = new_epoch;

//     k_mutex_unlock(&epoch_mutex);
//     return 0;
// }

uint32_t rtc_get_epoch(void)
{
    return epoch_time;
}

// /* ============================================================================================== */
// /*                                         PRIVATE FUNCTIONS                                      */
// static void rtc_handler(nrfx_rtc_int_type_t int_type)
// {
//     switch (int_type)
//     {
//     case RTC_ALARM_COMPARE:
//         if (alarm_cb)
//             alarm_cb();
//         break;
//     case RTC_EPOCH_SECONDS_COMPARE:
//         rtc_epoch_update(epoch_time + 1);
//         nrfx_rtc_cc_set(&rtc, RTC_EPOCH_SECONDS_COMPARE, nrfx_rtc_counter_get(&rtc) + RTC_SECONDS(1), true);
//     default:
//         break;
//     }
// }