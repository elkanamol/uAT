# uAT - UART AT Command Framework for STM32

[![Tests](https://github.com/elkanamol/uAT/actions/workflows/tests.yml/badge.svg)](https://github.com/elkanamol/uAT/actions/workflows/tests.yml)

## Description
uAT is a lightweight, FreeRTOS-friendly AT command parser for STM32 microcontrollers. It provides a robust interface for communicating with modems or other devices that use AT command sets. The implementation supports both DMA and interrupt-driven UART communication, offering efficient handling of asynchronous command responses with minimal CPU overhead.

The framework is designed to work seamlessly with FreeRTOS, utilizing stream buffers for receiving data and semaphores for thread synchronization. It provides a simple callback-based API for handling AT command responses, making it easy to integrate into embedded applications.

## Features
- FreeRTOS-compatible AT command parser
- Support for both DMA and interrupt-driven UART communication
- Asynchronous command handling with callbacks
- Synchronous command-response functionality with timeouts
- Thread-safe implementation using FreeRTOS primitives
- Configurable buffer sizes and command handler capacity
- Efficient line-based parsing with delimiter detection
- Minimal CPU overhead using DMA for data reception
- Support for command registration and unregistration at runtime
- Standardized error handling with detailed error codes
- Priority-based handling of Unsolicited Result Codes (URCs)

## Getting Started

### Prerequisites
- STM32F7 series microcontroller (tested on Nucleo-F756ZG)
- STM32CubeMX or STM32CubeIDE for project generation
- GCC ARM toolchain (arm-none-eabi-gcc)
- FreeRTOS 10.x or later
- STM32 HAL Driver

### Installation

#### Adding to an STM32 Project
1. Copy the following files to your project:
   - `Core/Inc/uat_freertos.h`
   - `Core/Src/uat_freertos.c`
   - `Core/Inc/uat_parser.h` (optional, for response parsing)
   - `Core/Src/uat_parser.c` (optional, for response parsing)

2. Add the files to your build system:

   **For CMake projects:**
   Add the following to your CMakeLists.txt:
   ```cmake
   target_sources(${PROJECT_NAME} PRIVATE
     Core/Src/uat_freertos.c
     Core/Src/uat_parser.c
   )
   
   target_include_directories(${PROJECT_NAME} PRIVATE
     Core/Inc
   )
   ```

   **For Makefile projects:**
   Add the source files to your C_SOURCES variable and include directories to your C_INCLUDES variable.

3. Configure your UART peripheral with DMA (recommended) or interrupt mode using STM32CubeMX.

4. Implement the UART IDLE line detection in your UART IRQ handler (for DMA mode):
   ```c
   void USART2_IRQHandler(void)
   {
     if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE))
     {
       __HAL_UART_CLEAR_IDLEFLAG(&huart2);
       uAT_UART_IdleHandler();
     }
     HAL_UART_IRQHandler(&huart2);
   }
   ```

## Usage

### Initializing the AT Command Parser

```c
#include "uat_freertos.h"

// In your main.c or initialization code:
extern UART_HandleTypeDef huart2;  // Your UART handle

// Initialize the parser
uAT_Result_t result = uAT_Init(&huart2);
if (result != UAT_OK) {
   // Handle initialization error
   printf("uAT parser initialization failed with error code: %d\n", result);
   Error_Handler();
}

// Create the parser task
xTaskCreate(uAT_Task,
           "uAT_Task",
           512,  // Stack depth in words
           NULL,
           tskIDLE_PRIORITY + 1,
           NULL);
```

### Registering Command Handlers

```c
// Define a handler function for network registration notifications
void creg_handler(const char *args) {
   printf("[%lu] >>> Network registration URC: %s", HAL_GetTick(), args);
}

// Define a handler for OK responses
void ok_handler(const char *args) {
   printf("[%lu] >>> Got OK response%s", HAL_GetTick(), args);
}

// Register the handlers
uAT_Result_t result;
result = uAT_RegisterCommand("+CREG", creg_handler);
if (result != UAT_OK) {
   printf("Failed to register +CREG handler: %d\n", result);
}

result = uAT_RegisterCommand("OK", ok_handler);
if (result != UAT_OK) {
   printf("Failed to register OK handler: %d\n", result);
}
```

### Sending AT Commands

```c
// Send a simple AT command (asynchronously)
uAT_Result_t result = uAT_SendCommand("AT");
if (result != UAT_OK) {
   printf("Failed to send AT command: %d\n", result);
}

// Send a query command
result = uAT_SendCommand("AT+CREG?");
if (result != UAT_OK) {
   printf("Failed to send AT+CREG? command: %d\n", result);
}
```

### Synchronous Command-Response

```c
char response_buffer[1024];
uAT_Result_t result;

// Send command and wait for "OK" response with 1 second timeout
result = uAT_SendReceive("ATI", "OK", response_buffer, sizeof(response_buffer), pdMS_TO_TICKS(1000));

if (result == UAT_OK) {
   printf("Command successful, response:\n%s\n", response_buffer);
} else {
   printf("Command failed with error code: %d\n", result);
}
```

### Example Application with Sierra Wireless RC7120

Here's an example of using the uAT framework with a Sierra Wireless RC7120 modem:

```c
RC76XX_Result_t RC76XX_Initialize(RC76XX_Handle_t *h)
{
    if (!h || h->state != RC76XX_STATE_RESET)
    {
        return RC76XX_ERR_STATE;
    }

    char resp[256];

    // 1) Basic AT check
    if (uAT_SendReceive("AT", "OK", resp, sizeof(resp), CMD_TIMEOUT) != UAT_OK)
    {
        return RC76XX_ERR_AT;
    }

    // 2) Get IMEI
    if (uAT_SendReceive("ATI", "OK", resp, sizeof(resp), CMD_TIMEOUT) != UAT_OK)
    {
        return RC76XX_ERR_AT;
    }
    // parse full line into h->imei
    if (!uAT_ParseString(resp, "IMEI: ", h->imei, sizeof(h->imei)))
    {
        return RC76XX_ERR_PARSE;
    }

    // parse model from resp to h->model
    if (!uAT_ParseString(resp, "Model: ", h->model, sizeof(h->model)))
    {
        return RC76XX_ERR_PARSE;
    }

    // 3) Disable echo
    if (uAT_SendReceive("ATE0", "OK", resp, sizeof(resp), CMD_TIMEOUT) != UAT_OK)
    {
        return RC76XX_ERR_AT;
    }

    // State transition: RESET → INITIALIZED
    h->state = RC76XX_STATE_INITIALIZED;
    return RC76XX_OK;
}
```

## Development Environment

- **Operating System**: Cross-platform (Windows, Linux, macOS)
- **Programming Language**: C (C11)
- **Frameworks/Libraries**:
  - FreeRTOS 10.x
  - STM32 HAL Driver
  - CMSIS
- **Hardware**:
  - STM32F756ZG Nucleo board
  - Any STM32F7 series microcontroller with UART and DMA support
  - Tested with Sierra Wireless RC7120 modem
- **Development Tools**:
  - STM32CubeIDE or Visual Studio Code with STM32 extension
  - GCC ARM Embedded Toolchain
  - STM32CubeMX for initial project generation
  - ST-Link utility for flashing

## Workspace Setup (For Developers)

1. Ensure you have the ARM GCC toolchain installed and in your PATH.

2. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/uAT.git
   cd uAT
   ```

3. If using Visual Studio Code, install the following extensions:
   - C/C++ Extension Pack
   - Cortex-Debug
   - CMake Tools
   - STM32 extension

4. Configure your project with CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

5. Build the project:
   ```bash
   cmake --build .
   ```

6. Flash to your device using ST-Link utility or directly from your IDE.

## Contributing

Contributions to improve the AT command parser are welcome. Please feel free to submit a Pull Request.

## License

MIT License

Copyright (c) 2023 Elkana Molson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
