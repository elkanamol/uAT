#ifndef UAT_PARSER_H
#define UAT_PARSER_H

#include <stdbool.h>
#include <stddef.h>

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
 * @return true if successful, false otherwise
 */
bool uAT_ParseInt(const char *response, const char *prefix, char delimiter, int *value);

/**
 * @brief Extract a string value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the string value
 * @param buffer Buffer to store the extracted string
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool uAT_ParseString(const char *response, const char *prefix, char *buffer, size_t bufferSize);

/**
 * @brief Extract a quoted string value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the quoted string
 * @param buffer Buffer to store the extracted string (without quotes)
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool uAT_ParseQuotedString(const char *response, const char *prefix, char *buffer, size_t bufferSize);

/**
 * @brief Extract an IP address from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the IP address
 * @param buffer Buffer to store the extracted IP address
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool uAT_ParseIPAddress(const char *response, const char *prefix, char *buffer, size_t bufferSize);

#endif // UAT_PARSER_H