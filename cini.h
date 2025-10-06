/*
cini.h - v0.01 - Dylaris 2025
===================================================

BRIEF:
  Read/Write ini file in C.

DEPENDENCY:
  This library uses data structures from 'aris.h'. For convenience, these
  are integrated into this file. If you already have 'aris.h' in your project,
  define ARIS_H/ARIS_IMPLEMENTATION before inclusion to use your existing
  implementation.

NOTE:
  Not compatible with C++. (no test)

USAGE:
  In exactly one source file, define the implementation macro
  before including this header:
  ```
    #define CINI_IMPLEMENTATION
    #include "cini.h"
  ```
  In other files, just include the header without the macro.

LICENSE:
  See the end of this file for further details.
*/

#ifndef CINI_H
#define CINI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

///////////////////
// GOTO CINI_START

#if !defined(ARIS_H)

/*
 * memory layout
 */
#define aris_offset_of(type, member) ((size_t)&(((type*)0)->member))
#define aris_container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - aris_offset_of(type, member)))

/*
 * dynamic array
 */
typedef struct Aris_Vector_Header {
    size_t size;
    size_t capacity;
} Aris_Vector_Header;

#define aris_vec_header(vec)   ((Aris_Vector_Header*)((char*)(vec) - sizeof(Aris_Vector_Header)))
#define aris_vec_size(vec)     ((vec) ? aris_vec_header(vec)->size : 0)
#define aris_vec_capacity(vec) ((vec) ? aris_vec_header(vec)->capacity : 0)
#define aris_vec_push(vec, item)                               \
    do {                                                       \
        if (aris_vec_size(vec) + 1 > aris_vec_capacity(vec)) { \
            (vec) = aris_vec_grow(vec, sizeof(*vec));          \
        }                                                      \
        (vec)[aris_vec_header(vec)->size++] = (item);          \
    } while (0)
#define aris_vec_free(vec)                   \
    do {                                     \
        if (vec) free(aris_vec_header(vec)); \
        (vec) = NULL;                        \
    } while (0)
#define aris_vec_foreach(type, vec, iter) for (type *iter = (vec); iter < (vec)+aris_vec_size(vec); iter++)
void *aris_vec_grow(void *vec, size_t item_size);

/*
 * mini hash table
 */
typedef struct Aris_Mini_Hash_Bucket {
    uint32_t key;
    uint32_t val;
} Aris_Mini_Hash_Bucket;

typedef struct Aris_Mini_Hash {
    uint32_t count;
    uint32_t capacity;
    Aris_Mini_Hash_Bucket *buckets;
} Aris_Mini_Hash;

#define ARIS_MINI_HASH_MAX_LOAD 0.75

uint32_t fnv_hash(const char *str, size_t length);
bool aris_hash_expand(Aris_Mini_Hash *lookup, size_t new_capacity);
bool aris_hash_set(Aris_Mini_Hash *lookup, uint32_t key, uint32_t val);
bool aris_hash_get(Aris_Mini_Hash *lookup, uint32_t key, uint32_t *out_val);
void aris_hash_free(Aris_Mini_Hash *lookup);

/*
 * file
 */
typedef enum Aris_File_Type {
    ARIS_TEXT_FILE,
    ARIS_BINARY_FILE
} Aris_File_Type;

typedef struct Aris_File_Line {
    const char *start;
    size_t length;
    size_t number;
} Aris_File_Line;

typedef struct Aris_File_Header {
    size_t size;
    Aris_File_Line *lines;
    char source[];
} Aris_File_Header;

#define aris_file_header(file)   aris_container_of(file, Aris_File_Header, source)
#define aris_file_size(file)     ((file) ? aris_file_header(file)->size : 0)
#define aris_file_lines(file) ((file) ? aris_file_header(file)->lines : NULL)
#define aris_file_free(file)                            \
    do {                                                \
        if (!(file)) break;                             \
        Aris_File_Header *hdr = aris_file_header(file); \
        if (hdr->lines) aris_vec_free(hdr->lines);      \
        free(hdr);                                      \
        (file) = NULL;                                  \
    } while (0)
#define aris_file_nlines(file) aris_vec_size(aris_file_lines(file))
#define aris_file_foreach(file, iter) \
    for (Aris_File_Line *iter = aris_file_lines(file); iter < aris_file_lines(file)+aris_file_nlines(file); iter++)

