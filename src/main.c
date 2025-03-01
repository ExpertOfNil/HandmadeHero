#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_haptic.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <immintrin.h>

#define MAX_CONTROLLERS 4

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

typedef struct OffScreenBuffer {
    SDL_Texture* texture;
    void* memory;
    int width;
    int height;
    int pitch;
} OffScreenBuffer;

static OffScreenBuffer gBackBuffer = {0};
static int gCurrentSineSample = 0;
static int gSampleFreq = 48000;

static bool handleEvent(SDL_Event* event);
static void resizeTexture(
    OffScreenBuffer* buffer, SDL_Renderer* renderer, int width, int height);
static void updateWindow(
    SDL_Window* window, SDL_Renderer* renderer, OffScreenBuffer buffer);
static void renderWeirdGradient(
    OffScreenBuffer buffer, int blue_offset, int green_offset);
static void audioCallback(
    void* userdata,
    SDL_AudioStream* astream,
    int additional_amount,
    int total_amount);
static float squareWave(int phase, float volume);
static float sinWave(int phase, float volume);

int main(int argc, char* argv[]) {
    if (!SDL_Init(
            SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC |
            SDL_INIT_AUDIO)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to initialize: %s\n",
            SDL_GetError());
    }

    SDL_Window* window =
        SDL_CreateWindow("Handmade Hero", 1280, 960, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create window: %s\n",
            SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create renderer: %s\n",
            SDL_GetError());
        return 1;
    }

    SDL_AudioSpec spec = {};
    spec.freq = gSampleFreq;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audioCallback, NULL);
    if (!stream) {
        SDL_LogError(
            SDL_LOG_CATEGORY_AUDIO,
            "Couldn't create audio stream: %s",
            SDL_GetError());
    }
    SDL_ResumeAudioStreamDevice(stream);

    int width;
    int height;
    SDL_GetWindowSize(window, &width, &height);
    resizeTexture(&gBackBuffer, renderer, width, height);
    int xoffset = 0;
    int yoffset = 0;
    bool running = true;

    u64 perfCountFreq = SDL_GetPerformanceFrequency();
    u64 lastCounter = SDL_GetPerformanceCounter();
    u64 lastCycleCount = _rdtsc();

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
            // bool b = SDL_GetJoystickButton(joystick, 0);
            // bool a = SDL_GetJoystickButton(joystick, 1);
            // bool y = SDL_GetJoystickButton(joystick, 2);
            // bool x = SDL_GetJoystickButton(joystick, 3);

            u8 dpad = SDL_GetJoystickHat(joystick, 0);
            bool up = dpad & 0x01;
            bool rt = dpad & 0x02;
            bool dn = dpad & 0x04;
            bool lt = dpad & 0x08;

            // bool uprt = dpad & 0x03;
            // bool dnrt = dpad & 0x06;
            // bool dnlt = dpad & 0x012;
            // bool uplt = dpad & 0x09;

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

        u64 endCounter = SDL_GetPerformanceCounter();
        u64 endCycleCount = _rdtsc();

        u64 counterElapsed = endCounter - lastCounter;
        u64 cyclesElapsed = endCycleCount - lastCycleCount;

        f64 frameDuration_ms = (1000.0f * (f64)counterElapsed) / (f64)perfCountFreq;
        f64 fps = (f64)perfCountFreq / (f64)counterElapsed;
        f64 frameDuration_Mc = (f64)cyclesElapsed / (1000.0f * 1000.0f);

        printf("%.02f ms/f, %.02f fps, %.02f Mcpf\n", frameDuration_ms, fps, frameDuration_Mc);
        lastCounter = endCounter;
        lastCycleCount = endCycleCount;
    }

    SDL_Quit();
    return 0;
}

bool handleEvent(SDL_Event* event) {
    bool should_quit = false;
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            SDL_Log("SDL_QUIT");
            int n_joysticks;
            SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&n_joysticks);
            for (int i = 0; i < n_joysticks; ++i) {
                SDL_Joystick* joystick = SDL_GetJoystickFromID(joystick_ids[i]);
                if (NULL != joystick) {
                    SDL_Haptic* haptic = SDL_OpenHapticFromJoystick(joystick);
                    if (NULL != haptic) {
                        SDL_Log("Haptic %d closed.\n", joystick_ids[i]);
                    }
                    SDL_CloseJoystick(joystick);
                    SDL_Log("Joystick %d closed.\n", joystick_ids[i]);
                }
            }
            should_quit = true;
        } break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
            SDL_Log(
                "SDL_EVENT_WINDOW_RESIZED (%d, %d)\n",
                event->window.data1,
                event->window.data2);
        } break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            SDL_Log("SDL_EVENT_WINDOW_FOCUS_GAINED\n");
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
                SDL_LogError(
                    SDL_LOG_CATEGORY_ERROR,
                    "Failed to open joystick %d: %s\n",
                    id,
                    SDL_GetError());
            } else {
                SDL_Log("Joystick %d open.\n", id);
                SDL_Haptic* haptic = SDL_OpenHapticFromJoystick(joystick);
                if (NULL == haptic) {
                    SDL_LogError(
                        SDL_LOG_CATEGORY_INPUT,
                        "Open haptic error. %s\n",
                        SDL_GetError());
                } else {
                    SDL_Log("Haptic %d open.\n", id);
                    if (SDL_InitHapticRumble(haptic)) {
                        SDL_Log("Haptic %d initialized.\n", id);
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
                SDL_Log("Joystick %d closed.\n", id);
                SDL_Haptic* haptic = SDL_OpenHapticFromJoystick(joystick);
                if (NULL != haptic) {
                    SDL_Log("Haptic %d closed.\n", id);
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
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to update texture: %s\n",
            SDL_GetError());
        return;
    }
    if (!SDL_RenderTexture(renderer, buffer.texture, NULL, NULL)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to render texture: %s\n",
            SDL_GetError());
        return;
    }

    if (!SDL_RenderPresent(renderer)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to present texture: %s\n",
            SDL_GetError());
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
            /* NOTE: Alpha channel not explicitly set in Handmade Penguin
             * source */
            *pixel = (alpha | (green << 8) | blue);
            pixel += 1;
        }
        row += buffer.pitch;
    }
}

void audioCallback(
    void* userdata,
    SDL_AudioStream* astream,
    int additional_amount,
    int total_amount) {
    /* convert from bytes to samples */
    additional_amount /= sizeof(float);
    while (additional_amount > 0) {
        /* this will feed 128 samples each iteration until we have enough. */
        float samples[128];
        const int total = SDL_min(additional_amount, SDL_arraysize(samples));
        int i;

        for (i = 0; i < total; i++) {
            const int freq = 200;
            samples[i] = sinWave(freq, 0.2);
            gCurrentSineSample++;
        }

        /* wrapping around to avoid floating-point errors */
        gCurrentSineSample %= gSampleFreq;

        /* feed the new data to the stream. It will queue at the end, and
         * trickle out as the hardware needs more data. */
        SDL_PutAudioStreamData(astream, samples, total * sizeof(float));
        /* subtract what we've just fed the stream. */
        additional_amount -= total;
    }
}

float squareWave(int freq, float volume) {
    int phase = gCurrentSineSample * freq / gSampleFreq;
    return volume * (phase % 2);
}

float sinWave(int freq, float volume) {
    float phase = gCurrentSineSample * freq / (float)gSampleFreq;
    return volume * SDL_sinf(phase * 2 * SDL_PI_F);
}
