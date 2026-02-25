// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// huffman_tree.h - Huffman tree construction
#pragma once
#include "fmt.h"
#include "mem.h"

typedef struct Huffman_Tree Huffman_Tree;

struct Huffman_Tree {
    bool is_leaf;
    u32 symbol;
    u32 freq;
    Huffman_Tree *a;
    Huffman_Tree *b;
};

// Construct a new leaf node
static Huffman_Tree *huffman_tree_leaf(Memory *mem, u32 value, u32 freq) {
    Huffman_Tree *node = mem_struct(mem, Huffman_Tree);
    node->is_leaf = 1;
    node->symbol = value;
    node->freq = freq;
    return node;
}

// Construct a new tree non-leaf node
static Huffman_Tree *huffman_tree_node(Memory *mem, Huffman_Tree *a, Huffman_Tree *b) {
    Huffman_Tree *node = mem_struct(mem, Huffman_Tree);
    node->freq = a->freq + b->freq;
    node->a = a;
    node->b = b;
    return node;
}

// Remove least frequent node from the unordered list
static Huffman_Tree *_huffman_tree_remove_least_frequent_node(u32 *count, Huffman_Tree **nodes) {
    try(*count > 0);
    u32 smallest = 0;
    for (u32 i = 0; i < *count; ++i) {
        if (nodes[i]->freq >= nodes[smallest]->freq) continue;
        smallest = i;
    }
    // Swap remove
    Huffman_Tree *node = nodes[smallest];
    nodes[smallest] = nodes[--*count];
    return node;
}

// Construct a huffman tree for the given frequencies
static Huffman_Tree *huffman_tree_from(Memory *mem, u32 freq_count, u32 *freq_list, u32 min_freq) {
    Huffman_Tree **nodes = mem_array(mem, Huffman_Tree *, freq_count);

    // Add all leaf nodes
    u32 node_count = 0;
    for (u32 i = 0; i < freq_count; ++i) {
        u32 freq = freq_list[i];

        // Skip symbols that don't exist
        if (freq == 0) continue;

        // Increase frequency if it is below a threshold
        // This can be used to reduce final bit length
        if (freq < min_freq) freq = min_freq;

        // Allocate a leaf node
        nodes[node_count++] = huffman_tree_leaf(mem, i, freq);
    }

    // There should be at least one node
    try(node_count > 0);

    // Where there are more nodes, combine two least frequent nodes
    while (node_count > 1) {
        // Pop the two least frequent nodes
        Huffman_Tree *a = _huffman_tree_remove_least_frequent_node(&node_count, nodes);
        Huffman_Tree *b = _huffman_tree_remove_least_frequent_node(&node_count, nodes);

        // Add new combined node to list
        nodes[node_count++] = huffman_tree_node(mem, a, b);
    }
    return nodes[0];
}

// Compute maximum bit size needed for the tree
static u32 huffman_tree_max_depth(Huffman_Tree *tree) {
    if (tree->is_leaf) return 0;
    u32 a = huffman_tree_max_depth(tree->a);
    u32 b = huffman_tree_max_depth(tree->b);
    return MAX(a, b) + 1;
}

// Reduce required bit width of the frequency list by increasing the
// frequency of the lowest frequent item.
//
// This is not the most computationally efficient method, but it is simple and might even be optimal (I have no idea).
// Find lowest frequency that is higher than a threshold
// Returns 0 on failure
static u32 _huffman_tree_next_min_freq(u32 count, u32 *freq_list, u32 threshold) {
    // Find smallest freq
    u32 min_freq = -1;
    for (u32 i = 0; i < count; ++i) {
        u32 freq = freq_list[i];
        if (freq > threshold && freq < min_freq) {
            min_freq = freq;
        }
    }
    if (min_freq == -1) return 0;
    return min_freq;
}

// Construct a length limited huffman tree
static Huffman_Tree *huffman_tree_from_length_limited(Memory *mem, u32 count, u32 *freq_list, u32 max_depth) {
    u32 min_freq = 0;
    for (;;) {
        // Construct huffman tree
        Huffman_Tree *tree = huffman_tree_from(mem, count, freq_list, min_freq);
        try(tree);

        // Check if max depth was exceeded
        u32 depth = huffman_tree_max_depth(tree);
        if (depth <= max_depth) return tree;

        // Find next lowest frequency higher than the previous
        u32 next_min_freq = _huffman_tree_next_min_freq(count, freq_list, min_freq);

        // The lowest is never zero, so the first time we have to find the lowest twice to find the next lowest>
        if (min_freq == 0) next_min_freq = _huffman_tree_next_min_freq(count, freq_list, next_min_freq);

        // Returns zero when frequency cannot be reduced anymore
        try(next_min_freq);
        min_freq = next_min_freq;
    }
}

