// SDL2 platform layer for the Waternet Dingoo port.
//
// The original game draws everything into a 320x240 software surface and
// calls SDL_Flip(). Here SDL_Flip uploads that surface to a texture, scales
// it crisply to the phone screen and draws Dingoo-style touch controls
// (D-pad, A, B, SELECT, L, R) in the remaining space. Touches, keyboards and
// gamepads are translated into the Dingoo button codes the game expects.

#include <SDL.h>
#include <SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <deque>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "gp2x.h"

bool Game_HandleTap(int x, int y);
void Game_SaveState();

// ---------------------------------------------------------------------------
// state
// ---------------------------------------------------------------------------
static SDL_Window   *Win = NULL;
static SDL_Renderer *Ren = NULL;
static SDL_Texture  *GameTex = NULL;     // 320x240, nearest
static SDL_Texture  *UpTex = NULL;       // 320k x 240k render target, linear
static int           UpK = 0;
static SDL_Surface  *ScreenSurf = NULL;
static int           GameW = 320, GameH = 240;
static bool          TexturesLost = false;
static Uint32        LastFlip = 0;
static std::deque<SDL_Keycode> KeyQueue;

enum Ctrl { C_NONE = -1, C_DPAD = 0, C_A, C_B, C_SELECT, C_L, C_R, C_GAME, C_COUNT };

struct Circle { float x, y, r; };
struct Layout {
    int W, H;
    SDL_Rect game;
    Circle dpad;               // r = half the cross size
    Circle a, b;
    SDL_FRect select, l, r;    // pill buttons
    bool portrait;
};
static Layout L;
static int LayoutW = -1, LayoutH = -1;

struct Finger {
    SDL_FingerID id;
    int ctrl;
    float sx, sy, x, y;
    Uint32 t0;
    int dir;                   // current D-pad direction (DINGOO key) or 0
    Uint32 nextRepeat;
};
static std::vector<Finger> Fingers;
static Uint32 FlashUntil[C_COUNT];
static int    FlashDir = 0;

static TTF_Font    *LabelFont = NULL;
static int          LabelFontSize = 0;
static SDL_Texture *LabelTex[C_COUNT];
static int          LabelW[C_COUNT], LabelH[C_COUNT];

// ---------------------------------------------------------------------------
// surfaces (SDL 1.2 semantics)
// ---------------------------------------------------------------------------
SDL_Surface *Port_DisplayFormat(SDL_Surface *s)
{
    if (!s) return NULL;
    SDL_Surface *c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGB888, 0);
    if (c) {
        SDL_SetSurfaceBlendMode(c, SDL_BLENDMODE_NONE);
        SDL_SetSurfaceAlphaMod(c, 255);
    }
    return c;
}

SDL_Surface *Port_DisplayFormatAlpha(SDL_Surface *s)
{
    if (!s) return NULL;
    SDL_Surface *c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ARGB8888, 0);
    if (c) {
        SDL_SetSurfaceBlendMode(c, SDL_BLENDMODE_BLEND);
        SDL_SetSurfaceAlphaMod(c, 255);
    }
    return c;
}

int Port_SetAlpha(SDL_Surface *s, Uint32 flags, Uint8 alpha)
{
    if (!s) return -1;
    if (flags & 0x00010000) {
        SDL_SetSurfaceAlphaMod(s, alpha);
        SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);
    } else {
        SDL_SetSurfaceAlphaMod(s, 255);
        SDL_SetSurfaceBlendMode(s, s->format->Amask ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    }
    return 0;
}

SDL_Surface *zoomSurface(SDL_Surface *src, double zoomx, double zoomy, int smooth)
{
    if (!src) return NULL;
    int w = (int)(src->w * zoomx + 0.5), h = (int)(src->h * zoomy + 0.5);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    SDL_Surface *s = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_Surface *d = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s && d) {
        SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);
        if (!smooth || SDL_SoftStretchLinear(s, NULL, d, NULL) != 0)
            SDL_SoftStretch(s, NULL, d, NULL);
        SDL_SetSurfaceBlendMode(d, SDL_BLENDMODE_BLEND);
    }
    if (s) SDL_FreeSurface(s);
    return d;
}

