# pini.h

## Brief

Parser of 'ini' file implemented with C99 as a stb-style library.

## Usage

```c
    Pini_Context ctx;
    pini_load(&ctx, "config.ini");

    Pini_Query query = {.section = "profile"};

    query.key = "name";
    const char *name = pini_get_string(&ctx, &query);
    if (query->errcode != PINI_OK) {...}

    query.key = "age";
    size_t age = pini_get_number(&ctx, &query);

    query.key = "flag";
    bool flag = pini_get_boolean(&ctx, &query);

    pini_unload(&ctx);
```
