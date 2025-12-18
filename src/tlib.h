// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// tlib - Helper functions for parsing tlib modules
#pragma once
#include "fmt.h"
#include "list.h"
#include "mem.h"
#include "parse.h"
#include "type.h"

typedef struct Module Module;
typedef struct Module_Link Module_Link;
typedef struct TLib TLib;

struct Module {
    char *name;
    char *path;
    char *info;
    u32 sort_index;
    Module_Link *deps;
    Module *next;
};

struct Module_Link {
    Module *module;
    Module_Link *next;
};

struct TLib {
    Memory *mem;
    Module *modules;
};

static TLib *tlib_new(Memory *mem) {
    TLib *lib = mem_struct(mem, TLib);
    lib->mem = mem;
    return lib;
}

// Parse module and f
static Module *module_get(TLib *lib, char *name) {
    for (Module *mod = lib->modules; mod; mod = mod->next) {
        if (str_eq(mod->name, name)) return mod;
    }

    Module *mod = mem_struct(lib->mem, Module);
    mod->name = name;
    LIST_PUSH(lib->modules, mod);
    return mod;
}

// Get or create a link between modules
static Module_Link *module_get_link(TLib *lib, Module *mod, Module *dep) {
    // Check if link already exists
    for (Module_Link *link = mod->deps; link; link = link->next) {
        if (link->module == dep) return link;
    }

    // Create a new link
    Module_Link *link = mem_struct(lib->mem, Module_Link);
    link->module = dep;
    LIST_PUSH(mod->deps, link);
    return link;
}

// Read entire file into memory
static Buffer os_read_file(Memory *mem, char *path) {
    File_Info info = {};
    assert(os_stat(path, &info));

    File *fd = os_open(path, Open_Read);
    u8 *file_data = mem_array(mem, u8, info.size);
    u64 bytes_read = 0;
    assert(os_read(fd, file_data, info.size, &bytes_read));
    assert(bytes_read == info.size);
    assert(os_close(fd));
    return (Buffer){file_data, info.size};
}

// Get file name of a path
static char *fs_base(char *path) {
    u32 i = str_len(path) - 1;
    for (;;) {
        if (path[i] == '/') return path + i + 1;
        if (i == 0) break;
        i--;
    }
    return path;
}

static char *buf_to_str(Memory *mem, Buffer buf) {
    char *str = mem_array(mem, char, buf.size + 1);
    std_memcpy((u8 *)str, buf.data, buf.size);
    str[buf.size] = 0;
    return str;
}

// Parse module and f
static Module *module_parse(TLib *lib, char *path) {
    char *name = fs_base(path);

    Module *mod = module_get(lib, name);
    mod->path = path;

    Memory *tmp = mem_new();
    Buffer buf = os_read_file(tmp, path);
    Parse *parse = parse_new(lib->mem, buf.data, buf.size);

    for (;;) {
        if (parse_eof(parse)) break;

        // Ignore copyright statement,
        if (parse_symbol(parse, "// Copyright")) {
            parse_line(parse);
            continue;
        }

        // First comment is the description
        if (parse_symbol(parse, "//")) {
            Buffer line = parse_line(parse);
            if (mod->info) continue;

            // Split on first '-' char
            for (u32 i = 0; i < line.size; ++i) {
                if (((char *)line.data)[i] != '-') continue;
                line.data += i + 1;
                line.size -= i + 1;
                break;
            }

            // Remove whitespace
            while (line.size > 0 && ((char *)line.data)[0] == ' ') {
                line.data++;
                line.size--;
            }

            mod->info = buf_to_str(lib->mem, line);
            continue;
        }

        // Ignore '#pragma once'
        if (parse_symbol(parse, "#pragma")) {
            parse_line(parse);
            continue;
        }

        // Parse includes
        if (parse_symbol(parse, "#include \"")) {
            Buffer line = parse_line(parse);

            // Drop ending '"'
            line.size -= 1;

            char *dep_name = buf_to_str(lib->mem, line);
            Module *dep = module_get(lib, dep_name);
            module_get_link(lib, mod, dep);
            continue;
        }
        break;
    }
    mem_free(tmp);
    return mod;
}

static void mod_expand_links(TLib *lib) {
    for (Module *mod = lib->modules; mod; mod = mod->next) {
        for (Module_Link *link = mod->deps; link; link = link->next) {
            for (Module_Link *link2 = link->module->deps; link2; link2 = link2->next) {
                module_get_link(lib, mod, link2->module);
            }
        }
    }
}

// Split linked list into two
static Module *mod_sort_split(Module *mod) {
    Module *fast = mod;
    Module *slow = mod;
    while (fast && fast->next) {
        fast = fast->next->next;
        if (fast) slow = slow->next;
    }
    Module *mid = slow->next;
    slow->next = 0;
    return mid;
}

// Compare two modules for sorting
static i32 mod_compare(Module *left, Module *right) {
    i32 cmp = 0;
    if (cmp == 0) cmp = (i32)left->sort_index - (i32)right->sort_index;
    if (cmp == 0) cmp = (i32)str_len(left->name) - (i32)str_len(right->name);
    return cmp;
}

static void mod_number(Module *mod) {
    if (mod->sort_index > 0) return;
    mod->sort_index = 1;

    for (Module_Link *link = mod->deps; link; link = link->next) {
        Module *dep = link->module;
        mod_number(dep);
        if (dep->sort_index >= mod->sort_index) {
            mod->sort_index = dep->sort_index + 1;
        }
    }
}

static i32 link_compare(Module_Link *left, Module_Link *right) {
    return mod_compare(left->module, right->module);
}

static void mod_sort(TLib *lib) {
    Module *sorted = 0;

    for (Module *mod = lib->modules; mod; mod = mod->next) {
        mod->sort_index = 0;
    }

    for (Module *mod = lib->modules; mod; mod = mod->next) {
        mod_number(mod);
        mod->deps = list_sort(mod->deps, offset_of(Module_Link, next), (void *)link_compare);
    }
    lib->modules = list_sort(lib->modules, offset_of(Module, next), (void *)mod_compare);
}
