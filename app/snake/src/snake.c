// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// snake.c - A simple snake game
#include "fmt.h"
#include "os.h"
#include "pix.h"
#include "rand.h"
#include "vec.h"

typedef enum {
    SnakeCell_Empty,
    SnakeCell_Wall,
    SnakeCell_Snake,
    SnakeCell_Food,
} SnakeCell;

// Game state
typedef struct {
    Memory *mem;
    Pix *pix;

    // Level
    Memory *level_mem;
    u32 sx, sy;
    u8 *grid;

    v2i snake_dir;
    bool input_up;
    bool input_down;
    bool input_left;
    bool input_right;
    u32 segment_target;
    u32 segment_count;
    v2i *segment;

    // When to continue
    time_t next_step;

    u32 score;
    bool game_over;
    Rand rand;
} Snake;

static Snake *snake;

static u8 *grid_at(Snake *snake, i32 x, i32 y) {
    if (x < 0 || y < 0) return 0;
    if (x >= snake->sx || y >= snake->sy) return 0;
    return snake->grid + y * snake->sx + x;
}

static void grid_set(Snake *snake, i32 x, i32 y, SnakeCell value) {
    u8 *cell = grid_at(snake, x, y);
    if (!cell) return;
    *cell = value;
}

static SnakeCell grid_get(Snake *snake, i32 x, i32 y) {
    u8 *cell = grid_at(snake, x, y);
    if (!cell) return SnakeCell_Wall;
    return *cell;
}

// Start a new level
static void snake_init(Snake *snake) {
    // Free previous level
    if (snake->level_mem) mem_free(snake->level_mem);
    snake->level_mem = mem_new();
    snake->sx = 30;
    snake->sy = 20;
    snake->grid = mem_array(snake->level_mem, u8, snake->sx * snake->sy);
    for (i32 y = 0; y < snake->sy; ++y) {
        for (i32 x = 0; x < snake->sx; ++x) {
            if (x == 0 || y == 0 || x == snake->sx - 1 || y == snake->sy - 1) {
                grid_set(snake, x, y, SnakeCell_Wall);
                continue;
            }
            grid_set(snake, x, y, SnakeCell_Empty);
        }
    }

    // Set snake starting position
    v2i pos = {snake->sx / 2, snake->sy / 2};
    snake->segment_target = 4;

    // Add first segment
    snake->segment_count = 1;
    snake->segment = mem_array(snake->level_mem, v2i, snake->sx * snake->sy + 1);
    snake->segment[0] = pos;
    grid_set(snake, pos.x, pos.y, SnakeCell_Snake);

    // Apply initial cooldown
    snake->next_step = os_time();
    snake->score = 0;
    snake->game_over = 0;
}

static bool snake_move(Snake *snake) {
    // Grow
    for (i32 i = snake->segment_count; i > 0; --i) {
        snake->segment[i] = snake->segment[i - 1];
    }
    snake->segment_count++;

    // Move
    v2i head = snake->segment[0];
    head.x += snake->snake_dir.x;
    head.y += snake->snake_dir.y;

    SnakeCell head_cell = grid_get(snake, head.x, head.y);
    if (head_cell == SnakeCell_Food) {
        snake->segment_target += 4;
        snake->score++;
    }
    snake->segment[0] = head;

    // Shrink
    if (snake->segment_count > snake->segment_target) {
        v2i last = snake->segment[--snake->segment_count];
        grid_set(snake, last.x, last.y, SnakeCell_Empty);
    }

    // Update head cell after shrinking
    head_cell = grid_get(snake, head.x, head.y);
    if (head_cell == SnakeCell_Snake) return false;
    if (head_cell == SnakeCell_Wall) return false;
    grid_set(snake, head.x, head.y, SnakeCell_Snake);
    return true;
}

static void snake_draw_pix(Snake *snake, Pix *pix) {
    Memory *tmp = mem_new();
    u8 *canvas = mem_array(tmp, u8, 3 * snake->sx * snake->sy);
    u8 *px = canvas;
    for (i32 y = 0; y < snake->sy; ++y) {
        for (i32 x = 0; x < snake->sx; ++x) {
            switch (grid_get(snake, x, y)) {
            case SnakeCell_Empty:
                *px++ = 0;
                *px++ = 0;
                *px++ = 0;
                break;
            case SnakeCell_Wall:
                *px++ = 255;
                *px++ = 255;
                *px++ = 255;
                break;
            case SnakeCell_Snake:
                *px++ = 0;
                *px++ = 255;
                *px++ = 0;
                break;
            case SnakeCell_Food:
                *px++ = 255;
                *px++ = 0;
                *px++ = 0;
                break;
            }
        }
    }
    pix_draw(pix, (v2i){snake->sx, snake->sy}, canvas);
    mem_free(tmp);
}

