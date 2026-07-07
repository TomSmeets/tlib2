// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// type.h - Basic constant size type definitions
#pragma once

// Shorthands for unsigned types
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

// Fixed size types
typedef __UINT8_TYPE__ u8;
typedef __INT8_TYPE__ i8;
#define U8_MAX 0xff

typedef __UINT16_TYPE__ u16;
typedef __INT16_TYPE__ i16;
#define U16_MAX 0xffff

typedef __UINT32_TYPE__ u32;
typedef __INT32_TYPE__ i32;
#define U32_MAX 0xffffffff

typedef __UINT64_TYPE__ u64;
typedef __INT64_TYPE__ i64;
#define U64_MAX 0xffffffffffffffff

// Floats
typedef float f32;
typedef double f64;

// Special types
typedef __INTPTR_TYPE__ intptr_t;

// Size Type
typedef __SIZE_TYPE__ size_t;
#define SIZE_KB ((size_t)1 << 10)
#define SIZE_MB ((size_t)1 << 20)
#define SIZE_GB ((size_t)1 << 30)

// Boolean
#if __STDC_VERSION__ <= 201710L && !__cplusplus
typedef _Bool bool;
#define true 1
#define false 0
#endif
static_assert(sizeof(bool) == 1);
