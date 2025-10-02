ifeq ($(OS),Windows_NT)
    TARGET = pini.dll
else
    TARGET = libpini.so
endif

CC      = clang
SOURCE  = pini_wrap.c
INCLUDE = ../..

$(TARGET): $(SOURCE)
	$(CC) -shared -fPIC -o $@ $^ -I $(INCLUDE)
