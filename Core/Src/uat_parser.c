#include "uat_parser.h"
#include "uat_freertos.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"

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

bool uAT_IsOK(const char *response)
{
    if (response == NULL)
    {
        return false;
    }
    return uAT_HasPrefix(response, "OK");
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
 * @param value Pointer to store the extracted integers
 * @param delimiter The delimiter character separating the integer values
 * @return true if successful, false otherwise
 */
bool uAT_ParseInt(const char *response, const char *prefix, char delimiter, int *value)
{
    if (response == NULL || prefix == NULL || value == NULL || delimiter == '\0')
    {
        return false;
    }
    
    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return false;
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
        return false;
    }
    
    // Convert the string to an integer
    char *end;
    *value = (int)strtol(start, &end, 10);
    
    // Check if conversion was successful
    if (end == start)
    {
        return false;
    }
    
    return true;
}

/**
 * @brief Extract a string value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the string value
 * @param buffer Buffer to store the extracted string
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool uAT_ParseString(const char *response, const char *prefix, char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return false;
    }
    
    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return false;
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
        return false;
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
        length = bufferSize - 1; // Leave room for null terminator
    }
    
    // Copy the string to the buffer
    strncpy(buffer, start, length);
    buffer[length] = '\0'; // Ensure null termination
    
    return true;
}

/**
 * @brief Extract a quoted string value from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the quoted string
 * @param buffer Buffer to store the extracted string (without quotes)
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool uAT_ParseQuotedString(const char *response, const char *prefix, char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return false;
    }
    
    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return false;
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
        return false;
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
        return false;
    }
    
    // Calculate string length
    size_t length = end - start;
    if (length >= bufferSize)
    {
        length = bufferSize - 1; // Leave room for null terminator
    }
    
    // Copy the string to the buffer
    strncpy(buffer, start, length);
    buffer[length] = '\0'; // Ensure null termination
    
    return true;
}

/**
 * @brief Extract an IP address from a response after a specific prefix
 *
 * @param response The response string to parse
 * @param prefix The prefix before the IP address
 * @param buffer Buffer to store the extracted IP address
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool uAT_ParseIPAddress(const char *response, const char *prefix, char *buffer, size_t bufferSize)
{
    if (response == NULL || prefix == NULL || buffer == NULL || bufferSize == 0)
    {
        return false;
    }
    
    // Find the prefix in the response
    const char *start = strstr(response, prefix);
    if (start == NULL)
    {
        return false;
    }
    
    // Move past the prefix
    start += strlen(prefix);
    
    // Skip any whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }
    
    // Check if we have a valid IP address format (simple check)
    const char *ptr = start;
    int dots = 0;
    int digits = 0;
    bool valid = true;
    
    while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n' && *ptr != ' ' && valid)
    {
        if (*ptr == '.')
        {
            if (digits == 0)
            {
                valid = false; // No digits before dot
            }
            dots++;
            digits = 0;
        }
        else if (*ptr >= '0' && *ptr <= '9')
        {
            digits++;
            if (digits > 3)
            {
                valid = false; // Too many digits in an octet
            }
        }
        else
        {
            valid = false; // Invalid character
        }
        ptr++;
    }
    
    // A valid IPv4 address has 3 dots and ends with digits
    if (!valid || dots != 3 || digits == 0)
    {
        return false;
    }
    
    // Calculate IP address string length
    size_t length = ptr - start;
    if (length >= bufferSize)
    {
        return false; // Buffer too small
    }
    
    // Copy the IP address to the buffer
    strncpy(buffer, start, length);
    buffer[length] = '\0'; // Ensure null termination
    
    return true;
}
