#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "ina239.h"
#include "sd_card.h"
#include <dk_buttons_and_leds.h>


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);


static ATOMIC_DEFINE(start_read, 1);
static struct k_work_delayable erase_work;

static void button_changed_callback(uint32_t button_state, uint32_t has_changed)
{
    if ((has_changed & DK_BTN1_MSK) && (button_state & DK_BTN1_MSK))
    {
        atomic_set(start_read, !atomic_get(start_read));
        dk_set_led(DK_LED1, atomic_get(start_read));
        printk("Button\n");
    }
    else if ((has_changed & DK_BTN4_MSK))
    {
        if ((button_state & DK_BTN4_MSK))
            k_work_schedule(&erase_work, K_SECONDS(5));
        else
            k_work_cancel_delayable(&erase_work);
    }
}

void erase(struct k_work *work)
{
    uint32_t buttons = dk_get_buttons();

    if ((buttons & DK_BTN4_MSK))
    {
        dk_set_led_on(DK_LED4);
        sd_card_erase();
        dk_set_led_off(DK_LED4);
        NVIC_SystemReset();
    }
}

int main(void)
{
    int err;
 
    err = dk_buttons_init(button_changed_callback);
    if (err)
    {
        printk("Failed to initialize buttons (err %d)\n", err);
        return err;
    }

    err = dk_leds_init();
    if (err) {
        printk("Failed to initialize LEDs (err %d)\n", err);
        return err;
    }

    k_work_init_delayable(&erase_work, erase);

    err = sd_card_init();
    if (!err)
        LOG_INF("Init success!");

    uint16_t current_raw;

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
