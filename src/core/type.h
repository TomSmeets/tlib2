// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// type.h - Basic constant size type definitions
#pragma once

// Helper macros
#define static_assert(cond) _Static_assert(cond, "")
#define array_count(a) (sizeof(a) / sizeof(a[0]))

// Base types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

typedef float f32;
typedef double f64;
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

#if __STDC_VERSION__ <= 201710L && !__cplusplus
typedef _Bool bool;
#define true 1
#define false 0
#endif
static_assert(sizeof(bool) == 1);

// Assertions
#define TO_STRING0(x) #x
#define TO_STRING(x) TO_STRING0(x)
#define assert(cond)                                                                                                                                 \
    if (!(cond)) os_fail(__FILE__ ":" TO_STRING(__LINE__) ": assert(" #cond ") failed\n")
#define assert_msg(cond, msg)                                                                                                                        \
    if (!(cond)) os_fail(__FILE__ ":" TO_STRING(__LINE__) ": assert(" #cond ") failed, " msg "\n")

#define MIN(A, B) ((A) <= (B) ? (A) : (B))
#define MAX(A, B) ((A) >= (B) ? (A) : (B))

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

static void std_memcpy(u8 *restrict dst, const u8 *restrict src, u32 size) {
    while (size--) *dst++ = *src++;
}

static void std_memmove(u8 *dst, const u8 *src, u32 size) {
    while (size--) *dst++ = *src++;
}

static void std_memzero(u8 *dst, u32 size) {
    while (size--) *dst++ = 0;
}

static void std_memset(u8 *dst, u8 value, u32 size) {
    while (size--) *dst++ = value;
}

static bool std_memcmp(const u8 *restrict a, const u8 *restrict b, u32 size) {
    while (size--) {
        if (*a++ != *b++) return false;
    }
    return true;
}