int boxRGBA(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (x1 > x2) { Sint16 t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { Sint16 t = y1; y1 = y2; y2 = t; }
    SDL_Rect rc = { x1, y1, x2 - x1 + 1, y2 - y1 + 1 };
    if (a == 255)
        return SDL_FillRect(dst, &rc, SDL_MapRGB(dst->format, r, g, b));
    SDL_Surface *t = SDL_CreateRGBSurfaceWithFormat(0, rc.w, rc.h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!t) return -1;
    SDL_FillRect(t, NULL, SDL_MapRGBA(t->format, r, g, b, a));
    SDL_SetSurfaceBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(t, NULL, dst, &rc);
    SDL_FreeSurface(t);
    return 0;
}

// ---------------------------------------------------------------------------
// layout
// ---------------------------------------------------------------------------
static void ComputeLayout(int W, int H)
{
    L.W = W; L.H = H;
    L.portrait = H > W;
    if (!L.portrait) {
        // game picture in the middle, controls in the side bars
        float minSide = 0.36f * H;
        float gw = H * 4.0f / 3.0f;
        if (gw > W - 2 * minSide) gw = W - 2 * minSide;
        if (gw < W * 0.5f) gw = W * 0.5f;
        float gh = gw * 3.0f / 4.0f;
        L.game.w = (int)gw; L.game.h = (int)gh;
        L.game.x = (W - L.game.w) / 2;
        L.game.y = (H - L.game.h) / 2;
        float side = (float)L.game.x;
        // controls sit a little towards the picture, away from the screen
        // edges where a camera cutout may be
        float lx = side * 0.54f, rx = W - side * 0.54f;
        float s = fminf(side * 0.74f, H * 0.42f);
        L.dpad = { lx, H * 0.60f, s / 2 };
        float br = fminf(side * 0.17f, H * 0.085f);
        L.a = { rx + br * 1.25f, H * 0.53f, br };
        L.b = { rx - br * 1.25f, H * 0.53f + br * 1.35f, br };
        float pw = fminf(side * 0.55f, H * 0.24f), ph = fminf(pw * 0.30f, H * 0.07f);
        L.l = { lx - pw / 2, H * 0.10f, pw, ph };
        L.r = { rx - pw / 2, H * 0.10f, pw, ph };
        L.select = { rx - pw / 2, H * 0.86f, pw, ph };
    } else {
        // game picture on top, controls underneath (like a Game Boy)
        L.game.w = W; L.game.h = W * 3 / 4;
        L.game.x = 0; L.game.y = (int)(H * 0.05f);
        float top = (float)(L.game.y + L.game.h);
        float ch = H - top;
        float s = fminf(W * 0.44f, ch * 0.55f);
        float cy = top + ch * 0.44f;
        L.dpad = { W * 0.26f, cy, s / 2 };
        float br = fminf(W * 0.085f, ch * 0.09f);
        L.a = { W * 0.83f, cy - br * 0.7f, br };
        L.b = { W * 0.83f - br * 2.5f, cy + br * 0.7f, br };
        float pw = fminf(W * 0.22f, ch * 0.22f), ph = pw * 0.32f;
        L.l = { W * 0.05f, top + ch * 0.07f, pw, ph };
        L.r = { W * 0.95f - pw, top + ch * 0.07f, pw, ph };
        L.select = { W * 0.5f - pw / 2, top + ch * 0.80f, pw, ph };
    }
}

// ---------------------------------------------------------------------------
// drawing helpers
// ---------------------------------------------------------------------------
static void FillCircle(float cx, float cy, float r, SDL_Color c)
{
    const int N = 48;
    SDL_Vertex v[N + 1];
    int idx[N * 3];
    v[0].position = { cx, cy }; v[0].color = c; v[0].tex_coord = { 0, 0 };
    for (int i = 0; i < N; i++) {
        float t = (float)i / N * 6.2831853f;
        v[i + 1].position = { cx + cosf(t) * r, cy + sinf(t) * r };
        v[i + 1].color = c; v[i + 1].tex_coord = { 0, 0 };
        idx[i * 3] = 0; idx[i * 3 + 1] = i + 1; idx[i * 3 + 2] = (i + 1) % N + 1;
    }
    SDL_RenderGeometry(Ren, NULL, v, N + 1, idx, N * 3);
}

static void FillRoundRect(SDL_FRect r, float rad, SDL_Color c)
{
    if (rad * 2 > r.w) rad = r.w / 2;
    if (rad * 2 > r.h) rad = r.h / 2;
    // build as a polygon fan around the centre
    const int Q = 10;
    SDL_Vertex v[4 * Q + 1];
    int idx[4 * Q * 3];
    float cxs[4] = { r.x + r.w - rad, r.x + rad, r.x + rad, r.x + r.w - rad };
    float cys[4] = { r.y + rad, r.y + rad, r.y + r.h - rad, r.y + r.h - rad };
    v[0].position = { r.x + r.w / 2, r.y + r.h / 2 }; v[0].color = c; v[0].tex_coord = { 0, 0 };
    int n = 1;
    for (int k = 0; k < 4; k++)
        for (int i = 0; i < Q; i++) {
            float t = (k * 90.0f + i * 90.0f / (Q - 1)) * 3.14159265f / 180.0f;
            v[n].position = { cxs[k] + cosf(t) * rad, cys[k] - sinf(t) * rad };
            v[n].color = c; v[n].tex_coord = { 0, 0 };
            n++;
        }
    int m = 0;
    for (int i = 1; i < n; i++) {
        idx[m++] = 0; idx[m++] = i; idx[m++] = (i % (n - 1)) + 1;
    }
    SDL_RenderGeometry(Ren, NULL, v, n, idx, m);
}

static void FillTriangle(float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color c)
{
    SDL_Vertex v[3];
    v[0].position = { x1, y1 }; v[1].position = { x2, y2 }; v[2].position = { x3, y3 };
    for (int i = 0; i < 3; i++) { v[i].color = c; v[i].tex_coord = { 0, 0 }; }
    SDL_RenderGeometry(Ren, NULL, v, 3, NULL, 0);
}

static void FreeLabels()
{
    for (int i = 0; i < C_COUNT; i++)
        if (LabelTex[i]) { SDL_DestroyTexture(LabelTex[i]); LabelTex[i] = NULL; }
}

static void MakeLabels()
{
    int size = (int)(L.a.r * 0.95f);
    if (size < 8) size = 8;
    if (LabelTex[C_A] && size == LabelFontSize) return;
    FreeLabels();
    if (LabelFont) TTF_CloseFont(LabelFont);
    LabelFont = TTF_OpenFont("font.ttf", size);
    LabelFontSize = size;
    if (!LabelFont) return;
    const char *txt[C_COUNT] = { NULL, "A", "B", "SELECT", "L", "R", NULL };
    SDL_Color white = { 235, 235, 225, 255 };
    for (int i = 0; i < C_COUNT; i++) {
        if (!txt[i]) continue;
        // small labels for the pills
        if (i == C_SELECT || i == C_L || i == C_R) TTF_SetFontSize(LabelFont, (int)(L.select.h * 0.55f));
        else TTF_SetFontSize(LabelFont, size);
        SDL_Surface *s = TTF_RenderText_Blended(LabelFont, txt[i], white);
        if (!s) continue;
        LabelTex[i] = SDL_CreateTextureFromSurface(Ren, s);
        LabelW[i] = s->w; LabelH[i] = s->h;
        SDL_FreeSurface(s);
    }
}

static void DrawLabel(int i, float cx, float cy, Uint8 alpha)
{
    if (!LabelTex[i]) return;
    SDL_SetTextureAlphaMod(LabelTex[i], alpha);
    SDL_FRect d = { cx - LabelW[i] / 2.0f, cy - LabelH[i] / 2.0f, (float)LabelW[i], (float)LabelH[i] };
    SDL_RenderCopyF(Ren, LabelTex[i], NULL, &d);
}

static bool CtrlActive(int c)
{
    for (size_t i = 0; i < Fingers.size(); i++)
        if (Fingers[i].ctrl == c) return true;
    return SDL_GetTicks() < FlashUntil[c];
}

static int DpadActiveDir()
{
    for (size_t i = 0; i < Fingers.size(); i++)
        if (Fingers[i].ctrl == C_DPAD && Fingers[i].dir) return Fingers[i].dir;
    if (SDL_GetTicks() < FlashUntil[C_DPAD]) return FlashDir;
    return 0;
}

static void DrawControls()
{
    const SDL_Color base   = { 58, 58, 56, 255 };
    const SDL_Color baseHi = { 120, 116, 96, 255 };
    const SDL_Color rim    = { 32, 32, 31, 255 };
    const SDL_Color arrow  = { 175, 172, 160, 255 };
    const SDL_Color gold   = { 176, 156, 74, 255 };
    const SDL_Color goldHi = { 230, 208, 120, 255 };

    SDL_SetRenderDrawBlendMode(Ren, SDL_BLENDMODE_BLEND);

    // D-pad: a cross on a shallow round well
    float cx = L.dpad.x, cy = L.dpad.y, s = L.dpad.r;
    float t = s * 0.36f;
    FillCircle(cx, cy, s * 1.08f, rim);
    SDL_FRect h = { cx - s, cy - t, 2 * s, 2 * t };
    SDL_FRect v = { cx - t, cy - s, 2 * t, 2 * s };
    FillRoundRect(h, t * 0.35f, base);
    FillRoundRect(v, t * 0.35f, base);
    int d = DpadActiveDir();
    if (d) {
        SDL_FRect arm;
        if (d == DINGOO_BUTTON_UP)    arm = { cx - t, cy - s, 2 * t, s };
        if (d == DINGOO_BUTTON_DOWN)  arm = { cx - t, cy, 2 * t, s };
        if (d == DINGOO_BUTTON_LEFT)  arm = { cx - s, cy - t, s, 2 * t };
        if (d == DINGOO_BUTTON_RIGHT) arm = { cx, cy - t, s, 2 * t };
        FillRoundRect(arm, t * 0.35f, baseHi);
    }
    FillCircle(cx, cy, t * 0.45f, rim);
    float a1 = s * 0.80f, a2 = s * 0.52f, aw = t * 0.55f;
    FillTriangle(cx, cy - a1, cx - aw, cy - a2, cx + aw, cy - a2, arrow);
    FillTriangle(cx, cy + a1, cx + aw, cy + a2, cx - aw, cy + a2, arrow);
    FillTriangle(cx - a1, cy, cx - a2, cy + aw, cx - a2, cy - aw, arrow);
    FillTriangle(cx + a1, cy, cx + a2, cy - aw, cx + a2, cy + aw, arrow);

    // A and B
    Circle btn[2] = { L.a, L.b };
    int ids[2] = { C_A, C_B };
    for (int i = 0; i < 2; i++) {
        bool on = CtrlActive(ids[i]);
        FillCircle(btn[i].x, btn[i].y + btn[i].r * 0.08f, btn[i].r * 1.06f, rim);
        FillCircle(btn[i].x, btn[i].y, btn[i].r, on ? goldHi : gold);
        DrawLabel(ids[i], btn[i].x, btn[i].y, 255);
    }

    // pills
    SDL_FRect pills[3] = { L.select, L.l, L.r };
    int pid[3] = { C_SELECT, C_L, C_R };
    for (int i = 0; i < 3; i++) {
        bool on = CtrlActive(pid[i]);
        SDL_FRect p = pills[i];
        FillRoundRect(p, p.h / 2, on ? baseHi : base);
        DrawLabel(pid[i], p.x + p.w / 2, p.y + p.h / 2, 220);
    }
}

// ---------------------------------------------------------------------------
// desktop test driver: WATERNET_SCRIPT=file runs lines like
//   wait 500 | tap 0.5 0.4 | drag x1 y1 x2 y2 | key RETURN | shot out.bmp | quit
// (coordinates are fractions of the window). Not compiled for Android.
// ---------------------------------------------------------------------------
#ifndef __ANDROID__
static FILE  *Script = NULL;
static Uint32 ScriptWaitUntil = 0;
static char   ShotName[512] = "";
static bool   ScriptStarted = false;

static void PushFinger(Uint32 type, float x, float y)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = type;
    e.tfinger.touchId = 1; e.tfinger.fingerId = 1;
    e.tfinger.x = x; e.tfinger.y = y; e.tfinger.pressure = 1;
    SDL_PushEvent(&e);
}

