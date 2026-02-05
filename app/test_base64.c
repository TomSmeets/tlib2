

void os_main(u32 argc, char **argv) {
    base64_test();
    for (u32 i = 1; i < argc; ++i) {
        Memory *mem = mem_new();

        Buffer input  = str_buf(argv[i]);
        Buffer output = base64_encode(mem, input);
        fmt_ss(fout, "> ", output.data, "\n");
        mem_free(mem);
    }
    os_exit(0);
}
