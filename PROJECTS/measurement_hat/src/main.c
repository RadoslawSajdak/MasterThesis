#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "ina239.h"
#include "sd_card.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    int err;
    err = sd_card_init();
    if (!err)
        LOG_INF("Init success!");

    uint16_t current_raw;
    double current;

    err = ina239_init();
    if (err)
        LOG_ERR("ina239_init returned %d", err);

    LOG_INF("Shunt calibrated successfully");

    while (1) 
    {
        current_raw = ina239_get_value();
        sd_card_write(&current_raw, sizeof(current_raw));
        k_usleep(50);
    }

    return 0;
}
