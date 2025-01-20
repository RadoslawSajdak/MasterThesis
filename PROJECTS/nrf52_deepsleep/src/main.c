/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <nrfx.h>

int main(void)
{
    printk("Entering SYSTEMOFF mode\n");

    NRF_POWER->SYSTEMOFF = 1;

    while (1) {
        __WFI();
    }
}
