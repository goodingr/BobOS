#include "terminal.h"
#include "framebuffer.h"
#include "font.h"
#include "string.h"
#include <stddef.h>



typedef struct {
    size_t x;
    size_t y;
} Cursor;

static uint32_t bg_color = 0x00000000;
static uint32_t fg_color = 0x00FFFFFF;
static Cursor cursor;

void terminal_init(uint32_t _bg_color, uint32_t _fg_color) {
    bg_color = _bg_color;
    fg_color = _fg_color;
    clear_screen(bg_color);
    cursor.x = TERMINAL_PADDING;
    cursor.y = TERMINAL_PADDING;
}

void terminal_write(const char *s) {
    while (*s != '\0') {
        terminal_putchar(*s);
        s += 1;
    }
}

void scroll_terminal(void) {
    const size_t line_height = FONT_HEIGHT + 2;
    const size_t height = get_framebuffer_height();
    const size_t width = get_framebuffer_width();

    for (size_t j = TERMINAL_PADDING; j + line_height < height; j++) {
        uint32_t *dst = fb_row(j);
        uint32_t *src = fb_row(j + line_height);
        memcpy(dst, src, width * sizeof(uint32_t));
    }

    fill_rect(0, height - line_height, width, line_height, bg_color);
}

static void terminal_newline(void) {
    cursor.x = TERMINAL_PADDING;
    if (cursor.y + FONT_HEIGHT + 2 > get_framebuffer_height() - FONT_HEIGHT) {
        scroll_terminal();
    }
    else {
        cursor.y += FONT_HEIGHT + 2;
    }
}

void terminal_putchar(const char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    if (cursor.x + FONT_WIDTH > get_framebuffer_width()) {
        terminal_newline();
    }

    const uint8_t *glyph = font_get(c);
    for (size_t y = 0; y < FONT_HEIGHT; y++) {
        for (int bit = FONT_WIDTH - 1; bit >= 0; bit--) {
            if (glyph[y] & (1 << bit)) {
                put_pixel(cursor.x + (7 - bit), cursor.y + y, fg_color);
            }
        }
    }
    cursor.x += FONT_WIDTH + 2;

}

