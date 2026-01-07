// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// type.h - Basic constant size type definitions
#pragma once

// Helper macros
#define static_assert(cond) _Static_assert(cond, "")
#define offset_of(type, field) __builtin_offsetof(type, field)
#define array_count(a) (sizeof(a) / sizeof(a[0]))

// Base types
typedef __UINT8_TYPE__ u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;
static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

typedef __INT8_TYPE__ i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
typedef __INT64_TYPE__ i64;
static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

// Biggest allocation size (unsigned)
typedef __SIZE_TYPE__ size_t;
static_assert(sizeof(size_t) == sizeof(void *));

#define SIZE_KB ((size_t)1 << 10)
#define SIZE_MB ((size_t)1 << 20)
#define SIZE_GB ((size_t)1 << 30)

// Signed pointer storage (signed)
typedef __INTPTR_TYPE__ intptr_t;
static_assert(sizeof(intptr_t) == sizeof(void *));

// Floats
typedef float f32;
typedef double f64;
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

// Time in micro seconds
// - if Absolute -> Unix epoch time
typedef i64 time_t;
#define TIME_MS ((time_t)1000)
#define TIME_SEC ((time_t)1000000)

// Boolean
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

// Other macros
#define MIN(A, B) ((A) <= (B) ? (A) : (B))
#define MAX(A, B) ((A) >= (B) ? (A) : (B))

#define U32_MAX 0xffffffff

#define TYPEDEF_STRUCT(NAME) typedef struct NAME NAME

// OS Detection
// Linux
#if __unix__
#define OS_LINUX 1
#define OS_WINDOWS 0
#define OS_WASM 0

// Windows
#elif _WIN32
#define OS_LINUX 0
#define OS_WINDOWS 1
#define OS_WASM 0

// WebAssembly
#elif __wasm__
#define OS_LINUX 0
#define OS_WINDOWS 0
#define OS_WASM 1
#endif
