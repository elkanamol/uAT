/**
 * @file uat_freertos.c
 * @brief Implementation of FreeRTOS-friendly uAT command parser
 *
 * This file implements the uAT command parser interface defined in uat_freertos.h.
 * It uses FreeRTOS primitives for thread safety and provides both DMA and
 * interrupt-driven UART reception options.
 *
 * @author [Elkana Molson]
 * @date [06/05/2025]
 */

#include "stm32f756xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_uart.h"
#include "usart.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"
#include "main.h"

#include "uat_freertos.h"

#ifdef UAT_USE_DMA
static uint8_t uart_dma_rx_buf[UAT_DMA_RX_SIZE];
static volatile size_t dma_last_pos __attribute__((aligned(4))) = 0;
#endif

// Forward declaration
struct uAT_HandleStruct;
typedef void (*uAT_CommandHandler)(const char *args);

/**
 * @brief Command handler entry structure
 *
 * Stores a command string and its associated handler function.
 * Used in the command dispatch system.
 */
typedef struct
{
    const char *command;         ///< Command string to match
    uAT_CommandHandler handler;  ///< Function to call when command is received
} uAT_CommandEntry;

/**
 * @brief Main uAT handle structure
 *
 * Contains all state information for the uAT parser, including
 * UART handle, synchronization primitives, and command handlers.
 */
typedef struct uAT_HandleStruct
{
    UART_HandleTypeDef *huart;                          // UART handle that connect to modem (e.g. UART2)
    StreamBufferHandle_t rxStream;                      // Stream buffer for RX
    SemaphoreHandle_t txComplete;                       // For UART transmission
    SemaphoreHandle_t txMutex;                          // For UART transmission
    SemaphoreHandle_t handlerMutex;                     // For command handler management
    SemaphoreHandle_t sendReceiveSem;                   // For SendReceive
    uint8_t txBuffer[UAT_TX_BUFFER_SIZE];               // Transmit buffer
    uAT_CommandEntry cmdHandlers[UAT_MAX_CMD_HANDLERS]; // Registered commands
    size_t cmdCount;                                    // Number of registered commands

    // SendReceive state
    bool inSendReceive;  // True if currently in SendReceive
    char *srBuffer;      // Buffer for SendReceive
    size_t srBufferSize; // Size of srBuffer
    size_t srBufferPos;  // Current position in srBuffer
} uAT_Handle_t;

static uAT_Handle_t uat;

// Push single received byte into stream buffer
static inline void uAT_PushRxByte(uint8_t byte)
{
    BaseType_t xHigher = pdFALSE;
    xStreamBufferSendFromISR(uat.rxStream, &byte, 1, &xHigher);
    portYIELD_FROM_ISR(xHigher);
}

#ifdef UAT_USE_DMA
// Idle line IRQ handler to copy new DMA data into stream buffer
/**
 * @brief Handles UART IDLE line interrupt for DMA-based reception
 * 
 * This function processes incoming data from the DMA receive buffer when an IDLE line
 * interrupt occurs. It manages circular buffer wrapping and transfers received data
 * to the stream buffer, tracking the last processed position.
 * 
 * @note This function is designed to be called from the UART IDLE line interrupt handler
 * @return true if data was successfully processed, false on error
 */
