#pragma once
#include "mem.h"
#include "rand.h"
#include "vec.h"

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

static u8 *level_at(Level *level, i32 x, i32 y) {
    if (x < 0 || y < 0) return 0;
    if (x >= level->sx || y >= level->sy) return 0;
    return level->grid + y * level->sx + x;
}

static void level_set(Level *level, i32 x, i32 y, SnakeCell value) {
    u8 *cell = level_at(level, x, y);
    if (!cell) return;
    *cell = value;
}

static SnakeCell level_get(Level *level, i32 x, i32 y) {
    u8 *cell = level_at(level, x, y);
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

    SnakeCell head_cell = level_get(level, head.x, head.y);
    if (head_cell == SnakeCell_Food) {
        level->segment_target += 4;
        level->score++;
    }
    level->segment[0] = head;

    // Shrink
    if (level->segment_count > level->segment_target) {
        v2i last = level->segment[--level->segment_count];
        level_set(level, last.x, last.y, SnakeCell_Empty);
    }

    // Update head cell after shrinking
    head_cell = level_get(level, head.x, head.y);

    // Collision
    if (head_cell == SnakeCell_Snake) return head_cell;
    if (head_cell == SnakeCell_Wall) return head_cell;

    // OK
    level_set(level, head.x, head.y, SnakeCell_Snake);
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
            level_set(level, x, y, SnakeCell_Wall);
        }
    }

    // Set snake starting position and size
    v2i pos = {level->sx / 2, level->sy / 2};
    level->segment_target = 4;

    // Add first segment
    level->segment_count = 1;
    level->segment = mem_array(mem, v2i, level->sx * level->sy + 1);
    level->segment[0] = pos;
    level_set(level, pos.x, pos.y, SnakeCell_Snake);
    return level;
}
