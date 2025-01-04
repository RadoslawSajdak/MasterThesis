#!/bin/bash

west build -b nrf52840dk/nrf52840 -- -DDTC_OVERLAY_FILE="prj.overlay;rtc.overlay"