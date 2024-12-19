/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define INA239_REG_CONFIG                   (0x00)
    #define INA239_REG_CONFIG_RST           (1 << 15)
    #define INA239_REG_CONFIG_CONVDLY(x)    (x << 6)    // Conversion delay. 8bytes 2ms step        
    #define INA239_REG_CONFIG_ADCRANGE      (1 << 4)    // Shunt full scale 
#define INA239_REG_ADC_CONFIG               (0x01)
    #define INA239_ADC_CONFIG_MODE(x)       (x << 12)   // Mode from datasheet
    #define INA239_ADC_CONFIG_VBUSCT(x)     (x << 9)    // VBus convertion time
    #define INA239_ADC_CONFIG_VSHCT(x)      (x << 6)    // Shunt conversion time
    #define INA239_ADC_CONFIG_VTCT(x)       (x << 3)    // Temperature conversion time
    #define INA239_ADC_CONFIG_AVG(x)        (x << 0)    // Averaging count
#define INA239_REG_SHUNT_CAL                (0x02)
    #define INA239_SHUNT_CAL(x)             (x & 0x7fff)  // Masked shunt cal value. Should be multipled by 4 while ADC range is set
#define INA239_REG_VSHUNT                   (0x04)
#define INA239_REG_VBUS                     (0x05)
#define INA239_REG_DIETEMP                  (0x06)
#define INA239_REG_CURRENT                  (0x07)
#define INA239_REG_DEVICE_ID                (0x3F)
    #define INA239_DEVICE_ID_VALUE          (0x2391)


#define INA239_CS_PIN 4
#define INA239_SHUNT_CAL_VALUE 1250
/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */

static int write_register(uint8_t reg, uint16_t value);
static int read_register(uint8_t reg, uint16_t *value);
/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */

static const struct device *spi_dev;
static const struct spi_cs_control cs_ctrl = {
    .delay = 0,
    .gpio.port = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
    .gpio.pin = INA239_CS_PIN,
    .gpio.dt_flags = GPIO_ACTIVE_LOW
};
static const struct spi_config spi_cfg = {
    .frequency = 8000000,
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA,
    .slave = 0,
    .cs = cs_ctrl
};

LOG_MODULE_REGISTER(ina239, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int ina239_init(void)
{
    int err;
    uint16_t whoami = 0;
    spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
    if (!spi_dev)
        return -EINVAL;

    if (!device_is_ready(spi_dev))
    {
        LOG_ERR("SPI device not ready");
        return -ENODEV;
    }

    if (!device_is_ready(cs_ctrl.gpio.port))
    {
        LOG_ERR("GPIO device for CS not ready");
        return -ENODEV;
    }

    err = write_register(INA239_REG_CONFIG, INA239_REG_CONFIG_RST);
    if (err)
    {
        LOG_ERR("write_register returned error %d", err);
        return err;
    }

    err = read_register(INA239_REG_DEVICE_ID, &whoami);
    if (!err)
    {
        LOG_INF("WHOAMI register: 0x%04x", whoami);
        if (whoami == INA239_DEVICE_ID_VALUE)
            LOG_DBG("Init done");
        else
            return -EBUSY;
    }
    else
    {
        LOG_ERR("Failed to read WHOAMI register");
        return -ENODEV;
    }

    err = write_register(INA239_REG_CONFIG, INA239_REG_CONFIG_ADCRANGE);
    if (err)
    {
        LOG_ERR("Failed to update INA239_REG_CONFIG");
        return err;
    }

    err = write_register(INA239_REG_ADC_CONFIG, INA239_ADC_CONFIG_MODE(0x0e));
    if (err) {
        LOG_ERR("Failed to update INA239_REG_ADC_CONFIG");
        return err;
    }

    err = write_register(INA239_REG_SHUNT_CAL, INA239_SHUNT_CAL(INA239_SHUNT_CAL_VALUE * 4));
    if (err) {
        LOG_ERR("Failed to update INA239_REG_SHUNT_CAL");
        return err;
    }

    return 0;
}

uint16_t ina239_get_value(void)
{
    uint16_t val = 0;
    read_register(INA239_REG_CURRENT, &val);
    return val;
}
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
static int write_register(uint8_t reg, uint16_t value)
{
    int err;
    uint8_t tx_buf[3] = {(reg & 0x3F) << 2, (uint8_t)(value >> 8), (uint8_t)value};

    const struct spi_buf tx_spi_buf =
    {
        .buf = tx_buf,
        .len = sizeof(tx_buf),
    };
    const struct spi_buf_set tx_bufs =
    {
        .buffers = &tx_spi_buf,
        .count = 1,
    };

    err = spi_write(spi_dev, &spi_cfg, &tx_bufs);
    if (err)
    {
        LOG_ERR("Failed to write register 0x%02x: %d", reg, err);
        return err;
    }

    return 0;
}

static int read_register(uint8_t reg, uint16_t *value)
{
    int err;
    uint8_t tx_buf[1] = {(reg & 0x3F) << 2 | 0x01};
    uint8_t rx_buf[2] = {0};

    const struct spi_buf tx_spi_buf =
    {
        .buf = tx_buf,
        .len = sizeof(tx_buf),
    };
    const struct spi_buf rx_spi_buf[] =
    {
        {
            .buf = NULL,
            .len = sizeof(tx_buf)
        },
        {
            .buf = rx_buf,
            .len = sizeof(rx_buf),
        }
    };
    const struct spi_buf_set tx_bufs =
    {
        .buffers = &tx_spi_buf,
        .count = 1,
    };
    const struct spi_buf_set rx_bufs =
    {
        .buffers = rx_spi_buf,
        .count = 2,
    };


    err = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
    if (err)
    {
        LOG_ERR("Failed to read register 0x%02x: %d", reg, err);
        return err;
    }

    *value = (rx_buf[0] << 8) | rx_buf[1];
    return 0;
}