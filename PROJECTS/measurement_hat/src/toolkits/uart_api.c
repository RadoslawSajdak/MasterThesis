// /* ============================================================================================== */
// /*                                            INCLUDES                                            */
// #include <zephyr/kernel.h>
// #include <zephyr/logging/log.h>
// #include <zephyr/drivers/uart.h>
// #include <string.h>


// /* ============================================================================================== */

// /* ============================================================================================== */
// /*                                        DEFINES/TYPEDEFS                                        */
// #define UART_BUFFER_SIZE 256

// /* ============================================================================================== */
// /*                                   PRIVATE FUNCTION DEFINITIONS                                 */
// static void uart_irq_handler(const struct device *dev, void *user_data);

// /* ============================================================================================== */
// /*                                        PRIVATE VARIABLES                                       */
// const struct device *uart_dev;
// static uint8_t uart_rx_buf[UART_BUFFER_SIZE];
// static bool uart_enabled = false;
// K_MSGQ_DEFINE(uart_rx_q, sizeof(uart_rx_buf), 5, 2);

// LOG_MODULE_REGISTER(UART, LOG_LEVEL_INF);
// /* ============================================================================================== */
// /*                                        PUBLIC FUNCTIONS                                        */


// void uart_enable(void)
// {
//     if (!uart_enabled) {
//         uart_rx_enable(uart_dev, uart_rx_buf, sizeof(uart_rx_buf), SYS_FOREVER_MS);
//         uart_enabled = true;
//         LOG_INF("UART enabled.");
//     }
// }

// void uart_disable(void)
// {
//     if (uart_enabled) {
//         uart_rx_disable(uart_dev);
//         uart_enabled = false;
//         LOG_INF("UART disabled for low power.");
//     }
// }

// int uart_init(void)
// {
//     uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
//     if (!device_is_ready(uart_dev)) {
//         LOG_ERR("UART device not ready.");
//         return -ENODEV;
//     }

//     uart_irq_callback_set(uart_dev, uart_irq_handler);
//     uart_irq_rx_enable(uart_dev);

//     uart_enable();
//     LOG_INF("UART initialized successfully.");
//     return 0;
// }

// void uart_print(uint8_t *buf, uint16_t len)
// {
//     for (int i = 0; i < len; i++)
//     {
//         uart_poll_out(uart_dev, buf[i]);
//     }
// }

// int uart_wait_for_msg(uint8_t *buffer, uint16_t max_len, k_timeout_t timeout)
// {
//     if (!buffer)
//         return -EINVAL;
//     return k_msgq_get(&uart_rx_q, buffer, timeout);
// }
// /* ============================================================================================== */
// /*                                         PRIVATE FUNCTIONS                                      */
// static void uart_irq_handler(const struct device *dev, void *user_data)
// {
//     static size_t pos = 0;
//     uint8_t byte;

//     while (uart_irq_update(dev) && uart_irq_is_pending(dev))
//     {
//         if (uart_irq_rx_ready(dev))
//         {
//             uart_fifo_read(dev, &byte, 1);
//             uart_rx_buf[pos++] = byte;

//             if (byte == '\n' || pos >= UART_BUFFER_SIZE - 1) {
//                 uart_rx_buf[pos] = '\0';
//                 LOG_DBG("Got msg");
//                 k_msgq_put(&uart_rx_q, uart_rx_buf, K_NO_WAIT);
//                 pos = 0;
//             }
//         }
//     }
// }