#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

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
void *pini__vec_grow(void *vec, size_t item_size);

void *pini__vec_grow(void *vec, size_t item_size)
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

typedef struct Pini_Pair {
    char *key;
    char *val;
} Pini_Pair;

typedef struct Pini_Section {
    char *name;
    Pini_Pair *pairs;
} Pini_Section;

typedef struct Pini_Context {
    Pini_Section *current_section;
    Pini_Section *sections;
} Pini_Context;

bool pini__parse_line(Pini_Context *ctx, const char *line)
{
    // skip whitepsace
    while (isspace(*line)) line++;

    // skip empty line or comment
    while (*line == '\0' || *line == ';' || *line == '#') return true;

    // parse [section]
    if (*line == '[') {
        const char *end = strchr(line, ']');
        if (!end) return false;

        Pini_Section section = {
            .name  = strndup(line+1, end-(line+1)),
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

    Pini_Pair pair = {
        .key = strndup(key_start, key_end-key_start+1),
        .val = strndup(val_start, val_end-val_start+1),
    };
    pini__vec_push(ctx->current_section->pairs, pair);

    return true;
}

void pini_dump(Pini_Context *ctx)
{
    pini__vec_foreach(Pini_Section, ctx->sections, section) {
        printf("[%s]\n", section->name);
        pini__vec_foreach(Pini_Pair, section->pairs, pair) {
            printf("  key: %s\n", pair->key);
            printf("  val: %s\n", pair->val);
        }
    }
}

bool pini_load(Pini_Context *ctx, const char *filename)
{
    char line[1024];
    FILE *f;

    f = fopen(filename, "r");
    if (!f) return false;

    while (fgets(line, sizeof(line), f)) {
        if (!pini__parse_line(ctx, line)) return false;
    }

    return true;
}

void pini_unload(Pini_Context *ctx)
{
    pini__vec_foreach(Pini_Section, ctx->sections, section) {
        // TODO: free section
        pini__vec_foreach(Pini_Pair, section->pairs, pair) {
            // TODO: free pair
        }
    }
}

int main(void)
{
    Pini_Context ctx;
    if (!pini_load(&ctx, "config.ini")) return 1;
    pini_dump(&ctx);
    pini_unload(&ctx);
    return 0;
}