static void RunScript()
{
    if (!ScriptStarted) {
        ScriptStarted = true;
        const char *f = SDL_getenv("WATERNET_SCRIPT");
        if (f) Script = fopen(f, "r");
    }
    if (!Script || SDL_GetTicks() < ScriptWaitUntil || ShotName[0]) return;
    char line[512];
    while (fgets(line, sizeof line, Script)) {
        float a, b, c, d; int ms; char s[400];
        if (sscanf(line, "wait %d", &ms) == 1) { ScriptWaitUntil = SDL_GetTicks() + ms; return; }
        if (sscanf(line, "tap %f %f", &a, &b) == 2) {
            PushFinger(SDL_FINGERDOWN, a, b); PushFinger(SDL_FINGERUP, a, b);
            ScriptWaitUntil = SDL_GetTicks() + 120; return;
        }
        if (sscanf(line, "press %f %f", &a, &b) == 2) { PushFinger(SDL_FINGERDOWN, a, b); return; }
        if (sscanf(line, "release %f %f", &a, &b) == 2) { PushFinger(SDL_FINGERUP, a, b); return; }
        if (sscanf(line, "drag %f %f %f %f", &a, &b, &c, &d) == 4) {
            PushFinger(SDL_FINGERDOWN, a, b);
            for (int i = 1; i <= 5; i++) PushFinger(SDL_FINGERMOTION, a + (c - a) * i / 5, b + (d - b) * i / 5);
            PushFinger(SDL_FINGERUP, c, d);
            ScriptWaitUntil = SDL_GetTicks() + 120; return;
        }
        if (sscanf(line, "key %399s", s) == 1) {
            SDL_Event e; SDL_zero(e);
            e.type = SDL_KEYDOWN; e.key.state = SDL_PRESSED;
            e.key.keysym.sym = SDL_GetKeyFromName(s);
            SDL_PushEvent(&e);
            ScriptWaitUntil = SDL_GetTicks() + 120; return;
        }
        if (sscanf(line, "shot %399s", s) == 1) { snprintf(ShotName, sizeof ShotName, "%s", s); return; }
        if (strncmp(line, "quit", 4) == 0) {
            SDL_Event e; SDL_zero(e); e.type = SDL_QUIT; SDL_PushEvent(&e); return;
        }
    }
}

