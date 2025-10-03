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

Pini_Value *pini_lookup(Pini_Context *ctx, const char *section_name, const char *key_name)
{
    if (!section_name) return NULL;
    Pini_Section *section = pini__get_section(ctx->sections, section_name);
    if (!section) return NULL;
    Pini_Pair *pair = pini__get_pair(section->pairs, key_name);
    if (!pair) return NULL;
    return &pair->val;
}

