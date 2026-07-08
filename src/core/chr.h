// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// chr.h - Methods on ASCII characters
#pragma once
#include "type.h"

// Check if the character is a whitespace character
static bool chr_is_whitespace(u8 chr) {
    return chr == ' ' || chr == '\n' || chr == '\t' || chr == '\r';
}

static bool chr_is_lower(u8 chr) {
    return chr >= 'a' && chr <= 'z';
}

static bool chr_is_upper(u8 chr) {
    return chr >= 'A' && chr <= 'Z';
}

static bool chr_is_alpha(u8 chr) {
    return chr_is_lower(chr) || chr_is_upper(chr);
}

static bool chr_is_digit(u8 chr) {
    return chr >= '0' && chr <= '9';
}
