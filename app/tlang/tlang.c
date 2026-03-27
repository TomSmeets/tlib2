#include "tlang.h"
#include "os2.h"

void os_main(u32 argc, char **argv) {
    Memory *mem = mem_new();
    check_or(argc == 2) return;

    Buffer input = os_read_file(mem, argv[1]);
    Ast *ast = tlang_lex(mem, input);
    for (Ast *tok = ast; tok; tok = tok->next_token) {
        print("Tok: ", " type ", tok->type, " text ", tok->text);
    }
    Parse p = {.token = ast};
    ast = tlang_parse(&p);
    tlang_fmt(fout, ast, 0);
    Stack *env = tlang_eval_block(mem, ast);
    for (Stack *s = env; s; s = s->next) {
        print(s->name, " = ", s->value);
    }
    os_exit();
}
