#pragma once
#include "os_headers.h"

static void js_append_style(char *css) {
    js_vp(
        "(css) => {"
        "  let el = document.createElement('style');"
        "  el.innerHTML = str_c(css);"
        "  document.head.appendChild(el);"
        "}",
        css
    );
}

static void js_set_html(char *html) {
    js_vp("(html) => document.body.innerHTML = str_c(html)", html);
}
