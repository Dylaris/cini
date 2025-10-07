# cini

## Brief

Write/Read 'ini' file implemented with C99 as a stb-style library.

## Usage

- read

```c
    Cini_Context ctx = {0};
    cini_load(&ctx, "config.ini");
    Cini_String val = cini_get(&ctx, "aaa", "name");
    if (cini_ok(val)) printf("[aaa] name = %.*s\n", (int)val.length, val.start);
```

- write

```c
    Cini_Context ctx = {0};
    cini_set(&ctx, "aaa", "name", "aris");
    cini_generate_file(&ctx, "config.ini");
```
