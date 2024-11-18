/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrfx_rtc.h>
#include <hal/nrf_rtc.h>


/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define RTC_SECONDS(s)                      (s * 8)
#define RTC_ALARM_COMPARE                   (NRFX_RTC_INT_COMPARE0)
#define RTC_EPOCH_SECONDS_COMPARE           (NRFX_RTC_INT_COMPARE1)


/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
static void rtc_handler(nrfx_rtc_int_type_t int_type);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */

static const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(0);
static uint32_t epoch_time = 0U;

LOG_MODULE_REGISTER(RTC, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */

int rtc_init(void)
{
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = 4095; // 125ms per tick

    nrfx_rtc_init(&rtc, &config, rtc_handler);

    /* Rejestracja przerwań RTC2 */
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_RTC0), 1, nrfx_isr, nrfx_rtc_0_irq_handler, 0);
    irq_enable(NRFX_IRQ_NUMBER_GET(NRF_RTC0));

    nrfx_rtc_tick_enable(&rtc, true);
    nrfx_rtc_enable(&rtc);

    nrfx_rtc_cc_set(&rtc, RTC_EPOCH_SECONDS_COMPARE, nrfx_rtc_counter_get(&rtc) + RTC_SECONDS(1), true);

    LOG_INF("RTC initialized and started.");
}

int rtc_set_alarm_for(int seconds)
{
    return nrfx_rtc_cc_set(&rtc, RTC_ALARM_COMPARE, nrfx_rtc_counter_get(&rtc) + RTC_SECONDS(seconds), true);
}

int epoch_update(uint32_t new_epoch)
{
    epoch_time = new_epoch;
    return 0;
}


/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
static void rtc_handler(nrfx_rtc_int_type_t int_type)
{
    switch (int_type)
    {
    case RTC_ALARM_COMPARE:
        LOG_DBG("RTC Alarm event triggered!");
        break;
    case RTC_EPOCH_SECONDS_COMPARE:
        epoch_time++;
        nrfx_rtc_cc_set(&rtc, RTC_EPOCH_SECONDS_COMPARE, nrfx_rtc_counter_get(&rtc) + RTC_SECONDS(1), true);
    default:
        break;
    }
}