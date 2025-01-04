/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "bt_api.h"

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define RS_BLE_GATT_DATA_SVC_UUID16             (0xDEAD)
#define RS_DATA_SVC_UUID                        0xFB, 0x34, 0x9B, 0x5F,\
                                                0x80, 0x00, 0x00, 0x80,\
                                                0x00, 0x10, 0x00, 0x00,\
                                                (RS_BLE_GATT_DATA_SVC_UUID16 & 0xFF),\
                                                (RS_BLE_GATT_DATA_SVC_UUID16 >> 8), 0x00, 0x00

#define RS_BLE_GATT_DATA_CHRC_UUID16            (0xBEEF)
#define RS_DATA_CHRC_UUID                       0xFB, 0x34, 0x9B, 0x5F,\
                                                0x80, 0x00, 0x00, 0x80,\
                                                0x00, 0x10, 0x00, 0x00,\
                                                (RS_BLE_GATT_DATA_CHRC_UUID16 & (0xFF)),\
                                                (RS_BLE_GATT_DATA_CHRC_UUID16 >> 8), 0x00, 0x00

#define BT_UUID_RS_SVC                          BT_UUID_DECLARE_128(RS_DATA_SVC_UUID)
#define BT_UUID_RS_DATA                         BT_UUID_DECLARE_128(RS_DATA_CHRC_UUID)

/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);

static ssize_t data_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                         uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t data_tx(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(RS_BLE_GATT_DATA_SVC_UUID16))
};

BT_GATT_SERVICE_DEFINE(bt_service, 
    BT_GATT_PRIMARY_SERVICE(BT_UUID_RS_SVC),
        BT_GATT_CHARACTERISTIC( BT_UUID_RS_DATA,
                                BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_READ,
                                   BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
                                   data_tx, data_rx, NULL)
);

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected
};

static epoch_set_cb_t  epoch_set_cb = NULL;
static epoch_get_cb_t  epoch_get_cb = NULL;
LOG_MODULE_REGISTER(bt_api, LOG_LEVEL_DBG);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */

int bt_init(epoch_set_cb_t set_cb, epoch_get_cb_t get_cb)
{
    int err;

    if (!set_cb || !get_cb)
        return -EINVAL;
    
    epoch_set_cb = set_cb;
    epoch_get_cb = get_cb;

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_DBG("Bluetooth initialized");
    return 0;
}

int bt_adv_start(void)
{
    int err;

    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME),
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    };

    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Failed to start advertising (err %d)", err);
        return err;
    }

    LOG_DBG("Advertising started successfully");
    return 0;
}

/* ============================================================================================== */
/*                                        PRIVATE FUNCTIONS                                       */
static void connected(struct bt_conn *conn, uint8_t err)
{
    LOG_DBG("Connected");
    bt_le_adv_stop();
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_DBG("Disconnected (reason 0x%02x)\n", reason);
}

static ssize_t data_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                         uint16_t len, uint16_t offset, uint8_t flags)
{
    if (len != sizeof(uint32_t))
        return 0;
    uint8_t *data = (uint8_t *)buf;
    uint32_t new_epoch = 0U;

    new_epoch = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
    if (epoch_set_cb)
        epoch_set_cb(new_epoch);
    return 0;
}

static ssize_t data_tx(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset)
{
    uint32_t current_epoch = 0;
    uint8_t *data = (uint8_t *)&current_epoch;
    static uint32_t current_epoch_inverted = 0;
    if (epoch_get_cb)
        current_epoch = epoch_get_cb();
    
    current_epoch_inverted = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
    return bt_gatt_attr_read(conn, attr, (void *)buf, (uint16_t)len, offset, &current_epoch_inverted, sizeof(current_epoch_inverted));
}
