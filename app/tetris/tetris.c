// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// tetris.c - A simple tetris game
#include "fmt.h"
#include "mem.h"
#include "os_main.h"
#include "pix.h"
#include "rand.h"
#include "sound.h"

#if OS_WASM
WASM_IMPORT(snake_update) void snake_update(u32 score, u32 highscore);
#endif

typedef struct {
    u8 x;
    u8 y;
} Pos;

// Game state
typedef struct {
    Memory *mem;
    Pix *pix;
    Rand rand;
    u8 level[16][8];

    u8 piece_color;
    Pos piece[4];
    u64 last_move;
} Tetris;

static void tetris_draw(Tetris *tetris) {
    u32 sy = 16;
    u32 sx = 8;

    Memory *tmp = mem_new();
    u32 *canvas = mem_array(tmp, u32, sx * sy);
    u32 *px = canvas;
    for (u32 i = 0; i < sx * sy; ++i) {
        u32 x = i % sx;
        u32 y = 15 - i / sx;
        u8 color = tetris->level[y][x];

        if (tetris->piece_color) {
            for (u32 i = 0; i < 4; ++i) {
                if (tetris->piece[i].x != x) continue;
                if (tetris->piece[i].y != y) continue;
                color = tetris->piece_color;
                break;
            }
        }

        switch (color) {
        case 0:
            *px++ = 0xff000000;
            break;
        case 1:
            *px++ = 0xff0000ff;
            break;
        case 2:
            *px++ = 0xff00ff00;
            break;
        case 3:
            *px++ = 0xffff0000;
            break;
        case 4:
            *px++ = 0xffffff00;
            break;
        case 5:
            *px++ = 0xffff00ff;
            break;
        case 6:
            *px++ = 0xff00ffff;
            break;
        default:
            *px++ = 0xffffffff;
            break;
        }
    }
    pix_draw(tetris->pix, (v2i){sx, sy}, (u8 *)canvas);
    mem_free(tmp);
}

static void tetris_collapse(Tetris *tetris) {
    u32 remove_count = 0;
    for (u32 y = 0; y < 16; ++y) {
        bool full = true;
        for (u32 x = 0; x < 8; ++x) {
            if (tetris->level[y][x]) continue;
            full = false;
            break;
        }
        if (full) {
            remove_count++;
        }

        if (remove_count > 0) {
            for (u32 x = 0; x < 8; ++x) {
                if (y + remove_count < 16) {
                    tetris->level[y][x] = tetris->level[y + remove_count][x];
                } else {
                    tetris->level[y][x] = 0;
                }
            }
        }

        if (full) y--;
    }
}

static void tetris_next_piece(Tetris *tetris) {
    u32 type = rand_u32(&tetris->rand, 0, 4);
    switch (type) {
    case 0:
        tetris->piece_color = 1;
        tetris->piece[0] = (Pos){3, 14};
        tetris->piece[1] = (Pos){3, 15};
        tetris->piece[2] = (Pos){4, 14};
        tetris->piece[3] = (Pos){4, 15};
        break;
    case 1:
        tetris->piece_color = 2;
        tetris->piece[0] = (Pos){3, 15};
        tetris->piece[1] = (Pos){3, 14};
        tetris->piece[2] = (Pos){3, 13};
        tetris->piece[3] = (Pos){3, 12};
        break;
    case 2:
        tetris->piece_color = 3;
        tetris->piece[0] = (Pos){3, 15};
        tetris->piece[1] = (Pos){3, 14};
        tetris->piece[2] = (Pos){3, 13};
        tetris->piece[3] = (Pos){4, 13};
        break;
    case 3:
        tetris->piece_color = 4;
        tetris->piece[0] = (Pos){4, 15};
        tetris->piece[1] = (Pos){4, 14};
        tetris->piece[2] = (Pos){4, 13};
        tetris->piece[3] = (Pos){3, 13};
        break;
    }
}

static void os_main(void) {
    static Tetris *tetris;

    time_t now = time_now();
    if (!tetris) {
        print("Hello World!");
        Memory *mem = mem_new();
        tetris = mem_struct(mem, Tetris);
        tetris->mem = mem;
        tetris->pix = pix_new(mem, "Tetris", (v2i){8 * 40, 16 * 40});
        tetris->rand = rand_new();
    }

    bool move_down = 0;
    bool move_left = 0;
    bool move_right = 0;

    if (now > tetris->last_move + 500 * TIME_MS) move_down = 1;

    // Gather input
    while (1) {
        Input in = pix_input(tetris->pix);
        if (in.type == InputEvent_None) break;
        if (in.type == InputEvent_Quit) os_exit();
        if (in.type == InputEvent_KeyDown) {
            if (in.key_down == Key_J) move_left = 1;
            if (in.key_down == Key_A) move_left = 1;
            if (in.key_down == Key_Left) move_left = 1;
            if (in.key_down == Key_D) move_right = 1;
            if (in.key_down == Key_L) move_right = 1;
            if (in.key_down == Key_Right) move_right = 1;
            if (in.key_down == Key_S) move_down = 1;
            if (in.key_down == Key_K) move_down = 1;
            if (in.key_down == Key_Down) move_down = 1;
        }
    }

    if (move_down || (move_left && move_right)) {
        move_left = 0;
        move_right = 0;
    }

    if (tetris->piece_color) {
        bool game_over = false;
        bool hit_y = false;
        bool hit_x = false;
        for (u32 i = 0; i < array_count(tetris->piece); ++i) {
            Pos p = tetris->piece[i];
            if (p.y == 0 || tetris->level[p.y - 1][p.x]) hit_y = true;
            if (move_left && (p.x == 0 || tetris->level[p.y][p.x - 1])) hit_x = true;
            if (move_right && (p.x == 7 || tetris->level[p.y][p.x + 1])) hit_x = true;
        }

        if (move_down) {
            tetris->last_move = now;
            if (hit_y) {
                for (u32 i = 0; i < array_count(tetris->piece); ++i) {
                    Pos p = tetris->piece[i];
                    if (p.y == 15) game_over = true;
                    tetris->level[p.y][p.x] = tetris->piece_color;
                }
                tetris->piece_color = 0;
            } else {
                for (u32 i = 0; i < array_count(tetris->piece); ++i) {
                    tetris->piece[i].y--;
                }
            }
        }
        if (move_left && !hit_x) {
            for (u32 i = 0; i < array_count(tetris->piece); ++i) tetris->piece[i].x--;
        }
        if (move_right && !hit_x) {
            for (u32 i = 0; i < array_count(tetris->piece); ++i) tetris->piece[i].x++;
        }

        if (game_over) {
            tetris->piece_color = 0;
            ptr_zero(tetris->level, sizeof(tetris->level));
        }
    } else {
        tetris_collapse(tetris);
        tetris_next_piece(tetris);
    }

    tetris_draw(tetris);

    time_t delay = 1000 * TIME_MS / 60;
    time_t diff = now + delay - time_now();
    os_sleep(diff);
}
