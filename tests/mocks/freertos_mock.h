/**
 * @file freertos_mock.h
 * @brief Mock FreeRTOS definitions for testing uAT library
 */

#ifndef FREERTOS_MOCK_H
#define FREERTOS_MOCK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Mock FreeRTOS types
typedef uint32_t TickType_t;
typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef void* TaskHandle_t;
typedef void* SemaphoreHandle_t;
typedef void* StreamBufferHandle_t;
typedef struct {
    uint32_t dummy;
} TimeOut_t;

// Mock constants
#define pdTRUE                    (1)
#define pdFALSE                   (0)
#define portMAX_DELAY             (0xFFFFFFFF)
#define pdMS_TO_TICKS(ms)         (ms)

// Mock critical section macros
#define taskENTER_CRITICAL_FROM_ISR()  (0)
#define taskEXIT_CRITICAL_FROM_ISR(x)  do { (void)(x); } while(0)
#define portYIELD_FROM_ISR(x)          do { (void)(x); } while(0)

// Mock stream buffer functions
StreamBufferHandle_t xStreamBufferCreate(size_t xBufferSizeBytes, size_t xTriggerLevelBytes);
void vStreamBufferDelete(StreamBufferHandle_t xStreamBuffer);
size_t xStreamBufferSend(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes, TickType_t xTicksToWait);
size_t xStreamBufferSendFromISR(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes, BaseType_t *pxHigherPriorityTaskWoken);
size_t xStreamBufferReceive(StreamBufferHandle_t xStreamBuffer, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait);

// Mock semaphore functions
SemaphoreHandle_t xSemaphoreCreateBinary(void);
SemaphoreHandle_t xSemaphoreCreateMutex(void);
void vSemaphoreDelete(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken);

// Mock task functions
BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName, uint16_t usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask);
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskSetTimeOutState(TimeOut_t *pxTimeOut);
BaseType_t xTaskCheckForTimeOut(TimeOut_t *pxTimeOut, TickType_t *pxTicksToWait);

// Mock stream buffer functions (additional)
void xStreamBufferReset(StreamBufferHandle_t xStreamBuffer);

// Mock control variables for testing
extern bool mock_freertos_init_success;
extern BaseType_t mock_semaphore_take_result;
extern BaseType_t mock_semaphore_give_result;
extern size_t mock_stream_buffer_receive_bytes;

// Test helper functions
void mock_freertos_reset(void);
void mock_freertos_set_failure_mode(bool enable);

#endif // FREERTOS_MOCK_H