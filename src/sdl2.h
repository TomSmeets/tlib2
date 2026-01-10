#pragma once
#include "os.h"
#include "type.h"
#if 0
#include <SDL2/SDL.h>
#else
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
#define SDL_INIT_TIMER 0x00000001u
#define SDL_INIT_AUDIO 0x00000010u
#define SDL_INIT_VIDEO 0x00000020u
#define SDL_INIT_JOYSTICK 0x00000200u
#define SDL_INIT_HAPTIC 0x00001000u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS 0x00004000u
#define SDL_INIT_SENSOR 0x00008000u
#define SDL_INIT_NOPARACHUTE 0x00100000u

#define SDL_WINDOWPOS_CENTERED 0x2FFF0000u

typedef enum {
    SDL_RENDERER_SOFTWARE = 0x00000001,
    SDL_RENDERER_ACCELERATED = 0x00000002,
    SDL_RENDERER_PRESENTVSYNC = 0x00000004,
    SDL_RENDERER_TARGETTEXTURE = 0x00000008,
} SDL_RendererFlags;

typedef enum {
    SDL_PIXELFORMAT_RGB24 = 0x17101803,
} SDL_PixelFormatEnum;

typedef enum {
    SDL_TEXTUREACCESS_STATIC,
    SDL_TEXTUREACCESS_STREAMING,
    SDL_TEXTUREACCESS_TARGET,
} SDL_TextureAccess;

typedef u32 SDL_AudioDeviceID;
typedef u16 SDL_AudioFormat;
typedef void (*SDL_AudioCallback)(void *userdata, u8 *stream, int len);