bool uAT_UART_IdleHandler(void)
{
    if (uat.huart == NULL || uat.huart->hdmarx == NULL || uat.rxStream == NULL) {
        return false; // Safety check for null pointers
    }

    UBaseType_t uxSavedInterruptStatus;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bool success = true;
    
    // Get current DMA position atomically
    size_t current_pos = UAT_DMA_RX_SIZE - __HAL_DMA_GET_COUNTER(uat.huart->hdmarx);
    
    // Enter critical section to get last position
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    size_t last_pos = dma_last_pos;
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    
    // No new data
    if (current_pos == last_pos) {
        return true; // No error, just no new data
    }
    
    // Handle normal case (no buffer wrap)
    if (current_pos > last_pos) {
        size_t data_len = current_pos - last_pos;
        
        // Check for buffer overflow
        if (data_len > UAT_DMA_RX_SIZE) {
            // This should never happen, but handle it safely
            data_len = UAT_DMA_RX_SIZE - last_pos;
            success = false;
        }
        
        // Send data to stream buffer
        size_t bytes_sent = xStreamBufferSendFromISR(
            uat.rxStream,
            &uart_dma_rx_buf[last_pos], 
            data_len, 
            &xHigherPriorityTaskWoken
        );
        
        // Check if all bytes were sent
        if (bytes_sent < data_len) {
            success = false; // Stream buffer might be full
        }
    }
    // Handle buffer wrap case
    else {
        // Data from last_pos to end of buffer
        size_t tail_len = UAT_DMA_RX_SIZE - last_pos;
        
        if (tail_len > 0) {
            size_t bytes_sent = xStreamBufferSendFromISR(
                uat.rxStream,
                &uart_dma_rx_buf[last_pos], 
                tail_len, 
                &xHigherPriorityTaskWoken
            );
            
            if (bytes_sent < tail_len) {
                success = false;
            }
        }
        
        // Data from start of buffer to current position
        if (current_pos > 0 && success) {
            size_t bytes_sent = xStreamBufferSendFromISR(
                uat.rxStream,
                &uart_dma_rx_buf[0], 
                current_pos, 
                &xHigherPriorityTaskWoken
            );
            
            if (bytes_sent < current_pos) {
                success = false;
            }
        }
    }
    
    // Update position tracking atomically
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    dma_last_pos = current_pos;
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    
    // Yield if needed
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
    return success;
}

// ISR: called from UART IRQ when IDLE flag set
// add to USARTx_IRQHandler in stm32f7xx_it.c file
// Note: the checking and calling uAT_UART_IdleHandler() is done before USARTx_IRQHandler.
void USARTx_IRQHandler(void)
{
    // Check if this is our UART and if IDLE flag is set
    if (uat.huart != NULL && __HAL_UART_GET_FLAG(uat.huart, UART_FLAG_IDLE))
    {
        // Clear the IDLE flag
        __HAL_UART_CLEAR_IDLEFLAG(uat.huart);
        
        // Process received data
        uAT_UART_IdleHandler();
    }
    
    // Call the HAL UART IRQ handler
    HAL_UART_IRQHandler(uat.huart);
}

#else
// Byte-by-byte interrupt-driven receive
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static uint8_t rxByte;
    
    // Check if this is our UART
    if (huart == uat.huart)
    {
        // Push the received byte to the stream buffer
        uAT_PushRxByte(rxByte);
        
        // Restart reception for the next byte
        HAL_UART_Receive_IT(uat.huart, &rxByte, 1);
    }
}
#endif

// Common TX complete callback (for both IT and DMA)
/**
 * @brief Callback function for UART transmission complete event
 * 
 * This function is called when a UART transmission is completed. It gives a binary semaphore
 * to signal the transmission is finished, allowing waiting tasks to proceed.
 * 
 * @param huart Pointer to the UART handle that completed transmission
 * 
 * @note This is an ISR (Interrupt Service Routine) callback function
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // Check if this is our UART
    if (huart == uat.huart)
    {
        BaseType_t xHigher = pdFALSE;
        
        // Signal that transmission is complete
        xSemaphoreGiveFromISR(uat.txComplete, &xHigher);
        
        // Yield if a higher priority task was woken
        portYIELD_FROM_ISR(xHigher);
    }
}

// === CORE API ===

/**
 * @brief  Initialize the uAT parser module
 * @param  huart Pointer to HAL UART handle
 * @return UAT_OK if successful, or appropriate error code on failure
 */
