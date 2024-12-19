#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "ina239.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define MAX_EXPECTED_CURRENT 50.0e-3f



int main(void)
{
    uint16_t current_raw;
    double current;

    int err = ina239_init();
    if (err)
        LOG_ERR("ina239_init returned %d", err);

    LOG_INF("Shunt calibrated successfully");

    while (1) 
    {
        current_raw = ina239_get_value();
        if (current_raw != 0)
        {
            float current_lsb = MAX_EXPECTED_CURRENT / 32768.0f;
            current = (current_raw * current_lsb) * 1000.0;
            printk("Current: %2.4fmA\n", current);
        }
        else
            LOG_ERR("Failed to read current");

        k_sleep(K_MSEC(10));
    }

    return 0;
}