typedef struct {
    int freq;
    SDL_AudioFormat format;
    u8 channels;
    u8 silence;
    u16 samples;
    u16 padding;
    u32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

typedef i32 SDL_Keycode;
typedef int SDL_Scancode;

typedef struct {
    SDL_Scancode scancode;
    SDL_Keycode sym;
    u16 mod;
    u32 unused;
} SDL_Keysym;

typedef struct {
    u32 type;
    u32 timestamp;
    u32 windowID;
    u8 state;
    u8 repeat;
    u8 padding2;
    u8 padding3;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct {
    u32 type;
    u32 timestamp;
    u32 windowID;
    u32 which;
    u8 button;
    u8 state;
    u8 clicks;
    u8 padding1;
    i32 x;
    i32 y;
} SDL_MouseButtonEvent;

typedef union {
    u32 type;
    SDL_KeyboardEvent key;
    SDL_MouseButtonEvent button;
    u8 padding[sizeof(void *) <= 8 ? 56 : sizeof(void *) == 16 ? 64 : 3 * sizeof(void *)];
} SDL_Event;
assert_static(sizeof(SDL_Event) == sizeof(((SDL_Event *)0)->padding));

typedef struct {
    int x, y;
    int w, h;
} SDL_Rect;

#define SDLK_UNKNOWN 0x00000000
#define SDLK_RETURN 0x0000000d
#define SDLK_ESCAPE 0x0000001b
#define SDLK_BACKSPACE 0x00000008
#define SDLK_TAB 0x00000009
#define SDLK_SPACE 0x00000020
#define SDLK_EXCLAIM 0x00000021
#define SDLK_QUOTEDBL 0x00000022
#define SDLK_HASH 0x00000023
#define SDLK_PERCENT 0x00000025
#define SDLK_DOLLAR 0x00000024
#define SDLK_AMPERSAND 0x00000026
#define SDLK_QUOTE 0x00000027
#define SDLK_LEFTPAREN 0x00000028
#define SDLK_RIGHTPAREN 0x00000029
#define SDLK_ASTERISK 0x0000002a
#define SDLK_PLUS 0x0000002b
#define SDLK_COMMA 0x0000002c
#define SDLK_MINUS 0x0000002d
#define SDLK_PERIOD 0x0000002e
#define SDLK_SLASH 0x0000002f
#define SDLK_0 0x00000030
#define SDLK_1 0x00000031
#define SDLK_2 0x00000032
#define SDLK_3 0x00000033
#define SDLK_4 0x00000034
#define SDLK_5 0x00000035
#define SDLK_6 0x00000036
#define SDLK_7 0x00000037
#define SDLK_8 0x00000038
#define SDLK_9 0x00000039
#define SDLK_COLON 0x0000003a
#define SDLK_SEMICOLON 0x0000003b
#define SDLK_LESS 0x0000003c
#define SDLK_EQUALS 0x0000003d
#define SDLK_GREATER 0x0000003e
#define SDLK_QUESTION 0x0000003f
#define SDLK_AT 0x00000040
#define SDLK_LEFTBRACKET 0x0000005b
#define SDLK_BACKSLASH 0x0000005c
#define SDLK_RIGHTBRACKET 0x0000005d
#define SDLK_CARET 0x0000005e
#define SDLK_UNDERSCORE 0x0000005f
#define SDLK_BACKQUOTE 0x00000060
#define SDLK_a 0x00000061
#define SDLK_b 0x00000062
#define SDLK_c 0x00000063
#define SDLK_d 0x00000064
#define SDLK_e 0x00000065
#define SDLK_f 0x00000066
#define SDLK_g 0x00000067
#define SDLK_h 0x00000068
#define SDLK_i 0x00000069
#define SDLK_j 0x0000006a
#define SDLK_k 0x0000006b
#define SDLK_l 0x0000006c
#define SDLK_m 0x0000006d
#define SDLK_n 0x0000006e
#define SDLK_o 0x0000006f
#define SDLK_p 0x00000070
#define SDLK_q 0x00000071
#define SDLK_r 0x00000072
#define SDLK_s 0x00000073
#define SDLK_t 0x00000074
#define SDLK_u 0x00000075
#define SDLK_v 0x00000076
#define SDLK_w 0x00000077
#define SDLK_x 0x00000078
#define SDLK_y 0x00000079
#define SDLK_z 0x0000007a
#define SDLK_CAPSLOCK 0x40000039
#define SDLK_F1 0x4000003a
#define SDLK_F2 0x4000003b
#define SDLK_F3 0x4000003c
#define SDLK_F4 0x4000003d
#define SDLK_F5 0x4000003e
#define SDLK_F6 0x4000003f
#define SDLK_F7 0x40000040
#define SDLK_F8 0x40000041
#define SDLK_F9 0x40000042
#define SDLK_F10 0x40000043
#define SDLK_F11 0x40000044
#define SDLK_F12 0x40000045
#define SDLK_PRINTSCREEN 0x40000046
#define SDLK_SCROLLLOCK 0x40000047
#define SDLK_PAUSE 0x40000048
#define SDLK_INSERT 0x40000049
#define SDLK_HOME 0x4000004a
#define SDLK_PAGEUP 0x4000004b
#define SDLK_DELETE 0x0000007f
#define SDLK_END 0x4000004d
#define SDLK_PAGEDOWN 0x4000004e
#define SDLK_RIGHT 0x4000004f
#define SDLK_LEFT 0x40000050
#define SDLK_DOWN 0x40000051
#define SDLK_UP 0x40000052
#define SDLK_NUMLOCKCLEAR 0x40000053
#define SDLK_KP_DIVIDE 0x40000054
#define SDLK_KP_MULTIPLY 0x40000055
#define SDLK_KP_MINUS 0x40000056
#define SDLK_KP_PLUS 0x40000057
#define SDLK_KP_ENTER 0x40000058
#define SDLK_KP_1 0x40000059
#define SDLK_KP_2 0x4000005a
#define SDLK_KP_3 0x4000005b
#define SDLK_KP_4 0x4000005c
#define SDLK_KP_5 0x4000005d
#define SDLK_KP_6 0x4000005e
#define SDLK_KP_7 0x4000005f
#define SDLK_KP_8 0x40000060
#define SDLK_KP_9 0x40000061
#define SDLK_KP_0 0x40000062
#define SDLK_KP_PERIOD 0x40000063
#define SDLK_APPLICATION 0x40000065
#define SDLK_POWER 0x40000066
#define SDLK_KP_EQUALS 0x40000067
#define SDLK_F13 0x40000068
#define SDLK_F14 0x40000069
#define SDLK_F15 0x4000006a
#define SDLK_F16 0x4000006b
#define SDLK_F17 0x4000006c
#define SDLK_F18 0x4000006d
#define SDLK_F19 0x4000006e
#define SDLK_F20 0x4000006f
#define SDLK_F21 0x40000070
#define SDLK_F22 0x40000071
#define SDLK_F23 0x40000072
#define SDLK_F24 0x40000073
#define SDLK_EXECUTE 0x40000074
#define SDLK_HELP 0x40000075
#define SDLK_MENU 0x40000076
#define SDLK_SELECT 0x40000077
#define SDLK_STOP 0x40000078
#define SDLK_AGAIN 0x40000079
#define SDLK_UNDO 0x4000007a
#define SDLK_CUT 0x4000007b
#define SDLK_COPY 0x4000007c
#define SDLK_PASTE 0x4000007d
#define SDLK_FIND 0x4000007e
#define SDLK_MUTE 0x4000007f
#define SDLK_VOLUMEUP 0x40000080
#define SDLK_VOLUMEDOWN 0x40000081
#define SDLK_KP_COMMA 0x40000085
#define SDLK_KP_EQUALSAS400 0x40000086
#define SDLK_ALTERASE 0x40000099
#define SDLK_SYSREQ 0x4000009a
#define SDLK_CANCEL 0x4000009b
#define SDLK_CLEAR 0x4000009c
#define SDLK_PRIOR 0x4000009d
#define SDLK_RETURN2 0x4000009e
#define SDLK_SEPARATOR 0x4000009f
#define SDLK_OUT 0x400000a0
#define SDLK_OPER 0x400000a1
#define SDLK_CLEARAGAIN 0x400000a2
#define SDLK_CRSEL 0x400000a3
#define SDLK_EXSEL 0x400000a4
#define SDLK_KP_00 0x400000b0
#define SDLK_KP_000 0x400000b1
#define SDLK_THOUSANDSSEPARATOR 0x400000b2
#define SDLK_DECIMALSEPARATOR 0x400000b3
#define SDLK_CURRENCYUNIT 0x400000b4
#define SDLK_CURRENCYSUBUNIT 0x400000b5
#define SDLK_KP_LEFTPAREN 0x400000b6
#define SDLK_KP_RIGHTPAREN 0x400000b7
#define SDLK_KP_LEFTBRACE 0x400000b8
#define SDLK_KP_RIGHTBRACE 0x400000b9
#define SDLK_KP_TAB 0x400000ba
#define SDLK_KP_BACKSPACE 0x400000bb
#define SDLK_KP_A 0x400000bc
#define SDLK_KP_B 0x400000bd
#define SDLK_KP_C 0x400000be
#define SDLK_KP_D 0x400000bf
#define SDLK_KP_E 0x400000c0
#define SDLK_KP_F 0x400000c1
#define SDLK_KP_XOR 0x400000c2
#define SDLK_KP_POWER 0x400000c3
#define SDLK_KP_PERCENT 0x400000c4
#define SDLK_KP_LESS 0x400000c5
#define SDLK_KP_GREATER 0x400000c6
#define SDLK_KP_AMPERSAND 0x400000c7
#define SDLK_KP_DBLAMPERSAND 0x400000c8
#define SDLK_KP_VERTICALBAR 0x400000c9
#define SDLK_KP_DBLVERTICALBAR 0x400000ca
#define SDLK_KP_COLON 0x400000cb
#define SDLK_KP_HASH 0x400000cc
#define SDLK_KP_SPACE 0x400000cd
#define SDLK_KP_AT 0x400000ce
#define SDLK_KP_EXCLAM 0x400000cf
#define SDLK_KP_MEMSTORE 0x400000d0
#define SDLK_KP_MEMRECALL 0x400000d1
#define SDLK_KP_MEMCLEAR 0x400000d2
#define SDLK_KP_MEMADD 0x400000d3
#define SDLK_KP_MEMSUBTRACT 0x400000d4
#define SDLK_KP_MEMMULTIPLY 0x400000d5
#define SDLK_KP_MEMDIVIDE 0x400000d6
#define SDLK_KP_PLUSMINUS 0x400000d7
#define SDLK_KP_CLEAR 0x400000d8
#define SDLK_KP_CLEARENTRY 0x400000d9
#define SDLK_KP_BINARY 0x400000da
#define SDLK_KP_OCTAL 0x400000db
#define SDLK_KP_DECIMAL 0x400000dc
#define SDLK_KP_HEXADECIMAL 0x400000dd
#define SDLK_LCTRL 0x400000e0
#define SDLK_LSHIFT 0x400000e1
#define SDLK_LALT 0x400000e2
#define SDLK_LGUI 0x400000e3
#define SDLK_RCTRL 0x400000e4
#define SDLK_RSHIFT 0x400000e5
#define SDLK_RALT 0x400000e6
#define SDLK_RGUI 0x400000e7
#define SDLK_MODE 0x40000101
#define SDLK_AUDIONEXT 0x40000102
#define SDLK_AUDIOPREV 0x40000103
#define SDLK_AUDIOSTOP 0x40000104
#define SDLK_AUDIOPLAY 0x40000105
#define SDLK_AUDIOMUTE 0x40000106
#define SDLK_MEDIASELECT 0x40000107
#define SDLK_WWW 0x40000108
#define SDLK_MAIL 0x40000109
#define SDLK_CALCULATOR 0x4000010a
#define SDLK_COMPUTER 0x4000010b
#define SDLK_AC_SEARCH 0x4000010c
#define SDLK_AC_HOME 0x4000010d
#define SDLK_AC_BACK 0x4000010e
#define SDLK_AC_FORWARD 0x4000010f
#define SDLK_AC_STOP 0x40000110
#define SDLK_AC_REFRESH 0x40000111
#define SDLK_AC_BOOKMARKS 0x40000112
#define SDLK_BRIGHTNESSDOWN 0x40000113
#define SDLK_BRIGHTNESSUP 0x40000114
#define SDLK_DISPLAYSWITCH 0x40000115
#define SDLK_KBDILLUMTOGGLE 0x40000116
#define SDLK_KBDILLUMDOWN 0x40000117
#define SDLK_KBDILLUMUP 0x40000118
#define SDLK_EJECT 0x40000119
#define SDLK_SLEEP 0x4000011a
#define SDLK_APP1 0x4000011b
#define SDLK_APP2 0x4000011c
#define SDLK_AUDIOREWIND 0x4000011d
#define SDLK_AUDIOFASTFORWARD 0x4000011e
#define SDLK_SOFTLEFT 0x4000011f
#define SDLK_SOFTRIGHT 0x40000120
#define SDLK_CALL 0x40000121
#define SDLK_ENDCALL 0x40000122

#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON_X1 4
#define SDL_BUTTON_X2 5

typedef enum {
    SDL_FIRSTEVENT = 0,
    SDL_QUIT = 0x100,
    SDL_APP_TERMINATING,
    SDL_APP_LOWMEMORY,
    SDL_APP_WILLENTERBACKGROUND,
    SDL_APP_DIDENTERBACKGROUND,
    SDL_APP_WILLENTERFOREGROUND,
    SDL_APP_DIDENTERFOREGROUND,
    SDL_LOCALECHANGED,
    SDL_DISPLAYEVENT = 0x150,
    SDL_WINDOWEVENT = 0x200,
    SDL_SYSWMEVENT,
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP,
    SDL_TEXTEDITING,
    SDL_TEXTINPUT,
    SDL_KEYMAPCHANGED,
    SDL_TEXTEDITING_EXT,
    SDL_MOUSEMOTION = 0x400,
    SDL_MOUSEBUTTONDOWN,
    SDL_MOUSEBUTTONUP,
    SDL_MOUSEWHEEL,
    SDL_JOYAXISMOTION = 0x600,
    SDL_JOYBALLMOTION,
    SDL_JOYHATMOTION,
    SDL_JOYBUTTONDOWN,
    SDL_JOYBUTTONUP,
    SDL_JOYDEVICEADDED,
    SDL_JOYDEVICEREMOVED,
    SDL_JOYBATTERYUPDATED,
    SDL_CONTROLLERAXISMOTION = 0x650,
    SDL_CONTROLLERBUTTONDOWN,
    SDL_CONTROLLERBUTTONUP,
    SDL_CONTROLLERDEVICEADDED,
    SDL_CONTROLLERDEVICEREMOVED,
    SDL_CONTROLLERDEVICEREMAPPED,
    SDL_CONTROLLERTOUCHPADDOWN,
    SDL_CONTROLLERTOUCHPADMOTION,
    SDL_CONTROLLERTOUCHPADUP,
    SDL_CONTROLLERSENSORUPDATE,
    SDL_CONTROLLERUPDATECOMPLETE_RESERVED_FOR_SDL3,
    SDL_CONTROLLERSTEAMHANDLEUPDATED,
    SDL_FINGERDOWN = 0x700,
    SDL_FINGERUP,
    SDL_FINGERMOTION,
    SDL_DOLLARGESTURE = 0x800,
    SDL_DOLLARRECORD,
    SDL_MULTIGESTURE,
    SDL_CLIPBOARDUPDATE = 0x900,
    SDL_DROPFILE = 0x1000,
    SDL_DROPTEXT,
    SDL_DROPBEGIN,
    SDL_DROPCOMPLETE,
    SDL_AUDIODEVICEADDED = 0x1100,
    SDL_AUDIODEVICEREMOVED,
    SDL_SENSORUPDATE = 0x1200,
    SDL_RENDER_TARGETS_RESET = 0x2000,
    SDL_RENDER_DEVICE_RESET,
    SDL_POLLSENTINEL = 0x7F00,
    SDL_USEREVENT = 0x8000,
    SDL_LASTEVENT = 0xFFFF,
} SDL_EventType;

typedef enum {
    SDL_ScaleModeNearest,
    SDL_ScaleModeLinear,
    SDL_ScaleModeBest,
} SDL_ScaleMode;

#define AUDIO_F32 0x8120

#endif

typedef struct {
    int (*SDL_InitSubSystem)(u32 flags);
    SDL_Window *(*SDL_CreateWindow)(const char *title, int x, int y, int w, int h, u32 flags);
    SDL_Renderer *(*SDL_CreateRenderer)(SDL_Window *window, int index, u32 flags);
    SDL_Texture *(*SDL_CreateTexture)(SDL_Renderer *renderer, u32 format, int access, int w, int h);
    int (*SDL_RenderCopy)(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
    int (*SDL_UpdateTexture)(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
    void (*SDL_RenderPresent)(SDL_Renderer *renderer);
    SDL_AudioDeviceID (*SDL_OpenAudioDevice)(
        const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes
    );
    void (*SDL_PauseAudioDevice)(SDL_AudioDeviceID dev, int pause_on);
    void (*SDL_LockAudioDevice)(SDL_AudioDeviceID dev);
    void (*SDL_UnlockAudioDevice)(SDL_AudioDeviceID dev);
    void (*SDL_DestroyTexture)(SDL_Texture *texture);
    void (*SDL_CloseAudioDevice)(SDL_AudioDeviceID dev);
    void (*SDL_DestroyRenderer)(SDL_Renderer *renderer);
    void (*SDL_DestroyWindow)(SDL_Window *window);
    void (*SDL_Quit)(void);
    int (*SDL_PollEvent)(SDL_Event *event);
    int (*SDL_SetTextureScaleMode)(SDL_Texture *texture, SDL_ScaleMode scaleMode);
} SDL2;

static void sdl2_load(SDL2 *sdl) {
    Library *handle = os_dlopen("libSDL2.so");
    sdl->SDL_InitSubSystem = os_dlsym(handle, "SDL_InitSubSystem");
    sdl->SDL_CreateWindow = os_dlsym(handle, "SDL_CreateWindow");
    sdl->SDL_CreateRenderer = os_dlsym(handle, "SDL_CreateRenderer");
    sdl->SDL_CreateTexture = os_dlsym(handle, "SDL_CreateTexture");
    sdl->SDL_RenderCopy = os_dlsym(handle, "SDL_RenderCopy");
    sdl->SDL_UpdateTexture = os_dlsym(handle, "SDL_UpdateTexture");
    sdl->SDL_RenderPresent = os_dlsym(handle, "SDL_RenderPresent");
    sdl->SDL_OpenAudioDevice = os_dlsym(handle, "SDL_OpenAudioDevice");
    sdl->SDL_PauseAudioDevice = os_dlsym(handle, "SDL_PauseAudioDevice");
    sdl->SDL_LockAudioDevice = os_dlsym(handle, "SDL_LockAudioDevice");
    sdl->SDL_UnlockAudioDevice = os_dlsym(handle, "SDL_UnlockAudioDevice");
    sdl->SDL_DestroyTexture = os_dlsym(handle, "SDL_DestroyTexture");
    sdl->SDL_CloseAudioDevice = os_dlsym(handle, "SDL_CloseAudioDevice");
    sdl->SDL_DestroyRenderer = os_dlsym(handle, "SDL_DestroyRenderer");
    sdl->SDL_DestroyWindow = os_dlsym(handle, "SDL_DestroyWindow");
    sdl->SDL_Quit = os_dlsym(handle, "SDL_Quit");
    sdl->SDL_PollEvent = os_dlsym(handle, "SDL_PollEvent");
    sdl->SDL_SetTextureScaleMode = os_dlsym(handle, "SDL_SetTextureScaleMode");
}
