// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// tl.c - Cli utils using tlib
#include "arg.h"
#include "base64.h"
#include "gzip.h"
#include "os.h"
#include "stream.h"
#include "error2.h"

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
        if(compress) try(gzip_write(mem, stream_to_buffer(input), output));
        if(decompress) try(gzip_read(mem, input, output));
        try(stream_to_file(output, os_stdout()));
        return ok();
    }

    if (arg_match(&arg, "dump", "Hexdump")) {
        bool bin = arg_match(&arg, "bin", "Base 2");
        bool hex = arg_match(&arg, "hex", "Base 16");
        bool wide = arg_match(&arg, "wide", "Wide");
        bool compact = arg_match(&arg, "compact", "Less wide");
        if (!bin && !hex) hex = 1;
        if (!wide && !compact) wide = hex;
        arg_help_opt(&arg);

        Stream *input = stream_new(mem);
        try(stream_from_file(input, os_stdin()));

        u32 base = hex ? 16 : 2;
        u32 width= wide ? 16 : 4;
        fmt_hexdump_x(fout, stream_to_buffer(input), base, width);
        return ok();
    }
    arg_help(&arg);
    os_exit(1);
}

void os_main(u32 argc, char **argv) {
    if (!os_main2(argc, argv)) error_exit();
    os_exit(0);
}
