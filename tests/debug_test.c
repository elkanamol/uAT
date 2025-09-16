#include "../Core/Inc/uat_parser.h"
#include <stdio.h>

// Helper function declarations not in header
bool uAT_HasPrefix(const char *response, const char *prefix);
bool uAT_IsOK(const char *response);

int main() {
    // Test 1: "OKAY" should not match "OK"
    bool result1 = uAT_IsOK("OKAY");
    printf("uAT_IsOK(\"OKAY\") = %s (expected: false)\n", result1 ? "true" : "false");
    
    // Test 2: ParseString with empty value
    char buffer[100];
    uAT_ParseResult_t result2 = uAT_ParseString("Empty: ", "Empty: ", buffer, sizeof(buffer));
    printf("uAT_ParseString(\"Empty: \", \"Empty: \", buffer, 100) = %d (expected: 0)\n", result2);
    printf("Extracted string: '%s'\n", buffer);
    
    return 0;
}
