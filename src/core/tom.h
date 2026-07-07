#pragma once
#include "buf.h"
#include "fmt.h"
#include "read.h"

typedef struct {
    u32 indent;
    Buffer key;
    Buffer val;
} Tom_Line;

static Tom_Line tom_read(Read *read) {
    for (;;) {
        Buffer line = read_line(read);

        // Calculate indent
        u32 indent = 0;
        while (indent < line.size && line.data[indent] == ' ') indent++;

        // Remove indent
        Buffer kv = buf_drop(line, indent);
        if (kv.size == 0) continue;

        u32 split = 0;
        while (split < kv.size && kv.data[split] != ' ') split++;

        Buffer key = buf_take(kv, split);
        Buffer val = buf_drop(kv, split + 1);

        key = buf_trim(key);
        val = buf_trim(val);

        return (Tom_Line){
            .indent = indent,
            .key = key,
            .val = val,
        };
    }
}

static void test_tom(void) {
    {
        Read read = read_from(str_buf("key value"));
        Tom_Line line = tom_read(&read);
        check(buf_eq(line.key, str_buf("key")));
        check(buf_eq(line.val, str_buf("value")));
        check(line.indent == 0);
        check(read_eof(&read));
    }

    {
        Read read = read_from(str_buf("  key value \nkey2       val2  \n\nx y\n"));
        Tom_Line line = tom_read(&read);
        check(buf_eq(line.key, str_buf("key")));
        check(buf_eq(line.val, str_buf("value")));
        check(line.indent == 2);

        line = tom_read(&read);
        check(buf_eq(line.key, str_buf("key2")));
        check(buf_eq(line.val, str_buf("val2")));
        check(line.indent == 0);

        line = tom_read(&read);
        check(buf_eq(line.key, str_buf("x")));
        check(buf_eq(line.val, str_buf("y")));
        check(line.indent == 0);
        check(read_eof(&read));
    }
}
