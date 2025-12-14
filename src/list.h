// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// list.h - Generic linked list functions
#pragma once
#include "type.h"

// Append an element to a singly linked list with first and last pointers
#define LIST_APPEND(FIRST, LAST, EL)                                                                                                                 \
    do {                                                                                                                                             \
        if ((FIRST)) {                                                                                                                               \
            (LAST)->next = (EL);                                                                                                                     \
            (LAST) = (EL);                                                                                                                           \
        } else {                                                                                                                                     \
            (FIRST) = (LAST) = (EL);                                                                                                                 \
        }                                                                                                                                            \
    } while (0)

// Prepend item onto a singly linked list
#define LIST_PUSH(FIRST, EL)                                                                                                                         \
    do {                                                                                                                                             \
        (EL)->next = (FIRST);                                                                                                                        \
        (FIRST) = (EL);                                                                                                                              \
    } while (0)

#define LIST_NEXT(LIST, NEXT) (*(void **)((LIST) + (NEXT)))

static void list_append(void **first, void **last, void *el, u32 next) {
    // First time init
    if (!*first) {
        *first = *last = el;
        return;
    }

    LIST_NEXT(*last, next) = el;
    LIST_NEXT(el, next) = 0;
    *last = el;
}

static void list_prepend(void **first, void **last, void *el, u32 next) {
    if (last && !*last) *last = el;
    LIST_NEXT(el, next) = *first;
    *first = el;
}

// Split linked list into two equal parts
// Used for merge sorting
static void *list_sort_split(void *list, u32 next) {
    void *fast = list;
    void *slow = list;
    while (fast) {
        if (fast) fast = LIST_NEXT(fast, next);
        if (fast) fast = LIST_NEXT(fast, next);
        if (fast) slow = LIST_NEXT(slow, next);
    }

    // Split off slow
    void *mid = LIST_NEXT(slow, next);
    LIST_NEXT(slow, next) = 0;
    return mid;
}

static void ptr_swap(void **left, void **right) {
    void *tmp_left = *left;
    void *tmp_right = *right;
    *left = tmp_right;
    *right = tmp_left;
}

// Merge two sorted lists into one
static void *list_sort_merge(void *left, void *right, u32 next, int (*compare)(void *, void *)) {
    if (!left) return right;
    if (!right) return left;

    void *list_start = 0;
    void *list_end = 0;
    while (left || right) {
        if (!(!right || (left && compare(left, right) <= 0))) {
            ptr_swap(&left, &right);
        }

        void *new_left = LIST_NEXT(left, next);
        list_append(&list_start, &list_end, left, next);
        left = new_left;
    }
    return list_start;
}

// Sort a linked list
static void *list_sort(void *list, u32 next, int (*compare)(void *, void *)) {
    if (!list) return list;
    if (!LIST_NEXT(list, next)) return list;
    void *left = list;
    void *right = list_sort_split(list, next);
    left = list_sort(left, next, compare);
    right = list_sort(right, next, compare);
    return list_sort_merge(left, right, next, compare);
}

// Find the last item in a list
static void *list_last(void *list, u32 next) {
    if (!list) return 0;
    while (1) {
        void *next_item = LIST_NEXT(list, next);
        if (!next_item) return list;
        list = next_item;
    }
}