const char *aris_file_read(const char *filename, Aris_File_Type type);
void aris_file_split(const char *source);

#endif // !defined(ARIS_H)

///////////////////
// CINI_START

typedef struct Cini_String {
    const char *start;
    size_t length;
} Cini_String;

typedef struct Cini_Pair {
    Cini_String key;
    Cini_String val;
} Cini_Pair;

typedef struct Cini_Section {
    Cini_String name;
    // hash table: key name => kv pair
    Cini_Pair *pairs;
    Aris_Mini_Hash pair_lookup;
} Cini_Section;

typedef struct Cini_Context {
    const char *source;
    Cini_Section *current_section;
    // hash table: section name => section
    Cini_Section *sections;
    Aris_Mini_Hash section_lookup;
} Cini_Context;

bool cini_load(Cini_Context *ctx, const char *filename);
Cini_String cini_get(Cini_Context *ctx, const char *section_name, const char *key_name);
void cini_set(Cini_Context *ctx, const char *section, const char *key, const char *val);
void cini_dump(Cini_Context *ctx, FILE *file);
void cini_free(Cini_Context *ctx);
bool cini_generate_file(Cini_Context *ctx, const char *filename);
#define cini_ok(cs) ((cs).start != NULL)

#endif // CINI_H

#ifdef CINI_IMPLEMENTATION

#if !defined(ARIS_IMPLEMENTATION)

void *aris_vec_grow(void *vec, size_t item_size)
{
    size_t new_capacity, alloc_size;
    Aris_Vector_Header *new_header;

    new_capacity = aris_vec_capacity(vec) < 16
                   ? 16 : 2 * aris_vec_capacity(vec);
    alloc_size = sizeof(Aris_Vector_Header) + new_capacity*item_size;

    if (vec) {
        new_header = realloc(aris_vec_header(vec), alloc_size);
        assert(new_header != NULL && "out of memory");
    } else {
        new_header = malloc(alloc_size);
        assert(new_header != NULL && "out of memory");
        new_header->size = 0;
    }
    new_header->capacity = new_capacity;

    return (void*)((char*)new_header + sizeof(Aris_Vector_Header));
}

static Aris_Mini_Hash_Bucket *aris_hash__find_bucket(Aris_Mini_Hash_Bucket *buckets, uint32_t capacity, uint32_t key)
{
    uint32_t index = key % capacity;
    uint32_t start_index = index;

    for (;;) {
        if (buckets[index].key == key || buckets[index].key == 0) return &buckets[index];
        index = (index + 1) % capacity;
        if (index == start_index) return NULL;
    }
}

uint32_t fnv_hash(const char *str, size_t length)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}

bool aris_hash_expand(Aris_Mini_Hash *lookup, size_t new_capacity)
{
    if (new_capacity <= lookup->capacity) return false;

    Aris_Mini_Hash_Bucket *new_buckets = malloc(sizeof(Aris_Mini_Hash_Bucket)*new_capacity);
    if (!new_buckets) return false;
    for (uint32_t i = 0; i < new_capacity; i++) new_buckets[i].key = 0;

    for (uint32_t i = 0; i < lookup->capacity; i++) {
        Aris_Mini_Hash_Bucket *bucket = &lookup->buckets[i];
        if (bucket->key == 0) continue;
        Aris_Mini_Hash_Bucket *dest = aris_hash__find_bucket(new_buckets, new_capacity, bucket->key);
        dest->key = bucket->key;
        dest->val = bucket->val;
    }

    lookup->capacity = new_capacity;
    if (lookup->buckets) free(lookup->buckets);
    lookup->buckets = new_buckets;

    return true;
}

bool aris_hash_set(Aris_Mini_Hash *lookup, uint32_t key, uint32_t val)
{
    if (lookup->count + 1 > lookup->capacity * ARIS_MINI_HASH_MAX_LOAD) {
        uint32_t new_capacity = lookup->capacity < 16 ? 16 : 2*lookup->capacity;
        if (!aris_hash_expand(lookup, new_capacity)) return false;
    }

    Aris_Mini_Hash_Bucket *bucket = aris_hash__find_bucket(lookup->buckets, lookup->capacity, key);
    if (!bucket) return false;

    bucket->key = key;
    bucket->val = val;
    lookup->count++;

    return true;
}