static void snake_play_sound(Pix *pix, f32 freq, f32 duration, f32 attack, f32 decay) {
    Memory *mem = mem_new();

    // Construct sample array
    u32 sample_count = duration * PIX_AUDIO_RATE;
    Pix_Audio_Sample *samples = mem_array(mem, Pix_Audio_Sample, sample_count);

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
    mem_free(mem);
}

static void snake_place_food(Snake *snake) {
    u32 empty_count = 0;
    u32 food_count = 0;
    for (i32 y = 0; y < snake->sy; ++y) {
        for (i32 x = 0; x < snake->sx; ++x) {
            SnakeCell cell = grid_get(snake, x, y);
            if (cell == SnakeCell_Empty) empty_count++;
            if (cell == SnakeCell_Food) food_count++;
        }
    }

    if (food_count < 4 && empty_count > 0) {
        u32 new_ix = rand_u32(&snake->rand, 0, empty_count);
        for (i32 y = 0; y < snake->sy; ++y) {
            for (i32 x = 0; x < snake->sx; ++x) {
                SnakeCell cell = grid_get(snake, x, y);
                if (cell != SnakeCell_Empty) continue;
                if (new_ix == 0) {
                    grid_set(snake, x, y, SnakeCell_Food);
                    snake_play_sound(snake->pix, 220.0f, 0.1f, 0, 0.1);
                    goto end;
                }
                new_ix--;
            }
        }
    }

end:
    (void)0;
}

void os_main(u32 argc, char **argv) {
    time_t now = os_time();

    if (!snake) {
        fmt_s(fout, "Hello World!\n");
        Memory *mem = mem_new();
        snake = mem_struct(mem, Snake);
        snake->mem = mem;
        snake->pix = pix_new("Snake", (v2i){800, 600});
        snake->rand = rand_new(os_rand());
        snake_init(snake);
    }

    while (1) {
        Input in = pix_input(snake->pix);
        if (in.type == InputEvent_None) break;
        if (in.type == InputEvent_Quit) os_exit(0);
        if (in.type == InputEvent_KeyDown) {
            snake_play_sound(snake->pix, 440.0, 0.1, 0, 0.1);
            if (in.key_down == Key_W) snake->input_up = 1;
            if (in.key_down == Key_S) snake->input_down = 1;
            if (in.key_down == Key_A) snake->input_left = 1;
            if (in.key_down == Key_D) snake->input_right = 1;
        }
    }

    if (snake->game_over) {
        snake_init(snake);
    }

    // Grow
    fmt_si(fout, "NOW: ", now, "\n");
    fmt_si(fout, "NEXT: ", snake->next_step, "\n");
    if (now > snake->next_step) {
        snake->next_step += 200 * TIME_MS;
        if (now > snake->next_step) {
            os_exit(1);
        }

        bool move_x = snake->snake_dir.x == 0;
        bool move_y = snake->snake_dir.y == 0;
        if (move_y) {
            if (snake->input_up) snake->snake_dir = (v2i){0, -1};
            if (snake->input_down) snake->snake_dir = (v2i){0, 1};
            snake->input_up = 0;
            snake->input_down = 0;
        }
        if (move_x) {
            if (snake->input_left) snake->snake_dir = (v2i){-1, 0};
            if (snake->input_right) snake->snake_dir = (v2i){1, 0};
            snake->input_left = 0;
            snake->input_right = 0;
        }

        if (snake->snake_dir.x != 0 || snake->snake_dir.y != 0) {
            bool ok = snake_move(snake);
            if (!ok) snake->game_over = true;
        }
        snake_place_food(snake);
    }

    // Draw Grid
    // snake_draw_ascii(snake, fout);
    snake_draw_pix(snake, snake->pix);

    time_t diff = now + (TIME_SEC / 200) - os_time();
    os_sleep(diff);
}
