/*

MIT License

Copyright (c) 2018-2023 Mika Tuupola

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-cut-

This file is part of the HAGL graphics library:
https://github.com/tuupola/hagl


SPDX-License-Identifier: MIT

*/

#include "hagl/color.h"
#include "hagl/bitmap.h"
#include "hagl/blit.h"
#include "hagl.h"
#include "fontx.h"

uint8_t
hagl_put_char_scaled(void const *_surface, wchar_t code, int16_t x0, int16_t y0, hagl_color_t color, int scale, const uint8_t *font)
{
    static uint8_t *buffer = NULL;
    const hagl_surface_t *surface = _surface;
    uint8_t set, status;
    hagl_bitmap_t bitmap;
    fontx_glyph_t glyph;

    status = fontx_glyph(&glyph, code, font);

    if (0 != status) {
        return 0;
    }

    /* Initialize character buffer when first called. */
    if (NULL == buffer) {
        buffer = calloc(HAGL_CHAR_BUFFER_SIZE, sizeof(uint8_t));
    }

    hagl_bitmap_init(&bitmap, glyph.width, glyph.height, surface->depth, (uint8_t *)buffer);

    hagl_color_t *ptr = (hagl_color_t *) bitmap.buffer;

    for (uint8_t y = 0; y < glyph.height; y++) {
        for (uint8_t x = 0; x < glyph.width; x++) {
            set = *(glyph.buffer + x / 8) & (0x80 >> (x % 8));
            if (set) {
                *(ptr++) = color;
            } else {
                *(ptr++) = 0x0000;
            }
        }
        glyph.buffer += glyph.pitch;
    }

    hagl_blit_xywh(surface, x0, y0, glyph.width*scale, glyph.height*scale, &bitmap);

    return bitmap.width*scale;
}

/*
 * Write a string of text by calling hagl_put_char() repeadetly. CR and LF
 * continue from the next line.
 */

uint16_t
hagl_put_text_scaled(void const *surface, const wchar_t *str, int16_t x0, int16_t y0, hagl_color_t color, int scale, const unsigned char *font)
{
    wchar_t temp;
    uint8_t status;
    uint16_t original = x0;
    fontx_meta_t meta;

    status = fontx_meta(&meta, font);
    if (0 != status) {
        return 0;
    }

    do {
        temp = *str++;
        if (13 == temp || 10 == temp) {
            x0 = 0;
            y0 += meta.height;
        } else {
            x0 += hagl_put_char_scaled(surface, temp, x0, y0, color, scale, font);
        }
    } while (*str != 0);

    return x0 - original;
}
