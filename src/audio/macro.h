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
