#define CINI_IMPLEMENTATION
#include "cini.h"

int main(void)
{
    Cini_Context ctx = {0};
    Cini_String val;
    const char *section;

    if (!cini_load(&ctx, "config.ini")) return 1;

    cini_dump(&ctx, stdout);

    section = "server";
    val = cini_get(&ctx, section, NULL);
    if (!cini_ok(val)) printf("no found [server]\n");

    section = "aaa";
    val = cini_get(&ctx, section, "name");
    if (cini_ok(val)) printf("[aaa] name = %.*s\n", (int)val.length, val.start);

    cini_free(&ctx);
    return 0;
}