uAT_Result_t uAT_Init(UART_HandleTypeDef *huart)
{
    // Validate input parameters
    if (!huart) {
        return UAT_ERR_INVALID_ARG;
    }

    // Clear the handle structure
    memset(&uat, 0, sizeof(uat));
    uat.huart = huart;
    
    // Create FreeRTOS primitives
    uat.rxStream = xStreamBufferCreate(UAT_RX_BUFFER_SIZE, 1);
    if (!uat.rxStream) {
        return UAT_ERR_RESOURCE;
    }
    
    uat.txComplete = xSemaphoreCreateBinary();
    if (!uat.txComplete) {
        vStreamBufferDelete(uat.rxStream);
        return UAT_ERR_RESOURCE;
    }
    
    uat.txMutex = xSemaphoreCreateMutex();
    if (!uat.txMutex) {
        vStreamBufferDelete(uat.rxStream);
        vSemaphoreDelete(uat.txComplete);
        return UAT_ERR_RESOURCE;
    }
    
    uat.handlerMutex = xSemaphoreCreateMutex();
    if (!uat.handlerMutex) {
        vStreamBufferDelete(uat.rxStream);
        vSemaphoreDelete(uat.txComplete);
        vSemaphoreDelete(uat.txMutex);
        return UAT_ERR_RESOURCE;
    }
    
    uat.sendReceiveSem = xSemaphoreCreateBinary();
    if (!uat.sendReceiveSem) {
        vStreamBufferDelete(uat.rxStream);
        vSemaphoreDelete(uat.txComplete);
        vSemaphoreDelete(uat.txMutex);
        vSemaphoreDelete(uat.handlerMutex);
        return UAT_ERR_RESOURCE;
    }
    
    // Initialize state variables
    uat.inSendReceive = false;
    uat.srBuffer = NULL;
    uat.srBufferSize = 0;
    uat.srBufferPos = 0;
    uat.cmdCount = 0;

#ifdef UAT_USE_DMA
    // Reset DMA position tracking
    dma_last_pos = 0;
    
    // Start circular DMA reception
    __HAL_RCC_DMA1_CLK_ENABLE();
    if (HAL_UART_Receive_DMA(huart, uart_dma_rx_buf, UAT_DMA_RX_SIZE) != HAL_OK) {
        // Clean up all resources on failure
        vStreamBufferDelete(uat.rxStream);
        vSemaphoreDelete(uat.txComplete);
        vSemaphoreDelete(uat.txMutex);
        vSemaphoreDelete(uat.handlerMutex);
        vSemaphoreDelete(uat.sendReceiveSem);
        return UAT_ERR_INIT_FAIL;
    }

    // Enable IDLE interrupt
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    __HAL_UART_ENABLE_IT(uat.huart, UART_IT_IDLE);
#else
    // Start byte-by-byte IRQ reception
    static uint8_t rxByte;
    if (HAL_UART_Receive_IT(huart, &rxByte, 1) != HAL_OK) {
        // Clean up all resources on failure
        vStreamBufferDelete(uat.rxStream);
        vSemaphoreDelete(uat.txComplete);
        vSemaphoreDelete(uat.txMutex);
        vSemaphoreDelete(uat.handlerMutex);
        vSemaphoreDelete(uat.sendReceiveSem);
        return UAT_ERR_INIT_FAIL;
    }
#endif

    return UAT_OK;
}

/**
 * @brief  Register a command string and its handler
 * @param  cmd     Null-terminated string to match at start of line
 * @param  handler Function called when a line beginning with cmd arrives
 * @return UAT_OK if registered, or appropriate error code on failure
 */
