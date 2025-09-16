/**
 * @file test_parser.c
 * @brief Comprehensive tests for uAT parser functions
 * 
 * This file contains unit tests for all parser functions in the uAT library.
 * Tests cover normal operation, edge cases, and error conditions.
 */

#include "test_framework.h"
#include "uat_parser.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>

// Helper function declarations not in header
bool uAT_HasPrefix(const char *response, const char *prefix);
bool uAT_IsError(const char *response);
bool uAT_IsOK(const char *response);

void test_uAT_HasPrefix(void)
{
    TEST_SUITE_START("uAT_HasPrefix");
    
    // Normal cases
    TEST_ASSERT_TRUE(uAT_HasPrefix("OK\r\n", "OK"), "Should detect OK prefix");
    TEST_ASSERT_TRUE(uAT_HasPrefix("ERROR\r\n", "ERROR"), "Should detect ERROR prefix");
    TEST_ASSERT_TRUE(uAT_HasPrefix("+CREG: 1,2", "+CREG"), "Should detect +CREG prefix");
    TEST_ASSERT_FALSE(uAT_HasPrefix("AT+CREG?\r\n", "+CREG"), "Should not detect +CREG prefix in AT command");
    
    // Edge cases
    TEST_ASSERT_FALSE(uAT_HasPrefix(NULL, "OK"), "Should handle null response");
    TEST_ASSERT_FALSE(uAT_HasPrefix("OK", NULL), "Should handle null prefix");
    TEST_ASSERT_FALSE(uAT_HasPrefix(NULL, NULL), "Should handle both null");
    TEST_ASSERT_TRUE(uAT_HasPrefix("", ""), "Should handle empty strings");
    TEST_ASSERT_FALSE(uAT_HasPrefix("O", "OK"), "Should handle short response");
    
    TEST_SUITE_END("uAT_HasPrefix");
}

void test_uAT_IsError(void)
{
    TEST_SUITE_START("uAT_IsError");
    
    // Normal cases
    TEST_ASSERT_TRUE(uAT_IsError("ERROR\r\n"), "Should detect ERROR response");
    TEST_ASSERT_FALSE(uAT_IsError("OK\r\n"), "Should not detect error in OK response");
    TEST_ASSERT_FALSE(uAT_IsError("+CME ERROR: 3"), "Should not detect error in CME error (different pattern)");
    
    // Edge cases
    TEST_ASSERT_FALSE(uAT_IsError(NULL), "Should handle null response");
    TEST_ASSERT_FALSE(uAT_IsError(""), "Should handle empty string");
    TEST_ASSERT_FALSE(uAT_IsError("ERR"), "Should not match partial error");
    
    TEST_SUITE_END("uAT_IsError");
}

void test_uAT_IsOK(void)
{
    TEST_SUITE_START("uAT_IsOK");
    
    // Normal cases
    TEST_ASSERT_TRUE(uAT_IsOK("OK\r\n"), "Should detect OK response");
    TEST_ASSERT_FALSE(uAT_IsOK("ERROR\r\n"), "Should not detect OK in ERROR response");
    TEST_ASSERT_TRUE(uAT_IsOK("OKAY"), "Should match extended OK (prefix matching)");
    
    // Edge cases
    TEST_ASSERT_FALSE(uAT_IsOK(NULL), "Should handle null response");
    TEST_ASSERT_FALSE(uAT_IsOK(""), "Should handle empty string");
    TEST_ASSERT_FALSE(uAT_IsOK("O"), "Should not match partial OK");
    
    TEST_SUITE_END("uAT_IsOK");
}

