#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "audio.h"

#define AUDIO_FORMAT AUDIO_S16LSB
#define AUDIO_FREQUENCY 22050
#define AUDIO_CHANNELS 1
#define AUDIO_SAMPLES 4096
#define AUDIO_MAX_SOUNDS 25
#define AUDIO_MUSIC_FADE_VALUE 2
#define SDL_AUDIO_ALLOW_CHANGES SDL_AUDIO_ALLOW_ANY_CHANGE

typedef struct privateAudioDevice {
    SDL_AudioDeviceID device;
    SDL_AudioSpec want;
    uint8_t audioEnabled;
} privateAudioDevice;

privateAudioDevice * gDevice;
uint32_t gSoundCount;
void addMusic(Audio * root, Audio * newAudio);
void playAudio(const char * filename, Audio * audio, uint8_t loop, int volume);
void addAudio(Audio * root, Audio * newAudio);
void audioCallback(void * userdata, uint8_t * stream, int len);

void playSound(const char * filename, int volume) {
    playAudio(filename, NULL, 0, volume);
}

void playMusic(const char * filename, int volume) {
    playAudio(filename, NULL, 1, volume);
}

void playSoundFromMemory(Audio * audio, int volume) {
    playAudio(NULL, audio, 0, volume);
}

void playMusicFromMemory(Audio * audio, int volume) {
    playAudio(NULL, audio, 1, volume);
}

void initAudio(void) {
    Audio* global;
    gDevice = (privateAudioDevice*)calloc(1, sizeof(privateAudioDevice));
    gSoundCount = 0;
    if (!gDevice) {
        fprintf(stderr, "Fatal Error: Memory c-allocation error\n");
        return;
    }
    gDevice->audioEnabled = 0;
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        fprintf(stderr, "Error: SDL_INIT_AUDIO not initialized\n");
        return;
    }
    SDL_memset(&(gDevice->want), 0, sizeof(gDevice->want));
    (gDevice->want).freq = AUDIO_FREQUENCY;
    (gDevice->want).format = AUDIO_FORMAT;
    (gDevice->want).channels = AUDIO_CHANNELS;
    (gDevice->want).samples = AUDIO_SAMPLES;
    (gDevice->want).callback = audioCallback;
    (gDevice->want).userdata = calloc(1, sizeof(Audio));
    global = (Audio *) (gDevice->want).userdata;
    if (!global) {
        fprintf(stderr, "Error: Memory allocation error\n");
        return;
    }
    global->buffer = NULL;
    global->next = NULL;
    if (!(gDevice->device = SDL_OpenAudioDevice(NULL, 0, &(gDevice->want), NULL, SDL_AUDIO_ALLOW_CHANGES))) {
        fprintf(stderr, "Warning: failed to open audio device: %s\n", SDL_GetError());
    } else {
        gDevice->audioEnabled = 1;
        unpauseAudio();
    }
}

void endAudio(void) {
    if (gDevice->audioEnabled) {
        pauseAudio();
        freeAudio((Audio *) (gDevice->want).userdata);
        SDL_CloseAudioDevice(gDevice->device);
    }
    free(gDevice);
}

void pauseAudio(void) {
    if (gDevice->audioEnabled) {
        SDL_PauseAudioDevice(gDevice->device, 1);
    }
}

void unpauseAudio(void) {
    if(gDevice->audioEnabled) {
        SDL_PauseAudioDevice(gDevice->device, 0);
    }
}

void freeAudio(Audio * audio) {
    Audio * temp;
    while (audio) {
        if (audio->free == 1) {
            SDL_FreeWAV(audio->bufferTrue);
        }
        temp = audio;
        audio = audio->next;
        free(temp);
    }
}

