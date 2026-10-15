// SDL 1.2 -> SDL2 compatibility layer for the Waternet Dingoo sources.
// Included by waternet.cpp in place of the SDL 1.2 headers.
#ifndef PORT_H
#define PORT_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#ifdef __ANDROID__
// the game's printf() messages go to the Android log (adb logcat -s Waternet)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "Waternet", __VA_ARGS__)
#endif

// ---- file paths ----
// On Android SDL_RWFromFile() ignores the current directory, so files the
// game loads through SDL get an absolute path (current directory + name).
const char *Port_Path(const char *file);
#define TTF_OpenFont(file, size) TTF_OpenFont(Port_Path(file), size)
#define IMG_Load(file)           IMG_Load(Port_Path(file))
#define Mix_LoadWAV(file)        Mix_LoadWAV(Port_Path(file))
#define Mix_LoadMUS(file)        Mix_LoadMUS(Port_Path(file))

// ---- SDL 1.2 flags that no longer exist ----
#define SDL_HWSURFACE   0
#define SDL_SRCCOLORKEY SDL_TRUE
#define SDL_SRCALPHA    0x00010000

// ---- video ----
SDL_Surface *Port_SetVideoMode(int w, int h, int bpp, Uint32 flags);
int Port_Flip(SDL_Surface *screen);
SDL_Surface *Port_DisplayFormat(SDL_Surface *s);
SDL_Surface *Port_DisplayFormatAlpha(SDL_Surface *s);
int Port_SetAlpha(SDL_Surface *s, Uint32 flags, Uint8 alpha);

#define SDL_SetVideoMode       Port_SetVideoMode
#define SDL_Flip               Port_Flip
#define SDL_DisplayFormat      Port_DisplayFormat
#define SDL_DisplayFormatAlpha Port_DisplayFormatAlpha
#define SDL_SetAlpha           Port_SetAlpha
#define SDL_WM_SetCaption(a, b) ((void)0)
#define SDL_EnableUNICODE(a)    ((void)0)

// ---- SDL_gfx replacements (only what the game uses) ----
SDL_Surface *zoomSurface(SDL_Surface *src, double zoomx, double zoomy, int smooth);
int boxRGBA(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

// ---- input ----
// Returns real events plus key events synthesized from touch controls,
// keyboards and gamepads (as DINGOO_BUTTON_* key codes).
int Port_PollEvent(SDL_Event *event);
#define SDL_PollEvent Port_PollEvent

// Queue a synthesized Dingoo button press.
void Port_PushKey(SDL_Keycode key);

// Implemented in waternet.cpp: a tap on the game picture at game
// coordinates (0..319, 0..239). Return true if the tap was handled.
bool Game_HandleTap(int x, int y);
// Implemented in waternet.cpp: called before Android may kill the app.
void Game_SaveState();

// ---- startup ----
// Android: unpacks the game data to internal storage and chdir()s there.
// Desktop: chdir()s to the data directory next to the executable.
bool Port_PrepareData();
void Port_Shutdown();

#endif
