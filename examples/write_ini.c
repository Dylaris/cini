#define CINI_IMPLEMENTATION
#include "cini.h"

int main(void)
{
    Cini_Context ctx = {0};
    const char *section;

    section = "aaa";
    cini_set(&ctx, section, "name", "aris");
    cini_set(&ctx, section, "age", "10");

    section = "bbb";
    cini_set(&ctx, section, "man", "true");
    cini_set(&ctx, section, "name", "aris");

    section = "ccc";
    cini_set(&ctx, section, NULL, NULL);

    section = "aaa";
    cini_set(&ctx, section, "name", "jack");

    cini_generate_file(&ctx, "config.ini");

    cini_free(&ctx);
    return 0;
}
