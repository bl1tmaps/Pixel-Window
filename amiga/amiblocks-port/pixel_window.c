#include "pixel_window.h"

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TAG_DONE
#define TAG_DONE 0UL
#endif

#ifndef WFLG_SIZEGADGET
#define WFLG_SIZEGADGET  0x00000001UL
#endif
#ifndef WFLG_DRAGBAR
#define WFLG_DRAGBAR     0x00000002UL
#endif
#ifndef WFLG_DEPTHGADGET
#define WFLG_DEPTHGADGET 0x00000004UL
#endif
#ifndef WFLG_CLOSEGADGET
#define WFLG_CLOSEGADGET 0x00000008UL
#endif
#ifndef WFLG_REPORTMOUSE
#define WFLG_REPORTMOUSE 0x00000200UL
#endif
#ifndef WFLG_ACTIVATE
#define WFLG_ACTIVATE    0x00001000UL
#endif

#ifndef IDCMP_MOUSEBUTTONS
#define IDCMP_MOUSEBUTTONS 0x00000008UL
#endif
#ifndef IDCMP_MOUSEMOVE
#define IDCMP_MOUSEMOVE    0x00000010UL
#endif
#ifndef IDCMP_CLOSEWINDOW
#define IDCMP_CLOSEWINDOW  0x00000200UL
#endif
#ifndef IDCMP_RAWKEY
#define IDCMP_RAWKEY       0x00000400UL
#endif

#ifndef SELECTDOWN
#define SELECTDOWN 0x68
#endif
#ifndef SELECTUP
#define SELECTUP   0xE8
#endif
#ifndef MENUDOWN
#define MENUDOWN   0x69
#endif
#ifndef MENUUP
#define MENUUP     0xE9
#endif
#ifndef MIDDLEDOWN
#define MIDDLEDOWN 0x6A
#endif
#ifndef MIDDLEUP
#define MIDDLEUP   0xEA
#endif

struct IntuitionBase *IntuitionBase = NULL;
struct Library *CyberGfxBase = NULL;
struct Library *GadToolsBase = NULL;
struct Library *AslBase = NULL;


struct PixelWindow {
    int width;
    int height;
    bool is_running;
    PwInputState input;
    struct Window* amg_win;
    uint32_t* rtg_buffer;
};

static void pw__clear_single_frame_input(PixelWindow* win) {
    memset(win->input.mouse_pressed, 0, sizeof(win->input.mouse_pressed));
    memset(win->input.mouse_released, 0, sizeof(win->input.mouse_released));
    memset(win->input.keys_pressed, 0, sizeof(win->input.keys_pressed));
    memset(win->input.keys_released, 0, sizeof(win->input.keys_released));
}

const PwInputState* pw_get_input(PixelWindow* win) {
    return &win->input;
}

static void pw__set_key(PixelWindow* win, int code, bool is_down) {
    if (code > 0 && code < 512) {
        if (is_down && !win->input.keys_down[code]) win->input.keys_pressed[code] = true;
        if (!is_down && win->input.keys_down[code]) win->input.keys_released[code] = true;
        win->input.keys_down[code] = is_down;
    }
}

static void pw__set_mouse(PixelWindow* win, int btn, bool is_down) {
    if (btn >= 0 && btn < 3) {
        if (is_down && !win->input.mouse_down[btn]) win->input.mouse_pressed[btn] = true;
        if (!is_down && win->input.mouse_down[btn]) win->input.mouse_released[btn] = true;
        win->input.mouse_down[btn] = is_down;
    }
}

static int pw__map_amiga_key(uint16_t code) {
    uint16_t base_code = code & 0x7F;
    static const int amiga_key_map[64] = {
        '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\\', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0, 0, 0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', 0, 0, 0, 0, 0,
        'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0, 0, 0, 0, 0
    };
    if (base_code < 64) return amiga_key_map[base_code];
    if (base_code == 0x40) return PW_KEY_SPACE;
    if (base_code == 0x41) return PW_KEY_BACKSPACE;
    if (base_code == 0x42) return PW_KEY_TAB;
    if (base_code == 0x44) return PW_KEY_ENTER;
    if (base_code == 0x45) return PW_KEY_ESCAPE;
    if (base_code == 0x4C) return PW_KEY_UP;
    if (base_code == 0x4D) return PW_KEY_DOWN;
    if (base_code == 0x4E) return PW_KEY_RIGHT;
    if (base_code == 0x4F) return PW_KEY_LEFT;
    return 0;
}

