#define PINI_IMPLEMENTATION
#include "pini.h"

int main(void)
{
    Pini_Context ctx;
    if (!pini_load(&ctx, "config.ini")) {
        printf("Error: Failed to load configuration file\n");
        return 1;
    }

    printf("=== Application Configuration ===\n\n");

    // Database configuration
    printf("Database Settings:\n");
    printf("  Host: %s\n",              pini_get_string(&ctx, "database", "host"));
    printf("  Port: %.0f\n",            pini_get_number(&ctx, "database", "port"));
    printf("  Username: %s\n",          pini_get_string(&ctx, "database", "username"));
    printf("  Use SSL: %s\n",           pini_get_bool(&ctx, "database", "use_ssl") ? "Yes" : "No");
    printf("  Timeout: %.1f seconds\n", pini_get_number(&ctx, "database", "connection_timeout"));
    printf("\n");

    // Logging configuration
    printf("Logging Settings:\n");
    printf("  Level: %s\n",             pini_get_string(&ctx, "logging", "level"));
    printf("  File: %s\n",              pini_get_string(&ctx, "logging", "file_path"));
    printf("  Max Size: %.0f bytes\n",  pini_get_number(&ctx, "logging", "max_file_size"));
    printf("  Console Output: %s\n",    pini_get_bool(&ctx, "logging", "enable_console") ? "Enabled" : "Disabled");
    printf("\n");

    // Server configuration
    printf("Server Settings:\n");
    printf("  Bind Address: %s\n",       pini_get_string(&ctx, "server", "host"));
    printf("  Port: %.0f\n",             pini_get_number(&ctx, "server", "port"));
    printf("  Thread Pool Size: %.0f\n", pini_get_number(&ctx, "server", "thread_pool_size"));
    printf("  Compression: %s\n",        pini_get_bool(&ctx, "server", "enable_compression") ? "Enabled" : "Disabled");
    printf("\n");

    // Demonstrate configuration usage in application logic
    printf("=== Runtime Configuration Usage ===\n");

    // Simulate application startup using loaded configuration
    if (pini_get_bool(&ctx, "database", "use_ssl")) {
        printf("• Initializing database connection with SSL...\n");
    } else {
        printf("• Initializing database connection without SSL...\n");
    }

    double thread_pool_size = pini_get_number(&ctx, "server", "thread_pool_size");
    printf("• Starting server with %.0f threads...\n", thread_pool_size);

    const char *log_level = pini_get_string(&ctx, "logging", "level");
    printf("• Setting log level to: %s\n", log_level);

    // Display raw configuration dump
    // printf("\n=== Raw Configuration Dump ===\n");
    // pini_dump(&ctx);

    pini_unload(&ctx);
    return 0;
}
