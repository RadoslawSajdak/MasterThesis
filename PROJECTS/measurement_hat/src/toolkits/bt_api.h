#ifndef __BT_API__
#define  __BT_API__
/* ============================================================================================== */
/*                                            INCLUDES                                            */
#include <zephyr/kernel.h>
#include <stdint.h>

/* ============================================================================================== */

/* ============================================================================================== */
/*                                        DEFINES/TYPEDEFS                                        */
typedef int (*epoch_set_cb_t)(uint32_t);
typedef uint32_t (*epoch_get_cb_t)(void);
/* ============================================================================================== */
/*                                        PUBLIC FUNCTIONS                                        */
int bt_init(epoch_set_cb_t set_cb, epoch_get_cb_t get_cb);
int bt_adv_start(void);
/* ============================================================================================== */
/*                                         PRIVATE FUNCTIONS                                      */
#endif