// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// tl.c - Cli utils using tlib
#include "os.h"
#include "base64.h"
#include "stream.h"
#include "gzip.h"
#include "arg.h"


static Buffer os_read_all(Memory *mem, File *file) {
    Stream *stream = stream_new(mem);
    u8 buffer[1024];
    for(;;) {
        size_t requested = sizeof(buffer);
        size_t used = 0;
        bool ok = os_read(file, buffer, requested, &used);
        if(!ok) return (Buffer){};
        stream_write_bytes(stream, used, buffer);
        if(used < requested) break;
    }
    return (Buffer){stream->buffer, stream->size};
}

static bool os_write_all(Buffer buffer, File *output) {
    return os_write(output, buffer.data, buffer.size, 0);
}

void os_main(u32 argc, char **argv){
    Memory *mem = mem_new();
    Arg arg = arg_new(argc, argv);

    if (arg_match(&arg, "hello", "Print Hello World")) {
        fmt_s(fout, "Hello World!\n");
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "base64", "Encode / Decode base64")) {
        bool encode = arg_match(&arg, "encode", "Encode Base64 data");
        bool decode = arg_match(&arg, "decode", "Decode Base64 data");
        if (!encode) decode = 1;
        arg_help_opt(&arg);

        Buffer input  = os_read_all(mem, os_stdin());
        Buffer output;
        if(encode) output = base64_encode(mem, input);
        if(decode) output = base64_decode(mem, input);
        assert(os_write_all(output, os_stdout()));
        os_exit(0);
        return;
    }

    if (arg_match(&arg, "gzip", "Read / Write Gzip")) {
        bool compress = arg_match(&arg, "compress", "Encode Base64 data");
        bool decompress = arg_match(&arg, "decompress", "Decode Base64 data");
        if (!compress) decompress = 1;
        arg_help_opt(&arg);

        Buffer input = os_read_all(mem, os_stdin());
        Buffer output = gzip_read(mem, input);
        assert(os_write_all(output, os_stdout()));

        os_exit(0);
        return;
    }

    arg_help(&arg);
    os_exit(1);
}