Audio* createAudio(const char* filename, uint8_t loop, int volume){
    Audio* newAudio = (Audio*)calloc(1, sizeof(Audio));
    if(!newAudio) {
        fprintf(stderr, "Error: Memory allocation error\n");
        return NULL;
    }
    if (!filename) {
        fprintf(stderr, "Warning: filename NULL\n");
        return NULL;
    }
    newAudio->next = NULL;
    newAudio->loop = loop;
    newAudio->fade = 0;
    newAudio->free = 1;
    newAudio->volume = volume;
    if (!SDL_LoadWAV(filename, &(newAudio->audio), &(newAudio->bufferTrue), &(newAudio->lengthTrue))) {
        fprintf(stderr, "Warning: failed to open wave file: %s error: %s\n", filename, SDL_GetError());
        free(newAudio);
        return NULL;
    }
    newAudio->buffer = newAudio->bufferTrue;
    newAudio->length = newAudio->lengthTrue;
    (newAudio->audio).callback = NULL;
    (newAudio->audio).userdata = NULL;
    return newAudio;
}

void playAudio(const char * filename, Audio * audio, uint8_t loop, int volume) {
    Audio* newAudio;
    if (!gDevice->audioEnabled) return;
    if (!loop) {
        if (gSoundCount >= AUDIO_MAX_SOUNDS) {
            return;
        } else {
            gSoundCount++;
        }
    }
    if (filename) {
        newAudio = createAudio(filename, loop, volume);
    } else if (audio) {
        newAudio = (Audio*)malloc(sizeof(Audio));
        if (newAudio == NULL) {
            fprintf(stderr, "Fatal Error: Memory allocation error\n");
            return;
        }
        memcpy(newAudio, audio, sizeof(Audio));
        newAudio->volume = volume;
        newAudio->loop = loop;
        newAudio->free = 0;
    } else {
        fprintf(stderr, "Warning: filename and Audio parameters NULL\n");
        return;
    }
    SDL_LockAudioDevice(gDevice->device);
    if (loop == 1) {
        addMusic((Audio*)(gDevice->want).userdata, newAudio);
    } else {
        addAudio((Audio*)(gDevice->want).userdata, newAudio);
    }
    SDL_UnlockAudioDevice(gDevice->device);
}

void addMusic(Audio * root, Audio * newAudio) {
    uint8_t musicFound = 0;
    Audio* rootNext = root->next;
    while (rootNext != NULL) {
        if (rootNext->loop == 1 && rootNext->fade == 0) {
            if (musicFound) {
                rootNext->length = 0;
                rootNext->volume = 0;
            }
            rootNext->fade = 1;
        } else if (rootNext->loop == 1 && rootNext->fade == 1) {
            musicFound = 1;
        }
        rootNext = rootNext->next;
    }
    addAudio(root, newAudio);
}

void audioCallback(void * userdata, uint8_t * stream, int len) {
    Audio* audio = (Audio*)userdata;
    Audio* previous = audio;
    int tempLength;
    uint8_t music = 0;
    SDL_memset(stream, 0, len);
    audio = audio->next;
    while (audio) {
        if (audio->length > 0) {
            if (audio->fade == 1 && audio->loop == 1) {
                music = 1;
                if (audio->volume > 0) {
                    if (audio->volume - AUDIO_MUSIC_FADE_VALUE < 0) {
                        audio->volume = 0;
                    } else {
                        audio->volume -= AUDIO_MUSIC_FADE_VALUE;
                    }
                } else {
                    audio->length = 0;
                }
            }

            if (music && audio->loop == 1 && audio->fade == 0) {
                tempLength = 0;
            } else {
                tempLength = ((uint32_t) len > audio->length) ? audio->length : (uint32_t) len;
            }
            SDL_MixAudioFormat(stream, audio->buffer, AUDIO_FORMAT, tempLength, audio->volume);
            audio->buffer += tempLength;
            audio->length -= tempLength;
            previous = audio;
            audio = audio->next;
        } else if (audio->loop == 1 && audio->fade == 0) {
            audio->buffer = audio->bufferTrue;
            audio->length = audio->lengthTrue;
        } else {
            previous->next = audio->next;
            if (audio->loop == 0) {
                gSoundCount--;
            }
            audio->next = NULL;
            freeAudio(audio);
            audio = previous->next;
        }
    }
}

void addAudio(Audio * root, Audio * newAudio) {
    if (!root) return;
    while (root->next) {
        root = root->next;
    }
    root->next = newAudio;
}

