/**
 * @file stm32_hal_mock.h
 * @brief Mock HAL definitions for testing uAT library
 */

#ifndef STM32_HAL_MOCK_H
#define STM32_HAL_MOCK_H

#include <stdint.h>
#include <stddef.h>

// Mock HAL status
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

// Mock UART definitions
typedef struct {
    void* Instance;
    void* Init;
    void* hdmatx;
    void* hdmarx;
    // Add other members as needed for testing
} UART_HandleTypeDef;

// Mock HAL functions (will be implemented in .c file)
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *huart);
uint32_t HAL_GetTick(void);

// Mock interrupt flags and DMA macros
#define UART_FLAG_IDLE    0x10U
#define UART_IT_IDLE      0x10U

#define __HAL_UART_GET_FLAG(huart, flag) (mock_uart_flag_state)
#define __HAL_UART_CLEAR_IDLEFLAG(huart) do { mock_uart_flag_state = 0; } while(0)
#define __HAL_UART_ENABLE_IT(huart, it) do { /* mock */ } while(0)
#define __HAL_DMA_GET_COUNTER(dma) (mock_dma_counter)
#define __HAL_RCC_DMA1_CLK_ENABLE() do { /* mock */ } while(0)

// Mock variables for testing
extern uint32_t mock_uart_flag_state;
extern uint32_t mock_dma_counter;
extern HAL_StatusTypeDef mock_hal_status;

#endif // STM32_HAL_MOCK_H