PixelWindow* pw_create_window(int width, int height, const char* title, bool resizable) {
    ULONG flags;
    PixelWindow* win;

    if (!IntuitionBase) IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 39);
    if (!CyberGfxBase) CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library", 0);
    if (!IntuitionBase) return NULL;

    win = (PixelWindow*)calloc(1, sizeof(PixelWindow));
    if (!win) return NULL;

    win->width = width;
    win->height = height;
    win->is_running = true;
    win->rtg_buffer = (uint32_t*)malloc(width * height * 4);

    flags = WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_ACTIVATE | WFLG_REPORTMOUSE;
    if (resizable) flags |= WFLG_SIZEGADGET;

    win->amg_win = OpenWindowTags(NULL,
        WA_Left, 20,
        WA_Top, 20,
        WA_Width, width + 8,
        WA_Height, height + 20,
        WA_Title, (ULONG)title,
        WA_Flags, flags,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY,
        TAG_DONE);

    if (!win->amg_win) {
        free(win->rtg_buffer);
        free(win);
        return NULL;
    }
    return win;
}

bool pw_process_events(PixelWindow* win) {
    struct IntuiMessage *msg;
    if (!win) return false;

    pw__clear_single_frame_input(win);

    while ((msg = (struct IntuiMessage *)GetMsg(win->amg_win->UserPort)) != NULL) {
        ULONG msgClass = msg->Class;
        UWORD msgCode = msg->Code;
        int16_t mx = msg->MouseX;
        int16_t my = msg->MouseY;

        ReplyMsg((struct Message *)msg);

        if (msgClass == IDCMP_CLOSEWINDOW) {
            win->is_running = false;
        } else if (msgClass == IDCMP_MOUSEMOVE) {
            float nx = (float)(mx - win->amg_win->BorderLeft) / (float)win->width;
            float ny = (float)(my - win->amg_win->BorderTop)  / (float)win->height;
            if (nx < 0.0f) nx = 0.0f;
            if (nx > 1.0f) nx = 1.0f;
            if (ny < 0.0f) ny = 0.0f;
            if (ny > 1.0f) ny = 1.0f;
            win->input.mouse_x = nx;
            win->input.mouse_y = ny;
        } else if (msgClass == IDCMP_MOUSEBUTTONS) {
            if (msgCode == SELECTDOWN) pw__set_mouse(win, 0, true);
            else if (msgCode == SELECTUP) pw__set_mouse(win, 0, false);
            else if (msgCode == MENUDOWN) pw__set_mouse(win, 1, true);
            else if (msgCode == MENUUP) pw__set_mouse(win, 1, false);
            else if (msgCode == MIDDLEDOWN) pw__set_mouse(win, 2, true);
            else if (msgCode == MIDDLEUP) pw__set_mouse(win, 2, false);
        } else if (msgClass == IDCMP_RAWKEY) {
            bool is_down = (msgCode & 0x80) == 0;
            pw__set_key(win, pw__map_amiga_key(msgCode), is_down);
        }
    }
    return win->is_running;
}

/* Declared in assembly below to call CyberGfxBase LVO -126 (WritePixelArray) */
extern ULONG cgx_WritePixelArray(APTR src, ULONG srcMod, struct RastPort *rp,
                                 ULONG dx, ULONG dy, ULONG w, ULONG h);

void pw_update_window(PixelWindow* win, const uint8_t* rgb_buffer) {
    uint32_t* dst;
    int pixels;
    int i;

    if (!win || !win->rtg_buffer || !CyberGfxBase) return;

    dst = win->rtg_buffer;
    pixels = win->width * win->height;

    for (i = 0; i < pixels; ++i) {
        *dst++ = ((uint32_t)rgb_buffer[i * 3] << 24) |
                 ((uint32_t)rgb_buffer[i * 3 + 1] << 16) |
                 ((uint32_t)rgb_buffer[i * 3 + 2] << 8) |
                 0xFFUL;
    }

    cgx_WritePixelArray(win->rtg_buffer, win->width * 4,
                        win->amg_win->RPort,
                        win->amg_win->BorderLeft,
                        win->amg_win->BorderTop,
                        win->width, win->height);
}

void pw_destroy_window(PixelWindow* win) {
    if (!win) return;
    if (win->amg_win) CloseWindow(win->amg_win);
    if (win->rtg_buffer) free(win->rtg_buffer);
    free(win);

    if (CyberGfxBase) { CloseLibrary(CyberGfxBase); CyberGfxBase = NULL; }
    if (IntuitionBase) { CloseLibrary((struct Library *)IntuitionBase); IntuitionBase = NULL; }
}
