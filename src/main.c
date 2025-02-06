#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>

bool handleEvent(SDL_Event* event);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const char* err = SDL_GetError();
        fprintf(stderr, "ERROR: %s", err);
    }

    SDL_Window* window =
        SDL_CreateWindow("Handmade Hero", 640, 480, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

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
        case SDL_EVENT_WINDOW_RESIZED: {
            printf(
                "SDL_EVENT_WINDOW_RESIZED (%d, %d)\n",
                event->window.data1,
                event->window.data2);
        } break;
        case SDL_EVENT_WINDOW_EXPOSED: {
            SDL_Window* window = SDL_GetWindowFromID(event->window.windowID);
            SDL_Renderer* renderer = SDL_GetRenderer(window);
            static bool is_white = true;
            if (is_white) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                is_white = false;
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                is_white = true;
            }
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        } break;
    }
    return (should_quit);
}
