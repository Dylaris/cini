# pini.h

## Brief

Parser of 'ini' file implemented with C99 as a stb-style library.

## Usage

```c
    pini_load(&ctx, "config.ini");
    const char *name = pini_get_string(&ctx, "profile", "name");
    size_t age       = (size_t)pini_get_number(&ctx, "profile", "age");
    bool is_student  = pini_get_bool(&ctx, "profile", "is_student");
    pini_unload(&ctx);
```
