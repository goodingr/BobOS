#include "framebuffer.h"

static struct limine_framebuffer *fb;

void framebuffer_init(struct limine_framebuffer *_fb) {
    fb = _fb;
}

void put_pixel(uint64_t x, uint64_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) {
        return;
    }
    volatile uint32_t *fb_ptr = fb->address;
    fb_ptr[y * (fb->pitch / 4) + x] = color;
}

void clear_screen(uint32_t color) {
    volatile uint32_t *fb_ptr = fb->address;

    for (size_t y = 0; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = color;
        }
    }
}

uint32_t* fb_row(uint64_t row) {
    volatile uint32_t *fb_ptr = fb->address;
    return fb_ptr + row * (fb->pitch / 4);
}

void fill_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color) {
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            put_pixel(x + i, y + j, color);
        }
    }
}

size_t get_framebuffer_width(void) {
    return fb->width;
}

size_t get_framebuffer_height(void) {
    return fb->height;
}
