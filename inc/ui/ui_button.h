#pragma once
#include "ut/ut_std.h"
#include "ui/ui_view.h"

begin_c

typedef struct ui_button_s ui_button_t;

typedef struct ui_button_s {
    ui_view_t view;
    void (*cb)(ui_button_t* b); // callback
    fp64_t armed_until;   // seconds - when to release
} ui_button_t;

void ui_button_init(ui_button_t* b, const char* label, fp64_t ems,
    void (*cb)(ui_button_t* b));

void ui_button_init_(ui_view_t* view); // do not call use ui_button() macro

#define ui_button(name, s, w, code)                                   \
    static void name ## _callback(ui_button_t* name) {                \
        (void)name; /* no warning if unused */                        \
        code                                                          \
    }                                                                 \
    static                                                            \
    ui_button_t name = {                                              \
    .view = { .type = ui_view_button, .init = ui_button_init_,        \
    .child = null, .width = w, .text = s}, .cb = name ## _callback }

// usage:
// ui_button(button, 7.0, "&Button", { b->view.pressed = !b->view.pressed; })

end_c
