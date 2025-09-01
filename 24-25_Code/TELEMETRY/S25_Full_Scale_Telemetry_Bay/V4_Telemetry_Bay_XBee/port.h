/**
 * @file port.h
 * @brief Platform-specific abstraction layer for hardware interfaces.
 *
 * This file defines the platform-specific functions and macros used by the XBee library
 * to interface with hardware. It provides abstraction for serial communication, timing,
 * GPIO control, and other low-level operations required by the XBee modules.
 *
 * @version 1.0
 * @date 2024-08-08
 *
 * @license MIT
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author Felix Galindo
 * @contact felix.galindo@digi.com
 */

#ifndef PORT_H
#define PORT_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdint.h>

    // Enum for UART read status
    typedef enum {
        UART_SUCCESS = 0,
        UART_INIT_FAILED,
        UART_ERROR_TIMEOUT,
        UART_ERROR_OVERRUN,
        UART_ERROR_UNKNOWN
    } uart_status_t;

    int portUartRead(uint8_t* buffer, int length);
    int portUartWrite(const uint8_t* buf, uint16_t len);
    uint32_t portMillis(void);
    void PortFlushRx(void);
    int portUartInit(uint32_t baudrate, void* device);
    void portDelay(uint32_t ms);
    void portDebugPrintf(const char* format, ...);

#if defined(__cplusplus)
}
#endif

#endif // UART_H
