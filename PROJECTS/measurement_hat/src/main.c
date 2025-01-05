/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "ina239.h"
#include "sd_card.h"
#include "bt_api.h"
#include "rtc_api.h"
#include "uart_api.h"
#include <dk_buttons_and_leds.h>
/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define ALARM_PERIOD_SECONDS 300
#define MAX_MEASUREMENT_TIME 120 // must be shorter than ALARM_PERIOD_SECONDS

/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
static void button_changed_callback(uint32_t button_state, uint32_t has_changed);
static void button_hold_handle(struct k_work *work);
static void measurement_timeout_handle(struct k_work *work);
static int app_rtc_epoch_update(uint32_t epoch);
static void rtc_cb_triggered(void);
static void read_set_off(void);
static void read_set_on(void);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */
static ATOMIC_DEFINE(reading, 1);
static struct k_work_delayable button_hold_work;
static uint32_t start_timestamp = 0;
static uint32_t stop_timestamp = 0;
static K_WORK_DELAYABLE_DEFINE(measurement_timeout, measurement_timeout_handle);
static bool timeout = false;

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */


int main(void)
{
    int err;
    uint16_t current_raw;
    uint8_t data_buffer[256] = {0};
 
    err = dk_buttons_init(button_changed_callback);
    if (err)
    {
        LOG_ERR("Failed to initialize buttons (err %d)", err);
        return err;
    }

    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("Failed to initialize LEDs (err %d)", err);
        return err;
    }

    err = rtc_init();
    if (err)
    {
        LOG_ERR("Failed to initialize RTC (err %d)", err);
        return err;
    }

    err = uart_init();
    if (err)
    {
        LOG_ERR("Failed to initialize UART (err %d)", err);
        return err;
    }

    rtc_alarm_cb_register(rtc_cb_triggered);


    k_work_init_delayable(&button_hold_work, button_hold_handle);

    err = bt_init(app_rtc_epoch_update, rtc_get_epoch);
    if (err)
        LOG_ERR("bt_init error %d", err);

    err = sd_card_init();
    if (!err)
        LOG_INF("Init success!");

    err = ina239_init();
    if (err)
        LOG_ERR("ina239_init returned %d", err);

    LOG_INF("Shunt calibrated successfully");

    while (1) 
    {
        while (uart_wait_for_msg(data_buffer, sizeof(data_buffer), K_NO_WAIT) && atomic_get(reading))
        {
            current_raw = ina239_get_value();
            if (current_raw)
                sd_card_write(&current_raw, sizeof(current_raw));
            k_yield();
        }
        if (strlen(data_buffer) || timeout)
        {
            if (timeout)
                snprintk(data_buffer, sizeof(data_buffer), "FAIL");

            LOG_DBG("Got msg: %s", data_buffer);
            sd_card_direct_write(data_buffer, strlen(data_buffer));
            
            read_set_off();
            stop_timestamp = rtc_get_epoch();

            k_work_cancel_delayable(&measurement_timeout);
            rtc_set_alarm_for(ALARM_PERIOD_SECONDS - (stop_timestamp - start_timestamp));
            memset(data_buffer, 0 , sizeof(data_buffer));
            timeout = false;
        }
        k_msleep(1000);
    }

    return 0;
}

/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
static void measurement_timeout_handle(struct k_work *work)
{
    if (atomic_get(reading))
    {
        timeout = true;
        read_set_off();
        uart_print("STOP\n", sizeof("STOP"));
        return;
    }
    else
    {
        LOG_INF("RTC ALARM TRIGGERED: %u", start_timestamp);
        uart_print("START\n", sizeof("START"));
        read_set_on();
        k_work_schedule(work, K_SECONDS(MAX_MEASUREMENT_TIME));
    }
}

static int app_rtc_epoch_update(uint32_t epoch)
{
    rtc_set_alarm_for(5); // Set for sync
    return rtc_epoch_update(epoch);
}

static void read_set_off(void)
{
    ina239_power(false);
    atomic_set(reading, false);
    dk_set_led(DK_LED1, false);
    LOG_DBG("New reading state: stopped");
}

static void read_set_on(void)
{
    ina239_power(true);
    atomic_set(reading, true);
    dk_set_led(DK_LED1, true);
    LOG_DBG("New reading state: reading");
}

static void button_changed_callback(uint32_t button_state, uint32_t has_changed)
{
    if ((has_changed & DK_BTN1_MSK) && (button_state & DK_BTN1_MSK))
    {
        if (atomic_get(reading))
            read_set_off();
        else
            read_set_on();
    }
    else if ((has_changed & DK_BTN2_MSK))
    {
        if ((button_state & DK_BTN2_MSK))
            k_work_schedule(&button_hold_work, K_SECONDS(2));
        else
            k_work_cancel_delayable(&button_hold_work);
    }
    else if ((has_changed & DK_BTN3_MSK))
    {
        if ((button_state & DK_BTN3_MSK))
        {
            LOG_DBG("RTC SET %d", rtc_get_epoch());
            rtc_set_alarm_for(10); 
        }
    }
    else if ((has_changed & DK_BTN4_MSK))
    {
        if ((button_state & DK_BTN4_MSK))
            k_work_schedule(&button_hold_work, K_SECONDS(5));
        else
            k_work_cancel_delayable(&button_hold_work);
    }
}

static void button_hold_handle(struct k_work *work)
{
    uint32_t buttons = dk_get_buttons();

    if ((buttons & DK_BTN4_MSK))
    {
        dk_set_led_on(DK_LED4);
        sd_card_erase();
        dk_set_led_off(DK_LED4);
        NVIC_SystemReset();
    }
    else if ((buttons & DK_BTN2_MSK))
        bt_adv_start();
}

static void rtc_cb_triggered(void)
{
    start_timestamp = rtc_get_epoch();
    uint32_t remainder = start_timestamp % ALARM_PERIOD_SECONDS;
    if (0 == remainder)
    {
        k_work_schedule(&measurement_timeout, K_NO_WAIT);
        return;
    }
    // Autosync mechanism
    LOG_DBG("Reset RTC for %u",ALARM_PERIOD_SECONDS - remainder);
    rtc_set_alarm_for(ALARM_PERIOD_SECONDS - remainder);

}
