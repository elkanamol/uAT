/**
 * @file uat_parser.c
 * @brief Implementation of AT command response parser utility functions
 *
 * This file implements the functions declared in uat_parser.h for parsing
 * AT command responses and extracting various data types.
 *
 * @author Elkana Molson
 * @date 2023
 */

#include "uat_parser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Check if a response contains a specific prefix
 *
 * @param response The response string to check
 * @param prefix The prefix to look for
 * @return true if the prefix is found, false otherwise
 */
bool uAT_HasPrefix(const char *response, const char *prefix)
{
    if (response == NULL || prefix == NULL)
    {
        return false;
    }
    return strstr(response, prefix) != NULL;
}

/**
 * @brief Check if a response is an error response
 *
 * @param response The response string to check
 * @return true if the response is an error, false otherwise
 */
bool uAT_IsError(const char *response)
{
    if (response == NULL)
    {
        return false;
    }
    return uAT_HasPrefix(response, "ERROR");
}

/**
 * @brief Check if a response is an OK response
 *
 * @param response The response string to check
 * @return true if the response is OK, false otherwise
 */
bool uAT_IsOK(const char *response)
{
    if (response == NULL)
    {
        return false;
    }
    return uAT_HasPrefix(response, "OK");
}

/**
 * @brief Check if a response contains a CME error
 *
 * @param response The response string to check
 * @param errorCode Pointer to store the error code if found
 * @return true if the response contains a CME error, false otherwise
 */
