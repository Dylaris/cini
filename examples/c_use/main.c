#define PINI_IMPLEMENTATION
#include "pini.h"

int main(void)
{
    Pini_Context ctx;
    if (!pini_load(&ctx, "../config.ini")) {
        printf("Error: Failed to load configuration file\n");
        return 1;
    }

    Pini_Query query = {.section = "server"};

    query.key = "port";
    size_t port = (size_t)pini_get_number(&ctx, &query);
    if (query.errcode == PINI_OK) {
        printf("server: listening port: %zu\n", port);
    } else {
        printf("server: default port is 5050\n");
    }

    if (pini_has(&ctx, "logging", NULL)) {
        printf("section [logging] exists\n");
    } else {
        printf("section [logging] do not exist\n");
    }

    pini_unload(&ctx);
    return 0;
}
