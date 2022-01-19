/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <pico/stdlib.h>

#include "tusb.h"

#include "picoprobe_config.h"
#include "cdc_uart.h"

void cdc_uart_init(void) {
    gpio_set_function(PICOPROBE_UART0_TX, GPIO_FUNC_UART);
    gpio_set_function(PICOPROBE_UART0_RX, GPIO_FUNC_UART);
    uart_init(PICOPROBE_UART0_INTERFACE, PICOPROBE_UART0_BAUDRATE);

    gpio_set_function(PICOPROBE_UART1_TX, GPIO_FUNC_UART);
    gpio_set_function(PICOPROBE_UART1_RX, GPIO_FUNC_UART);
    uart_init(PICOPROBE_UART1_INTERFACE, PICOPROBE_UART1_BAUDRATE);
}

#define MAX_UART_PKT 64
void cdc_uart_task(void) {
    uint8_t rx0_buf[MAX_UART_PKT];
    uint8_t tx0_buf[MAX_UART_PKT];
    uint8_t rx1_buf[MAX_UART_PKT];
    uint8_t tx1_buf[MAX_UART_PKT];

    // Consume uart fifo regardless even if not connected
    uint rx0_len = 0;
    uint rx1_len = 0;
    while(uart_is_readable(PICOPROBE_UART0_INTERFACE) && (rx0_len < MAX_UART_PKT)) {
        rx0_buf[rx0_len++] = uart_getc(PICOPROBE_UART0_INTERFACE);
    }
    while(uart_is_readable(PICOPROBE_UART1_INTERFACE) && (rx1_len < MAX_UART_PKT)) {
        rx1_buf[rx1_len++] = uart_getc(PICOPROBE_UART1_INTERFACE);
    }

    if (tud_cdc_n_connected(0)) {
        // Do we have anything to display on the host's terminal?
        if (rx0_len) {
            for (uint i = 0; i < rx0_len; i++) {
                tud_cdc_n_write_char(0, rx0_buf[i]);
            }
            tud_cdc_n_write_flush(0);
        }
        if (tud_cdc_n_available(0)) {
            // Is there any data from the host for us to tx
            uint tx0_len = tud_cdc_n_read(0, tx0_buf, sizeof(tx0_buf));
            uart_write_blocking(PICOPROBE_UART0_INTERFACE, tx0_buf, tx0_len);
        }
    }

    if (tud_cdc_n_connected(1)) {
        // Do we have anything to display on the host's terminal?
        if (rx1_len) {
            for (uint i = 0; i < rx1_len; i++) {
                tud_cdc_n_write_char(1, rx1_buf[i]);
            }
            tud_cdc_n_write_flush(1);
        }
        if (tud_cdc_n_available(1)) {
            // Is there any data from the host for us to tx
            uint tx1_len = tud_cdc_n_read(1, tx1_buf, sizeof(tx1_buf));
            uart_write_blocking(PICOPROBE_UART1_INTERFACE, tx1_buf, tx1_len);
        }
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* line_coding) {
    picoprobe_info("CDC %d: new baud rate %d\n", itf, line_coding->bit_rate);
    if (itf == 0) {
        uart_init(PICOPROBE_UART0_INTERFACE, line_coding->bit_rate);
    } else if (itf == 1) {
        uart_init(PICOPROBE_UART1_INTERFACE, line_coding->bit_rate);
    } else {
    }
}
