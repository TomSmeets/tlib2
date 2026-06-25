#include "fmt.h"
#include "os_main.h"
#include "time.h"
#include "os_headers.h"

static bool update = 1;
static u64 value = 0;
static bool init = 0;

#if OS_WASM
void js_inc(void) {
    value++;
    update = 1;
}

void js_dec(void) {
    value--;
    update = 1;
}
#endif

typedef struct Todo Todo;
struct Todo {
    char *text;
    bool done;
    Todo *next;
    Todo *child;
};

static Todo *list = 0;

static void list_add(char *task) {
    Todo *t = mem_struct(mem_perm(), Todo);
    t->text = task;
    t->done = false;
    t->next = list;
    list = t;
}

static Todo *todo_new(char *text) {
    Todo *t = mem_struct(mem_tmp(), Todo);
    t->text = text;
    t->done = false;
    return t;
}

static char *str_copy(char *s) {
    return mem_clone(mem_tmp(), s, str_len(s) + 1);
}

static Todo *todo_copy(Todo *todo) {
    if (!todo) return 0;
    Todo *new = mem_struct(mem_tmp(), Todo);
    new->text = str_copy(todo->text);
    new->done = todo->done;
    new->next = todo_copy(todo->next);
    new->child = todo_copy(todo->child);
    return new;
}

static char *todo_fmt(Todo *todo) {
    if(!todo) return "";
    char *line  = fstr(mem_tmp(), "Todo: ", todo->text, "");
    char *child = todo_fmt(todo->child);
    char *next  = todo_fmt(todo->next);
    return fstr(mem_tmp(), line, child, next);
}

typedef struct {
    Write content;

    u32 callback_count;
    u32 callback_cap;
    bool callback;
} Html;

typedef struct {
    u32 index;
    bool value;
} HtmlCallback;

static HtmlCallback html_callback(Html *html) {
    int i = html->callback_count++;
    return (HtmlCallback) { i, false };
}

static void html_emit(Html *html, char *text) {
    html->content = fmt(html->content, text);
}

#define fmt(...) ""

static bool html_button(Html *html, char *text) {
    HtmlCallback cb = html_callback(html);
    html_emit(html, fmt("<button onclick=toggle(", cb.index, ")>"));
    html_emit(html, "</button>");
}

static void os_main(void) {
    print("Hello World: ", value);
    // value++;

    if (!init) {
        init = 1;
        js_append_style(R"(
            button {
                width:  100px;
                height: 40px;
            }
        )");
    }

    if (update) {
        if(value > 1000) value = 1000;
        
        Fmt *f = fmt_new(mem_tmp());
        fmt_g(f, "<h1>Hello World!</h1>\n");
        fmt_g(f, "<p>Value: ", value, "</p>\n");
        fmt_g(f, "<button onclick='tlib.import.js_dec()'>Dec</button>\n");
        fmt_g(f, "<button onclick='tlib.import.js_inc()'>Inc</button>\n");

        fmt_g(f, "<ul>\n");
        for (u32 i = 0; i < value; ++i) {
            fmt_g(f, "<li>", "i = ", i * i, "</li>\n");
        }
        fmt_g(f, "</ul>\n");

        js_set_html((char *)fmt_end(f).data);
        update = 0;
    }

    os_sleep(10 * TIME_MS);
}
