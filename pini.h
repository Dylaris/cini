/*
pini.h - v0.01 - Dylaris 2025
===================================================

BRIEF:
  'ini' file parser implemented with C99.

NOTICE:
  Do not support nested section and array.
  Not compatible with C++. (no test)
  Error handling is minimal. Ensure correct usage:
    - Keys and sections must exist when querying
    - File must be properly formatted
    - No type safety guarantees

USAGE:
  In exactly one source file, define the implementation macro
  before including this header:
  ```
    #define PINI_IMPLEMENTATION
    #include "pini.h"
  ```
  In other files, just include the header without the macro.

HISTORY:

LICENSE:
  See the end of this file for further details.
*/

#ifndef PINI_H
#define PINI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

typedef enum Pini_Value_Type {
    PINI_VALUE_NUMBER,
    PINI_VALUE_STRING,
    PINI_VALUE_BOOLEAN
} Pini_Value_Type;

typedef struct Pini_Value {
    Pini_Value_Type type;
    union {
        double number;
        char *string;
        bool boolean;
    } as;
} Pini_Value;

typedef struct Pini_Pair {
    char *key;
    Pini_Value val;
} Pini_Pair;

typedef struct Pini_Section {
    char *name;
    Pini_Pair *pairs;
} Pini_Section;

typedef struct Pini_Context {
    Pini_Section *current_section;
    Pini_Section *sections;
} Pini_Context;

bool pini_load(Pini_Context *ctx, const char *filename);
void pini_unload(Pini_Context *ctx);
void pini_dump(Pini_Context *ctx);
double pini_get_number(Pini_Context *ctx, const char *section_name, const char *key_name);
const char *pini_get_string(Pini_Context *ctx, const char *section_name, const char *key_name);
bool pini_get_bool(Pini_Context *ctx, const char *section_name, const char *key_name);

#endif // PINI_H

#ifdef PINI_IMPLEMENTATION

typedef struct pini__vector_header {
    size_t size;
    size_t capacity;
} pini__vector_header;

#define pini__vec_header(vec)   ((pini__vector_header*)((char*)(vec) - sizeof(pini__vector_header)))
#define pini__vec_size(vec)     ((vec) ? pini__vec_header(vec)->size : 0)
#define pini__vec_capacity(vec) ((vec) ? pini__vec_header(vec)->capacity : 0)
#define pini__vec_push(vec, item)                                \
    do {                                                         \
        if (pini__vec_size(vec) + 1 > pini__vec_capacity(vec)) { \
            (vec) = pini__vec_grow(vec, sizeof(*vec));           \
        }                                                        \
        (vec)[pini__vec_header(vec)->size++] = (item);           \
    } while (0)
#define pini__vec_free(vec)                   \
    do {                                      \
        if (vec) free(pini__vec_header(vec)); \
        (vec) = NULL;                         \
    } while (0)
#define pini__vec_foreach(type, vec, iter) for (type *iter = (vec); iter < (vec)+pini__vec_size(vec); iter++)

static void *pini__vec_grow(void *vec, size_t item_size)
{
    size_t new_capacity, alloc_size;
    pini__vector_header *new_header;

    new_capacity = pini__vec_capacity(vec) < 16
                   ? 16 : 2 * pini__vec_capacity(vec);
    alloc_size = sizeof(pini__vector_header) + new_capacity*item_size;

    if (vec) {
        new_header = realloc(pini__vec_header(vec), alloc_size);
        assert(new_header != NULL && "out of memory");
    } else {
        new_header = malloc(alloc_size);
        assert(new_header != NULL && "out of memory");
        new_header->size = 0;
    }
    new_header->capacity = new_capacity;

    return (void*)((char*)new_header + sizeof(pini__vector_header));
}

static char *pini__strdup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy) memcpy(copy, s, len+1);
    return copy;
}

static char *pini__strndup(const char *s, size_t n)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    if (len > n) len = n;
    char *copy = malloc(len + 1);
    if (copy) {
        memcpy(copy, s, len);
        copy[len] = '\0';
    }
    return copy;
}

// TODO: use hash table
static Pini_Section *pini__get_section(Pini_Section *sections, const char *name)
{
    pini__vec_foreach(Pini_Section, sections, section) {
        if (strcmp(section->name, name) == 0) return section;
    }
    return NULL;
}

// TODO: use hash table
static Pini_Pair *pini__get_pair(Pini_Pair *pairs, const char *name)
{
    pini__vec_foreach(Pini_Pair, pairs, pair) {
        if (strcmp(pair->key, name) == 0) return pair;
    }
    return NULL;
}

static Pini_Value *pini__get_value(Pini_Context *ctx, const char *section_name, const char *key_name)
{
    Pini_Section *section = pini__get_section(ctx->sections, section_name);
    if (!section) return NULL;
    Pini_Pair *pair = pini__get_pair(section->pairs, key_name);
    if (!pair) return NULL;
    return &pair->val;
}