static bool _huffman_tree_to_lengths_at_depth(Huffman_Tree *tree, u32 count, u8 *symbol_length_list, u32 depth) {
    if (tree->is_leaf) {
        try(tree->symbol < count);
        symbol_length_list[tree->symbol] = depth;
    } else {
        try(_huffman_tree_to_lengths_at_depth(tree->a, count, symbol_length_list, depth + 1));
        try(_huffman_tree_to_lengths_at_depth(tree->b, count, symbol_length_list, depth + 1));
    }
    return ok();
}

// Convert a huffman tree to a list of symbol lengths
// NOTE: the list should be zero initialized and the correct length
static bool huffman_tree_to_lengths(Huffman_Tree *tree, u32 count, u8 *symbol_length_list) {
    return _huffman_tree_to_lengths_at_depth(tree, count, symbol_length_list, 0);
}

static bool huffman_tree_freq_to_lengths(u32 count, u32 *freq_list, u8 *len_list, u32 max_len){
    Memory *tmp = mem_new();
    Huffman_Tree *tree = huffman_tree_from_length_limited(tmp, count, freq_list, max_len);
    if(!tree) mem_free(tmp);
    try(tree);

    // Construct list of bit lengths
    bool ret = huffman_tree_to_lengths(tree, count, len_list);
    mem_free(tmp);
    return ret;
}

static bool huffman_tree_test(void) {
    char *input = "aaaaaaaabbbbcccdde";
    u32 freq_list[256] = {};

    // Count symbols
    for (u32 i = 0; input[i]; ++i) freq_list[(u8)input[i]]++;

    if (0) {
        // Debug printing
        for (u32 i = 0; i < array_count(freq_list); ++i) {
            if (!freq_list[i]) continue;
            fmt_c(fout, i);
            fmt_s(fout, ": ");
            fmt_u(fout, freq_list[i]);
            fmt_s(fout, "\n");
        }
    }

    try(freq_list['a'] == 8);
    try(freq_list['b'] == 4);
    try(freq_list['c'] == 3);
    try(freq_list['d'] == 2);
    try(freq_list['e'] == 1);
    try(freq_list['x'] == 0);

    Memory *mem = mem_new();

    {
        Huffman_Tree *tree = huffman_tree_from(mem, array_count(freq_list), freq_list, 0);
        try(tree);

        u8 len_list[256] = {};
        try(huffman_tree_to_lengths(tree, array_count(len_list), len_list));

        if (0) {
            // Debug printing
            fmt_s(fout, "Len:\n");
            for (u32 i = 0; i < array_count(len_list); ++i) {
                if (!len_list[i]) continue;
                fmt_c(fout, i);
                fmt_s(fout, ": ");
                fmt_u(fout, len_list[i]);
                fmt_s(fout, "\n");
            }
        }
        try(len_list['a'] == 1);
        try(len_list['b'] == 2);
        try(len_list['c'] == 3);
        try(len_list['d'] == 4);
        try(len_list['e'] == 4);
        try(len_list['x'] == 0);
    }

    {
        Huffman_Tree *tree = huffman_tree_from_length_limited(mem, array_count(freq_list), freq_list, 3);
        try(tree);

        u8 len_list[256] = {};
        try(huffman_tree_to_lengths(tree, array_count(len_list), len_list));

        if (0) {
            fmt_s(fout, "Len:\n");
            for (u32 i = 0; i < array_count(len_list); ++i) {
                if (!len_list[i]) continue;
                fmt_c(fout, i);
                fmt_s(fout, ": ");
                fmt_u(fout, len_list[i]);
                fmt_s(fout, "\n");
            }
        }
        try(len_list['a'] == 1);
        try(len_list['b'] == 3);
        try(len_list['c'] == 3);
        try(len_list['d'] == 3);
        try(len_list['e'] == 3);
        try(len_list['x'] == 0);
    }

    return ok();
}
