#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

char *read_entire_file(const char *filename)
{
#define return_defer(code) do {status = code; goto defer;} while (0)
    FILE *f = NULL;
    int status = 0;
    char *buffer = NULL;
    size_t size;

    f = fopen(filename, "r");
    if (!f) return_defer(1);

    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    rewind(f);

    buffer = malloc(size+1);
    if (!buffer) return_defer(1);

    if (fread(buffer, size, 1, f) != 1) return_defer(1);
    buffer[size] = '\0';

defer:
    if (f) fclose(f);
    if (status != 0) {
        if (buffer) free(buffer);
        return NULL;
    } else {
        return buffer;
    }
#undef return_defer
}

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
#define pini__vec_pop(vec)     ((vec)[--pini__vec_header(vec)->size])
#define pini__vec_isempty(vec) ((vec) ? pini__vec_size(vec) == 0 : false)
#define pini__vec_free(vec)                   \
    do {                                      \
        if (vec) free(pini__vec_header(vec)); \
        (vec) = NULL;                         \
    } while (0)
#define pini__vec_reset(vec)                      \
    do {                                          \
        if (vec) pini__vec_header(vec)->size = 0; \
    } while (0)
#define pini__vec_foreach(type, vec, it) for (type *it = (vec); it < (vec)+pini__vec_size(vec); it++)
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

typedef enum Pini_Token_Type {
    PINI_TOKEN_IDENTIFIER,
    PINI_TOKEN_STRING,
    PINI_TOKEN_NUMBER,

    PINI_TOKEN_LEFT_SQUARE_BRACKET,
    PINI_TOKEN_RIGHT_SQUARE_BRACKET,
    PINI_TOKEN_EQUAL, PINI_TOKEN_COMMA,

    PINI_TOKEN_ERROR, PINI_TOKEN_EOF
} Pini_Token_Type;

typedef struct Pini_Token {
    Pini_Token_Type type;
    const char *start;
    size_t length;
    size_t line;
} Pini_Token;

static struct {
    const char *start;
    const char *current;
    size_t line;
} _lexer;

static struct {
    Pini_Token previous;
    Pini_Token current;
} _parser;

#define PINI_MAX_SECTION_NAME_SIZE 64

typedef enum Pini_Value_Type {
    PINI_VALUE_NUMBER,
    PINI_VALUE_STRING,
    PINI_VALUE_BOOLEAN,
    PINI_VALUE_ARRAY
} Pini_Value_Type;

typedef struct Pini_Value {
    Pini_Value_Type type;
    union {
        int integer;
        char *string;
        bool boolean;
    } as;
} Pini_Value;

typedef struct Pini_Entry {
    char *key;
    Pini_Value value;
} Pini_Entry;

typedef struct Pini_Section {
    char *name;
    Pini_Entry *entries;
} Pini_Section;

typedef struct Pini_Context {
    char *source;
    Pini_Section *sections;
} Pini_Context;

void pini__lexer_init(const char *source)
{
    _lexer.start   = source;
    _lexer.current = source;
    _lexer.line    = 1;
}

static inline char pini__lexer_advance(void)
{
    return *_lexer.current++;
}

static inline char pini__lexer_peek(void)
{
    return *_lexer.current;
}

static inline bool pini__lexer_at_end(void)
{
    return *_lexer.current == '\0';
}

static inline char pini__lexer_peek_next(void)
{
    if (pini__lexer_at_end()) return '\0';
    return *(_lexer.current + 1);
}

static inline bool pini__lexer_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_' || c == '.';
}

static inline bool pini__lexer_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool pini__lexer_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == ';';
}

Pini_Token pini__lexer_make_token(Pini_Token_Type type)
{
    return (Pini_Token){
        .type   = type,
        .start  = _lexer.start,
        .length = _lexer.current - _lexer.start,
        .line   = _lexer.line
    };
}

Pini_Token pini__lexer_error_token(const char *fmt, ...)
{
    va_list args;
    static char tmp[256];
    static char message[256];

    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    snprintf(message, sizeof(message), "ERROR: %s at line %zu", tmp, _lexer.line);

    return (Pini_Token){
        .type   = PINI_TOKEN_ERROR,
        .start  = message,
        .length = strlen(message),
        .line   = _lexer.line
    };
}

