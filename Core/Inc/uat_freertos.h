/**
 * @file uat_freertos.h
 * @brief FreeRTOS-friendly uAT command parser with IRQ and DMA support
 *
 * This module provides a thread-safe AT command parser that works with FreeRTOS.
 * It supports both DMA and interrupt-driven UART reception, and provides
 * mechanisms for registering command handlers and sending/receiving AT commands.
 *
 * Features:
 * - DMA or interrupt-based UART reception
 * - Command registration and callback system
 * - Thread-safe operation with FreeRTOS primitives
 * - Synchronous command/response handling
 *
 * @author [Elkana Molson]
 * @date [06/05/2025]
 */

#ifndef UAT_FREERTOS_H
#define UAT_FREERTOS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f7xx_hal.h" // or your HAL header
#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"
#include <stddef.h>
#include <stdbool.h>

/* -------------------- Configuration -------------------- */
/* These values can be overridden by defining them before including this file */

#ifndef UAT_RX_BUFFER_SIZE
#define UAT_RX_BUFFER_SIZE 512     /**< Size of RX buffer */
#endif

#ifndef UAT_TX_BUFFER_SIZE
#define UAT_TX_BUFFER_SIZE 512     /**< Size of TX buffer */
#endif

#ifndef UAT_MAX_CMD_HANDLERS
#define UAT_MAX_CMD_HANDLERS 10    /**< Max number of command handlers */
#endif

#ifndef UAT_LINE_TERMINATOR
#define UAT_LINE_TERMINATOR "\r\n" /**< Line terminator for AT commands */
#endif

#ifndef UAT_TX_TIMEOUT_MS
#define UAT_TX_TIMEOUT_MS 1000     /**< Timeout for TX operations in ms */
#endif

#ifndef UAT_MUTEX_TIMEOUT_MS
#define UAT_MUTEX_TIMEOUT_MS 500   /**< Timeout for mutex acquisition in ms */
#endif

#ifndef UAT_DMA_RX_SIZE
#define UAT_DMA_RX_SIZE 512        /**< Size of DMA RX buffer */
#endif

/* Enable/disable DMA reception */
#ifndef UAT_USE_DMA
#define UAT_USE_DMA               /**< Define to use DMA for reception */
#endif

