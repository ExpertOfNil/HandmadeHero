#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_haptic.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_oldnames.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define MAX_CONTROLLERS 4

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct OffScreenBuffer {
    SDL_Texture* texture;
    void* memory;
    int width;
    int height;
    int pitch;
} OffScreenBuffer;
static OffScreenBuffer gBackBuffer = {0};

static bool handleEvent(SDL_Event* event);
static void resizeTexture(
    OffScreenBuffer* buffer, SDL_Renderer* renderer, int width, int height);
static void updateWindow(
    SDL_Window* window, SDL_Renderer* renderer, OffScreenBuffer buffer);
static void renderWeirdGradient(
    OffScreenBuffer buffer, int blue_offset, int green_offset);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
    }

    SDL_Window* window =
        SDL_CreateWindow("Handmade Hero", 1280, 960, SDL_WINDOW_RESIZABLE);
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
    resizeTexture(&gBackBuffer, renderer, width, height);
    int xoffset = 0;
    int yoffset = 0;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Handle key events
            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode code = event.key.key;
                switch (code) {
                    case SDLK_W: {
                        yoffset -= 2;
                    } break;
                    case SDLK_S: {
                        yoffset += 2;
                    } break;
                    case SDLK_A: {
                        xoffset -= 2;
                    } break;
                    case SDLK_D: {
                        xoffset += 2;
                    } break;
                    default:
                        continue;
                }
            } else if (handleEvent(&event)) {
                running = false;
            }
        }

        // Poll joysticks for state of buttons and D-pad
        int n_joysticks;
        SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&n_joysticks);
        for (int i = 0; i < n_joysticks; ++i) {
            SDL_Joystick* joystick = SDL_GetJoystickFromID(joystick_ids[i]);
            if (!SDL_JoystickConnected(joystick)) {
                continue;
            }
            //bool b = SDL_GetJoystickButton(joystick, 0);
            //bool a = SDL_GetJoystickButton(joystick, 1);
            //bool y = SDL_GetJoystickButton(joystick, 2);
            //bool x = SDL_GetJoystickButton(joystick, 3);

            u8 dpad = SDL_GetJoystickHat(joystick, 0);
            bool up = dpad & 0x01;
            bool rt = dpad & 0x02;
            bool dn = dpad & 0x04;
            bool lt = dpad & 0x08;

            //bool uprt = dpad & 0x03;
            //bool dnrt = dpad & 0x06;
            //bool dnlt = dpad & 0x012;
            //bool uplt = dpad & 0x09;

            if (up) {
                yoffset -= 1;
            } else if (dn) {
                yoffset += 1;
            }

            if (lt) {
                xoffset -= 1;
            } else if (rt) {
                xoffset += 1;
            }
        }

        renderWeirdGradient(gBackBuffer, xoffset, yoffset);
        updateWindow(window, renderer, gBackBuffer);
    }

    SDL_Quit();
    return 0;
}

bool handleEvent(SDL_Event* event) {
    bool should_quit = false;
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            printf("SDL_QUIT\n");
            int n_joysticks;
            SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&n_joysticks);
            for (int i = 0; i < n_joysticks; ++i) {
                SDL_Joystick* joystick = SDL_GetJoystickFromID(joystick_ids[i]);
                if (NULL != joystick) {
                    SDL_CloseJoystick(joystick);
                    printf("Joystick %d closed.\n", joystick_ids[i]);
                    SDL_Haptic* haptic = SDL_OpenHapticFromJoystick(joystick);
                    if (NULL != haptic) {
                        printf("Haptic %d closed.\n", joystick_ids[i]);
                    }
                }
            }
            should_quit = true;
        } break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
            printf(
                "SDL_EVENT_WINDOW_RESIZED (%d, %d)\n",
                event->window.data1,
                event->window.data2);
        } break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            printf("SDL_EVENT_WINDOW_FOCUS_GAINED\n");
        } break;
        case SDL_EVENT_WINDOW_EXPOSED: {
            SDL_Window* window = SDL_GetWindowFromID(event->window.windowID);
            SDL_Renderer* renderer = SDL_GetRenderer(window);
            updateWindow(window, renderer, gBackBuffer);
        } break;
        case SDL_EVENT_JOYSTICK_ADDED: {
            SDL_JoystickID id = event->jdevice.which;
            SDL_Joystick* joystick = SDL_OpenJoystick(id);
            if (NULL == joystick) {
                const char* err = SDL_GetError();
                fprintf(
                    stderr, "ERROR: Failed to open joystick %d: %s\n", id, err);
            } else {
                printf("Joystick %d open.\n", id);
                SDL_Haptic* haptic = SDL_OpenHapticFromJoystick(joystick);
                if (NULL == haptic) {
                    printf("No haptic available on joystick %d.\n", id);
                } else {
                    printf("Haptic %d open.\n", id);
                    if (SDL_InitHapticRumble(haptic)) {
                        printf("Haptic %d initialized.\n", id);
                        SDL_PlayHapticRumble(haptic, 0.5f, 2000);
                    }
                }
            }
        } break;
        case SDL_EVENT_JOYSTICK_REMOVED: {
            SDL_JoystickID id = event->jdevice.which;
            SDL_Joystick* joystick = SDL_GetJoystickFromID(id);
            if (NULL != joystick) {
                SDL_CloseJoystick(joystick);
                printf("Joystick %d closed.\n", id);
                SDL_Haptic* haptic = SDL_OpenHapticFromJoystick(joystick);
                if (NULL != haptic) {
                    printf("Haptic %d closed.\n", id);
                }
            }
        } break;
    }
    return (should_quit);
}

void resizeTexture(
    OffScreenBuffer* buffer, SDL_Renderer* renderer, int width, int height) {
    if (buffer->memory != NULL) {
        munmap(buffer->memory, buffer->pitch * buffer->height);
    }
    if (buffer->texture != NULL) {
        SDL_DestroyTexture(buffer->texture);
    }
    buffer->texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);

    buffer->width = width;
    buffer->height = height;
    // Pixels are always 32-bit, BGRA (0xAARRGGBB, little endian)
    const int gBytesPerPixel = 4;
    buffer->pitch = buffer->width * gBytesPerPixel;
    buffer->memory = mmap(
        NULL,
        height * buffer->pitch,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);
}

void updateWindow(
    SDL_Window* window, SDL_Renderer* renderer, OffScreenBuffer buffer) {
    if (!SDL_UpdateTexture(buffer.texture, NULL, buffer.memory, buffer.pitch)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s\n", err);
        return;
    }
    if (!SDL_RenderTexture(renderer, buffer.texture, NULL, NULL)) {
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

void renderWeirdGradient(
    OffScreenBuffer buffer, int blue_offset, int green_offset) {
    u8* row = (u8*)buffer.memory;
    const int alpha = 255 << 24;
    for (int y = 0; y < buffer.height; ++y) {
        u32* pixel = (u32*)row;
        for (int x = 0; x < buffer.width; ++x) {
            u8 blue = (x + blue_offset);
            u8 green = (y + green_offset);
            // NOTE: Alpha channel not explicitly set in Handmade Penguin
            // source
            *pixel = (alpha | (green << 8) | blue);
            pixel += 1;
        }
        row += buffer.pitch;
    }
}
