#include "os.h"
#include "fmt.h"

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

typedef struct {
    Memory *mem;
    u32 sx, sy;
    u8 *grid;

    u32 segment_target;
    u32 segment_count;
    v2i segment[64];

    // When to continue
    time_t next_step;
} Snake;

static Snake *snake;

static u8 *grid_at(Snake *snake, i32 x, i32 y) {
    if (x < 0 || y < 0) return 0;
    if (x >= snake->sx || y >= snake->sy) return 0;
    return snake->grid + y * snake->sx + x;
}

static void grid_set(Snake *snake, i32 x, i32 y, SnakeCell value) {
    u8 *cell = grid_at(snake, x, y);
    if(!cell) return;
    *cell = value;
}

static SnakeCell grid_get(Snake *snake, i32 x, i32 y) {
    u8 *cell = grid_at(snake, x, y);
    if (!cell) return SnakeCell_Wall;
    return *cell;
}

static void grid_fill(Snake *snake, i32 x0, i32 y0, i32 sx, i32 sy, SnakeCell value) {
    for (i32 y = 0; y < sy; ++y) {
        for (i32 x = 0; x < sx; ++x) {
            grid_set(snake, x0 + x, y0 + y, value);
        }
    }
}

// Start a new level
static void snake_init(Snake *snake) {
}

void os_main(u32 argc, char **argv) {
    time_t now = os_time();
    if(!snake) {
        fmt_s(fout, "Hello World!\n");

        Memory *mem = mem_new();
        snake = mem_struct(mem, Snake);
        snake->mem = mem;
        snake->sx = 32;
        snake->sy = 16;
        snake->grid = mem_array(mem, u8, snake->sx * snake->sy);

        // Top
        grid_fill(snake, 0, 0, snake->sx, 1, SnakeCell_Wall);

        // Bottom
        grid_fill(snake, 0, snake->sy - 1, snake->sx, 1, SnakeCell_Wall);

        // Left
        grid_fill(snake, 0, 0, 1, snake->sy, SnakeCell_Wall);

        // Right
        grid_fill(snake, snake->sx - 1, 0, 1, snake->sy, SnakeCell_Wall);

        // Set snake starting position
        v2i pos = {snake->sx / 2, snake->sy / 2};
        snake->segment_target = 4;

        // Add first segment
        snake->segment_count = 1;
        snake->segment[0] = pos;
        grid_set(snake, pos.x, pos.y, SnakeCell_Snake);

        snake->next_step = os_time() + TIME_SEC * 1;
    }


    // Update

    // Grow
    if (now > snake->next_step) {
        snake->next_step += 500 * TIME_MS;

        for (i32 i = snake->segment_count; i > 0; --i) {
            snake->segment[i] = snake->segment[i - 1];
        }
        snake->segment_count++;

        // Shrink
        if (snake->segment_count > snake->segment_target) {
            v2i last = snake->segment[--snake->segment_count];
            grid_set(snake, last.x, last.y, SnakeCell_Empty);
        }

        // Move
        v2i head = snake->segment[0];
        head.y--;
        SnakeCell head_cell = grid_get(snake, head.x, head.y);
        snake->segment[0] = head;
        grid_set(snake, head.x, head.y, SnakeCell_Snake);

        if (head_cell != SnakeCell_Empty) {
            os_exit(0);
        }
    }

    // Draw Grid
    fmt_s(fout, "\n");
    fmt_s(fout, "\n");
    fmt_s(fout, "\n");
    for (i32 y = 0; y < snake->sy; ++y) {
        for (i32 x = 0; x < snake->sx; ++x) {
            switch(grid_get(snake, x, y)) {
                case SnakeCell_Empty:
                    fmt_s(fout, "  ");
                    break;
                case SnakeCell_Wall:
                    fmt_s(fout, "##");
                    break;
                default:
                    fmt_s(fout, " S");
                    break;
            }
        }
        fmt_s(fout, "\n");
    }
    os_sleep(snake->next_step - now);
}