bool aris_hash_get(Aris_Mini_Hash *lookup, uint32_t key, uint32_t *out_val)
{
    if (lookup->count == 0) return false;

    Aris_Mini_Hash_Bucket *bucket = aris_hash__find_bucket(lookup->buckets, lookup->capacity, key);
    if (!bucket || bucket->key != key) return false;

    if (out_val) *out_val = bucket->val;
    return true;
}

void aris_hash_free(Aris_Mini_Hash *lookup)
{
    if (lookup->buckets) free(lookup->buckets);
    lookup->buckets = NULL;
    lookup->capacity = 0;
    lookup->count = 0;
}

const char *aris_file_read(const char *filename, Aris_File_Type type)
{
#define return_defer(flag) do { ok = flag; goto defer; } while (0)
    FILE *f = NULL;
    Aris_File_Header *header = NULL;
    const char *mode; /* file open mode */
    long size;
    bool ok = true;

    switch (type) {
    case ARIS_TEXT_FILE:   mode = "r";  break;
    case ARIS_BINARY_FILE: mode = "rb"; break;
    default:               return NULL;
    }

    f = fopen(filename, mode);
    if (!f) return NULL;

    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    rewind(f);

    header = malloc(sizeof(Aris_File_Header) + size + 1);
    if (!header) return_defer(false);
    header->size = size;
    header->lines = NULL;

    if (fread(header->source, size, 1, f) != 1) return_defer(false);
    header->source[size] = '\0';

defer:
    if (f) fclose(f);
    if (!ok) {
        if (header) free(header);
        return NULL;
    }
    return header->source;
#undef return_defer
}

void aris_file_split(const char *source)
{
    const char *start = source;
    const char *current = start;
    size_t line_number = 0;
    Aris_File_Line *lines = aris_file_header(source)->lines;

    while (*current) {
        if (*current == '\n') {
            Aris_File_Line line = {
                .start  = start,
                .length = current - start,
                .number = ++line_number
            };
            aris_vec_push(lines, line);
            start = current + 1;
        }
        current++;
    }

    aris_file_header(source)->lines = lines;
}

#endif // !defined(ARIS_IMPLEMENTATION)

///////////////////
// CINI_START

static const char *cini__strnchr(const char *start, size_t length, char c)
{
    const char *current = start;
    while (current - start != length) {
        if (*current == c) return current;
        current++;
    }
    return NULL;
}

static bool cini__parse_line(Cini_Context *ctx, const char *start, size_t length)
{
    const char *current = start;

    // skip whitepsace
    while (isspace(*current) || current - start == length) current++;

    // skip empty line or comment
    if (current - start == length || *current == ';' || *current == '#') return true;

    // parse [section]
    if (*current == '[') {
        const char *end = cini__strnchr(current, length-(current-start), ']');
        if (!end) return false;

        Cini_String section_name = {
            .start = current + 1,
            .length = end - (current + 1)
        };
        uint32_t aris_hash = fnv_hash(section_name.start, section_name.length);

        // check if section exists
        if (aris_hash_get(&ctx->section_lookup, aris_hash, NULL)) return true;

        Cini_Section section = {
            .name  = section_name,
            .pairs = NULL,
            .pair_lookup = (Aris_Mini_Hash){0}
        };
        aris_vec_push(ctx->sections, section);
        uint32_t index = (uint32_t)(aris_vec_size(ctx->sections) - 1);
        aris_hash_set(&ctx->section_lookup, aris_hash, index);
        ctx->current_section = &ctx->sections[index];

        return true;
    }

    // parse key=value
    if (!ctx->current_section) return false;
    const char *equal = cini__strnchr(current, length-(current-start), '=');
    if (!equal) return false;
    const char *line_end = start + length;

    // trim whitespace
    const char *key_start = current, *key_end = equal-1,
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

    Cini_String key = {
        .start  = key_start,
        .length = key_end - key_start + 1
    };
    Cini_String val = {
        .start  = val_start,
        .length = val_end - val_start + 1
    };
    uint32_t aris_hash = fnv_hash(key.start, key.length);
    uint32_t index;
    Cini_Section *section = ctx->current_section;

    // check if pair exists
    if (aris_hash_get(&section->pair_lookup, aris_hash, &index)) {
        Cini_Pair *pair = &section->pairs[index];
        pair->val = val;
        return true;
    }

    // add a new pair
    Cini_Pair pair = {
        .key = key,
        .val = val
    };
    aris_vec_push(section->pairs, pair);
    index = (uint32_t)(aris_vec_size(section->pairs) - 1);
    aris_hash_set(&section->pair_lookup, aris_hash, index);

    return true;
}

