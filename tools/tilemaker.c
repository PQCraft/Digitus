#define _POSIX_C_SOURCE 999999L
#define _XOPEN_SOURCE 999999L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <errno.h>

uint8_t tile[16][16];

char in[32768];

void lcase(char* str) {
    for (int i = 0; i < 32768 && str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') str[i] += 32;
    }
}

int getnum(char* prompt) {
    fputs(prompt, stdout);
    fflush(stdout);
    in[0] = 0;
    scanf("%[^\n]s", in);
    while (getchar() != '\n') {}
    return atoi(in);
}

void printBits(size_t size, void* ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
}

void wprintBits(size_t size, void* ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            wprintf(L"%u", byte);
        }
    }
}

uint8_t getcolor(uint8_t o) {
    int r = getnum("red: ");
    if (in[0] == '\e') return 0;
    if (!in[0]) r = o & 3;
    o >>= 2;
    int g = getnum("green: ");
    if (in[0] == '\e') return 0;
    if (!in[0]) g = o & 3;
    o >>= 2;
    int b = getnum("blue: ");
    if (in[0] == '\e') return 0;
    if (!in[0]) b = o & 3;
    o >>= 2;
    int a = getnum("alpha: ");
    if (in[0] == '\e') return 0;
    if (!in[0]) a = o & 3;
    if (r < 0) r = 0;
    if (r > 3) r = 3;
    if (g < 0) g = 0;
    if (g > 3) g = 3;
    if (b < 0) b = 0;
    if (b > 3) b = 3;
    if (a < 0) a = 0;
    if (a > 3) a = 3;
    return r + (g << 2) + (b << 4) + (a << 6);
}

int main() {
    system("mkdir -p tmpmap/tiles/");
    setlocale(LC_CTYPE, "");
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            tile[y][x] = 0;
        }
    }
    uint8_t c = 0;
    bool invalidcmd = false;
    bool grid = false;
    bool err = false;
    while (1) {
        system("clear");
        FILE* ret = freopen(NULL, "w", stdout);
        (void)ret;
        for (int y = 0; y < 16; y++) {
            if (grid && y) putwchar(L'\n');
            for (int x = 0; x < 16; x++) {
                uint8_t pix = tile[y][x];
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
                wprintf(L"\e[38;2;%u;%u;%um", r, g, b);
                switch (a) {
                    case 0:
                        putwchar(L'░');
                        putwchar(L'░');
                        break;
                    case 1:
                        putwchar(L'▒');
                        putwchar(L'▒');
                        break;
                    case 2:
                        putwchar(L'▓');
                        putwchar(L'▓');
                        break;
                    case 3:
                        putwchar(L'█');
                        putwchar(L'█');
                        break;
                    default:
                        putwchar(L'E');
                        putwchar(L'E');
                        break;
                }
                if (grid) {
                    putwchar(L' ');
                    putwchar(L' ');
                }
            }
            putwchar(L'\n');
        }
        uint8_t pix = c;
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
        putwchar(L'\n');
        wprintf(L"\e[0mColor: \e[38;2;%u;%u;%um", r, g, b);
        switch (a) {
            case 0:
                putwchar(L'░');
                putwchar(L'░');
                break;
            case 1:
                putwchar(L'▒');
                putwchar(L'▒');
                break;
            case 2:
                putwchar(L'▓');
                putwchar(L'▓');
                break;
            case 3:
                putwchar(L'█');
                putwchar(L'█');
                break;
            default:
                putwchar(L'E');
                putwchar(L'E');
                break;
        }
        wprintf(L"\e[0m [%03u] (", c);
        wprintBits(1, &c);
        putwchar(L')');
        fflush(stdout);
        putwchar(L'\n');
        ret = freopen(NULL, "w", stdout);
        (void)ret;
        puts("\e[0m");
        fputs("command> ", stdout);
        scanf("%[^ \n]s", in);
        while (getchar() != '\n') {}
        lcase(in);
        fputs("\r\e[2K", stdout);
        fflush(stdout);
        invalidcmd = true;
        err = false;
        if (!in[0]) {
            invalidcmd = false;
        }
        if (!strcmp(in, "color")) {
            invalidcmd = false;
            c = getcolor(c);
        }
        if (!strcmp(in, "set")) {
            invalidcmd = false;
            int x = getnum("x: ");
            if (in[0] == '\e') goto cmdq;
            if (x < 0) x = 0;
            if (x > 15) x = 15;
            int y = getnum("y: ");
            if (in[0] == '\e') goto cmdq;
            if (y < 0) y = 0;
            if (y > 15) y = 15;
            tile[y][x] = c;
        }
        if (!strcmp(in, "cset")) {
            invalidcmd = false;
            int x = getnum("x: ");
            if (in[0] == '\e') goto cmdq;
            if (x < 0) x = 0;
            if (x > 15) x = 15;
            int y = getnum("y: ");
            if (in[0] == '\e') goto cmdq;
            if (y < 0) y = 0;
            if (y > 15) y = 15;
            tile[y][x] = getcolor(c);
        }
        if (!strcmp(in, "fill")) {
            invalidcmd = false;
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    tile[y][x] = c;
                }
            }
        }
        if (!strcmp(in, "cfill")) {
            invalidcmd = false;
            uint8_t tmpc = getcolor(c);
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    tile[y][x] = tmpc;
                }
            }
        }
        if (!strcmp(in, "save")) {
            invalidcmd = false;
            uint8_t tileno = (uint8_t)getnum("tile num: ");
            if (!in[0] || in[0] == '\e') goto cmdq;
            sprintf(in, "tmpmap/tiles/%03u.dtf", tileno);
            FILE* tilefile = fopen(in, "wb");
            if (!tilefile) {
                printf("File error (%d).", errno);
                err = true;
                goto cmde;
            }
            fwrite(*tile, 1, 256, tilefile);
            fclose(tilefile);
            if (in[0] == '\e') goto cmdq;
        }
        if (!strcmp(in, "load")) {
            invalidcmd = false;
            uint8_t tileno = (uint8_t)getnum("tile num: ");
            if (!in[0] || in[0] == '\e') goto cmdq;
            sprintf(in, "tmpmap/tiles/%03u.dtf", tileno);
            FILE* tilefile = fopen(in, "rb");
            if (!tilefile) {
                printf("File error (%d).\n", errno);
                err = true;
                goto cmde;
            }
            fread(*tile, 1, 256, tilefile);
            fclose(tilefile);
            if (in[0] == '\e') goto cmdq;
        }
        if (!strcmp(in, "grid")) {
            invalidcmd = false;
            grid = !grid;
        }
        if (!strcmp(in, "exit")) {
            invalidcmd = false;
            return 0;
        }
        if (invalidcmd) {
            puts("Invalid command.");
            err = true;
        }
        cmde:
        if (err) {
            puts("Press [ENTER]");
            while (getchar() != '\n') {}
            fputs("\e[18;1H\e[2K", stdout);
            fflush(stdout);
        }
        cmdq:
    }
    return 0;
}