uAT_Result_t uAT_RegisterCommand(const char *cmd, uAT_CommandHandler handler)
{
    // Validate input parameters
    if (!cmd || !handler) {
        return UAT_ERR_INVALID_ARG;
    }
    
    // Check command length
    size_t cmdLen = strlen(cmd);
    if (cmdLen == 0 || cmdLen >= UAT_RX_BUFFER_SIZE) {
        return UAT_ERR_INVALID_ARG;
    }

    // Try to acquire mutex with timeout
    if (xSemaphoreTake(uat.handlerMutex, portMAX_DELAY) != pdTRUE) {
        return UAT_ERR_BUSY;
    }

    // Check if command already exists
    for (size_t i = 0; i < uat.cmdCount; i++) {
        if (strcmp(uat.cmdHandlers[i].command, cmd) == 0) {
            // Update existing handler
            uat.cmdHandlers[i].handler = handler;
            xSemaphoreGive(uat.handlerMutex);
            return UAT_OK;
        }
    }
    
    // Add new command handler if space available
    uAT_Result_t result = UAT_ERR_RESOURCE;
    if (uat.cmdCount < UAT_MAX_CMD_HANDLERS) {
        // Store command string and handler
        uat.cmdHandlers[uat.cmdCount].command = cmd;
        uat.cmdHandlers[uat.cmdCount].handler = handler;
        uat.cmdCount++;
        result = UAT_OK;
    }
    
    xSemaphoreGive(uat.handlerMutex);
    return result;
}

/**
 * @brief  Unregister a previously registered command
 * @note   This function should be called with `uat.handlerMutex` already taken
 * @param  cmd Null-terminated string of the command to unregister
 * @return UAT_OK if unregistered, or appropriate error code on failure
 */
uAT_Result_t uAT_UnregisterCommand(const char *cmd)
{
    // Validate input parameters
    if (!cmd) {
        return UAT_ERR_INVALID_ARG;
    }
    
    // Search for the command in the handler array
    for (size_t i = 0; i < uat.cmdCount; i++) {
        if (strcmp(uat.cmdHandlers[i].command, cmd) == 0) {
            // Found the command, now remove it by shifting all subsequent entries
            for (size_t j = i; j < uat.cmdCount - 1; j++) {
                uat.cmdHandlers[j] = uat.cmdHandlers[j + 1];
            }
            
            // Clear the last entry and decrement count
            uat.cmdHandlers[uat.cmdCount - 1].command = NULL;
            uat.cmdHandlers[uat.cmdCount - 1].handler = NULL;
            uat.cmdCount--;
            
            return UAT_OK;
        }
    }
    
    // Command not found
    return UAT_ERR_NOT_FOUND;
}

/**
 * @brief Helper function to safely append data to the SendReceive buffer
 * 
 * @param data Data to append to the buffer
 * @param len Length of the data
 * @return true if data was appended successfully, false otherwise
 */
static bool uAT_AppendToResponseBuffer(const char *data, size_t len)
{
    // Validate parameters
    if (data == NULL || len == 0) {
        return false;
    }
    
    // Check if we're in a SendReceive operation and have a valid buffer
    if (!uat.inSendReceive || !uat.srBuffer || uat.srBufferPos >= uat.srBufferSize - 1) {
        return false;
    }
    
    // Calculate how much space is left in the buffer
    size_t spaceLeft = uat.srBufferSize - uat.srBufferPos - 1; // -1 for null terminator
    
    // Limit copy to available space
    if (len > spaceLeft) {
        len = spaceLeft;
    }
    
    // Copy data to buffer
    if (len > 0) {
        memcpy(uat.srBuffer + uat.srBufferPos, data, len);
        uat.srBufferPos += len;
        uat.srBuffer[uat.srBufferPos] = '\0'; // Ensure null termination
        return true;
    }
    
    return false;
}

/**
 * @brief Command handler for SendReceive operations
 * 
 * This function is called when the expected response is received during
 * a SendReceive operation. It signals the waiting task that the response
 * has been received.
 * 
 * @param args Arguments passed to the handler (unused)
 */
static void uAT_CommandHandler_SendReceive(const char *args)
{
    (void)args; // Unused parameter
    
    // Signal that we've received the expected response
    xSemaphoreGive(uat.sendReceiveSem);
}

