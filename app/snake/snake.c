// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// snake.c - A simple snake game
#include "fmt.h"
#include "level.h"
#include "os.h"
#include "os_main.h"
#include "pix.h"
#include "rand.h"
#include "sound.h"
#include "time.h"
#include "vec.h"

static void snake_play_sound(Pix *pix, f32 freq, f32 duration, f32 attack, f32 decay) {
    // Construct sample array
    u32 sample_count = duration * PIX_AUDIO_RATE;
    Pix_Audio_Sample *samples = mem_array(mem_tmp(), Pix_Audio_Sample, sample_count);

    f32 dt = 1.0f / PIX_AUDIO_RATE;
    f32 phase = 0;
    for (u32 i = 0; i < sample_count; ++i) {
        f32 ta = i * dt;
        f32 td = (sample_count - i - 1) * dt;
        f32 va = attack > ta ? ta / attack : 1;
        f32 vd = decay > td ? td / decay : 1;
        f32 s = phase > 0.5 ? 1 : -1;

        s = s * 0.25 * va * vd;
        samples[i].left = s;
        samples[i].right = s;
        phase = f_fract(phase + dt * freq);
    }
    pix_play(pix, sample_count, samples);
}

// Game state
typedef struct {
    Memory *mem;
    Rand rand;
    Pix *pix;
    Level *level;

    // Input handling
    bool input_up;
    bool input_down;
    bool input_left;
    bool input_right;

    bool input_sprint;
    bool input_sprint2;

    u32 high_score;
} Snake;

static void snake_draw(Snake *snake) {
    Level *level = snake->level;
    u8 *canvas = mem_array(mem_tmp(), u8, 4 * level->sx * level->sy);
    u8 *px = canvas;

    for (u32 i = 0; i < level->sy * level->sx; ++i) {
        u32 x = i % level->sx;
        u32 y = i / level->sx;

        SnakeCell cell = level_get(level, x, y);
        if (cell == SnakeCell_Wall) {
            *px++ = 255;
            *px++ = 255;
            *px++ = 255;
            *px++ = 255;
        } else if (cell == SnakeCell_Snake) {
            *px++ = 0;
            *px++ = 255;
            *px++ = 0;
            *px++ = 255;
        } else if (cell == SnakeCell_Food) {
            *px++ = 255;
            *px++ = 0;
            *px++ = 0;
            *px++ = 255;
        } else {
            *px++ = 0;
            *px++ = 0;
            *px++ = 0;
            *px++ = 255;
        }
    }
    pix_draw(snake->pix, (v2i){level->sx, level->sy}, canvas);
}

static void os_main(void) {
    static Snake *snake;

    time_t now = os_time();
    if (!snake) {
        print("Hello World!");
        Memory *mem = mem_perm();
        snake = mem_struct(mem, Snake);
        snake->mem = mem;
        snake->pix = pix_new(mem, "Snake", (v2i){800, 600});
        snake->rand = rand_new();
        snake->level = snake_level_new(&snake->rand);
        wasm_call_vp("(s) => document.body.innerHTML = str_c(s)", R"(
            <canvas id='canvas'></canvas>
            <table>
                <tr>
                    <td>Score:</td><td id='score'>?</td>
                    <td>Highscore:</td><td id='highscore'>?</td>
                </tr>
            </table>
        )");

        wasm_call_vp("(s) => { let e = document.createElement('style'); e.innerHTML = str_c(s); document.head.appendChild(e); }", R"(
html,body {
    width: 100%;
    height: 100%;
    overflow: hidden;
    background-color: #cccccc;
}
canvas, table {
    background-color: #cccccc;
    image-rendering: pixelated;
    padding-left: 0;
    padding-right: 0;
    margin-left: auto;
    margin-right: auto;
    display: block;
    width: 800px;
}

table, th, td {
    border-collapse: collapse;
}

th, td { padding: 8px; width: 100px; background-color: white; }
        )");
    }

    if (snake->level->game_over) {
        if (snake->level->score > snake->high_score) {
            snake->high_score = snake->level->score;
            snake_play_sound(snake->pix, 440.0, 0.5, 0, 0.5);
        } else {
            snake_play_sound(snake->pix, 110.0, 0.5, 0, 0.5);
        }
        print("");
        print("---- Game Over ----");
        print("Score:     ", snake->level->score);
        print("Highscore: ", snake->high_score);
        wasm_call_vpi("(msg, y) => alert(str_c(msg) + y)", "Hello World: ", snake->level->score);
        mem_free(snake->level->mem);
        snake->level = snake_level_new(&snake->rand);
    }

    Level *level = snake->level;
    bool can_move_x = level->snake_dir.x == 0;
    bool can_move_y = level->snake_dir.y == 0;

    // Gather input
    while (1) {
        Input in = pix_input(snake->pix);
        if (in.type == InputEvent_None) break;
        if (in.type == InputEvent_Quit) os_exit();
        if (in.type == InputEvent_KeyDown) {
            v2i dir = {};
            if (in.key_down == Key_W || in.key_down == Key_Up) dir.y = -1;
            if (in.key_down == Key_S || in.key_down == Key_Down) dir.y = +1;
            if (in.key_down == Key_A || in.key_down == Key_Left) dir.x = -1;
            if (in.key_down == Key_D || in.key_down == Key_Right) dir.x = +1;

            if ((can_move_x && dir.x) || (can_move_y && dir.y)) {
                level->next_dir = dir;
            } else if (level->next_dir.x || level->next_dir.y) {
                level->next_next_dir = dir;
            }
        }

        bool key_down = in.type == InputEvent_KeyDown;
        bool key_up = in.type == InputEvent_KeyUp;
        if (key_down || key_up) {
            if (in.key_down == Key_Shift) snake->input_sprint = key_down;
            if (in.key_down == Key_Space) snake->input_sprint2 = key_down;
        }
    }

    if (level->next_dir.x || level->next_dir.y) {
        level->snake_dir = level->next_dir;
        level->next_dir = level->next_next_dir;
        level->next_next_dir = (v2i){0, 0};
    }

    if (level->snake_dir.x || level->snake_dir.y) {
        SnakeCell cell = snake_move(level);
        if (cell == SnakeCell_Snake || cell == SnakeCell_Wall) level->game_over = true;
        if (cell == SnakeCell_Food) {
            snake_play_sound(snake->pix, OCT_3 * NOTE_B, 0.1f, 0.0, 0.1);
        }
    }
    if (snake_place_food(snake->level)) {
        snake_play_sound(snake->pix, OCT_3 * NOTE_A, 0.1f, 0.0, 0.1);
    }
    snake_draw(snake);

    time_t delay = (snake->input_sprint || snake->input_sprint2) ? 50 * TIME_MS : 150 * TIME_MS;
    time_t diff = now + delay - os_time();
    wasm_call_vi("(x) => document.getElementById('score').innerText = x", snake->level->score);
    wasm_call_vi("(x) => document.getElementById('highscore').innerText = x", snake->high_score);
    os_sleep(diff);
}
