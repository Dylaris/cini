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

typedef enum Pini_Token_Type {
    PINI_TOKEN_IDENTIFIER,
    PINI_TOKEN_STRING, PINI_TOKEN_NUMBER,
    PINI_TOKEN_TRUE, PINI_TOKEN_FALSE,

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
    bool had_error;
} _parser;

typedef enum Pini_Value_Type {
    PINI_VALUE_NUMBER,
    PINI_VALUE_STRING,
    PINI_VALUE_BOOLEAN,
    PINI_VALUE_ARRAY
} Pini_Value_Type;

typedef struct Pini_Value Pini_Value;
struct Pini_Value {
    Pini_Value_Type type;
    union {
        double number;
        char *string;
        bool boolean;
        Pini_Value *array;
    } as;
};

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
    static char message[256];

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

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
        pini__lexer_advance(); // consume '.'
        while (pini__lexer_is_digit(pini__lexer_peek())) {
            pini__lexer_advance();
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
    pini__lexer_advance(); // consume right '"'

    return pini__lexer_make_token(PINI_TOKEN_STRING);
}

Pini_Token pini__lexer_scan_identifier(void)
{
    while (pini__lexer_is_alpha(pini__lexer_peek()) ||
           pini__lexer_is_digit(pini__lexer_peek())) {
        pini__lexer_advance();
    }
    Pini_Token token = pini__lexer_make_token(PINI_TOKEN_IDENTIFIER);
    if (token.length == 4 && memcmp(token.start, "true", 4)  == 0) token.type = PINI_TOKEN_TRUE;
    if (token.length == 5 && memcmp(token.start, "false", 5) == 0) token.type = PINI_TOKEN_FALSE;
    return token;
}

