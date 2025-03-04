#ifndef HANDMADE_H
#define HANDMADE_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef struct GameOffScreenBuffer {
    void* memory;
    int width;
    int height;
    int pitch;
} GameOffScreenBuffer;

static void gameUpdateAndRender(
    GameOffScreenBuffer* buffer, int blue_offset, int green_offset);

#endif /* HANDMADE_H */
