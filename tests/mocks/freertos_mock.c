/**
 * @file freertos_mock.c
 * @brief Mock FreeRTOS implementation for testing
 */

#include "freertos_mock.h"
#include <stdlib.h>
#include <string.h>

// Mock control variables
bool mock_freertos_init_success = true;
BaseType_t mock_semaphore_take_result = pdTRUE;
BaseType_t mock_semaphore_give_result = pdTRUE;
size_t mock_stream_buffer_receive_bytes = 0;

// Internal mock state
static bool failure_mode = false;

void mock_freertos_reset(void)
{
    mock_freertos_init_success = true;
    mock_semaphore_take_result = pdTRUE;
    mock_semaphore_give_result = pdTRUE;
    mock_stream_buffer_receive_bytes = 0;
    failure_mode = false;
}

void mock_freertos_set_failure_mode(bool enable)
{
    failure_mode = enable;
}

// Stream buffer mock implementations
StreamBufferHandle_t xStreamBufferCreate(size_t xBufferSizeBytes, size_t xTriggerLevelBytes)
{
    (void)xTriggerLevelBytes;
    if (!mock_freertos_init_success || failure_mode) {
        return NULL;
    }
    // Return a non-null pointer to simulate successful creation
    return malloc(xBufferSizeBytes);
}

void vStreamBufferDelete(StreamBufferHandle_t xStreamBuffer)
{
    if (xStreamBuffer) {
        free(xStreamBuffer);
    }
}

size_t xStreamBufferSend(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes, TickType_t xTicksToWait)
{
    (void)xStreamBuffer;
    (void)pvTxData;
    (void)xTicksToWait;
    
    if (failure_mode) {
        return 0;
    }
    return xDataLengthBytes;
}

size_t xStreamBufferSendFromISR(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes, BaseType_t *pxHigherPriorityTaskWoken)
{
    (void)xStreamBuffer;
    (void)pvTxData;
    (void)pxHigherPriorityTaskWoken;
    
    if (failure_mode) {
        return 0;
    }
    return xDataLengthBytes;
}

size_t xStreamBufferReceive(StreamBufferHandle_t xStreamBuffer, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait)
{
    (void)xStreamBuffer;
    (void)xTicksToWait;
    
    if (failure_mode) {
        return 0;
    }
    
    size_t bytes_to_copy = (mock_stream_buffer_receive_bytes < xBufferLengthBytes) ? 
                           mock_stream_buffer_receive_bytes : xBufferLengthBytes;
    
    if (bytes_to_copy > 0 && pvRxData) {
        memset(pvRxData, 'A', bytes_to_copy); // Fill with test data
    }
    
    return bytes_to_copy;
}

// Semaphore mock implementations
SemaphoreHandle_t xSemaphoreCreateBinary(void)
{
    if (!mock_freertos_init_success || failure_mode) {
        return NULL;
    }
    return malloc(sizeof(int)); // Return a non-null pointer
}

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    if (!mock_freertos_init_success || failure_mode) {
        return NULL;
    }
    return malloc(sizeof(int)); // Return a non-null pointer
}

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore)
{
    if (xSemaphore) {
        free(xSemaphore);
    }
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)
{
    (void)xSemaphore;
    (void)xTicksToWait;
    return mock_semaphore_take_result;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    (void)xSemaphore;
    return mock_semaphore_give_result;
}

BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken)
{
    (void)xSemaphore;
    (void)pxHigherPriorityTaskWoken;
    return mock_semaphore_give_result;
}

// Task mock implementations
BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName, uint16_t usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask)
{
    (void)pxTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    
    if (pxCreatedTask) {
        *pxCreatedTask = failure_mode ? NULL : malloc(sizeof(int));
    }
    
    return failure_mode ? pdFALSE : pdTRUE;
}

void vTaskDelay(TickType_t xTicksToDelay)
{
    (void)xTicksToDelay;
    // No-op in test environment
}