void test_uAT_IsCMEError(void)
{
    TEST_SUITE_START("uAT_IsCMEError");
    
    int errorCode;
    
    // Normal cases
    TEST_ASSERT_TRUE(uAT_IsCMEError("+CME ERROR: 3\r\n", &errorCode), "Should detect CME error");
    TEST_ASSERT_EQUAL_INT(3, errorCode, "Should extract correct error code");
    
    TEST_ASSERT_TRUE(uAT_IsCMEError("+CME ERROR: 123", &errorCode), "Should detect CME error without CRLF");
    TEST_ASSERT_EQUAL_INT(123, errorCode, "Should extract larger error code");
    
    TEST_ASSERT_TRUE(uAT_IsCMEError("AT+COPS?\r\n+CME ERROR: 30\r\nOK", &errorCode), "Should find CME error in response");
    TEST_ASSERT_EQUAL_INT(30, errorCode, "Should extract error from multi-line response");
    
    // Negative cases
    TEST_ASSERT_FALSE(uAT_IsCMEError("OK\r\n", &errorCode), "Should not detect CME error in OK");
    TEST_ASSERT_FALSE(uAT_IsCMEError("ERROR\r\n", &errorCode), "Should not detect CME error in ERROR");
    TEST_ASSERT_FALSE(uAT_IsCMEError("+CMS ERROR: 123", &errorCode), "Should not detect CME in CMS error");
    
    // Edge cases
    TEST_ASSERT_FALSE(uAT_IsCMEError(NULL, &errorCode), "Should handle null response");
    TEST_ASSERT_FALSE(uAT_IsCMEError("+CME ERROR: 3", NULL), "Should handle null error code pointer");
    TEST_ASSERT_FALSE(uAT_IsCMEError("+CME ERROR: ", &errorCode), "Should handle missing error code");
    TEST_ASSERT_FALSE(uAT_IsCMEError("+CME ERROR: abc", &errorCode), "Should handle invalid error code");
    
    TEST_SUITE_END("uAT_IsCMEError");
}

void test_uAT_IsCMSError(void)
{
    TEST_SUITE_START("uAT_IsCMSError");
    
    int errorCode;
    
    // Normal cases
    TEST_ASSERT_TRUE(uAT_IsCMSError("+CMS ERROR: 123\r\n", &errorCode), "Should detect CMS error");
    TEST_ASSERT_EQUAL_INT(123, errorCode, "Should extract correct error code");
    
    // Negative cases
    TEST_ASSERT_FALSE(uAT_IsCMSError("+CME ERROR: 123", &errorCode), "Should not detect CMS in CME error");
    
    // Edge cases - same as CME error tests
    TEST_ASSERT_FALSE(uAT_IsCMSError(NULL, &errorCode), "Should handle null response");
    TEST_ASSERT_FALSE(uAT_IsCMSError("+CMS ERROR: 3", NULL), "Should handle null error code pointer");
    
    TEST_SUITE_END("uAT_IsCMSError");
}

void test_uAT_CountDelimiters(void)
{
    TEST_SUITE_START("uAT_CountDelimiters");
    
    // Normal cases
    TEST_ASSERT_EQUAL_INT(2, uAT_CountDelimiters("a,b,c", ','), "Should count commas");
    TEST_ASSERT_EQUAL_INT(0, uAT_CountDelimiters("abc", ','), "Should return 0 for no delimiters");
    TEST_ASSERT_EQUAL_INT(3, uAT_CountDelimiters("a:b:c:d", ':'), "Should count colons");
    TEST_ASSERT_EQUAL_INT(1, uAT_CountDelimiters("hello world", ' '), "Should count spaces");
    
    // Edge cases
    TEST_ASSERT_EQUAL_INT(0, uAT_CountDelimiters(NULL, ','), "Should handle null string");
    TEST_ASSERT_EQUAL_INT(0, uAT_CountDelimiters("", ','), "Should handle empty string");
    TEST_ASSERT_EQUAL_INT(3, uAT_CountDelimiters(",,,", ','), "Should handle only delimiters");
    
    TEST_SUITE_END("uAT_CountDelimiters");
}

