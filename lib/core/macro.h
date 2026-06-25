// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// macro.h - Useful macros
#pragma once

// Calculate byte offset of filed in a given type
#define offset_of(type, field) __builtin_offsetof(type, field)

// Count number of array items in a fixed size array
#define array_count(a) (sizeof(a) / sizeof(a[0]))

// Convert contents to string
#define TO_STRING0(x) #x
#define TO_STRING(x) TO_STRING0(x)

// Generic min and max macros
#define MIN(A, B) ((A) <= (B) ? (A) : (B))
#define MAX(A, B) ((A) >= (B) ? (A) : (B))

// Repeat 'M' For each argument
#define REPEAT_1(M, x) M(x)
#define REPEAT_2(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_1(M, __VA_ARGS__))
#define REPEAT_3(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_2(M, __VA_ARGS__))
#define REPEAT_4(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_3(M, __VA_ARGS__))
#define REPEAT_5(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_4(M, __VA_ARGS__))
#define REPEAT_6(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_5(M, __VA_ARGS__))
#define REPEAT_7(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_6(M, __VA_ARGS__))
#define REPEAT_8(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_7(M, __VA_ARGS__))
#define REPEAT_9(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_8(M, __VA_ARGS__))
#define REPEAT_10(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_9(M, __VA_ARGS__))
#define REPEAT_11(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_10(M, __VA_ARGS__))
#define REPEAT_12(M, x, ...) REPEAT_1(M, x) __VA_OPT__(; REPEAT_11(M, __VA_ARGS__))

// Call macro for each argument:
//   REPEAT(M, x, y, z, ...) => M(x); M(y); M(z);
#define REPEAT(M, ...) __VA_OPT__(REPEAT_12(M, __VA_ARGS__))