void pini__lexer_skip_whitespace(void)
{
    for (;;) {
        char c = pini__lexer_peek();
        switch (c) {
        // whitesapce
        case ' ':
        case '\r':
        case '\t':
            pini__lexer_advance();
            break;
        case '\n':
            _lexer.line++;
            pini__lexer_advance();
            break;
        // comment
        case ';':
        case '#':
            while (!pini__lexer_at_end() && pini__lexer_peek() != '\n') pini__lexer_advance();
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

    // literal
    if (pini__lexer_is_digit(c) || (c == '-' && pini__lexer_is_digit(pini__lexer_peek()))) {
        return pini__lexer_scan_number();
    }
    if (pini__lexer_is_alpha(c)) return pini__lexer_scan_identifier();
    if (c == '"') return pini__lexer_scan_string();

    // punctuation
    switch (c) {
    case '[': return pini__lexer_make_token(PINI_TOKEN_LEFT_SQUARE_BRACKET);
    case ']': return pini__lexer_make_token(PINI_TOKEN_RIGHT_SQUARE_BRACKET);
    case '=': return pini__lexer_make_token(PINI_TOKEN_EQUAL);
    case ',': return pini__lexer_make_token(PINI_TOKEN_COMMA);
    default:  return pini__lexer_error_token("unknown character: '%c'", c);
    }
}

static inline void pini__parser_error(const char *message, size_t line)
{
    if (_parser.had_error) return;
    fprintf(stderr, "ERROR: %s at line %zu\n", message, line);
    _parser.had_error = true;
}

static inline void pini__parser_advance(void)
{
    _parser.previous = _parser.current;
    _parser.current  = pini__lexer_scan_token();
    if (_parser.current.type == PINI_TOKEN_ERROR) {
        pini__parser_error(_parser.current.start, _parser.current.line);
    }
}

bool pini__parser_consume(Pini_Token_Type type, const char *message)
{
    if (_parser.current.type != type) {
        pini__parser_error(message, _parser.current.line);
        return false;
    }
    pini__parser_advance();
    return true;
}

bool pini__parser_consume_value(void)
{
    switch (_parser.current.type) {
    case PINI_TOKEN_STRING:
    case PINI_TOKEN_NUMBER:
    case PINI_TOKEN_TRUE:
    case PINI_TOKEN_FALSE:
        pini__parser_advance();
        return true;
    default:
        pini__parser_error("invalid value", _parser.current.line);
        return false;
    }
}

static inline Pini_Token_Type pini__parser_peek(void)
{
    return _parser.current.type;
}

static inline bool pini__parser_at_end(void)
{
    return _parser.current.type == PINI_TOKEN_EOF;
}

void pini__parser_init(void)
{
    _parser.previous.line = 0;
    _parser.current       = pini__lexer_scan_token();
    _parser.had_error     = false;

    if (_parser.current.type == PINI_TOKEN_ERROR) {
        pini__parser_error(_parser.current.start, _parser.current.line);
    }
}

char *pini__strdup(const char *s, size_t len)
{
    char *buffer = malloc(len + 1);
    assert(buffer != NULL);
    memcpy(buffer, s, len);
    buffer[len] = '\0';
    return buffer;
}

void pini__free_value(Pini_Value *value)
{
    switch (value->type) {
    case PINI_VALUE_STRING:
        if (value->as.string) free(value->as.string);
        value->as.string = NULL;
        break;
    case PINI_VALUE_ARRAY:
        if (value->as.array) {
            pini__vec_foreach(Pini_Value, value->as.array, iter) {
                pini__free_value(iter);
            }
            pini__vec_free(value->as.array);
        }
        break;
    default:
        break;
    }
}

void pini__free_entry(Pini_Entry *entry)
{
    if (entry->key) free(entry->key);
    entry->key = NULL;
    pini__free_value(&entry->value);
}

void pini__free_section(Pini_Section *section)
{
    if (section->name) free(section->name);
    section->name = NULL;
    pini__vec_foreach(Pini_Entry, section->entries, entry) {
        pini__free_entry(entry);
    }
    pini__vec_free(section->entries);
}

bool pini__parser_parse_value(Pini_Value *value);

bool pini__parser_parse_array(Pini_Value *value)
{
    // Empty array
    if (pini__parser_peek() == PINI_TOKEN_RIGHT_SQUARE_BRACKET) {
        pini__parser_advance();
        return true;
    }

    while (true) {
        Pini_Value item;
        if (!pini__parser_parse_value(&item)) return false;
        pini__vec_push(value->as.array, item);
        if (pini__parser_peek() != PINI_TOKEN_COMMA) break;
        pini__parser_advance();
        if (pini__parser_peek() == PINI_TOKEN_RIGHT_SQUARE_BRACKET) break;
    }
    if (!pini__parser_consume(PINI_TOKEN_RIGHT_SQUARE_BRACKET, "array should end with ']'")) return false;

    return true;
}

bool pini__parser_parse_value(Pini_Value *value)
{
    if (_parser.had_error) return false;

    pini__parser_advance(); // it may be a error token
    if (_parser.had_error) return false;

    switch (_parser.previous.type) {
    case PINI_TOKEN_STRING:
        value->type = PINI_VALUE_STRING;
        value->as.string = pini__strdup(_parser.previous.start+1, _parser.previous.length-2);
        return true;

    case PINI_TOKEN_NUMBER:
        // FIXME: strtod may overflow
        value->type = PINI_VALUE_NUMBER;
        char *tmp = malloc(_parser.previous.length + 1);
        assert(tmp != NULL);
        memcpy(tmp, _parser.previous.start, _parser.previous.length);
        tmp[_parser.previous.length] = '\0';
        value->as.number = strtod(tmp, NULL);
        free(tmp);
        return true;

    case PINI_TOKEN_TRUE:
        value->type = PINI_VALUE_BOOLEAN;
        value->as.boolean = true;
        return true;

    case PINI_TOKEN_FALSE:
        value->type = PINI_VALUE_BOOLEAN;
        value->as.boolean = false;
        return true;

    case PINI_TOKEN_LEFT_SQUARE_BRACKET:
        value->type = PINI_VALUE_ARRAY;
        value->as.array = NULL; // initialzed array for 'pini__vec_push'
        return pini__parser_parse_array(value);

    default:
        pini__parser_error("invalid value", _parser.previous.line);
        return false;
    }
}

bool pini__parser_parse_pair(Pini_Section *section)
{
    Pini_Entry entry;

    if (_parser.had_error) return false;

    if (!pini__parser_consume(PINI_TOKEN_IDENTIFIER, "invalid key")) return false;
    entry.key = pini__strdup(_parser.previous.start, _parser.previous.length);
    if (!pini__parser_consume(PINI_TOKEN_EQUAL, "it should be '=' after the key")) return false;
    if (!pini__parser_parse_value(&entry.value)) return false;
    pini__vec_push(section->entries, entry);

    return true;
}

bool pini__parser_parse_section(Pini_Context *ctx)
{
    Pini_Section section = {.entries = NULL}; // initialize array for 'pini__vec_push'

    if (_parser.had_error) return false;

    if (!pini__parser_consume(PINI_TOKEN_IDENTIFIER, "invalid section name after '['")) return false;
    section.name = pini__strdup(_parser.previous.start, _parser.previous.length);
    if (!pini__parser_consume(PINI_TOKEN_RIGHT_SQUARE_BRACKET, "']' should be after section name")) return false;

    while (!pini__parser_at_end() && pini__parser_peek() != PINI_TOKEN_LEFT_SQUARE_BRACKET &&
            pini__parser_peek() != PINI_TOKEN_ERROR) {
        if (!pini__parser_parse_pair(&section)) return false;
    }
    pini__vec_push(ctx->sections, section);

    return true;
}

bool pini__parser_parse(Pini_Context *ctx)
{
    while (1) {
        if (pini__parser_consume(PINI_TOKEN_LEFT_SQUARE_BRACKET, "start a section with [...]")) {
            if (!pini__parser_parse_section(ctx)) return false;
        } else {
            return false;
        }
        if (pini__parser_at_end()) break;
    }
    return true;
}

bool pini_load(Pini_Context *ctx, const char *filename)
{
    ctx->source = read_entire_file(filename);
    if (!ctx->source) return false;
    ctx->sections = NULL;

    pini__lexer_init(ctx->source);
#if 0
    while (1) {
        Pini_Token token = pini__lexer_scan_token();
        printf("type: %d, value: %.*s\n", token.type, (int)token.length, token.start);
        if (token.type == PINI_TOKEN_EOF) break;
    }
    return true;
#else
    pini__parser_init();
    bool status = pini__parser_parse(ctx);
    if (!status) return false;
    pini__vec_foreach(Pini_Section, ctx->sections, section) {
        printf("name: %s\n", section->name);
        pini__vec_foreach(Pini_Entry, section->entries, entry) {
            printf("key: %s\n", entry->key);
            switch (entry->value.type) {
            case PINI_VALUE_NUMBER:
                printf("val: %g\n", entry->value.as.number);
                break;
            case PINI_VALUE_STRING:
                printf("val: %s\n", entry->value.as.string);
                break;
            case PINI_VALUE_BOOLEAN:
                printf("val: %s\n", entry->value.as.boolean ? "true" : "false");
                break;
            case PINI_VALUE_ARRAY:
                printf("val: %zu\n", pini__vec_size(entry->value.as.array));
                break;
            }
        }
    }
    return true;
#endif
}

void pini_unload(Pini_Context *ctx)
{
    pini__vec_foreach(Pini_Section, ctx->sections, section) {
        pini__free_section(section);
    }
    pini__vec_free(ctx->sections);
    if (ctx->source) free(ctx->source);
    ctx->source = NULL;
}

int main(void)
{
    Pini_Context ctx;
    if (!pini_load(&ctx, "config.ini")) return 1;
    pini_unload(&ctx);
    return 0;
}