/**
 * @brief Helper function to safely clean up SendReceive state
 * 
 * @param expected Expected response string that was registered
 */
static void uAT_CleanupSendReceiveState(const char *expected)
{
    // This function should be called with handlerMutex already taken
    if (expected != NULL) {
        uAT_UnregisterCommand(expected);
    }
    
    // Reset SendReceive state
    uat.inSendReceive = false;
    uat.srBuffer = NULL;
    uat.srBufferSize = 0;
    uat.srBufferPos = 0;
}

/**
 * @brief Helper function to take mutex and clean up SendReceive state
 * 
 * @param expected Expected response string that was registered
 * @param timeoutTicks Maximum time to wait for mutex acquisition
 * @return true if cleanup was successful, false otherwise
 */
static bool uAT_SafeCleanupSendReceiveState(const char *expected, TickType_t timeoutTicks)
{
    // Try to acquire mutex with timeout
    if (xSemaphoreTake(uat.handlerMutex, timeoutTicks) == pdTRUE) {
        uAT_CleanupSendReceiveState(expected);
        xSemaphoreGive(uat.handlerMutex);
        return true;
    }
    return false;
}

/**
 * @brief Helper function to set up SendReceive state
 * 
 * @param expected Expected response string to register
 * @param outBuf Buffer to store the response
 * @param bufLen Size of the buffer
 * @return UAT_OK if setup was successful, error code otherwise
 */
static uAT_Result_t uAT_SetupSendReceiveState(const char *expected, char *outBuf, size_t bufLen)
{
    // This function should be called with handlerMutex already taken
    
    // Validate parameters
    if (expected == NULL || outBuf == NULL || bufLen == 0) {
        return UAT_ERR_INVALID_ARG;
    }
    
    // Set up the SendReceive state
    uat.inSendReceive = true;
    uat.srBuffer = outBuf;
    uat.srBufferSize = bufLen;
    uat.srBufferPos = 0;
    
    // Clear the output buffer
    memset(outBuf, 0, bufLen);
    
    // Register the command handler for the expected response
    if (uat.cmdCount < UAT_MAX_CMD_HANDLERS) {
        uat.cmdHandlers[uat.cmdCount].command = expected;
        uat.cmdHandlers[uat.cmdCount].handler = uAT_CommandHandler_SendReceive;
        uat.cmdCount++;
        return UAT_OK;
    }
    
    // If we couldn't register the handler, reset the state
    uat.inSendReceive = false;
    uat.srBuffer = NULL;
    uat.srBufferSize = 0;
    uat.srBufferPos = 0;
    
    return UAT_ERR_RESOURCE;
}

/**
 * @brief Sends an AT command and waits for a specific response
 * 
 * Implementation details:
 * 1. Takes handlerMutex to ensure exclusive access to command handlers
 * 2. Registers a temporary handler for the expected response
 * 3. Sends the command using uAT_SendCommand()
 * 4. Waits for the response with timeout
 * 5. Cleans up by unregistering the temporary handler
 * 
 * @param cmd Command to send
 * @param expected Expected response prefix till end of line
 * @param outBuf Buffer to store the response
 * @param bufLen Size of outBuf
 * @param timeoutTicks Maximum time to wait for response
 * @return UAT_OK on success, error code otherwise
 */