Pini_Token pini__lexer_scan_number(void)
{
    while (pini__lexer_is_digit(pini__lexer_peek())) {
        pini__lexer_advance();
    }

    if (pini__lexer_peek() == '.' &&
        pini__lexer_is_digit(pini__lexer_peek_next())) {
        pini__lexer_advance();
        while (pini__lexer_is_digit(pini__lexer_peek())) {
            pini__lexer_advance();
        }
        while (1) {
            char c = pini__lexer_peek();
            if (pini__lexer_is_digit(c)) {
                pini__lexer_advance();
            } else if (pini__lexer_is_whitespace(c)) {
                break;
            } else {
                return pini__lexer_error_token("invalid number");
            }
        }
    }

    return pini__lexer_make_token(PINI_TOKEN_NUMBER);
}

Pini_Token pini__lexer_scan_string(void)
{
    while (pini__lexer_peek() != '"' && !pini__lexer_at_end()) {
        if (pini__lexer_peek() == '\n') _lexer.line++;
        pini__lexer_advance();
    }
    if (pini__lexer_at_end()) {
        return pini__lexer_error_token("unterminated string");
    }

    pini__lexer_advance();
    return pini__lexer_make_token(PINI_TOKEN_STRING);
}

Pini_Token pini__lexer_scan_identifier(void)
{
    while (pini__lexer_is_alpha(pini__lexer_peek()) ||
           pini__lexer_is_digit(pini__lexer_peek())) {
        pini__lexer_advance();
    }
    return pini__lexer_make_token(PINI_TOKEN_IDENTIFIER);
}

void pini__lexer_skip_whitespace(void)
{
    for (;;) {
        char c = pini__lexer_peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            pini__lexer_advance();
            break;
        case '\n':
            _lexer.line++;
            pini__lexer_advance();
            break;
        default:
            return;
        }
    }
}

Pini_Token pini__lexer_scan_token(void)
{
    pini__lexer_skip_whitespace();

    _lexer.start = _lexer.current;
    if (pini__lexer_at_end()) return pini__lexer_make_token(PINI_TOKEN_EOF);

    char c = pini__lexer_advance();
    if (pini__lexer_is_digit(c)) return pini__lexer_scan_number();
    if (pini__lexer_is_alpha(c)) return pini__lexer_scan_identifier();

    switch (c) {
    case '[': return pini__lexer_make_token(PINI_TOKEN_LEFT_SQUARE_BRACKET);
    case ']': return pini__lexer_make_token(PINI_TOKEN_RIGHT_SQUARE_BRACKET);
    case '=': return pini__lexer_make_token(PINI_TOKEN_EQUAL);
    case ',': return pini__lexer_make_token(PINI_TOKEN_COMMA);
    case '"': return pini__lexer_scan_string();
    default:  return pini__lexer_error_token("unknown character: '%c'", c);
    }
}

void pini__parser_init(void)
{
    _parser.previous = (Pini_Token){ .line = 0 };
    _parser.current  = pini__lexer_scan_token();
}

void pini__parser_parse_value(void)
{
}

void pini__parser_parse_pair(void)
{
}

void pini__parser_parse_section(Pini_Context *ctx)
{
}

void pini__parser_parse(Pini_Context *ctx)
{
    while (_parser.current.type != PINI_TOKEN_EOF &&
           _parser.current.type != PINI_TOKEN_ERROR) {
        pini__parser_parse_section(ctx);
    }
}

bool pini_load(Pini_Context *ctx, const char *filename)
{
    ctx->source = read_entire_file(filename);
    if (!ctx->source) return false;
    ctx->sections = NULL;

    pini__lexer_init(ctx->source);
    pini__parser_init();

    while (1) {
        Pini_Token token = pini__lexer_scan_token();
        printf("type: %d, value: %.*s\n", token.type, (int)token.length, token.start);
        if (token.type == PINI_TOKEN_EOF) break;
    }

    // pini__parser_parse(ctx);

    return true;
}

void pini_unload(Pini_Context *ctx)
{
    if (ctx->source) free(ctx->source);
}

int main(void)
{
    Pini_Context ctx;
    if (!pini_load(&ctx, "config.ini")) return 1;
    pini_unload(&ctx);
    return 0;
}
