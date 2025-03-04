#include "handmade.h"

static void renderWeirdGradient(
    GameOffScreenBuffer* buffer, int blue_offset, int green_offset) {
    u8* row = (u8*)buffer->memory;
    const int alpha = 255 << 24;
    for (int y = 0; y < buffer->height; ++y) {
        u32* pixel = (u32*)row;
        for (int x = 0; x < buffer->width; ++x) {
            u8 blue = (x + blue_offset);
            u8 green = (y + green_offset);
            /* NOTE: Alpha channel not explicitly set in Handmade Penguin
             * source */
            *pixel = (alpha | (green << 8) | blue);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

static void gameUpdateAndRender(
    GameOffScreenBuffer* buffer, int blue_offset, int green_offset) {
    renderWeirdGradient(buffer, blue_offset, green_offset);
}
