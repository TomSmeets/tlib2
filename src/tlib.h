// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// tlib - Helper functions for parsing tlib modules
#pragma once
#include "type.h"
#include "fmt.h"
#include "mem.h"
#include "parse.h"

typedef struct Module Module;
typedef struct Module_Link Module_Link;
typedef struct TLib TLib;


struct Module {
    char *name;
    char *path;
    char *info;
    u32 sort_index;
    Module_Link *deps, *deps_last;
    Module *next;
};

struct Module_Link {
    Module *module;
    Module_Link *next;
};

struct TLib {
    Memory *mem;
    Module *modules, *modules_last;
};


static TLib *tlib_new(Memory *mem) {
    TLib *lib = mem_struct(mem, TLib);
    lib->mem = mem;
    return lib;
}

// Parse module and f
static Module *module_get(TLib *lib, char *name) {
    for(Module *mod = lib->modules; mod; mod = mod->next) {
        if(str_eq(mod->name, name)) return mod;
    }

    Module *mod = mem_struct(lib->mem, Module);
    mod->name = name;
    LIST_APPEND(lib->modules, lib->modules_last, mod);
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
    LIST_APPEND(mod->deps, mod->deps_last, link);
    return link;
}

// Read entire file into memory
static Buffer os_read_file(Memory *mem, char *path) {
    File *fd = os_open(path, Open_Read);
    u64 file_size = os_file_size(fd);
    u8 *file_data = mem_array(mem, u8, file_size);
    assert(os_read(fd, file_data, file_size) == file_size);
    os_close(fd);
    return (Buffer){file_data, file_size};
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
    for(Module *mod = lib->modules; mod; mod = mod->next) {
        for (Module_Link *link = mod->deps; link; link = link->next) {
            for (Module_Link *link2 = link->module->deps; link2; link2 = link2->next) {
                module_get_link(lib, mod, link2->module);
            }
        }
    }
}

static void mod_sort2(Module *mod, Module **unsorted, Module **sorted) {
    // Check if already sorted
    for (Module *m = *sorted; m; m = m->next) {
        if(m == mod) return;
    }

    // Remove from unsorted list
    for (Module **ref = unsorted; *ref; ref = &(*ref)->next) {
        if(*ref != mod) continue;

        *ref = mod->next;
        mod->next = 0;
        break;
    }

    // Sort children
    for (Module_Link *link = mod->deps; link; link = link->next) {
        mod_sort2(link->module, unsorted, sorted);
    }

    // Add self
    mod->next = *sorted;
    *sorted = mod;
}

static void mod_sort(TLib *lib) {
    Module *sorted = 0;

    while (lib->modules) {
        u32 min_count = -1;
        Module *min_mod = lib->modules;
        for(Module *mod = lib->modules; mod; mod = mod->next) {
            u32 count = 0;
            for(Module_Link *link = mod->deps; link; link = link->next) {
                count++;
            }
            if (count < min_count) {
                min_count = count;
                min_mod = mod;
            }
        }
        mod_sort2(min_mod, &lib->modules, &sorted);
    }
    lib->modules = 0;
    while(sorted) {
        Module *mod = sorted;
        sorted = mod->next;
        mod->next = lib->modules;
        lib->modules = mod;
    }
}