void test_uAT_ParseInt(void)
{
    TEST_SUITE_START("uAT_ParseInt");
    
    int value;
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseInt("+CREG: 1,2", "+CREG: ", ',', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse first integer");
    TEST_ASSERT_EQUAL_INT(1, value, "Should extract correct value");
    
    result = uAT_ParseInt("Signal: -75", "Signal: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse negative integer");
    TEST_ASSERT_EQUAL_INT(-75, value, "Should extract negative value");
    
    result = uAT_ParseInt("Count: +123", "Count: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse positive integer with sign");
    TEST_ASSERT_EQUAL_INT(123, value, "Should extract positive value");
    
    // Error cases
    result = uAT_ParseInt(NULL, "+CREG: ", ',', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    result = uAT_ParseInt("+CREG: 1,2", NULL, ',', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null prefix");
    
    result = uAT_ParseInt("+CREG: 1,2", "+CREG: ", ',', NULL);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null value pointer");
    
    result = uAT_ParseInt("+CREG: 1,2", "+MISSING: ", ',', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_PREFIX_NOT_FOUND, result, "Should handle missing prefix");
    
    result = uAT_ParseInt("+CREG: abc", "+CREG: ", ',', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle invalid format");
    
    // Test overflow detection
    char overflow_str[100];
    snprintf(overflow_str, sizeof(overflow_str), "Value: %lld", (long long)INT_MAX + 1);
    result = uAT_ParseInt(overflow_str, "Value: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OVERFLOW, result, "Should detect overflow");
    
    TEST_SUITE_END("uAT_ParseInt");
}

void test_uAT_ParseIntArray(void)
{
    TEST_SUITE_START("uAT_ParseIntArray");
    
    int values[10];
    size_t numValues;
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseIntArray("+CREG: 1,2,3", "+CREG: ", ',', values, 10, &numValues);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse multiple integers");
    TEST_ASSERT_EQUAL_INT(3, numValues, "Should extract correct count");
    TEST_ASSERT_EQUAL_INT(1, values[0], "Should extract first value");
    TEST_ASSERT_EQUAL_INT(2, values[1], "Should extract second value");
    TEST_ASSERT_EQUAL_INT(3, values[2], "Should extract third value");
    
    result = uAT_ParseIntArray("Values: -10,20,-30", "Values: ", ',', values, 10, &numValues);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse negative integers");
    TEST_ASSERT_EQUAL_INT(3, numValues, "Should extract correct count for negative values");
    TEST_ASSERT_EQUAL_INT(-10, values[0], "Should extract negative value");
    
    // Edge cases
    result = uAT_ParseIntArray("Single: 42", "Single: ", ',', values, 10, &numValues);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse single integer");
    TEST_ASSERT_EQUAL_INT(1, numValues, "Should extract single value count");
    TEST_ASSERT_EQUAL_INT(42, values[0], "Should extract single value");
    
    // Array too small
    result = uAT_ParseIntArray("+TEST: 1,2,3,4,5", "+TEST: ", ',', values, 3, &numValues);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should handle limited array size");
    TEST_ASSERT_EQUAL_INT(3, numValues, "Should extract up to array limit");
    
    // Error cases
    result = uAT_ParseIntArray(NULL, "+CREG: ", ',', values, 10, &numValues);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    result = uAT_ParseIntArray("+CREG: 1,abc,3", "+CREG: ", ',', values, 10, &numValues);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse valid integers before invalid one");
    TEST_ASSERT_EQUAL_INT(1, numValues, "Should extract one valid integer before invalid format");
    TEST_ASSERT_EQUAL_INT(1, values[0], "Should extract the valid integer");
    
    TEST_SUITE_END("uAT_ParseIntArray");
}

void test_uAT_ParseFloat(void)
{
    TEST_SUITE_START("uAT_ParseFloat");
    
    float value;
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseFloat("Temperature: 23.5", "Temperature: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse positive float");
    TEST_ASSERT(value > 23.4 && value < 23.6, "Should extract correct float value");
    
    result = uAT_ParseFloat("Signal: -12.75", "Signal: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse negative float");
    TEST_ASSERT(value > -12.8 && value < -12.7, "Should extract correct negative float");
    
    result = uAT_ParseFloat("Value: 0.0", "Value: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse zero float");
    TEST_ASSERT(value == 0.0f, "Should extract zero value");
    
    // Error cases  
    result = uAT_ParseFloat(NULL, "Temperature: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    result = uAT_ParseFloat("Temperature: abc", "Temperature: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle invalid format");
    
    TEST_SUITE_END("uAT_ParseFloat");
}

void test_uAT_ParseHex(void)
{
    TEST_SUITE_START("uAT_ParseHex");
    
    uint32_t value;
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseHex("ID: A5F2", "ID: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse hex value");
    TEST_ASSERT_EQUAL_INT(0xA5F2, value, "Should extract correct hex value");
    
    result = uAT_ParseHex("Address: 0x1234", "Address: 0x", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse hex with 0x prefix");
    TEST_ASSERT_EQUAL_INT(0x1234, value, "Should extract correct hex value with prefix");
    
    result = uAT_ParseHex("Value: ff", "Value: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse lowercase hex");
    TEST_ASSERT_EQUAL_INT(0xFF, value, "Should extract lowercase hex value");
    
    // Error cases
    result = uAT_ParseHex("ID: XYZ", "ID: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle invalid hex characters");
    
    result = uAT_ParseHex(NULL, "ID: ", '\0', &value);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    TEST_SUITE_END("uAT_ParseHex");
}

void test_uAT_ParseString(void)
{
    TEST_SUITE_START("uAT_ParseString");
    
    char buffer[100];
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseString("Name: TestDevice", "Name: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse string");
    TEST_ASSERT_EQUAL_STRING("TestDevice", buffer, "Should extract correct string");
    
    result = uAT_ParseString("Model: RC7120\r\n", "Model: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse string with CRLF");
    TEST_ASSERT_EQUAL_STRING("RC7120", buffer, "Should extract string without CRLF");
    
    // Edge cases
    result = uAT_ParseString("Empty: ", "Empty: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle empty string after prefix");
    
    result = uAT_ParseString("Value: test", "Value: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should handle valid string");
    TEST_ASSERT_EQUAL_STRING("test", buffer, "Should extract valid string");
    
    // Error cases
    result = uAT_ParseString(NULL, "Name: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    result = uAT_ParseString("Name: TestDevice", "Missing: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_PREFIX_NOT_FOUND, result, "Should handle missing prefix");
    
    // Buffer too small
    char small_buffer[5];
    result = uAT_ParseString("Name: VeryLongDeviceName", "Name: ", small_buffer, sizeof(small_buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_BUFFER_TOO_SMALL, result, "Should handle buffer too small");
    
    TEST_SUITE_END("uAT_ParseString");
}

void test_uAT_ParseQuotedString(void)
{
    TEST_SUITE_START("uAT_ParseQuotedString");
    
    char buffer[100];
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseQuotedString("Operator: \"Verizon\"", "Operator: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse quoted string");
    TEST_ASSERT_EQUAL_STRING("Verizon", buffer, "Should extract string without quotes");
    
    result = uAT_ParseQuotedString("Name: \"Test Device\"", "Name: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse quoted string with spaces");
    TEST_ASSERT_EQUAL_STRING("Test Device", buffer, "Should extract string with spaces");
    
    // Error cases
    result = uAT_ParseQuotedString("Name: NoQuotes", "Name: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle missing quotes");
    
    result = uAT_ParseQuotedString("Name: \"Unclosed", "Name: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle unclosed quote");
    
    TEST_SUITE_END("uAT_ParseQuotedString");
}

void test_uAT_ParseEscapedString(void)
{
    TEST_SUITE_START("uAT_ParseEscapedString");
    
    char buffer[100];
    uAT_ParseResult_t result;
    
    // Normal cases - basic escaped string
    result = uAT_ParseEscapedString("Text: \"Hello\\nWorld\"", "Text: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse escaped string");
    TEST_ASSERT_EQUAL_STRING("Hello\nWorld", buffer, "Should handle newline escape");
    
    result = uAT_ParseEscapedString("Path: \"C:\\\\temp\\\\file.txt\"", "Path: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse escaped backslashes");
    TEST_ASSERT_EQUAL_STRING("C:\\temp\\file.txt", buffer, "Should handle backslash escapes");
    
    result = uAT_ParseEscapedString("Quote: \"He said \\\"Hello\\\"\"", "Quote: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse escaped quotes");
    TEST_ASSERT_EQUAL_STRING("He said \"Hello\"", buffer, "Should handle quote escapes");
    
    // Error cases
    result = uAT_ParseEscapedString("Text: NoQuotes", "Text: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_INVALID_FORMAT, result, "Should handle missing quotes");
    
    result = uAT_ParseEscapedString(NULL, "Text: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    TEST_SUITE_END("uAT_ParseEscapedString");
}

void test_uAT_ParseIPAddress(void)
{
    TEST_SUITE_START("uAT_ParseIPAddress");
    
    char buffer[20];
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseIPAddress("IP: 192.168.1.1", "IP: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse IP address");
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", buffer, "Should extract correct IP");
    
    result = uAT_ParseIPAddress("Gateway: 10.0.0.1\r\n", "Gateway: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse IP with CRLF");
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", buffer, "Should extract IP without CRLF");
    
    // Error cases
    result = uAT_ParseIPAddress(NULL, "IP: ", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    TEST_SUITE_END("uAT_ParseIPAddress");
}

void test_uAT_ParseBinaryData(void)
{
    TEST_SUITE_START("uAT_ParseBinaryData");
    
    uint8_t buffer[100];
    size_t dataSize;
    uAT_ParseResult_t result;
    
    // Normal cases
    result = uAT_ParseBinaryData("Data: 5,HELLO", "Data: ", buffer, sizeof(buffer), &dataSize);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse binary data with length");
    TEST_ASSERT_EQUAL_INT(5, dataSize, "Should extract correct data size");
    TEST_ASSERT(memcmp(buffer, "HELLO", 5) == 0, "Should extract correct binary data");
    
    result = uAT_ParseBinaryData("Payload: TESTDATA", "Payload: ", buffer, sizeof(buffer), &dataSize);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_OK, result, "Should parse binary data without length");
    TEST_ASSERT_EQUAL_INT(8, dataSize, "Should extract available data size");
    
    // Error cases
    result = uAT_ParseBinaryData(NULL, "Data: ", buffer, sizeof(buffer), &dataSize);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_NULL_ARG, result, "Should handle null response");
    
    // Buffer too small
    uint8_t small_buffer[3];
    result = uAT_ParseBinaryData("Data: VERYLONGDATA", "Data: ", small_buffer, sizeof(small_buffer), &dataSize);
    TEST_ASSERT_EQUAL_INT(UAT_PARSE_BUFFER_TOO_SMALL, result, "Should handle buffer too small");
    TEST_ASSERT_EQUAL_INT(3, dataSize, "Should copy what fits in buffer");
    
    TEST_SUITE_END("uAT_ParseBinaryData");
}

int main(void)
{
    printf("=== uAT Parser Tests ===\n");
    
    test_framework_init();
    
    // Run all parser tests
    test_uAT_HasPrefix();
    test_uAT_IsError();
    test_uAT_IsOK();
    test_uAT_IsCMEError();
    test_uAT_IsCMSError();
    test_uAT_CountDelimiters();
    test_uAT_ParseInt();
    test_uAT_ParseIntArray();
    test_uAT_ParseFloat();
    test_uAT_ParseHex();
    test_uAT_ParseString();
    test_uAT_ParseQuotedString();
    test_uAT_ParseEscapedString();
    test_uAT_ParseIPAddress();
    test_uAT_ParseBinaryData();
    
    test_framework_summary();
    return test_framework_get_result();
}