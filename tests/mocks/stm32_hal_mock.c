/**
 * @file stm32_hal_mock.c
 * @brief Mock HAL implementation for testing
 */

#include "stm32_hal_mock.h"

// Mock variables
uint32_t mock_uart_flag_state = 0;
uint32_t mock_dma_counter = 0;
HAL_StatusTypeDef mock_hal_status = HAL_OK;
static uint32_t mock_tick = 0;

HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    (void)huart;
    (void)pData;
    (void)Size;
    return mock_hal_status;
}

HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    (void)huart;
    (void)pData;
    (void)Size;
    return mock_hal_status;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    (void)huart;
    (void)pData;
    (void)Size;
    return mock_hal_status;
}

uint32_t HAL_GetTick(void)
{
    return mock_tick++;
}