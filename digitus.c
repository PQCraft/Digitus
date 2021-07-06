#define _POSIX_C_SOURCE 999999L
#define _XOPEN_SOURCE 999999L

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    main(0, NULL);
}
#endif

struct timeval time1;
uint64_t tval[16];

uint64_t usec() {
    gettimeofday(&time1, NULL);
    return time1.tv_sec * 1000000 + time1.tv_usec;
}

uint64_t timer(uint8_t t) {
    return usec() - tval[t];
}

void setTimer(uint8_t t, uint64_t d) {
    tval[t] = usec() - d;
}

void waitusec(uint64_t d) {
    #ifdef __unix__
    struct timespec dts;
    dts.tv_sec = d / 1000000;
    dts.tv_nsec = (d % 1000000) * 1000;
    nanosleep(&dts, NULL);
    #else
    uint64_t t = d + usec();
    while (t > usec()) {}
    #endif
}

SDL_Window* win;
SDL_Renderer* ren;
SDL_Surface* sur;
SDL_Event event;

uint8_t tile[256][256];

int32_t mapw = 40;
int32_t maph = 30;
int32_t camx = 0;
int32_t camy = 0;
uint8_t* tilemap = NULL;

#include "data/lib/audio.c"

void cleanExit(int sig) {
    free(tilemap);
    SDL_FreeSurface(sur);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    if (sig == -1) {
	    exit(EXIT_FAILURE);
    } else {
        exit(EXIT_SUCCESS);
    }
}

bool hasFocus = false;
bool mouseFocus = false;

void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t* pixmap = (uint32_t*)(sur->pixels);
    int32_t i = x + y * 640;
    if (i < 0 || i > 307199) return;
    pixmap[i] = (a << 24) + (b << 16) + (g << 8) + r;
}

void render() {
    SDL_LockSurface(sur);
    for (int32_t y = camy; y < 30 && y < maph; y++) {
        for (int32_t x = camx; x < 40 && x < mapw; x++) {
            uint32_t tileno = x + y * mapw;
            int pxo = ((x - camx) * 16), pyo = ((y - camy) * 16);
            for (int py = 0; py < 16; py++) {
                for (int px = 0; px < 16; px++) {
                    uint8_t pix = tile[tilemap[tileno]][px + py * 16];
                    uint8_t r = pix & 3;
                    pix >>= 2;
                    uint8_t g = pix & 3;
                    pix >>= 2;
                    uint8_t b = pix & 3;
                    pix >>= 2;
                    uint8_t a = pix & 3;
                    r = r * 85;
                    g = g * 85;
                    b = b * 85;
                    a = a * 85;
                    setPixel(px + pxo, py + pyo, r, g, b, a);
                }
            }
        }
    }
    SDL_UnlockSurface(sur);
}

void gamelogic() {
    
}

#define FRAMETIME 16666u
#define DELAYTIME 16666u
uint64_t LOGICTIME = 500000;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    signal(SIGINT, cleanExit);
    srand(clock());
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (!(win = SDL_CreateWindow("Digitus", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN))) {
		fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
		cleanExit(-1);
    }
    if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC))) {
		fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
		cleanExit(-1);
    }
    sur = SDL_CreateRGBSurface(0, 640, 480, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
    tilemap = malloc(mapw * maph);
    for (int32_t i = 0; i < mapw * maph; i++) {
        tilemap[i] = (rand() % 256);
    }
    for (int t = 0; t < 256; t++) {
        for (int p = 0; p < 256; p++) {
            tile[t][p] = (rand() % 256);
        }
    }
    char fn[4096];
    for (int i = 0; i < 256; i++) {
        sprintf(fn, "tools/tmpmap/tiles/%03d.dtf", i);
        FILE* tilefile = fopen(fn, "rb");
        if (tilefile) {
            fread(tile[i], 1, 256, tilefile);
            fclose(tilefile);
            tilemap[i] = i;
        }
    }
    render();
    SDL_RenderPresent(ren);
    setTimer(0, 0);
    setTimer(1, 0);
    setTimer(2, 0);
    setTimer(3, 0);
    bool fullscr = false;
    bool setfscrmode = false;
    uint64_t frames = 0;
    initAudio();
    playMusic("data/music/17", SDL_MIX_MAXVOLUME);
    while (1) {
        setTimer(2, 0);
        if (timer(1) > LOGICTIME) {
            setTimer(1, 0);
            gamelogic();
        }
        if (timer(0) > 33333) {
            setTimer(0, 0);
            int flag = SDL_PollEvent(&event);
            if (flag) {
                switch (event.type) {
                    case SDL_QUIT:
                        cleanExit(SIGINT);
                        break;
                    case SDL_WINDOWEVENT:
                        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                            hasFocus = true;
                            SDL_ShowCursor(0);
                        }
                        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                            hasFocus = false;
                            mouseFocus = false;
                            SDL_ShowCursor(1);
                        }
                        if (event.window.event == SDL_WINDOWEVENT_ENTER && hasFocus) {
                            mouseFocus = true;
                        }
                        break;
                    case SDL_KEYUP:
                    case SDL_KEYDOWN:
                        const uint8_t* KBS = SDL_GetKeyboardState(NULL);
                        if ((KBS[SDL_SCANCODE_LALT] || KBS[SDL_SCANCODE_RALT]) && KBS[SDL_SCANCODE_RETURN]) {
                            fullscr = !fullscr;
                            setfscrmode = false;
                            while ((KBS[SDL_SCANCODE_LALT] || KBS[SDL_SCANCODE_RALT]) && KBS[SDL_SCANCODE_RETURN]) {
                                SDL_PollEvent(&event);
                            }
                        }
                        break;       
                    default:
                        break;
                }
                if (!setfscrmode) {
                    setfscrmode = true;
                    if (fullscr) {
                        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
                    } else {
                        SDL_SetWindowFullscreen(win, 0);
                    }
                }
            }
            SDL_RenderClear(ren);
            for (int32_t i = 0; i < mapw * maph; i++) {
                tilemap[i] = (rand() % 256);
            }
            render();
            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, sur);
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_DestroyTexture(tex);
            SDL_RenderPresent(ren);
            frames++;
        }
        if (timer(3) > 999999) {
            setTimer(3, 0);
            //printf("FPS: %lu\n", frames);
            frames = 0;
        }
        uint64_t tmp = timer(2);
        if (tmp < DELAYTIME - 0x40F0) {
            waitusec(DELAYTIME - tmp - 0x40F0);
        }
    }
    cleanExit(0);
}