// TODO: support array
static void pini__store_pair(Pini_Section *section, const char *key, const char *val)
{
    Pini_Pair new_pair;
    Pini_Pair *pair;
    bool isnew = true;

    pair = pini__get_pair(section->pairs, key);
    if (pair) isnew = false;
    else      pair = &new_pair;

    if (isnew) pair->key = pini__strdup(key);

    // FIXME: number may overflow
    char *endptr;
    double num = strtod(val, &endptr);
    if (endptr != val && *endptr == '\0') {
        pair->val.type = PINI_VALUE_NUMBER;
        pair->val.as.number = num;
        goto end;
    }

    if (strcmp(val, "true") == 0) {
        pair->val.type = PINI_VALUE_BOOLEAN;
        pair->val.as.boolean = true;
    } else if (strcmp(val, "false") == 0) {
        pair->val.type = PINI_VALUE_BOOLEAN;
        pair->val.as.boolean = false;
    } else {
        pair->val.type = PINI_VALUE_STRING;
        if (!isnew && pair->val.as.string) free(pair->val.as.string);
        pair->val.as.string = pini__strdup(val);
    }

end:
    if (isnew) pini__vec_push(section->pairs, new_pair);
}

static bool pini__parse_line(Pini_Context *ctx, const char *line)
{
    // skip whitepsace
    while (isspace(*line)) line++;

    // skip empty line or comment
    while (*line == '\0' || *line == ';' || *line == '#') return true;

    // parse [section]
    if (*line == '[') {
        const char *end = strchr(line, ']');
        if (!end) return false;

        char *section_name = pini__strndup(line+1, end-(line+1));
        ctx->current_section = pini__get_section(ctx->sections, section_name);
        if (ctx->current_section) {
            free(section_name);
            return true;
        }

        Pini_Section section = {
            .name  = section_name,
            .pairs = NULL
        };
        pini__vec_push(ctx->sections, section);
        ctx->current_section = ctx->sections+pini__vec_size(ctx->sections)-1;

        return true;
    }

    // parse key=value
    if (!ctx->current_section) return false;
    const char *equal = strchr(line, '=');
    if (!equal) return false;
    const char *line_end = strrchr(line, '\n');
    if (!line_end) line_end = strrchr(line, '\0');

    const char *key_start = line,    *key_end = equal-1,
               *val_start = equal+1, *val_end = line_end-1;
    while (true) {
        if (isspace(*key_start)) key_start++;
        if (isspace(*key_end)) key_end--;
        if (!isspace(*key_start) && !isspace(*key_end)) break;
        if (key_start > key_end) return false;
    }
    while (true) {
        if (isspace(*val_start)) val_start++;
        if (isspace(*val_end)) val_end--;
        if (!isspace(*val_start) && !isspace(*val_end)) break;
        if (val_start > val_end) return false;
    }

    char *key = pini__strndup(key_start, key_end-key_start+1);
    char *val = pini__strndup(val_start, val_end-val_start+1);
    pini__store_pair(ctx->current_section, key, val);

    free(key);
    free(val);

    return true;
}

void pini_dump(Pini_Context *ctx)
{
    pini__vec_foreach(Pini_Section, ctx->sections, section) {
        printf("[%s]\n", section->name);
        pini__vec_foreach(Pini_Pair, section->pairs, pair) {
            printf("  {\n    key: %s\n", pair->key);
            switch (pair->val.type) {
            case PINI_VALUE_NUMBER:
                printf("    val: %g\n  }\n", pair->val.as.number);
                break;
            case PINI_VALUE_STRING:
                printf("    val: %s\n  }\n", pair->val.as.string);
                break;
            case PINI_VALUE_BOOLEAN:
                printf("    val: %s\n  }\n", pair->val.as.boolean ? "true" : "false");
                break;
            }
        }
    }
}

bool pini_load(Pini_Context *ctx, const char *filename)
{
    char line[1024];
    FILE *f;

    f = fopen(filename, "r");
    if (!f) return false;

    ctx->current_section = NULL;
    ctx->sections = NULL;

    while (fgets(line, sizeof(line), f)) {
        if (!pini__parse_line(ctx, line)) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

void pini_unload(Pini_Context *ctx)
{
    pini__vec_foreach(Pini_Section, ctx->sections, section) {
        if (section->name) free(section->name);
        section->name = NULL;
        pini__vec_foreach(Pini_Pair, section->pairs, pair) {
            if (pair->key) free(pair->key);
            pair->key = NULL;
            switch (pair->val.type) {
            case PINI_VALUE_STRING:
                if (pair->val.as.string) free(pair->val.as.string);
                pair->val.as.string = NULL;
                break;
            default:
                break;
            }
        }
        pini__vec_free(section->pairs);
    }
    pini__vec_free(ctx->sections);
    ctx->current_section = NULL;
}

double pini_get_number(Pini_Context *ctx, const char *section_name, const char *key_name)
{
    Pini_Value *val = pini__get_value(ctx, section_name, key_name);
    assert(val && val->type == PINI_VALUE_NUMBER);
    return val->as.number;
}

const char *pini_get_string(Pini_Context *ctx, const char *section_name, const char *key_name)
{
    Pini_Value *val = pini__get_value(ctx, section_name, key_name);
    assert(val && val->type == PINI_VALUE_STRING);
    return val->as.string;
}

bool pini_get_bool(Pini_Context *ctx, const char *section_name, const char *key_name)
{
    Pini_Value *val = pini__get_value(ctx, section_name, key_name);
    assert(val && val->type == PINI_VALUE_BOOLEAN);
    return val->as.boolean;
}

#undef pini__vec_header
#undef pini__vec_size
#undef pini__vec_capacity
#undef pini__vec_push
#undef pini__vec_free
#undef pini__vec_foreach

#endif // PINI_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under MIT license.
------------------------------------------------------------------------------
Copyright (c) 2025 Dylaris
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
