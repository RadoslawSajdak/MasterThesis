/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <nrfx_rtc.h>
#include <hal/nrf_rtc.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>

#include "rtc_api.h"
#include "app_uart.h"

/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define GNSS_TIMEOUT                        K_SECONDS(120)
#define RTC_SECONDS(s)                      (s * 8)
/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */

static void print_gnss_data(const struct nrf_modem_gnss_pvt_data_frame *pvt);
static void gnss_cold_start(struct k_work *work);
/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */

static struct k_work_delayable gnss_work;

K_MSGQ_DEFINE(pvt_data_q, sizeof(struct nrf_modem_gnss_pvt_data_frame), 5, 4);

LOG_MODULE_REGISTER(gnss, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */

static void print_gnss_data(const struct nrf_modem_gnss_pvt_data_frame *pvt)
{
    LOG_INF("Latitude:  %.6f", pvt->latitude);
    LOG_INF("Longitude: %.6f", pvt->longitude);
    LOG_INF("Accuracy:  %d meters", pvt->accuracy);
}

static void gnss_event_handler(int event)
{
    struct nrf_modem_gnss_pvt_data_frame pvt_data;
    int err;
    switch (event)
    {
        case NRF_MODEM_GNSS_EVT_FIX:
            LOG_INF("GNSS fix acquired!");
            if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) == 0)
            {
                k_msgq_put(&pvt_data_q, &pvt_data, K_NO_WAIT);
            }
            k_work_cancel_delayable(&gnss_work);
            break;
    }
}
void rtc_cb_triggered(void)
{
    LOG_INF("RTC ALARM TRIGGERED");
    app_uart_write("Wakeup!\n", sizeof("Wakeup!\n"));
}
void main(void)
{
    int err;
    struct nrf_modem_gnss_pvt_data_frame pvt_data;



    LOG_INF("Initializing GNSS Example in Zephyr");
    err = nrf_modem_lib_init();
    if (err)
    {
        LOG_ERR("Modem library initialization failed, error: %d", err);
        return err;
    }

    if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0)
    {
        LOG_ERR("Failed to activate GNSS functional mode");
        return -1;
    }

    // Set GNSS use case
    nrf_modem_gnss_use_case_set(NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START);
    nrf_modem_gnss_event_handler_set(gnss_event_handler);
    uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_RMC_MASK |
                 NRF_MODEM_GNSS_NMEA_GGA_MASK |
                 NRF_MODEM_GNSS_NMEA_GLL_MASK |
                 NRF_MODEM_GNSS_NMEA_GSA_MASK |
                 NRF_MODEM_GNSS_NMEA_GSV_MASK;

    if (nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0) {
        LOG_ERR("Failed to set GNSS NMEA mask");
        return -1;
    }
    /* Make QZSS satellites visible in the NMEA output. */
    if (nrf_modem_gnss_qzss_nmea_mode_set(NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM) != 0) {
        LOG_WRN("Failed to enable custom QZSS NMEA mode");
    }

    if (nrf_modem_gnss_power_mode_set(NRF_MODEM_GNSS_PSM_DISABLED) != 0) {
        LOG_ERR("Failed to set GNSS power saving mode");
    }

    if (nrf_modem_gnss_start() != 0) {
        LOG_ERR("Failed to start GNSS");
        return -1;
    }
    // Schedule tasks for GNSS acquisition
    k_work_init_delayable(&gnss_work, gnss_cold_start);

    // Start Cold Start GNSS
    k_work_schedule(&gnss_work, K_NO_WAIT);

    rtc_init();
    rtc_alarm_cb_register(rtc_cb_triggered);
    rtc_set_alarm_for(7);

    while(1)
    {
        if (0 == k_msgq_get(&pvt_data_q, &pvt_data, K_FOREVER));

        print_gnss_data(&pvt_data);
    }
}

/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */

static void gnss_cold_start(struct k_work *work)
{
    LOG_INF("Starting GNSS cold start...");
    nrf_modem_gnss_fix_retry_set(120);
    nrf_modem_gnss_fix_interval_set(0);
    nrf_modem_gnss_start();
    k_work_schedule(&gnss_work, GNSS_TIMEOUT);
}