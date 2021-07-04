#ifndef D_AUDIO_H
#define D_AUDIO_H

typedef struct sound {
    uint32_t length;
    uint32_t lengthTrue;
    uint8_t* bufferTrue;
    uint8_t* buffer;
    uint8_t loop;
    uint8_t fade;
    uint8_t free;
    uint8_t volume;
    SDL_AudioSpec audio;
    struct sound* next;
} Audio;

Audio* createAudio(const char* filename, uint8_t loop, int volume);
void freeAudio(Audio* audio);
void playSound(const char* filename, int volume);
void playMusic(const char* filename, int volume);
void playSoundFromMemory(Audio* audio, int volume);
void playMusicFromMemory(Audio* audio, int volume);
void endAudio(void);
void initAudio(void);
void pauseAudio(void);
void unpauseAudio(void);

#endif
