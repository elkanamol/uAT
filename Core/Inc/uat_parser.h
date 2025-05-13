/**
 * @file uat_parser.h
 * @brief AT command response parser utility functions
 *
 * This module provides functions for parsing responses from AT commands.
 * It includes utilities for extracting various data types from responses
 * and checking for common response patterns.
 *
 * @author Elkana Molson
 * @date 2025
 */

#ifndef UAT_PARSER_H
#define UAT_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Result codes for parsing operations
 */
typedef enum {
    UAT_PARSE_OK = 0,           ///< Parsing successful
    UAT_PARSE_NULL_ARG,         ///< Null argument provided
    UAT_PARSE_PREFIX_NOT_FOUND, ///< Specified prefix not found
    UAT_PARSE_INVALID_FORMAT,   ///< Invalid format in the response
    UAT_PARSE_BUFFER_TOO_SMALL, ///< Destination buffer too small
    UAT_PARSE_INVALID_VALUE     ///< Parsed value is invalid
} uAT_ParseResult_t;

/**
 * @brief Check if a response contains a specific prefix
 *
 * @param response The response string to check
 * @param prefix The prefix to look for
 * @return true if the prefix is found, false otherwise
 */
bool uAT_HasPrefix(const char *response, const char *prefix);

/**
 * @brief Check if a response is an error response
 *
 * @param response The response string to check
 * @return true if the response is an error, false otherwise
 */
bool uAT_IsError(const char *response);

/**
 * @brief Check if a response is an OK response
 *
 * @param response The response string to check
 * @return true if the response is OK, false otherwise
 */
bool uAT_IsOK(const char *response);

/**
 * @brief Check if a response contains a CME error
 *
 * @param response The response string to check
 * @param errorCode Pointer to store the error code if found
 * @return true if the response contains a CME error, false otherwise
 */
bool uAT_IsCMEError(const char *response, int *errorCode);

/**
 * @brief Check if a response contains a CMS error
 *
 * @param response The response string to check
 * @param errorCode Pointer to store the error code if found
 * @return true if the response contains a CMS error, false otherwise
 */
bool uAT_IsCMSError(const char *response, int *errorCode);

/**
 * @brief Count the number of delimiters in a string
 *
 * @param str The string to analyze
 * @param delimiter The delimiter character to count
 * @return The number of occurrences of the delimiter
 */
int uAT_CountDelimiters(const char *str, char delimiter);

/**
 * @brief Extract an integer value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the integer value
 * @param delimiter The delimiter character separating the integer values
 * @param value Pointer to store the extracted integer
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseInt(const char *response, const char *prefix, 
                              char delimiter, int *value);

/**
 * @brief Extract multiple integer values from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the integer values
 * @param delimiter The delimiter character separating the integer values
 * @param values Array to store the extracted integers
 * @param maxValues Maximum number of values to extract
 * @param numValues Pointer to store the number of values extracted
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseIntArray(const char *response, const char *prefix, 
                                   char delimiter, int *values, size_t maxValues, 
                                   size_t *numValues);

/**
 * @brief Extract a floating-point value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the float value
 * @param delimiter The delimiter character separating values
 * @param value Pointer to store the extracted float
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseFloat(const char *response, const char *prefix, 
                                char delimiter, float *value);

/**
 * @brief Extract a hexadecimal value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the hex value
 * @param delimiter The delimiter character separating values
 * @param value Pointer to store the extracted hex value
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseHex(const char *response, const char *prefix, 
                              char delimiter, uint32_t *value);

/**
 * @brief Extract a string value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the string value
 * @param buffer Buffer to store the extracted string
 * @param bufferSize Size of the buffer
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseString(const char *response, const char *prefix, 
                                 char *buffer, size_t bufferSize);

/**
 * @brief Extract a quoted string value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the quoted string
 * @param buffer Buffer to store the extracted string (without quotes)
 * @param bufferSize Size of the buffer
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseQuotedString(const char *response, const char *prefix, 
                                       char *buffer, size_t bufferSize);

/**
 * @brief Extract a quoted string with escape sequences from a response
 *
 * @param response The response string to parse
 * @param prefix The prefix before the quoted string
 * @param buffer Buffer to store the extracted string (without quotes)
 * @param bufferSize Size of the buffer
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseEscapedString(const char *response, const char *prefix, 
                                        char *buffer, size_t bufferSize);

/**
 * @brief Extract an IP address from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the IP address
 * @param buffer Buffer to store the extracted IP address
 * @param bufferSize Size of the buffer
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseIPAddress(const char *response, const char *prefix, 
                                    char *buffer, size_t bufferSize);

/**
 * @brief Extract binary data from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the binary data
 * @param buffer Buffer to store the extracted binary data
 * @param bufferSize Size of the buffer
 * @param dataSize Pointer to store the size of the extracted data
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseBinaryData(const char *response, const char *prefix, 
                                     uint8_t *buffer, size_t bufferSize, 
                                     size_t *dataSize);

#endif // UAT_PARSER_H