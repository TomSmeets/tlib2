#pragma once
#include "fmt.h"
#include "list.h"
#include "mem.h"
#include "type.h"

typedef enum {
    Ast_Type_None,
    Ast_Type_Number,
    Ast_Type_Label,
    Ast_Type_Operator,
    Ast_Type_Comment,
    Ast_Type_Error,
} Ast_Type;

typedef struct Ast Ast;
struct Ast {
    Ast_Type type;
    Buffer text;
    Ast *next_token;
    Ast *next;
    Ast *child;
};

#define LIST_APPEND_NEXT(START, END, ITEM, NEXT) \
    ({ \
        if (!(START)) (START) = (ITEM); \
        if ((END)) (END)->NEXT = (ITEM); \
        (END) = (ITEM); \
    })

static Ast *tlang_lex(Memory *mem, Buffer input) {
    Ast *list_start = 0;
    Ast *list_end = 0;

    u8 *cursor = input.data;
    u8 *end = input.data + input.size;

    while (cursor < end) {
        u8 *start = cursor++;
        u8 chr = *start;

        // Skip whitespace
        if (chr_is_whitespace(chr)) continue;

        // Create node
        Ast *node = mem_struct(mem, Ast);
        if (chr_is_digit(chr)) {
            node->type = Ast_Type_Number;
            while (cursor < end && (chr_is_digit(*cursor) || chr_is_alpha(*cursor) || *cursor == '.' || *cursor == '\'')) cursor++;
        } else if (chr_is_alpha(chr)) {
            node->type = Ast_Type_Label;
            while (cursor < end && (chr_is_digit(*cursor) || chr_is_alpha(*cursor))) cursor++;
        } else if (chr == '/' && *cursor == '/') {
            start += 2;
            node->type = Ast_Type_Comment;
            while (cursor < end && *cursor != '\n' && *cursor != '\r') cursor++;
        } else {
            node->type = Ast_Type_Operator;
        }
        node->text = buf_from(start, cursor - start);
        LIST_APPEND_NEXT(list_start, list_end, node, next_token);
    }
    return list_start;
}

typedef struct {
    Memory *mem;
    Ast *token;
} Parse;

static Ast *parse_fail(Parse *p, char *message) {
    Ast *err = mem_struct(p->mem, Ast);
    err->type = Ast_Type_Error;
    err->text = str_buf(message);
    return err;
}

static Ast *tlang_parse_token_ex(Parse *p, Ast_Type type, char *text) {
    Ast *tok = p->token;
    if (!tok) return 0;
    if (type && tok->type != type) return 0;
    if (text && !buf_eq(tok->text, str_buf(text))) return 0;
    p->token = tok->next_token;
    tok->child = 0;
    tok->next = 0;
    return tok;
}

static Ast *tlang_parse_number(Parse *parse) {
    return tlang_parse_token_ex(parse, Ast_Type_Number, 0);
}

static Ast *tlang_parse_op(Parse *parse, char *op) {
    return tlang_parse_token_ex(parse, Ast_Type_Operator, op);
}

static Ast *tlang_parse_label(Parse *parse) {
    return tlang_parse_token_ex(parse, Ast_Type_Label, 0);
}

static Ast *tlang_parse_literal(Parse *parse) {
    Ast *num = tlang_parse_number(parse);
    if (num) return num;

    Ast *word = tlang_parse_label(parse);
    if (word) return word;
    return 0;
}

static Ast *tlang_parse_mul(Parse *p) {
    Ast *lhs = tlang_parse_literal(p);
    Ast *op = tlang_parse_op(p, "*");
    if (!op) return lhs;

    Ast *rhs = tlang_parse_mul(p);
    if (!rhs) return 0;
    op->child = lhs;
    lhs->next = rhs;
    return op;
}

static Ast *tlang_parse_expr(Parse *p) {
    Ast *lhs = tlang_parse_mul(p);
    Ast *op = tlang_parse_op(p, "+");
    if (!op) return lhs;

    Ast *rhs = tlang_parse_expr(p);
    if (!rhs) return 0;
    op->child = lhs;
    lhs->next = rhs;
    return op;
}

static Ast *tlang_parse_statement(Parse *p) {
    Ast *label = tlang_parse_label(p);
    if (!label) return 0;

    Ast *op = tlang_parse_op(p, "=");
    if (op) {
        // [label] = [expr]
        Ast *expr = tlang_parse_expr(p);
        if (!expr) return 0;
        Ast *end = tlang_parse_op(p, ";");
        if (!end) return 0;
        op->child = label;
        label->next = expr;
        return op;
    }

    // [label] [expr..] ;
    Ast *last_arg = 0;
    for (;;) {
        Ast *arg = tlang_parse_expr(p);
        if (!arg) break;
        LIST_APPEND(label->child, last_arg, arg);
    }

    Ast *end = tlang_parse_op(p, ";");
    if (!end) return 0;

    return label;
}

static Ast *tlang_parse(Parse *p) {
    Ast *block_start = 0;
    Ast *block_end = 0;
    for (;;) {
        Ast *stm = tlang_parse_statement(p);
        if (!stm) break;
        LIST_APPEND(block_start, block_end, stm);
    }
    check(p->token == 0);
    return block_start;
}

