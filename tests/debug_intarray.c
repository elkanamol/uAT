#include "../Core/Inc/uat_parser.h"
#include <stdio.h>

int main() {
    int values[10];
    size_t numValues;
    uAT_ParseResult_t result = uAT_ParseIntArray("+CREG: 1,abc,3", "+CREG: ", ',', values, 10, &numValues);
    printf("Result: %d, numValues: %zu\n", result, numValues);
    for (int i = 0; i < (int)numValues; i++) {
        printf("values[%d] = %d\n", i, values[i]);
    }
    return 0;
}
