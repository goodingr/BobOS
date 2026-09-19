#pragma once

#include <stdint.h>
#include "limine.h"
#include <stddef.h>

void framebuffer_init(struct limine_framebuffer *fb);
void put_pixel(uint64_t x, uint64_t y, uint32_t color);
void fill_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color);
void clear_screen(uint32_t color);
uint32_t* fb_row(uint64_t row);

size_t get_framebuffer_width(void);
size_t get_framebuffer_height(void);
