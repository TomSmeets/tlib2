// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// tl.c - Cli utils using tlib
#include "arg.h"
#include "base64.h"
#include "gzip.h"
#include "os.h"
#include "stream.h"

bool os_main2(u32 argc, char **argv) {
    Memory *mem = mem_new();
    Arg arg = arg_new(argc, argv);

    if (arg_match(&arg, "hello", "Print Hello World")) {
        fmt_s(fout, "Hello World!\n");
        return ok();
    }

    if (arg_match(&arg, "base64", "Encode / Decode base64")) {
        bool encode = arg_match(&arg, "encode", "Encode Base64 data");
        bool decode = arg_match(&arg, "decode", "Decode Base64 data");
        if (!encode) decode = 1;
        arg_help_opt(&arg);

        Stream *input = stream_new(mem);
        try(stream_from_file(input, os_stdin()));
        Buffer input_buffer = stream_to_buffer(input);
        Buffer output_buffer = {};
        if (encode) output_buffer = base64_encode(mem, input_buffer);
        if (decode) output_buffer = base64_decode(mem, input_buffer);
        Stream output = stream_from(output_buffer);
        stream_to_file(&output, os_stdout());
        return ok();
    }

    if (arg_match(&arg, "gzip", "Read / Write Gzip")) {
        bool compress = arg_match(&arg, "compress", "Encode Base64 data");
        bool decompress = arg_match(&arg, "decompress", "Decode Base64 data");
        if (!compress) decompress = 1;
        arg_help_opt(&arg);

        Stream *input = stream_new(mem);
        Stream *output = stream_new(mem);
        try(stream_from_file(input, os_stdin()));
        stream_seek(input, 0);
        try(gzip_read(mem, input, output));
        try(stream_to_file(output, os_stdout()));
        return ok();
    }

    if (arg_match(&arg, "dump", "Hexdump")) {
        Stream *input = stream_new(mem);
        try(stream_from_file(input, os_stdin()));
        fmt_hexdump(fout, stream_to_buffer(input));
        return ok();
    }
    arg_help(&arg);
    os_exit(1);
}

void os_main(u32 argc, char **argv) {
    if (!os_main2(argc, argv)) error_exit();
    os_exit(0);
}
