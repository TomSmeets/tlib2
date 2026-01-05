#include "fmt.h"
#include "os.h"

typedef enum {
    SnakeCell_Empty,
    SnakeCell_Wall,
    SnakeCell_Snake,
    SnakeCell_Food,
} SnakeCell;

typedef struct {
    i32 x;
    i32 y;
} v2i;

// Game state
typedef struct {
    Memory *mem;
    Memory *level_mem;
    u32 sx, sy;
    u8 *grid;

    u32 segment_target;
    u32 segment_count;
    v2i segment[64];

    // When to continue
    time_t next_step;

    u32 score;
    bool game_over;
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
    snake->segment[0] = pos;
    grid_set(snake, pos.x, pos.y, SnakeCell_Snake);

    // Apply initial cooldown
    snake->next_step = os_time() + TIME_SEC * 1;
    snake->score = 0;
    snake->game_over = 0;

    grid_set(snake, pos.x, 4, SnakeCell_Food);
}

static bool snake_move(Snake *snake, v2i dir) {
    // Grow
    for (i32 i = snake->segment_count; i > 0; --i) {
        snake->segment[i] = snake->segment[i - 1];
    }
    snake->segment_count++;

    // Move
    v2i head = snake->segment[0];
    head.y--;

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

void os_main(u32 argc, char **argv) {
    time_t now = os_time();

    if (!snake) {
        fmt_s(fout, "Hello World!\n");
        Memory *mem = mem_new();
        snake = mem_struct(mem, Snake);
        snake->mem = mem;
        snake_init(snake);
    }

    if (snake->game_over) {
        snake_init(snake);
    }

    // Update

    // Grow
    if (now > snake->next_step) {
        snake->next_step += 500 * TIME_MS;
        bool ok = snake_move(snake, (v2i){0, -1});

        if (!ok) {
            snake->game_over = true;
            snake->next_step = now + TIME_SEC * 2;
        }
    }

    // Draw Grid
    fmt_s(fout, "\n");
    fmt_s(fout, "\n");
    fmt_su(fout, "Score: ", snake->score, "\n");
    for (i32 y = 0; y < snake->sy; ++y) {
        for (i32 x = 0; x < snake->sx; ++x) {
            switch (grid_get(snake, x, y)) {
            case SnakeCell_Empty:
                fmt_s(fout, "  ");
                break;
            case SnakeCell_Wall:
                fmt_s(fout, "##");
                break;
            case SnakeCell_Snake:
                fmt_s(fout, " S");
                break;
            case SnakeCell_Food:
                fmt_s(fout, " *");
                break;
            }
        }
        fmt_s(fout, "\n");
    }
    os_sleep(snake->next_step - now);
}
