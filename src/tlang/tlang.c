#include "tlang.h"
#include "fs.h"
#include "os_main.h"

static void os_main(void) {
    Memory *mem = mem_perm();
    check_or(os_argc == 2) return;

    Buffer input = fs_read(mem, os_argv[1]);
    Ast *ast = tlang_lex(mem, input);
    for (Ast *tok = ast; tok; tok = tok->next_token) {
        print("Tok: ", " type ", tok->type, " text ", tok->text);
    }
    Parse p = {.token = ast};
    ast = tlang_parse(&p);

    Fmt *f = fmt_new(mem);
    tlang_fmt(f, ast, 0);
    io_write(io_stdout(), fmt_end(f));

    Stack *env = tlang_eval_block(mem, ast);
    for (Stack *s = env; s; s = s->next) {
        print(s->name, " = ", s->value);
    }
    os_exit();
}