static void cini_set_section(Cini_Context *ctx, const char *name)
{
    uint32_t index;
    uint32_t aris_hash = fnv_hash(name, strlen(name));

    // check if the section exists
    if (aris_hash_get(&ctx->section_lookup, aris_hash, &index)) {
        ctx->current_section = &ctx->sections[index];
        return;
    }

    // add a new section
    Cini_Section section = {
        .name        = (Cini_String){name, strlen(name)},
        .pairs       = NULL,
        .pair_lookup = (Aris_Mini_Hash){0}
    };
    aris_vec_push(ctx->sections, section);
    index = (uint32_t)(aris_vec_size(ctx->sections) - 1);
    aris_hash_set(&ctx->section_lookup, aris_hash, index);
    ctx->current_section = &ctx->sections[index];
}

static void cini_set_pair(Cini_Context *ctx, const char *key, const char *val)
{
    if (!ctx->current_section) return;

    uint32_t index;
    uint32_t aris_hash = fnv_hash(key, strlen(key));
    Cini_Section *section = ctx->current_section;

    // check if the pair exists
    if (aris_hash_get(&section->pair_lookup, aris_hash, &index)) {
        Cini_Pair *pair = &section->pairs[index];
        pair->val = (Cini_String){val, strlen(val)};
        return;
    }

    // add a new pair
    Cini_Pair pair = {
        .key = (Cini_String){key, strlen(key)},
        .val = (Cini_String){val, strlen(val)}
    };
    aris_vec_push(section->pairs, pair);
    index = (uint32_t)(aris_vec_size(section->pairs) - 1);
    aris_hash_set(&section->pair_lookup, aris_hash, index);
}

void cini_dump(Cini_Context *ctx, FILE *file)
{
    aris_vec_foreach(Cini_Section, ctx->sections, section) {
        fprintf(file, "[%.*s]\n", (int)section->name.length, section->name.start);
        aris_vec_foreach(Cini_Pair, section->pairs, pair) {
            fprintf(file, "  %.*s = %.*s\n",
                    (int)pair->key.length, pair->key.start,
                    (int)pair->val.length, pair->val.start);
        }
        if (section != ctx->sections+aris_vec_size(ctx->sections)-1) fprintf(file, "\n");
    }
}

void cini_free(Cini_Context *ctx)
{
    if (ctx->source) aris_file_free(ctx->source);
    aris_vec_foreach(Cini_Section, ctx->sections, section) {
        aris_hash_free(&section->pair_lookup);
        aris_vec_free(section->pairs);
    }
    aris_hash_free(&ctx->section_lookup);
    aris_vec_free(ctx->sections);
}

bool cini_load(Cini_Context *ctx, const char *filename)
{
    ctx->source = aris_file_read(filename, ARIS_TEXT_FILE);
    if (!ctx->source) return false;
    ctx->current_section = NULL;
    ctx->sections = NULL;
    ctx->section_lookup = (Aris_Mini_Hash){0};

    aris_file_split(ctx->source);

    aris_file_foreach(ctx->source, line) {
        if (!cini__parse_line(ctx, line->start, line->length)) return false;
    }

    return true;
}

bool cini_generate_file(Cini_Context *ctx, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) return false;

    cini_dump(ctx, f);

    fclose(f);
    return true;
}

Cini_String cini_get(Cini_Context *ctx, const char *section_name, const char *key_name)
{
    uint32_t aris_hash;
    uint32_t index;
    Cini_Section *section;
    Cini_Pair *pair;

    aris_hash = fnv_hash(section_name, strlen(section_name));
    if (!aris_hash_get(&ctx->section_lookup, aris_hash, &index)) return (Cini_String){0};
    section = &ctx->sections[index];
    if (!key_name) return section->name;

    aris_hash = fnv_hash(key_name, strlen(key_name));
    if (!aris_hash_get(&section->pair_lookup, aris_hash, &index)) return (Cini_String){0};
    pair = &section->pairs[index];

    return pair->val;
}

void cini_set(Cini_Context *ctx, const char *section, const char *key, const char *val)
{
    if (section) cini_set_section(ctx, section);
    if (key && val) cini_set_pair(ctx, key, val);
}

#endif // CINI_IMPLEMENTATION

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
