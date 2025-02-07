#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_Texture* gTexture;
static void* gPixels;
static int gTextureWidth;

bool handleEvent(SDL_Event* event);
void ResizeTexture(SDL_Renderer* renderer, int width, int height);
void UpdateWindow(SDL_Window* window, SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
    }

    SDL_Window* window =
        SDL_CreateWindow("Handmade Hero", 640, 480, SDL_WINDOW_RESIZABLE);
    if(!window) {
        fprintf(stderr, "ERROR: Failed to create window\n");
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    int width;
    int height;
    SDL_GetWindowSize(window, &width, &height);

    while (true) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        if (handleEvent(&event)) {
            break;
        }
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
            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            ResizeTexture(renderer, width, height);
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
    if (gTexture) {
        SDL_DestroyTexture(gTexture);
    }
    if (gPixels) {
        free(gPixels);
    }
    gTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);

    gPixels = malloc(width * height * 4);
    gTextureWidth = width;
}

void UpdateWindow(SDL_Window* window, SDL_Renderer* renderer) {
    if(!SDL_UpdateTexture(gTexture, NULL, gPixels, gTextureWidth * 4)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
        return;
    }

    if(!SDL_RenderTexture(renderer, gTexture, NULL, NULL)) {
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
