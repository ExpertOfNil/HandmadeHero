#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

static SDL_Texture* gTexture;
static void* gBitmapMemory;
static int gBitmapWidth;
static int gBitmapHeight;
static int gBytesPerPixel = 4;

static bool handleEvent(SDL_Event* event);
static void ResizeTexture(SDL_Renderer* renderer, int width, int height);
static void UpdateWindow(SDL_Window* window, SDL_Renderer* renderer);
static void RenderWeirdGradient(int blue_offset, int green_offset);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
    }

    SDL_Window* window =
        SDL_CreateWindow("Handmade Hero", 640, 480, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "ERROR: Failed to create window\n");
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "ERROR: Failed to create renderer\n");
        return 1;
    }

    int width;
    int height;
    SDL_GetWindowSize(window, &width, &height);
    ResizeTexture(renderer, width, height);
    int xoffset = 0;
    int yoffset = 0;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (handleEvent(&event)) {
                running = false;
            }
        }
        RenderWeirdGradient(xoffset, yoffset);
        UpdateWindow(window, renderer);
        xoffset += 1;
        yoffset += 2;
    }

    SDL_Quit();
    return 0;
}

bool handleEvent(SDL_Event* event) {
    bool should_quit = false;
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            printf("SDL_QUIT\n");
            should_quit = true;
        } break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
            printf(
                "SDL_EVENT_WINDOW_RESIZED (%d, %d)\n",
                event->window.data1,
                event->window.data2);
            SDL_Window* window = SDL_GetWindowFromID(event->window.windowID);
            SDL_Renderer* renderer = SDL_GetRenderer(window);
            ResizeTexture(renderer, event->window.data1, event->window.data2);
        } break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            printf("SDL_EVENT_WINDOW_FOCUS_GAINED\n");
        } break;
        case SDL_EVENT_WINDOW_EXPOSED: {
            SDL_Window* window = SDL_GetWindowFromID(event->window.windowID);
            SDL_Renderer* renderer = SDL_GetRenderer(window);
            UpdateWindow(window, renderer);
        } break;
    }
    return (should_quit);
}

void ResizeTexture(SDL_Renderer* renderer, int width, int height) {
    if (gBitmapMemory != NULL) {
        munmap(gBitmapMemory, gBitmapWidth * gBitmapHeight * gBytesPerPixel);
    }
    if (gTexture != NULL) {
        SDL_DestroyTexture(gTexture);
    }
    gTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);

    gBitmapWidth = width;
    gBitmapHeight = height;
    gBitmapMemory = mmap(
        NULL,
        width * height * gBytesPerPixel,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);
}

void UpdateWindow(SDL_Window* window, SDL_Renderer* renderer) {
    if (!SDL_UpdateTexture(
            gTexture, NULL, gBitmapMemory, gBitmapWidth * gBytesPerPixel)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
        return;
    }
    if (!SDL_RenderTexture(renderer, gTexture, NULL, NULL)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
        return;
    }

    if (!SDL_RenderPresent(renderer)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
        return;
    }
}

void RenderWeirdGradient(int blue_offset, int green_offset) {
    int width = gBitmapWidth;
    int height = gBitmapHeight;

    int pitch = width * gBytesPerPixel;
    u8* row = (u8*)gBitmapMemory;
    for (int y = 0; y < gBitmapHeight; ++y) {
        u32* pixel = (u32*)row;
        for (int x = 0; x < gBitmapWidth; ++x) {
            u8 blue = (x + blue_offset);
            u8 green = (y + green_offset);
            // NOTE: Alpha channel not explicitly set in Handmade Penguin source
            *pixel = ((255 << 24) | (green << 8) | blue);
            pixel += 1;
        }
        row += pitch;
    }
}
