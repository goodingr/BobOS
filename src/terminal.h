#pragma once
#include <stdint.h>

#define TERMINAL_PADDING 10

void terminal_init(uint32_t _bg_color, uint32_t _fg_color);
void terminal_write(const char *s);
void terminal_putchar(const char c);
