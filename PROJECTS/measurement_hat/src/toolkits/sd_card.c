/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/ring_buffer.h>

/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
#define DISK_DRIVE_NAME                 "SD"
#define DISK_MOUNT_PT                   "/"DISK_DRIVE_NAME":"
#define BLOCK_SIZE                      512
#define EXPECTED_DATA_SIZE              0x8000
#define BLOCKS_PER_DATA                 EXPECTED_DATA_SIZE / BLOCK_SIZE
#define STACK_SIZE                      0x2000
#define THREAD_PRIORITY                 8
#define SD_SETUP_SECTOR                 0x00

struct sd_data
{
    uint32_t timestamp;
    uint8_t data[EXPECTED_DATA_SIZE - sizeof(uint32_t)];
}__packed;

struct sd_init
{
    uint32_t current_sector;
    uint8_t unused[BLOCK_SIZE - sizeof(uint32_t)];
    // TODO: Any data? maybe run config? 
}__packed;

/* ============================================================================================== */
/*                                   PRIVATE FUNCTION DEFINITIONS                                 */
void sd_upload(void *unused0, void *unused1);

/* ============================================================================================== */
/*                                        PRIVATE VARIABLES                                       */
static struct sd_init sd_init_data = {0};
static struct sd_data data = {0};

K_THREAD_DEFINE(sd_upload_thread, STACK_SIZE, sd_upload, NULL, NULL, NULL, THREAD_PRIORITY, 0, 1000);
K_SEM_DEFINE(sd_upload_sem, 0, 1);
LOG_MODULE_REGISTER(sd_card, LOG_LEVEL_DBG);
RING_BUF_DECLARE(ring_buffer, 2 * EXPECTED_DATA_SIZE);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int sd_card_init(void)
{
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    if (disk_access_init(DISK_DRIVE_NAME) != 0)
    {
        LOG_ERR("Storage init ERROR!");
        return -ENODEV;
    }

    if (disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &block_count))
    {
        LOG_ERR("Unable to get sector count");
        return -EACCES;
    }
    LOG_INF("Block count %u", block_count);

    if (disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &block_size))
    {
        LOG_ERR("Unable to get sector size");
        return -EACCES;
    }
    LOG_INF("Block size %u", block_size);
    memory_size_mb = (uint64_t)block_count * block_size;
    if (memory_size_mb > 0)
        LOG_INF("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
    else
    {
        LOG_ERR("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
        return -EINVAL;
    }

    return 0;
}

int sd_card_write(const uint8_t *data, uint8_t len)
{
    static int sum = 0;
    // There's an edgecase with overflow but the code is designed to prevent overflows. I allowed it
    // to speed up the code.
    sum += ring_buf_put(&ring_buffer, data, len);
    if (sum >= EXPECTED_DATA_SIZE)
    {
        k_sem_give(&sd_upload_sem);
        sum = 0;
    }
    
    return 0;
}

int sd_card_erase(void)
{
    int err = 0;
    int blocks_to_erase = 0;
    k_sem_take(&sd_upload_sem, K_MSEC(200)); // just for sync

    blocks_to_erase = sd_init_data.current_sector + 1;
    LOG_WRN("Trying to erase %d sectors\n", blocks_to_erase);
    memset(&data, 0, sizeof(struct sd_init));

    for (int current = 0; current < blocks_to_erase; current += BLOCKS_PER_DATA)
    {
        err = disk_access_write(DISK_DRIVE_NAME, (uint8_t *)&data, current, BLOCKS_PER_DATA);
        if (err)
            LOG_ERR("Failed to write to SD card");
    }
    LOG_WRN("Erased!\n");
    return 0;
}
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
void sd_upload(void *unused0, void *unused1)
{
    int err;
    err = disk_access_read(DISK_DRIVE_NAME, (uint8_t *)&sd_init_data, SD_SETUP_SECTOR, 1);
    if (err)
        LOG_ERR("Failed to write to SD card");
    else
        LOG_INF("Starting at %d", sd_init_data.current_sector);

    while(1)
    {
        if (0 == k_sem_take(&sd_upload_sem, K_FOREVER))
        {
            ring_buf_get(&ring_buffer, data.data, EXPECTED_DATA_SIZE - sizeof(uint32_t));
            data.timestamp++;
            err = disk_access_write(DISK_DRIVE_NAME, (uint8_t *)&sd_init_data, SD_SETUP_SECTOR, 1);
            if (err)
                LOG_ERR("Failed to write sector to SD card %d", err);

            sd_init_data.current_sector += BLOCKS_PER_DATA;
            err = disk_access_write(DISK_DRIVE_NAME, (uint8_t *)&data, sd_init_data.current_sector, BLOCKS_PER_DATA);
            if (err)
                LOG_ERR("Failed to write data to SD card %d", err);
            
        }
    }
}