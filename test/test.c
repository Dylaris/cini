/**
 * Test program for pini.h INI parser
 * Compile: gcc -o test_pini test_pini.c -std=c99
 */

#define PINI_IMPLEMENTATION
#include "pini.h"

void test_basic_parsing(void) {
    printf("=== Testing Basic Parsing ===\n");

    // Create test INI file
    FILE *f = fopen("test_basic.ini", "w");
    fprintf(f, "[database]\n");
    fprintf(f, "host = localhost\n");
    fprintf(f, "port = 5432\n");
    fprintf(f, "ssl_enabled = true\n");
    fprintf(f, "\n[server]\n");
    fprintf(f, "name = Test Server\n");
    fprintf(f, "timeout = 30.5\n");
    fclose(f);

    Pini_Context ctx;
    bool success = pini_load(&ctx, "test_basic.ini");
    assert(success && "Failed to load INI file");

    // Test number values
    double port = pini_get_number(&ctx, "database", "port");
    assert(port == 5432.0 && "Port number mismatch");

    double timeout = pini_get_number(&ctx, "server", "timeout");
    assert(timeout == 30.5 && "Timeout number mismatch");

    // Test string values
    const char *host = pini_get_string(&ctx, "database", "host");
    assert(strcmp(host, "localhost") == 0 && "Host string mismatch");

    const char *name = pini_get_string(&ctx, "server", "name");
    assert(strcmp(name, "Test Server") == 0 && "Server name mismatch");

    // Test boolean values
    bool ssl = pini_get_bool(&ctx, "database", "ssl_enabled");
    assert(ssl == true && "SSL boolean mismatch");

    printf("All basic parsing tests passed!\n");
    pini_unload(&ctx);
}

void test_edge_cases(void) {
    printf("\n=== Testing Edge Cases ===\n");

    // Create test INI file with edge cases
    FILE *f = fopen("test_edge.ini", "w");
    fprintf(f, "# This is a comment\n");
    fprintf(f, "; This is also a comment\n");
    fprintf(f, "\n");
    fprintf(f, "[section_with_spaces]  \n");
    fprintf(f, "  key_with_spaces  =  value_with_spaces  \n");
    fprintf(f, "negative_number = -123.45\n");
    fprintf(f, "zero = 0\n");
    fprintf(f, "false_value = false\n");
    fclose(f);

    Pini_Context ctx;
    bool success = pini_load(&ctx, "test_edge.ini");
    assert(success && "Failed to load edge case INI file");

    // Test trimmed values
    const char *value = pini_get_string(&ctx, "section_with_spaces", "key_with_spaces");
    assert(strcmp(value, "value_with_spaces") == 0 && "Value trimming failed");

    // Test negative number
    double neg = pini_get_number(&ctx, "section_with_spaces", "negative_number");
    assert(neg == -123.45 && "Negative number parsing failed");

    // Test zero
    double zero = pini_get_number(&ctx, "section_with_spaces", "zero");
    assert(zero == 0.0 && "Zero parsing failed");

    // Test false boolean
    bool false_val = pini_get_bool(&ctx, "section_with_spaces", "false_value");
    assert(false_val == false && "False boolean parsing failed");

    printf("All edge case tests passed!\n");
    pini_unload(&ctx);
}

void test_error_handling(void) {
    printf("\n=== Testing Error Handling ===\n");

    // Test non-existent file
    Pini_Context ctx;
    bool success = pini_load(&ctx, "non_existent_file.ini");
    assert(!success && "Should fail to load non-existent file");

    // Create malformed INI file
    FILE *f = fopen("test_malformed.ini", "w");
    fprintf(f, "[unclosed_section\n");  // Missing closing bracket
    fprintf(f, "key_without_value\n");  // Missing equals sign
    fclose(f);

    success = pini_load(&ctx, "test_malformed.ini");
    // Note: Current implementation may or may not handle these errors
    // This test documents the current behavior

    printf("Error handling tests completed.\n");
}

int main(void) {
    printf("Starting pini.h test suite...\n");

    test_basic_parsing();
    test_edge_cases();
    test_error_handling();

    printf("\n=== All tests completed ===\n");

    // Clean up test files
    remove("test_basic.ini");
    remove("test_edge.ini");
    remove("test_malformed.ini");

    return 0;
}