bool uAT_IsCMEError(const char *response, int *errorCode)
{
    if (response == NULL || errorCode == NULL)
    {
        return false;
    }

    const char *prefix = "+CME ERROR: ";
    const char *start = strstr(response, prefix);

    if (start == NULL)
    {
        return false;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Convert the error code
    char *end;
    *errorCode = (int)strtol(start, &end, 10);

    // Check if conversion was successful
    if (end == start)
    {
        return false;
    }

    return true;
}

/**
 * @brief Check if a response contains a CMS error
 *
 * @param response The response string to check
 * @param errorCode Pointer to store the error code if found
 * @return true if the response contains a CMS error, false otherwise
 */
bool uAT_IsCMSError(const char *response, int *errorCode)
{
    if (response == NULL || errorCode == NULL)
    {
        return false;
    }

    const char *prefix = "+CMS ERROR: ";
    const char *start = strstr(response, prefix);

    if (start == NULL)
    {
        return false;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Convert the error code
    char *end;
    *errorCode = (int)strtol(start, &end, 10);

    // Check if conversion was successful
    if (end == start)
    {
        return false;
    }

    return true;
}

/**
 * @brief Count the number of delimiters in a string
 *
 * @param str The string to analyze
 * @param delimiter The delimiter character to count
 * @return The number of occurrences of the delimiter
 */
int uAT_CountDelimiters(const char *str, char delimiter)
{
    if (str == NULL || delimiter == '\0')
    {
        return 0;
    }

    int count = 0;
    while (*str)
    {
        if (*str == delimiter)
        {
            count++;
        }
        str++;
    }
    return count;
}

/**
 * @brief Extract an integer value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the integer value
 * @param delimiter The delimiter character separating the integer values (unused)
 * @param value Pointer to store the extracted integer
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseInt(const char *response, const char *prefix,
                               char delimiter, int *value)
{
    (void)delimiter; // Mark as intentionally unused
    
    if (response == NULL || prefix == NULL || value == NULL)
    {
        return UAT_PARSE_NULL_ARG;
    }
    
    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }
    
    // Move past the prefix
    start += strlen(prefix);
    
    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }
    
    // Check if we have a valid digit
    if (*start == '\0' || (*start != '-' && *start != '+' && (*start < '0' || *start > '9')))
    {
        return UAT_PARSE_INVALID_FORMAT;
    }
    
    // Convert the string to an integer
    char *end;
    *value = (int)strtol(start, &end, 10);
    
    // Check if conversion was successful
    if (end == start)
    {
        return UAT_PARSE_INVALID_FORMAT;
    }

    return UAT_PARSE_OK;
}

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
                                    size_t *numValues)
{
    if (response == NULL || prefix == NULL || values == NULL ||
        numValues == NULL || maxValues == 0)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Initialize number of values found
    *numValues = 0;

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // Parse integers until we reach the end or maxValues
    const char *current = start;
    size_t count = 0;

    while (count < maxValues && *current != '\0')
    {
        // Skip any whitespace
        while (*current == ' ' || *current == '\t')
        {
            current++;
        }

        // Check if we have a valid digit
        if (*current == '\0' || (*current != '-' && *current != '+' &&
                                 (*current < '0' || *current > '9')))
        {
            break;
        }

        // Convert the string to an integer
        char *end;
        values[count] = (int)strtol(current, &end, 10);

        // Check if conversion was successful
        if (end == current)
        {
            break;
        }

        // Move to the next value
        count++;
        current = end;

        // Skip any whitespace
        while (*current == ' ' || *current == '\t')
        {
            current++;
        }

        // Check if we have a delimiter
        if (*current == delimiter)
        {
            current++; // Skip the delimiter
        }
        else
        {
            // No more delimiters, we're done
            break;
        }
    }

    *numValues = count;
    return (count > 0) ? UAT_PARSE_OK : UAT_PARSE_INVALID_FORMAT;
}

/**
 * @brief Extract a floating-point value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the float value
 * @param delimiter The delimiter character separating values (unused)
 * @param value Pointer to store the extracted float
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseFloat(const char *response, const char *prefix,
                                 char delimiter, float *value)
{
    (void)delimiter; // Mark as intentionally unused
    
    if (response == NULL || prefix == NULL || value == NULL)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // Check if we have a valid digit or decimal point
    if (*start == '\0' || ((*start != '-' && *start != '+' && *start != '.') &&
                           (*start < '0' || *start > '9')))
    {
        return UAT_PARSE_INVALID_FORMAT;
    }

    // Convert the string to a float
    char *end;
    *value = (float)strtod(start, &end);

    // Check if conversion was successful
    if (end == start)
    {
        return UAT_PARSE_INVALID_FORMAT;
    }

    return UAT_PARSE_OK;
}

/**
 * @brief Extract a hexadecimal value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the hex value
 * @param delimiter The delimiter character separating values (unused)
 * @param value Pointer to store the extracted hex value
 * @return uAT_ParseResult_t result code
 */
uAT_ParseResult_t uAT_ParseHex(const char *response, const char *prefix,
                               char delimiter, uint32_t *value)
{
    (void)delimiter; // Mark as intentionally unused
    
    if (response == NULL || prefix == NULL || value == NULL)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // Check for "0x" or "0X" prefix and skip it if present
    if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X'))
    {
        start += 2;
    }

    // Check if we have a valid hex digit
    if (*start == '\0' || !isxdigit((unsigned char)*start))
    {
        return UAT_PARSE_INVALID_FORMAT;
    }

    // Convert the string to a hex value
    char *end;
    *value = (uint32_t)strtoul(start, &end, 16);

    // Check if conversion was successful
    if (end == start)
    {
        return UAT_PARSE_INVALID_FORMAT;
    }

    return UAT_PARSE_OK;
}

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
                                  char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Clear the buffer
    buffer[0] = '\0';

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }
    
    // Move past the prefix
    start += strlen(prefix);
    
    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }
    
    // Check if we have a valid string
    if (*start == '\0')
    {
        return UAT_PARSE_INVALID_FORMAT;
    }
    
    // Find the end of the string (newline, carriage return, or null terminator)
    const char *end = start;
    while (*end != '\0' && *end != '\r' && *end != '\n')
    {
        end++;
    }
    
    // Calculate string length
    size_t length = end - start;
    if (length >= bufferSize)
    {
        // Buffer too small, copy what we can
        length = bufferSize - 1; // Leave room for null terminator
        memcpy(buffer, start, length);
        buffer[length] = '\0'; // Ensure null termination
        return UAT_PARSE_BUFFER_TOO_SMALL;
    }
    
    // Copy the string to the buffer
    memcpy(buffer, start, length);
    buffer[length] = '\0'; // Ensure null termination

    return UAT_PARSE_OK;
}

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
                                        char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Clear the buffer
    buffer[0] = '\0';

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }
    
    // Move past the prefix
    start += strlen(prefix);
    
    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }
    
    // Check if we have a quoted string
    if (*start != '"')
    {
        return UAT_PARSE_INVALID_FORMAT;
    }
    
    // Move past the opening quote
    start++;
    
    // Find the closing quote
    const char *end = start;
    while (*end != '\0' && *end != '"')
    {
        end++;
    }
    
    // Check if we found a closing quote
    if (*end != '"')
    {
        return UAT_PARSE_INVALID_FORMAT;
    }
    
    // Calculate string length
    size_t length = end - start;
    if (length >= bufferSize)
    {
        // Buffer too small, copy what we can
        length = bufferSize - 1; // Leave room for null terminator
        memcpy(buffer, start, length);
        buffer[length] = '\0'; // Ensure null termination
        return UAT_PARSE_BUFFER_TOO_SMALL;
    }
    
    // Copy the string to the buffer
    memcpy(buffer, start, length);
    buffer[length] = '\0'; // Ensure null termination

    return UAT_PARSE_OK;
}

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
                                         char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Clear the buffer
    buffer[0] = '\0';

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // Check if we have a quoted string
    if (*start != '"')
    {
        return UAT_PARSE_INVALID_FORMAT;
    }

    // Move past the opening quote
    start++;

    // Process the string with escape sequences
    size_t bufIndex = 0;
    const char *current = start;

    while (*current != '\0' && *current != '"' && bufIndex < bufferSize - 1)
    {
        // Handle escape sequences
        if (*current == '\\' && *(current + 1) != '\0')
        {
            current++; // Move to the escaped character

            switch (*current)
            {
            case 'n':
                buffer[bufIndex++] = '\n';
                break;
            case 'r':
                buffer[bufIndex++] = '\r';
                break;
            case 't':
                buffer[bufIndex++] = '\t';
                break;
            case '\\':
                buffer[bufIndex++] = '\\';
                break;
            case '"':
                buffer[bufIndex++] = '"';
                break;
            default:
                // Unknown escape sequence, just copy the character
                buffer[bufIndex++] = *current;
                break;
            }
        }
        else
        {
            // Regular character
            buffer[bufIndex++] = *current;
        }

        current++;
    }

    // Check if we reached the end of the string properly
    if (*current != '"')
    {
        // If we didn't find a closing quote, it's an error
        // unless we ran out of buffer space
        if (bufIndex >= bufferSize - 1)
        {
            buffer[bufIndex] = '\0';
            return UAT_PARSE_BUFFER_TOO_SMALL;
        }
        return UAT_PARSE_INVALID_FORMAT;
    }

    // Null-terminate the buffer
    buffer[bufIndex] = '\0';

    return UAT_PARSE_OK;
}

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
                                     char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Clear the buffer
    buffer[0] = '\0';

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }
    
    // Move past the prefix
    start += strlen(prefix);
    
    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // Validate IP address format
    const char *ptr = start;
    int dots = 0;
    int digits = 0;
    int octet_value = 0;
    bool valid = true;
    
    while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n' && *ptr != ' ' && valid)
    {
        if (*ptr == '.')
        {
            if (digits == 0)
            {
                valid = false; // No digits before dot
            }
            else if (octet_value > 255)
            {
                valid = false; // Octet value too large
            }
            dots++;
            digits = 0;
            octet_value = 0;
        }
        else if (*ptr >= '0' && *ptr <= '9')
        {
            digits++;
            octet_value = octet_value * 10 + (*ptr - '0');
            if (digits > 3 || octet_value > 255)
            {
                valid = false; // Too many digits or value too large
            }
        }
        else
        {
            valid = false; // Invalid character
        }
        ptr++;
    }

    // Check final octet
    if (digits == 0 || octet_value > 255)
    {
        valid = false;
    }

    // A valid IPv4 address has 3 dots and ends with digits
    if (!valid || dots != 3 || digits == 0)
    {
        return UAT_PARSE_INVALID_FORMAT;
    }
    
    // Calculate IP address string length
    size_t length = ptr - start;
    if (length >= bufferSize)
    {
        return UAT_PARSE_BUFFER_TOO_SMALL;
    }
    
    // Copy the IP address to the buffer
    memcpy(buffer, start, length);
    buffer[length] = '\0'; // Ensure null termination

    return UAT_PARSE_OK;
}

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
                                      size_t *dataSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL ||
        bufferSize == 0 || dataSize == NULL)
    {
        return UAT_PARSE_NULL_ARG;
    }

    // Initialize dataSize
    *dataSize = 0;

    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return UAT_PARSE_PREFIX_NOT_FOUND;
    }

    // Move past the prefix
    start += strlen(prefix);

    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // Check for length indicator (common in AT commands)
    // Format might be like: "+IPD,123:" where 123 is the length
    size_t expectedLength = 0;
    const char *dataStart = start;

    // Check if there's a length indicator
    if (*start >= '0' && *start <= '9')
    {
        expectedLength = strtoul(start, (char **)&dataStart, 10);

        // Skip any delimiter after the length
        if (*dataStart == ',' || *dataStart == ':')
        {
            dataStart++;
        }
    }

    // Determine how much data to copy
    size_t availableData = strlen(dataStart);
    size_t copyLength = (expectedLength > 0 && expectedLength <= availableData)
                            ? expectedLength
                            : availableData;

    // Check if buffer is large enough
    if (copyLength > bufferSize)
    {
        copyLength = bufferSize;
        memcpy(buffer, dataStart, copyLength);
        *dataSize = copyLength;
        return UAT_PARSE_BUFFER_TOO_SMALL;
    }

    // Copy the data
    memcpy(buffer, dataStart, copyLength);
    *dataSize = copyLength;

    return UAT_PARSE_OK;
}