static void tlang_fmt(Fmt *fmt, Ast *ast, u32 indent) {
    if (!ast) return;
    if (ast->child) fmt_s(fmt, "(");
    fmt_buf(fmt, ast->text);
    if (ast->next || ast->child) fmt_s(fmt, " ");
    tlang_fmt(fmt, ast->child, indent + 1);
    if (ast->child) fmt_s(fmt, ")");
    if (indent == 0) fmt_s(fmt, "\n");
    tlang_fmt(fmt, ast->next, indent);
}

static u32 u32_from_buffer(Buffer in) {
    u32 value = 0;
    for (u32 i = 0; i < in.size; ++i) {
        u8 chr = in.data[i];
        check(chr >= '0' && chr <= '9');
        if (error) return 0;
        value *= 10;
        value += chr - '0';
    }
    return value;
}

typedef struct Stack Stack;

struct Stack {
    Buffer name;
    u32 value;
    Stack *next;
};

static u32 tlang_eval_expr(Ast *ast, Stack *stack) {
    if (ast->type == Ast_Type_Number) {
        return u32_from_buffer(ast->text);
    }

    if (ast->type == Ast_Type_Operator) {
        if (buf_eq(ast->text, str_buf("*"))) {
            u32 value = 1;
            Ast *child = ast->child;
            while (child) {
                value *= tlang_eval_expr(child, stack);
                child = child->next;
            }
            return value;
        }
        if (buf_eq(ast->text, str_buf("+"))) {
            u32 value = 0;
            Ast *child = ast->child;
            while (child) {
                value += tlang_eval_expr(child, stack);
                child = child->next;
            }
            return value;
        }
    }

    if (ast->type == Ast_Type_Label) {
        for (Stack *s = stack; s; s = s->next) {
            if (buf_eq(s->name, ast->text)) {
                return s->value;
            }
        }
        check(!"Word not found on stack");
        return 0;
    }

    check(!"Invalid AST node type");
    return 0;
}

static Stack *tlang_eval_block(Memory *mem, Ast *ast) {
    Stack *stack = 0;
    for (; ast; ast = ast->next) {
        Stack *item = mem_struct(mem, Stack);
        item->name = ast->child->text;
        item->value = tlang_eval_expr(ast->child->next, stack);
        item->next = stack;
        stack = item;
    }
    return stack;
}

static void test_tlang(void) {
    Memory *mem = mem_new();
    Ast *ast = tlang_lex(mem, str_buf("x = 1*2*3;\ny=4*5*6;z=x+y;w=x*y;"));
    // for (Ast *tok = ast; tok; tok = tok->next_token) {
    //     print("Tok: ", " type ", tok->type, " text ", tok->text);
    // }

    Parse p = {.token = ast};
    ast = tlang_parse(&p);
    tlang_fmt(fout, ast, 0);
    Stack *env = tlang_eval_block(mem, ast);
    // for (Stack *s = env; s; s = s->next) {
    //     print(s->name, " = ", s->value);
    // }
    mem_free(mem);
}

// static void parse_whitespace(Stream *stream) {
//     while (!stream_eof(stream)) {
//         size_t cursor = stream_cursor(stream);
//         u8 c = stream_read_u8(stream);
//         if (chr_is_space(c)) continue;
//         stream_seek(stream, cursor);
//         break;
//     }
// }

// static u8 parse_alphanum(Stream *stream) {
//     u8 c = stream_read_u8(stream);
//     check(chr_is_alpha(c) || chr_is_digit(c));
//     return c;
// }

// static u8 parse_digit(Stream *stream) {
//     u8 c = stream_read_u8(stream);
//     check(chr_is_digit(c));
//     return c;
// }

// static u8 parse_alpha(Stream *stream) {
//     u8 c = stream_read_u8(stream);
//     check(chr_is_digit(c));
//     return c;
// }

// #define parse_many() if(!error) while(!error_pop())

// static Buffer parse_word(Stream *str) {
//     size_t start = stream_cursor(str);
//     size_t end = start;
//     parse_alpha(str);
//     parse_many() {
//         end = stream_cursor(str);
//         parse_alphanum(str);
//     }
//     stream_seek(str, end);
//     return stream_slice(str, start, end);
// }

// static Buffer parse_number(Stream *str) {
//     size_t start = stream_cursor(str);
//     size_t end = start;
//     parse_digit(str);
//     parse_many() {
//         end = stream_cursor(str);
//         parse_alphanum(str);
//     }
//     stream_seek(str, end);
//     return stream_slice(str, start, end);
// }

// static void parse_literal(Stream *str, char *lit) {
//     parse_many() {
//         if(!*lit) return;
//         check_or(stream_peek_u8(str) == *lit++) return;
//         stream_read_u8(str);
//     }
// }

// static void parse_mul(Stream *str) {
//     size_t start = stream_cursor(str);
//     parse_number(str);
//     parse_literal(str, "*");
//     parse_mul(str);
// }

// static void parse_plus(Stream *str) {
//     size_t start = stream_cursor(str);
//     parse_mul(str);
//     parse_literal(str, "+");
//     parse_plus(str);
// }
