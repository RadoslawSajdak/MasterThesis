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
#include <dk_buttons_and_leds.h>
/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */

/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
static void button_changed_callback(uint32_t button_state, uint32_t has_changed);
static void button_hold_handle(struct k_work *work);
static void rtc_cb_triggered(void);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */
static ATOMIC_DEFINE(start_read, 1);
static struct k_work_delayable button_hold_work;

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */


int main(void)
{
    int err;
    uint16_t current_raw;
 
    err = dk_buttons_init(button_changed_callback);
    if (err)
    {
        LOG_ERR("Failed to initialize buttons (err %d)\n", err);
        return err;
    }

    err = dk_leds_init();
    if (err)
    {
        LOG_ERR("Failed to initialize LEDs (err %d)\n", err);
        return err;
    }

    err = rtc_init();
    if (err)
    {
        LOG_ERR("Failed to initialize RTC (err %d)\n", err);
        return err;
    }

    rtc_alarm_cb_register(rtc_cb_triggered);


    k_work_init_delayable(&button_hold_work, button_hold_handle);

    err = bt_init(rtc_epoch_update, rtc_get_epoch);
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
        while (atomic_get(start_read))
        {
            current_raw = ina239_get_value();
            if (current_raw)
                sd_card_write(&current_raw, sizeof(current_raw));
            k_yield();
        }
        k_yield();
    }

    return 0;
}

/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */

static void button_changed_callback(uint32_t button_state, uint32_t has_changed)
{
    if ((has_changed & DK_BTN1_MSK) && (button_state & DK_BTN1_MSK))
    {
        atomic_set(start_read, !atomic_get(start_read));
        dk_set_led(DK_LED1, atomic_get(start_read));
        LOG_DBG("Button\n");
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
            rtc_set_alarm_for(11); 
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
    LOG_INF("RTC ALARM TRIGGERED: %d", rtc_get_epoch());
    //TODO Add driving pin
}