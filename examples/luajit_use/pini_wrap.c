#define PINI_IMPLEMENTATION
#include "pini.h"

Pini_Context *pini_new_context(void)
{
    Pini_Context *ctx = malloc(sizeof(Pini_Context));
    ctx->current_section = NULL;
    ctx->sections = NULL;
    return ctx;
}

void pini_free_context(Pini_Context *ctx)
{
    if (ctx) {
        pini_unload(ctx);
        free(ctx);
    }
}
