// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// snake.c - A simple snake game
#include "fmt.h"
#include "os.h"
#include "pix.h"
#include "rand.h"
#include "sound.h"
#include "vec.h"

static void snake_play_sound(Pix *pix, f32 freq, f32 duration, f32 attack, f32 decay) {
    Memory *tmp = mem_new();

    // Construct sample array
    u32 sample_count = duration * PIX_AUDIO_RATE;
    Pix_Audio_Sample *samples = mem_array(tmp, Pix_Audio_Sample, sample_count);

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

    // Memory is not needed anymore
    mem_free(tmp);
}

typedef enum {
    SnakeCell_Empty,
    SnakeCell_Wall,
    SnakeCell_Snake,
    SnakeCell_Food,
} SnakeCell;

typedef struct {
    Memory *mem;

    // Grid
    u32 sx, sy;
    u8 *grid;

    // Snake
    v2i snake_dir;
    v2i next_dir;
    v2i next_next_dir;
    u32 segment_target;
    u32 segment_count;
    v2i *segment;

    u32 score;
    bool game_over;
    Rand rand;

    u32 food_timer;
    u32 snake_timer;
} Level;

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

static u8 *grid_at(Level *level, i32 x, i32 y) {
    if (x < 0 || y < 0) return 0;
    if (x >= level->sx || y >= level->sy) return 0;
    return level->grid + y * level->sx + x;
}

static void grid_set(Level *level, i32 x, i32 y, SnakeCell value) {
    u8 *cell = grid_at(level, x, y);
    if (!cell) return;
    *cell = value;
}

static SnakeCell grid_get(Level *level, i32 x, i32 y) {
    u8 *cell = grid_at(level, x, y);
    if (!cell) return SnakeCell_Wall;
    return *cell;
}

static SnakeCell snake_move(Level *level) {
    // Grow
    for (i32 i = level->segment_count; i > 0; --i) {
        level->segment[i] = level->segment[i - 1];
    }
    level->segment_count++;

    // Move
    v2i head = level->segment[0];
    head.x += level->snake_dir.x;
    head.y += level->snake_dir.y;

    SnakeCell head_cell = grid_get(level, head.x, head.y);
    if (head_cell == SnakeCell_Food) {
        level->segment_target += 4;
        level->score++;
    }
    level->segment[0] = head;

    // Shrink
    if (level->segment_count > level->segment_target) {
        v2i last = level->segment[--level->segment_count];
        grid_set(level, last.x, last.y, SnakeCell_Empty);
    }

    // Update head cell after shrinking
    head_cell = grid_get(level, head.x, head.y);

    // Collision
    if (head_cell == SnakeCell_Snake) return head_cell;
    if (head_cell == SnakeCell_Wall) return head_cell;

    // OK
    grid_set(level, head.x, head.y, SnakeCell_Snake);
    return head_cell;
}

static bool snake_place_food(Level *level) {
    u32 empty_count = 0;
    u32 food_count = 0;
    for (u32 i = 0; i < level->sx * level->sy; ++i) {
        SnakeCell cell = level->grid[i];
        if (cell == SnakeCell_Empty) empty_count++;
        if (cell == SnakeCell_Food) food_count++;
    }

    if (food_count < 4 && empty_count > 0) {
        if (++level->food_timer == 6) {
            level->food_timer = 0;
            u32 new_ix = rand_u32(&level->rand, 0, empty_count);
            for (u32 i = 0; i < level->sx * level->sy; ++i) {
                SnakeCell cell = level->grid[i];
                if (cell != SnakeCell_Empty) continue;
                if (new_ix-- != 0) continue;
                level->grid[i] = SnakeCell_Food;
                return true;
            }
        }
    }

    return false;
}

static Level *snake_level_new(Rand *rng) {
    Memory *mem = mem_new();
    Level *level = mem_struct(mem, Level);
    level->mem = mem;
    level->sx = 40;
    level->sy = 30;
    level->grid = mem_array_zero(level->mem, u8, level->sx * level->sy);
    level->rand = rand_fork(rng);

    // Draw border around level
    for (u32 i = 0; i < level->sx * level->sy; ++i) {
        u32 x = i % level->sx;
        u32 y = i / level->sx;
        if (x == 0 || y == 0 || x == level->sx - 1 || y == level->sy - 1) {
            grid_set(level, x, y, SnakeCell_Wall);
        }
    }

    // Set snake starting position and size
    v2i pos = {level->sx / 2, level->sy / 2};
    level->segment_target = 4;

    // Add first segment
    level->segment_count = 1;
    level->segment = mem_array(mem, v2i, level->sx * level->sy + 1);
    level->segment[0] = pos;
    grid_set(level, pos.x, pos.y, SnakeCell_Snake);
    return level;
}

static void snake_draw(Snake *snake) {
    Level *level = snake->level;
    Memory *tmp = mem_new();
    u8 *canvas = mem_array(tmp, u8, 4 * level->sx * level->sy);
    u8 *px = canvas;

    for (u32 i = 0; i < level->sy * level->sx; ++i) {
        u32 x = i % level->sx;
        u32 y = i / level->sx;

        SnakeCell cell = grid_get(level, x, y);
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
    mem_free(tmp);
}

void os_main(u32 argc, char **argv) {
    static Snake *snake;

    time_t now = os_time();
    if (!snake) {
        fmt_s(fout, "Hello World!\n");
        Memory *mem = mem_new();
        snake = mem_struct(mem, Snake);
        snake->mem = mem;
        snake->pix = pix_new("Snake", (v2i){800, 600});
        snake->rand = rand_new(os_rand());
        snake->level = snake_level_new(&snake->rand);
    }

    if (snake->level->game_over) {
        if (snake->level->score > snake->high_score) {
            snake->high_score = snake->level->score;
            snake_play_sound(snake->pix, 440.0, 0.5, 0, 0.5);
        } else {
            snake_play_sound(snake->pix, 110.0, 0.5, 0, 0.5);
        }
        fmt_s(fout, "\n");
        fmt_s(fout, "---- Game Over ----\n");
        fmt_su(fout, "Score:     ", snake->level->score, "\n");
        fmt_su(fout, "Highscore: ", snake->high_score, "\n");
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
        if (in.type == InputEvent_Quit) os_exit(0);
        if (in.type == InputEvent_KeyDown) {
            v2i dir = {};
            if (in.key_down == Key_W) dir.y = -1;
            if (in.key_down == Key_S) dir.y = +1;
            if (in.key_down == Key_A) dir.x = -1;
            if (in.key_down == Key_D) dir.x = +1;

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
    os_sleep(diff);
}