uAT_Result_t uAT_SendReceive(const char *cmd, const char *expected, char *outBuf, size_t bufLen, TickType_t timeoutTicks)
{
    // Validate input parameters
    if (!cmd || !expected || !outBuf || bufLen == 0) {
        return UAT_ERR_INVALID_ARG;
    }

    // Validate expected response isn't too long
    if (strlen(expected) >= UAT_RX_BUFFER_SIZE) {
        return UAT_ERR_INVALID_ARG;
    }

    // Clear the output buffer
    memset(outBuf, 0, bufLen);
    
    // 1) Serialize access to SendReceive operation
    if (xSemaphoreTake(uat.handlerMutex, timeoutTicks) != pdTRUE) {
        return UAT_ERR_BUSY;
    }
    
    // Check if we're already in a SendReceive operation
    if (uat.inSendReceive) {
        xSemaphoreGive(uat.handlerMutex);
        return UAT_ERR_BUSY;
    }
    
    // Set up the SendReceive state
    uAT_Result_t result = uAT_SetupSendReceiveState(expected, outBuf, bufLen);
    if (result != UAT_OK) {
        xSemaphoreGive(uat.handlerMutex);
        return UAT_ERR_INT;
    }
    
    // Release the mutex before sending command
    xSemaphoreGive(uat.handlerMutex);
    
    // 2) Send the AT command
    result = uAT_SendCommand(cmd);
    if (result != UAT_OK) {
        uAT_SafeCleanupSendReceiveState(expected, timeoutTicks);
        return UAT_ERR_SEND_FAIL;
    }
    
    // 3) Wait for the callback to fire (or timeout)
    if (xSemaphoreTake(uat.sendReceiveSem, timeoutTicks) != pdTRUE) {
        uAT_SafeCleanupSendReceiveState(expected, portMAX_DELAY);
        return UAT_ERR_TIMEOUT;
    }
    
    // 4) Success - unregister the command handler
    uAT_SafeCleanupSendReceiveState(expected, portMAX_DELAY);
    return UAT_OK;
}

uAT_Result_t uAT_SendCommand(const char *cmd)
{
    if (!cmd)
        return UAT_ERR_INVALID_ARG; // invalid command argument

    if (xSemaphoreTake(uat.txMutex, pdMS_TO_TICKS(500)) != pdTRUE)
        return UAT_ERR_BUSY;      // failed to take mutex, check if uart is busy

    int len = snprintf((char *)uat.txBuffer,
                       UAT_TX_BUFFER_SIZE, "%s\r\n", cmd);

    if (len <= 0 || len >= UAT_TX_BUFFER_SIZE) 
    {
        xSemaphoreGive(uat.txMutex);
        return UAT_ERR_INVALID_ARG; // buffer isn't valid length
    }

    // choose DMA or IT transmit automatically by HAL
    if (HAL_UART_Transmit_DMA(uat.huart, uat.txBuffer, len) != HAL_OK)
    {
        xSemaphoreGive(uat.txMutex);
        return UAT_ERR_SEND_FAIL;  // failed to transmit
    }
    
    // wait for completion with timeout
    BaseType_t result = xSemaphoreTake(uat.txComplete, pdMS_TO_TICKS(1000));
    xSemaphoreGive(uat.txMutex);

    return (result == pdTRUE) ? UAT_OK : UAT_ERR_TIMEOUT;
}

//
/**
 * @brief Helper function that dispatches an incoming AT command
 *  to the appropriate registered handler.
 *
 * Iterates through registered command handlers to find a matching command prefix.
 * When a match is found, the corresponding handler is called with the command arguments.
 *
 * @param line Received command line to dispatch
 * @param len Length of the received command line
 * @return bool True if a matching handler was found and executed, false otherwise
 */