/* -------------------- End Configuration -------------------- */

    /** 
     * @brief Result codes for all API operations
     * 
     * Standardized error codes used by all functions in the uAT API
     * to provide consistent error reporting and handling.
     */
    typedef enum {
        UAT_OK = 0,             ///< Operation successful
        UAT_ERR_INVALID_ARG,    ///< Invalid argument provided
        UAT_ERR_BUSY,           ///< Resource is busy
        UAT_ERR_TIMEOUT,        ///< Operation timed out
        UAT_ERR_NOT_FOUND,      ///< Item not found
        UAT_ERR_SEND_FAIL,      ///< Failed to send data
        UAT_ERR_INIT_FAIL,      ///< Initialization failed
        UAT_ERR_INT,            ///< Internal error
        UAT_ERR_RESOURCE        ///< Resource allocation failed
    } uAT_Result_t;

    // Forward declaration of the uAT handle (opaque in user code)
    typedef struct uAT_HandleStruct uAT_Handle_t;

    // Command handler callback prototype
    // args points to the first character after the registered command
    // Ex: if command == "OK", handler receives "param1,param2" when lineBuf == "OK param1,param2\r\n"
    typedef void (*uAT_CommandHandler)(const char *args);

    // API

    /**
     * @brief  Initialize the uAT parser module
     * @param  huart Pointer to HAL UART handle
     * @return UAT_OK if successful, or appropriate error code on failure:
     *         - UAT_ERR_INVALID_ARG: If huart is NULL
     *         - UAT_ERR_RESOURCE: If resource allocation fails
     *         - UAT_ERR_INIT_FAIL: If UART/DMA initialization fails
     */
    uAT_Result_t uAT_Init(UART_HandleTypeDef *huart);

    /**
     * @brief  Register a command string and its handler
     * @param  cmd     Null-terminated string to match at start of line
     * @param  handler Function called when a line beginning with cmd arrives
     * @return UAT_OK if registered, or appropriate error code on failure:
     *         - UAT_ERR_INVALID_ARG: If cmd or handler is NULL
     *         - UAT_ERR_BUSY: If mutex acquisition fails
     *         - UAT_ERR_RESOURCE: If handler table is full
     */
    uAT_Result_t uAT_RegisterCommand(const char *cmd, uAT_CommandHandler handler);

    /**
     * @brief  Register a URC handler with high priority
     * @param  cmd     Null-terminated string to match at start of line
     * @param  handler Function called when a line beginning with cmd arrives
     * @return UAT_OK if registered, or appropriate error code on failure:
     *         - UAT_ERR_INVALID_ARG: If cmd or handler is NULL
     *         - UAT_ERR_BUSY: If mutex acquisition fails
     *         - UAT_ERR_RESOURCE: If handler table is full
     */
    uAT_Result_t uAT_RegisterURC(const char *cmd, uAT_CommandHandler handler);

    /**
     * @brief  Unregister a previously registered command
     * @note   This function should be called with `uat.handlerMutex` already taken
     * @param  cmd Null-terminated string of the command to unregister
     * @return UAT_OK if unregistered, or appropriate error code on failure:
     *         - UAT_ERR_INVALID_ARG: If cmd is NULL
     *         - UAT_ERR_NOT_FOUND: If command not found in handler table
     */
    uAT_Result_t uAT_UnregisterCommand(const char *cmd);

    /**
     * @brief  Send an AT-style command (appends CR+LF)
     * @param  cmd Null-terminated command string without terminator
     * @return UAT_OK on success, or appropriate error code on failure:
     *         - UAT_ERR_INVALID_ARG: If cmd is NULL or too long
     *         - UAT_ERR_BUSY: If transmission mutex acquisition fails
     *         - UAT_ERR_SEND_FAIL: If UART transmission fails
     *         - UAT_ERR_TIMEOUT: If transmission times out
     */
    uAT_Result_t uAT_SendCommand(const char *cmd);

    /**
     * @brief  Send a command and wait for a specific response prefix.
     * @param  cmd            Null-terminated AT command (no CRLF)
     * @param  expected       Prefix to match (e.g. "OK" or "+CREG")
     * @param  outBuf         Buffer to receive the full line (incl. CRLF)
     * @param  bufLen         Length of outBuf
     * @param  timeoutTicks   How many RTOS ticks to wait
     * @return UAT_OK on success, or appropriate error code on failure:
     *         - UAT_ERR_INVALID_ARG: If any parameter is invalid
     *         - UAT_ERR_BUSY: If another SendReceive operation is in progress
     *         - UAT_ERR_INT: If internal error occurs
     *         - UAT_ERR_SEND_FAIL: If command transmission fails
     *         - UAT_ERR_TIMEOUT: If response not received within timeout
     */
    uAT_Result_t uAT_SendReceive(const char *cmd,
                                 const char *expected,
                                 char *outBuf,
                                 size_t bufLen,
                                 TickType_t timeoutTicks);

    /**
     * @brief  FreeRTOS task to process incoming lines and dispatch handlers
     * @param  params Unused
     */
    void uAT_Task(void *params);

// ISR hooks (implement or forward in application IRQ)
#ifdef UAT_USE_DMA
    /**
     * @brief  Must be called from UART IRQ handler on IDLE line event
     *         Extracts new bytes from DMA buffer into stream buffer
     * @return nothing
     */
    bool uAT_UART_IdleHandler(void);
#endif

    /**
     * @brief  Reset the AT command interface
     * @return UAT_OK on success, or appropriate error code on failure:
     *         - UAT_ERR_INIT_FAIL: If UART/DMA reinitialization fails
     */
    uAT_Result_t uAT_Reset(void);

#ifdef __cplusplus
}
#endif

#endif // UAT_FREERTOS_H
