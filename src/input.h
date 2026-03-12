// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// input.h - Generic input event type
#pragma once
#include "type.h"
#include "vec.h"

typedef enum {
    InputEvent_None,
    InputEvent_Quit,

    // Keyboard
    InputEvent_KeyDown,
    InputEvent_KeyUp,
    InputEvent_KeyUnicode,

    // Mouse
    InputEvent_MouseMove,
    InputEvent_MouseDown,
    InputEvent_MouseUp,

    // Window
    InputEvent_Window_Size,
} InputType;

typedef enum {
    Key_None,

    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    Key_Escape,
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,

    Key_Tilde,
    Key_Minus,
    Key_Equals,
    Key_Backspace,

    Key_Tab,
    Key_BracketOpen,
    Key_BracketClose,
    Key_Backslash,

    Key_Semicolon,
    Key_Quote,
    Key_Enter,

    Key_Shift,
    Key_Comma,
    Key_Dot,
    Key_Slash,

    Key_Control,
    Key_Win,
    Key_Alt,
    Key_Space,

    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,

    // Mouse keys
    Key_Mouse_Left,
    Key_Mouse_Middle,
    Key_Mouse_Right,
    Key_Mouse_Forward,
    Key_Mouse_Back,

    // Controller
    Key_Controller_Up,
    Key_Controller_Down,
    Key_Controller_Left,
    Key_Controller_Right,

    Key_Controller_A,
    Key_Controller_B,
    Key_Controller_X,
    Key_Controller_Y,

    Key_Controller_L1,
    Key_Controller_R1,
    Key_Controller_L2,
    Key_Controller_R2,
    Key_Controller_L3,
    Key_Controller_R3,
} Key;

typedef struct {
    InputType type;
    union {
        // Keyboard
        Key key_down;
        Key key_up;

        // unicode input
        u32 key_unicode;

        // Mouse
        v2i mouse_move;
        Key mouse_down;
        Key mouse_up;

        // Window
        v2i window_size;
    };
} Input;