static bool uAT_DispatchCommand(const char *line, size_t len)
{
    // Validate input parameters
    if (!line || len == 0 || len >= UAT_RX_BUFFER_SIZE) {
        return false;
    }
    
    // Ensure line is null-terminated for safety
    char safe_line[UAT_RX_BUFFER_SIZE];
    if (len >= UAT_RX_BUFFER_SIZE - 1) {
        len = UAT_RX_BUFFER_SIZE - 2; // Leave room for null terminator
    }
    memcpy(safe_line, line, len);
    safe_line[len] = '\0';
    
    // Search for matching command handler
    for (size_t i = 0; i < uat.cmdCount; i++) {
        const char *cmd = uat.cmdHandlers[i].command;
        if (!cmd) continue; // Skip invalid entries
        
        size_t cmdLen = strlen(cmd);
        if (cmdLen == 0) continue; // Skip empty commands
        
        // Check if command matches
        if (strncmp(safe_line, cmd, cmdLen) == 0) {
            // Get arguments (safely)
            const char *args = safe_line + cmdLen;
            
            // Skip leading spaces
            while (*args == ' ') {
                args++;
            }
            
            // Store handler to call after releasing mutex
            uAT_CommandHandler handler = uat.cmdHandlers[i].handler;
            
            // Validate handler before calling
            if (!handler) {
                xSemaphoreGive(uat.handlerMutex);
                return false;
            }
            
            xSemaphoreGive(uat.handlerMutex);
            
            // Call handler outside critical section
            handler(args);
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Helper function to receive data from a stream buffer until a delimiter is found
 *
 * @param stream Stream buffer to read from
 * @param dest Destination buffer to store received characters
 * @param maxLen Maximum number of characters to read
 * @param delim Delimiter string to search for
 * @param ticksToWait Maximum time to wait for data (in RTOS ticks)
 * @return Number of characters received
 */
size_t xStreamBufferReceiveUntilDelimiter(
    StreamBufferHandle_t stream,
    char *dest,
    size_t maxLen,
    const char *delim,
    TickType_t ticksToWait)
{
    // Validate parameters
    if (stream == NULL || dest == NULL || maxLen < 2 || delim == NULL) {
        return 0;
    }
    
    size_t total = 0;
    char ch;
    TickType_t xTimeToWait = ticksToWait;
    TimeOut_t xTimeOut;
    
    // Initialize timeout
    vTaskSetTimeOutState(&xTimeOut);
    
    // Clear destination buffer
    memset(dest, 0, maxLen);
    
    while (total < maxLen - 1) {
        // Check for timeout
        if (xTaskCheckForTimeOut(&xTimeOut, &xTimeToWait) == pdTRUE) {
            break;
        }
        
        // Try to receive a character
        if (xStreamBufferReceive(stream, &ch, 1, xTimeToWait) != 1) {
            break;
        }
        
        // Store the character
        dest[total++] = ch;
        dest[total] = '\0'; // Ensure null termination
        
        // Check if we've found the delimiter
        if (total >= strlen(delim)) {
            if (strstr(dest + total - strlen(delim), delim) != NULL) {
                break;
            }
        }
    }
    
    // Ensure null termination
    if (total < maxLen) {
        dest[total] = '\0';
    } else {
        dest[maxLen - 1] = '\0';
    }
    
    return total;
}

/**
 * @brief FreeRTOS task for handling UAT (UART AT) command processing
 *
 * This task continuously monitors the UAT receive stream for incoming commands.
 * When a complete command is received, it attempts to dispatch the command 
 * to a registered handler. In SendReceive mode, it also captures the response.
 *
 * @param params Unused task parameters
 */
void uAT_Task(void *params)
{
    (void)params;
    char lineBuf[UAT_RX_BUFFER_SIZE];
    
    // Task initialization
    printf("uAT_Task started\r\n");

    // Main task loop
    while (1) {
        // Clear buffer for safety
        memset(lineBuf, 0, sizeof(lineBuf));
        
        // Wait for data with timeout
        size_t len = xStreamBufferReceiveUntilDelimiter(
            uat.rxStream,
            lineBuf,
            sizeof(lineBuf),
            UAT_LINE_TERMINATOR,
            pdMS_TO_TICKS(1000));

        if (len > 0) {
            // Try to acquire mutex with timeout
            if (xSemaphoreTake(uat.handlerMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Always capture response if in SendReceive mode
                if (uat.inSendReceive && uat.srBuffer != NULL) {
                    uAT_AppendToResponseBuffer(lineBuf, len);
                }
                
                // Dispatch to appropriate handler
                if (!uAT_DispatchCommand(lineBuf, len)) {
                    // No handler found, release mutex
                    xSemaphoreGive(uat.handlerMutex);
                }
                // Note: If handler found, the dispatch function releases the mutex
            }
            // If we couldn't get the mutex, we'll try again on the next iteration
        }
        
        // Short delay to prevent CPU hogging
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief  Reset the AT command interface
 * @return UAT_OK on success, or appropriate error code on failure
 */
uAT_Result_t uAT_Reset(void)
{
    // Validate UART handle
    if (uat.huart == NULL)
    {
        return UAT_ERR_INVALID_ARG;
    }

    // Stop any ongoing transfers
    HAL_UART_AbortReceive(uat.huart);
    HAL_UART_AbortTransmit(uat.huart);

    // Clear stream buffer
    if (uat.rxStream != NULL)
    {
        xStreamBufferReset(uat.rxStream);
    }

#ifdef UAT_USE_DMA
    // Reset DMA
    dma_last_pos = 0;

    // Restart DMA reception
    if (HAL_UART_Receive_DMA(uat.huart, uart_dma_rx_buf, UAT_DMA_RX_SIZE) != HAL_OK)
    {
        return UAT_ERR_INIT_FAIL;
    }

    // Re-enable IDLE interrupt
    __HAL_UART_CLEAR_IDLEFLAG(uat.huart);
    __HAL_UART_ENABLE_IT(uat.huart, UART_IT_IDLE);
#else
    // Restart interrupt-driven reception
    static uint8_t rxByte;
    if (HAL_UART_Receive_IT(uat.huart, &rxByte, 1) != HAL_OK)
    {
        return UAT_ERR_INIT_FAIL;
    }
#endif

    return UAT_OK;
}

/**
 * @brief Register a URC handler with high priority
 * 
 * This function registers a command handler for Unsolicited Result Codes (URCs)
 * with higher priority than regular command handlers by inserting it at the
 * beginning of the handler array.
 * 
 * @param cmd Command string to match
 * @param handler Function to call when the command is received
 * @return UAT_OK if successful, error code otherwise
 */
uAT_Result_t uAT_RegisterURC(const char *cmd, uAT_CommandHandler handler)
{
    // Validate input parameters
    if (!cmd || !handler) {
        return UAT_ERR_INVALID_ARG;
    }
    
    // Check command length
    size_t cmdLen = strlen(cmd);
    if (cmdLen == 0 || cmdLen >= UAT_RX_BUFFER_SIZE) {
        return UAT_ERR_INVALID_ARG;
    }

    // Try to acquire mutex with timeout
    if (xSemaphoreTake(uat.handlerMutex, portMAX_DELAY) != pdTRUE) {
        return UAT_ERR_BUSY;
    }

    // Check if command already exists
    for (size_t i = 0; i < uat.cmdCount; i++) {
        if (strcmp(uat.cmdHandlers[i].command, cmd) == 0) {
            // Remove existing entry to reinsert at the beginning
            for (size_t j = i; j < uat.cmdCount - 1; j++) {
                uat.cmdHandlers[j] = uat.cmdHandlers[j + 1];
            }
            uat.cmdCount--;
            break;
        }
    }
    
    // Check if we have space for a new handler
    uAT_Result_t result = UAT_ERR_RESOURCE;
    if (uat.cmdCount < UAT_MAX_CMD_HANDLERS) {
        // Shift all handlers to make room at the beginning
        for (size_t i = uat.cmdCount; i > 0; i--) {
            uat.cmdHandlers[i] = uat.cmdHandlers[i-1];
        }
        
        // Insert the URC handler at the beginning
        uat.cmdHandlers[0].command = cmd;
        uat.cmdHandlers[0].handler = handler;
        uat.cmdCount++;
        result = UAT_OK;
    }
    
    xSemaphoreGive(uat.handlerMutex);
    return result;
}
