#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ina239_spi, LOG_LEVEL_INF);

#define SPI_NODE DT_NODELABEL(spi1)
#define INA239_CS_PIN 4
#define INA239_CS_PORT DT_NODELABEL(gpio1)

/* INA239 Register Addresses */
#define INA239_REG_CONFIG 0x00
#define INA239_REG_ADC_CONFIG 0x01
#define INA239_REG_VOLTAGE 0x04
#define INA239_REG_SHUNT_CAL 0x02
#define INA239_REG_CURRENT 0x07
#define INA239_REG_WHOAMI 0x3F
#define INA239_READ_COMMAND 0x01 /* Command to read */
#define INA239_WRITE_COMMAND 0x00 /* Command to write */

/* Shunt resistor value in ohms */
#define SHUNT_RESISTOR 1.0f
#define SHUNT_CAL_VALUE (1250 * 4)//multiplied by 4 because of inceased range
#define ADC_MODE_TEMP_AND_SHUNT 0x0e << 12
/* Maximum expected current in amperes */
#define MAX_EXPECTED_CURRENT 50.0e-3f

static const struct device *spi_dev;
static struct spi_cs_control cs_ctrl;
static struct spi_config spi_cfg;

static int ina239_read_register(uint8_t reg, uint16_t *value)
{
    uint8_t tx_buf[1] = {(reg & 0x3F) << 2 | 0x01}; // 6-bit address + Read bit (1)
    uint8_t rx_buf[2] = {0};

    const struct spi_buf tx_spi_buf = {
        .buf = tx_buf,
        .len = sizeof(tx_buf),
    };
    const struct spi_buf rx_spi_buf[] ={
    {
        .buf = NULL,
        .len = sizeof(tx_buf)
    },
    {
        .buf = rx_buf,
        .len = sizeof(rx_buf),
    }};
    const struct spi_buf_set tx_bufs = {
        .buffers = &tx_spi_buf,
        .count = 1,
    };
    const struct spi_buf_set rx_bufs = {
        .buffers = rx_spi_buf,
        .count = 2,
    };

    // Transmit and receive over SPI
    int ret = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
    if (ret < 0) {
        LOG_ERR("Failed to read register 0x%02x: %d", reg, ret);
        return ret;
    }

    *value = (rx_buf[0] << 8) | rx_buf[1]; // Combine MSB and LSB
    return 0;
}



static int ina239_write_register(uint8_t reg, uint16_t value)
{
    uint8_t tx_buf[3] = {(reg & 0x3F) << 2, (uint8_t)(value >> 8), (uint8_t)value}; // 6-bit address + Write bit (0)

    const struct spi_buf tx_spi_buf = {
        .buf = tx_buf,
        .len = sizeof(tx_buf),
    };
    const struct spi_buf_set tx_bufs = {
        .buffers = &tx_spi_buf,
        .count = 1,
    };

    int ret = spi_write(spi_dev, &spi_cfg, &tx_bufs);
    if (ret < 0) {
        LOG_ERR("Failed to write register 0x%02x: %d", reg, ret);
        return ret;
    }

    return 0;
}


void main(void)
{
    int ret;
    uint16_t whoami;
    uint16_t voltage;
    uint16_t current_raw;
    double current;

    LOG_INF("Starting INA239 SPI example");

    /* Get SPI device */
    spi_dev = DEVICE_DT_GET(SPI_NODE);
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI device not ready");
        return;
    }

    /* Get GPIO device for chip select */
    const struct device *cs_gpio_dev = DEVICE_DT_GET(INA239_CS_PORT);
    if (!device_is_ready(cs_gpio_dev)) {
        LOG_ERR("GPIO device for CS not ready");
        return;
    }

    /* Configure chip select control */
    cs_ctrl.gpio.port = cs_gpio_dev; /* gpio.port to wskaznik na urzadzenie GPIO */
    cs_ctrl.gpio.pin = INA239_CS_PIN;
    cs_ctrl.gpio.dt_flags = GPIO_ACTIVE_LOW;
    cs_ctrl.delay = 100;

    /* Configure SPI settings */
    spi_cfg.frequency = 8000000; /* 1 MHz */
    spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA;

    spi_cfg.slave = 0;
    spi_cfg.cs = cs_ctrl;

    ret = ina239_write_register(INA239_REG_CONFIG, 1 << 15);
    if (ret != 0) {
        LOG_ERR("Failed to updatae config");
        return;
    }
    k_msleep(500);

    /* Read WHOAMI register */
    ret = ina239_read_register(INA239_REG_WHOAMI, &whoami);
    if (ret == 0) {
        LOG_INF("WHOAMI register: 0x%04x", whoami);
    } else {
        LOG_ERR("Failed to read WHOAMI register");
        return;
    }

    ret = ina239_write_register(INA239_REG_CONFIG, (1 << 4));
    if (ret != 0) {
        LOG_ERR("Failed to updatae config");
        return;
    }

    ret = ina239_read_register(INA239_REG_CONFIG, &whoami);
    if (ret == 0) {
        LOG_INF("INA239_REG_CONFIG register: 0x%04x", whoami);
    } else {
        LOG_ERR("Failed to read INA239_REG_CONFIG register");
        return;
    }

    ret = ina239_write_register(INA239_REG_ADC_CONFIG, ADC_MODE_TEMP_AND_SHUNT | 0x3);
    if (ret != 0) {
        LOG_ERR("Failed to updatae config");
        return;
    }

    ret = ina239_read_register(INA239_REG_ADC_CONFIG, &whoami);
    if (ret == 0) {
        LOG_INF("INA239_REG_ADC_CONFIG register: 0x%04x", whoami);
    } else {
        LOG_ERR("Failed to read INA239_REG_ADC_CONFIG register");
        return;
    }

    /* Calibrate shunt */
    ret = ina239_write_register(INA239_REG_SHUNT_CAL, SHUNT_CAL_VALUE);
    if (ret != 0) {
        LOG_ERR("Failed to calibrate shunt");
        return;
    }

    ret = ina239_read_register(INA239_REG_SHUNT_CAL, &whoami);
    if (ret == 0)
        LOG_INF("INA239_REG_SHUNT_CAL register: 0x%04x", whoami);
    else if (ret != 0) {
        LOG_ERR("Failed to updatae config");
        return;
    }

    LOG_INF("Shunt calibrated successfully");

    while (1) {
        /* Read current register */
        ret = ina239_read_register(INA239_REG_CURRENT, &current_raw);
        if (ret == 0) {
            float current_lsb = MAX_EXPECTED_CURRENT / 32768.0f;
            current = (current_raw * current_lsb) * 1000.0;
            printf("Current: %2.4fmA 0x%04x\n", current, current_raw);
        } else {
            LOG_ERR("Failed to read current");
        }

        k_sleep(K_MSEC(10)); /* Wait 1 second */
    }
}