static void SaveShotIfRequested(int W, int H)
{
    if (!ShotName[0]) return;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (s && SDL_RenderReadPixels(Ren, NULL, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0)
        SDL_SaveBMP(s, ShotName);
    if (s) SDL_FreeSurface(s);
    ShotName[0] = 0;
}
#else
static void RunScript() {}
static void SaveShotIfRequested(int, int) {}
#endif

// ---------------------------------------------------------------------------
// video
// ---------------------------------------------------------------------------
static void CreateTextures()
{
    if (GameTex) SDL_DestroyTexture(GameTex);
    GameTex = SDL_CreateTexture(Ren, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, GameW, GameH);
    SDL_SetTextureScaleMode(GameTex, SDL_ScaleModeNearest);
    SDL_SetTextureBlendMode(GameTex, SDL_BLENDMODE_NONE);
    if (UpTex) SDL_DestroyTexture(UpTex);
    UpTex = NULL;
    UpK = 0;
    FreeLabels();
    LabelFontSize = 0;
}

SDL_Surface *Port_SetVideoMode(int w, int h, int bpp, Uint32 flags)
{
    (void)bpp; (void)flags;
    GameW = w; GameH = h;
    if (!Win) {
        Uint32 wf = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
        int ww = 1280, wh = 720;
#ifdef __ANDROID__
        wf |= SDL_WINDOW_FULLSCREEN;
#else
        const char *env = SDL_getenv("WATERNET_WINDOW");
        if (env) sscanf(env, "%dx%d", &ww, &wh);
#endif
        Win = SDL_CreateWindow("Waternet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh, wf);
        if (!Win) return NULL;
        Ren = SDL_CreateRenderer(Win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
        if (!Ren) Ren = SDL_CreateRenderer(Win, -1, SDL_RENDERER_TARGETTEXTURE);
        if (!Ren) return NULL;
    }
    CreateTextures();
    if (ScreenSurf) SDL_FreeSurface(ScreenSurf);
    ScreenSurf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGB888);
    return ScreenSurf;
}

int Port_Flip(SDL_Surface *screen)
{
    if (!Ren || !screen) return -1;
    if (TexturesLost) { CreateTextures(); TexturesLost = false; }

    int W, H;
    SDL_GetRendererOutputSize(Ren, &W, &H);
    if (W != LayoutW || H != LayoutH) {
        ComputeLayout(W, H);
        LayoutW = W; LayoutH = H;
        LabelFontSize = 0;   // re-render labels for the new size
        FreeLabels();
    }
    MakeLabels();

    SDL_UpdateTexture(GameTex, NULL, screen->pixels, screen->pitch);

    // sharp scaling: integer nearest upscale, then smooth downscale to fit
    int k = (int)ceil((double)L.game.h / GameH);
    if (k < 1) k = 1;
    if (k > 8) k = 8;
    if (k != UpK || !UpTex) {
        if (UpTex) SDL_DestroyTexture(UpTex);
        UpTex = SDL_CreateTexture(Ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, GameW * k, GameH * k);
        if (UpTex) {
            SDL_SetTextureScaleMode(UpTex, SDL_ScaleModeLinear);
            SDL_SetTextureBlendMode(UpTex, SDL_BLENDMODE_NONE);
        }
        UpK = k;
    }
    SDL_Texture *show = GameTex;
    if (UpTex && SDL_SetRenderTarget(Ren, UpTex) == 0) {
        SDL_RenderCopy(Ren, GameTex, NULL, NULL);
        SDL_SetRenderTarget(Ren, NULL);
        show = UpTex;
    }

    SDL_SetRenderDrawColor(Ren, 21, 21, 20, 255);
    SDL_RenderClear(Ren);
    SDL_RenderCopy(Ren, show, NULL, &L.game);
    DrawControls();
    SaveShotIfRequested(W, H);
    SDL_RenderPresent(Ren);

    // never run faster than ~60 fps, even without vsync
    Uint32 now = SDL_GetTicks();
    if (now - LastFlip < 16) SDL_Delay(16 - (now - LastFlip));
    LastFlip = SDL_GetTicks();
    return 0;
}

// ---------------------------------------------------------------------------
// input
// ---------------------------------------------------------------------------
void Port_PushKey(SDL_Keycode key)
{
    KeyQueue.push_back(key);
    int c = C_NONE;
    switch (key) {
        case DINGOO_BUTTON_A: c = C_A; break;
        case DINGOO_BUTTON_B: c = C_B; break;
        case DINGOO_BUTTON_SELECT: c = C_SELECT; break;
        case DINGOO_BUTTON_L: c = C_L; break;
        case DINGOO_BUTTON_R: c = C_R; break;
        case DINGOO_BUTTON_UP: case DINGOO_BUTTON_DOWN:
        case DINGOO_BUTTON_LEFT: case DINGOO_BUTTON_RIGHT:
            c = C_DPAD; FlashDir = key; break;
    }
    if (c != C_NONE) FlashUntil[c] = SDL_GetTicks() + 110;
}

static SDL_Keycode MapKey(SDL_Keycode k)
{
    switch (k) {
        case SDLK_UP: case SDLK_KP_8: return DINGOO_BUTTON_UP;
        case SDLK_DOWN: case SDLK_KP_2: return DINGOO_BUTTON_DOWN;
        case SDLK_LEFT: case SDLK_KP_4: return DINGOO_BUTTON_LEFT;
        case SDLK_RIGHT: case SDLK_KP_6: return DINGOO_BUTTON_RIGHT;
        case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_z: case SDLK_SPACE:
        case SDLK_LCTRL: case SDLK_SELECT: case SDLK_KP_5:
            return DINGOO_BUTTON_A;
        case SDLK_x: case SDLK_LALT: return DINGOO_BUTTON_B;
        case SDLK_ESCAPE: case SDLK_AC_BACK: case SDLK_BACKSPACE: return DINGOO_BUTTON_SELECT;
        case SDLK_q: case SDLK_PAGEUP: return DINGOO_BUTTON_L;
        case SDLK_w: case SDLK_e: case SDLK_PAGEDOWN: return DINGOO_BUTTON_R;
        case SDLK_EQUALS: case SDLK_PLUS: case SDLK_KP_PLUS: return DINGOO_BUTTON_X;
        case SDLK_MINUS: case SDLK_KP_MINUS: return DINGOO_BUTTON_Y;
    }
    return 0;
}

static SDL_Keycode MapPadButton(Uint8 b)
{
    switch (b) {
        case SDL_CONTROLLER_BUTTON_A: return DINGOO_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_B: return DINGOO_BUTTON_B;
        case SDL_CONTROLLER_BUTTON_X: return DINGOO_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_Y: return DINGOO_BUTTON_B;
        case SDL_CONTROLLER_BUTTON_BACK: return DINGOO_BUTTON_SELECT;
        case SDL_CONTROLLER_BUTTON_START: return DINGOO_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return DINGOO_BUTTON_L;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return DINGOO_BUTTON_R;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return DINGOO_BUTTON_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return DINGOO_BUTTON_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return DINGOO_BUTTON_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return DINGOO_BUTTON_RIGHT;
    }
    return 0;
}

static bool InCircle(const Circle &c, float x, float y, float scale)
{
    float dx = x - c.x, dy = y - c.y;
    return dx * dx + dy * dy <= c.r * c.r * scale * scale;
}

static bool InRect(const SDL_FRect &r, float x, float y, float pad)
{
    return x >= r.x - pad && x <= r.x + r.w + pad && y >= r.y - pad && y <= r.y + r.h + pad;
}

static int DpadDir(float x, float y)
{
    float dx = x - L.dpad.x, dy = y - L.dpad.y;
    if (dx * dx + dy * dy < (L.dpad.r * 0.18f) * (L.dpad.r * 0.18f)) return 0;
    if (fabsf(dx) > fabsf(dy)) return dx < 0 ? DINGOO_BUTTON_LEFT : DINGOO_BUTTON_RIGHT;
    return dy < 0 ? DINGOO_BUTTON_UP : DINGOO_BUTTON_DOWN;
}

static int HitControl(float x, float y)
{
    if (InCircle(L.dpad, x, y, 1.35f)) return C_DPAD;
    // A/B: nearest of the two if within reach
    float da = (x - L.a.x) * (x - L.a.x) + (y - L.a.y) * (y - L.a.y);
    float db = (x - L.b.x) * (x - L.b.x) + (y - L.b.y) * (y - L.b.y);
    float reach = L.a.r * 1.6f;
    if (fminf(da, db) <= reach * reach) return da <= db ? C_A : C_B;
    float pad = L.select.h * 0.5f;
    if (InRect(L.select, x, y, pad)) return C_SELECT;
    if (InRect(L.l, x, y, pad)) return C_L;
    if (InRect(L.r, x, y, pad)) return C_R;
    if (x >= L.game.x && x < L.game.x + L.game.w && y >= L.game.y && y < L.game.y + L.game.h) return C_GAME;
    return C_NONE;
}

static const SDL_Keycode CtrlKey[C_COUNT] = {
    0, DINGOO_BUTTON_A, DINGOO_BUTTON_B, DINGOO_BUTTON_SELECT, DINGOO_BUTTON_L, DINGOO_BUTTON_R, 0
};

static void FingerDown(SDL_FingerID id, float x, float y)
{
    Finger f;
    f.id = id; f.sx = f.x = x; f.sy = f.y = y; f.t0 = SDL_GetTicks();
    f.dir = 0; f.nextRepeat = 0;
    f.ctrl = HitControl(x, y);
    if (f.ctrl == C_DPAD) {
        f.dir = DpadDir(x, y);
        if (f.dir) { KeyQueue.push_back(f.dir); f.nextRepeat = f.t0 + 400; }
    } else if (f.ctrl != C_NONE && f.ctrl != C_GAME) {
        KeyQueue.push_back(CtrlKey[f.ctrl]);
    }
    Fingers.push_back(f);
}

static void FingerMove(SDL_FingerID id, float x, float y)
{
    for (size_t i = 0; i < Fingers.size(); i++) {
        Finger &f = Fingers[i];
        if (f.id != id) continue;
        f.x = x; f.y = y;
        if (f.ctrl == C_DPAD) {
            int d = DpadDir(x, y);
            if (d != f.dir) {
                f.dir = d;
                if (d) { KeyQueue.push_back(d); f.nextRepeat = SDL_GetTicks() + 400; }
            }
        }
    }
}

static void FingerUp(SDL_FingerID id, float x, float y)
{
    for (size_t i = 0; i < Fingers.size(); i++) {
        Finger f = Fingers[i];
        if (f.id != id) continue;
        Fingers.erase(Fingers.begin() + i);
        if (f.ctrl != C_GAME) return;
        float dx = x - f.sx, dy = y - f.sy;
        float thr = L.game.h * 0.06f;
        if (fabsf(dx) < thr && fabsf(dy) < thr) {
            int gx = (int)((f.sx - L.game.x) * GameW / L.game.w);
            int gy = (int)((f.sy - L.game.y) * GameH / L.game.h);
            if (!Game_HandleTap(gx, gy))
                Port_PushKey(DINGOO_BUTTON_A);
        } else if (fabsf(dx) > fabsf(dy)) {
            Port_PushKey(dx < 0 ? DINGOO_BUTTON_RIGHT : DINGOO_BUTTON_LEFT);   // swipe = turn the page
        } else {
            Port_PushKey(dy < 0 ? DINGOO_BUTTON_DOWN : DINGOO_BUTTON_UP);
        }
        return;
    }
}

static void RepeatHeldDpad()
{
    Uint32 now = SDL_GetTicks();
    for (size_t i = 0; i < Fingers.size(); i++) {
        Finger &f = Fingers[i];
        if (f.ctrl == C_DPAD && f.dir && f.nextRepeat && now >= f.nextRepeat) {
            KeyQueue.push_back(f.dir);
            f.nextRepeat = now + 130;
        }
    }
}

// left analog stick -> D-pad
static int StickDir = 0;
static void HandleAxis(SDL_GameController *pad)
{
    int x = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
    int y = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
    int d = 0;
    if (abs(x) > 20000 || abs(y) > 20000) {
        if (abs(x) > abs(y)) d = x < 0 ? DINGOO_BUTTON_LEFT : DINGOO_BUTTON_RIGHT;
        else d = y < 0 ? DINGOO_BUTTON_UP : DINGOO_BUTTON_DOWN;
    } else if (abs(x) > 8000 || abs(y) > 8000) {
        d = StickDir;   // hysteresis
    }
    if (d != StickDir) {
        StickDir = d;
        if (d) Port_PushKey(d);
    }
}

int Port_PollEvent(SDL_Event *event)
{
    SDL_Event ev;
    RunScript();
    RepeatHeldDpad();
    for (;;) {
        if (!KeyQueue.empty()) {
            SDL_zerop(event);
            event->type = SDL_KEYDOWN;
            event->key.state = SDL_PRESSED;
            event->key.keysym.sym = KeyQueue.front();
            KeyQueue.pop_front();
            return 1;
        }
        if (!SDL_PollEvent(&ev)) return 0;

        int W = LayoutW > 0 ? LayoutW : 1, H = LayoutH > 0 ? LayoutH : 1;
        switch (ev.type) {
            case SDL_QUIT:
                *event = ev;
                return 1;
            case SDL_APP_WILLENTERBACKGROUND:
            case SDL_APP_TERMINATING:
                Game_SaveState();
                Fingers.clear();
                break;
            case SDL_RENDER_DEVICE_RESET:
            case SDL_RENDER_TARGETS_RESET:
                TexturesLost = true;
                break;
            case SDL_KEYDOWN: {
                if (ev.key.repeat) break;
                SDL_Keycode k = MapKey(ev.key.keysym.sym);
                if (k) Port_PushKey(k);
                break;
            }
            case SDL_FINGERDOWN:
                FingerDown(ev.tfinger.fingerId, ev.tfinger.x * W, ev.tfinger.y * H);
                break;
            case SDL_FINGERMOTION:
                FingerMove(ev.tfinger.fingerId, ev.tfinger.x * W, ev.tfinger.y * H);
                break;
            case SDL_FINGERUP:
                FingerUp(ev.tfinger.fingerId, ev.tfinger.x * W, ev.tfinger.y * H);
                break;
            case SDL_CONTROLLERDEVICEADDED:
                SDL_GameControllerOpen(ev.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN: {
                SDL_Keycode k = MapPadButton(ev.cbutton.button);
                if (k) Port_PushKey(k);
                break;
            }
            case SDL_CONTROLLERAXISMOTION: {
                SDL_GameController *pad = SDL_GameControllerFromInstanceID(ev.caxis.which);
                if (pad) HandleAxis(pad);
                break;
            }
            default:
                break;   // mouse, joystick, text, window events: not used by the game
        }
    }
}

// ---------------------------------------------------------------------------
// data files
// ---------------------------------------------------------------------------
static void MakeDirs(const std::string &path)
{
    for (size_t i = 1; i < path.size(); i++)
        if (path[i] == '/') mkdir(path.substr(0, i).c_str(), 0755);
    mkdir(path.c_str(), 0755);
}

static std::string ReadAll(const char *file)
{
    std::string out;
    SDL_RWops *rw = SDL_RWFromFile(file, "rb");
    if (!rw) return out;
    char buf[8192];
    size_t n;
    while ((n = SDL_RWread(rw, buf, 1, sizeof buf)) > 0) out.append(buf, n);
    SDL_RWclose(rw);
    return out;
}

bool Port_PrepareData()
{
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");     // desktop: mouse acts as a finger
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight Portrait");
#if defined(__ANDROID__) || defined(PORT_TEST_UNPACK)
    // The game uses plain fopen()/opendir() on relative paths, so the data
    // inside the APK is unpacked once (and again after each app update).
#ifdef __ANDROID__
    const char *base = SDL_AndroidGetInternalStoragePath();
#else
    const char *base = SDL_getenv("PORT_TEST_STORAGE");   // desktop test of this code
#endif
    if (!base) return false;
    std::string dir = std::string(base) + "/data";
    MakeDirs(dir);
    std::string manifest = ReadAll("manifest.txt");
    if (manifest.empty()) return false;
    std::string version = manifest.substr(0, manifest.find('\n'));
    std::string have;
    FILE *vf = fopen((dir + "/.version").c_str(), "rb");
    if (vf) { char b[256] = { 0 }; fread(b, 1, sizeof b - 1, vf); fclose(vf); have = b; }
    if (have != version) {
        size_t pos = manifest.find('\n') + 1;
        while (pos < manifest.size()) {
            size_t e = manifest.find('\n', pos);
            if (e == std::string::npos) e = manifest.size();
            std::string rel = manifest.substr(pos, e - pos);
            pos = e + 1;
            if (rel.empty()) continue;
            std::string data = ReadAll(rel.c_str());
            std::string out = dir + "/" + rel;
            MakeDirs(out.substr(0, out.rfind('/')));
            FILE *f = fopen(out.c_str(), "wb");
            if (f) { fwrite(data.data(), 1, data.size(), f); fclose(f); }
        }
        FILE *w = fopen((dir + "/.version").c_str(), "wb");
        if (w) { fwrite(version.data(), 1, version.size(), w); fclose(w); }
    }
    return chdir(dir.c_str()) == 0;
#else
    const char *env = SDL_getenv("WATERNET_DATA");
    if (env) return chdir(env) == 0;
    char *bp = SDL_GetBasePath();
    if (bp) {
        std::string d = std::string(bp) + "data";
        SDL_free(bp);
        struct stat st;
        if (stat(d.c_str(), &st) == 0) return chdir(d.c_str()) == 0;
    }
    return true;
#endif
}

void Port_Shutdown()
{
    FreeLabels();
    if (LabelFont) { TTF_CloseFont(LabelFont); LabelFont = NULL; }
    if (GameTex) SDL_DestroyTexture(GameTex);
    if (UpTex) SDL_DestroyTexture(UpTex);
    if (Ren) SDL_DestroyRenderer(Ren);
    if (Win) SDL_DestroyWindow(Win);
    GameTex = UpTex = NULL; Ren = NULL; Win = NULL;
}
