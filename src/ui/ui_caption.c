/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#pragma push_macro("ui_caption_glyph_rest")
#pragma push_macro("ui_caption_glyph_menu")
#pragma push_macro("ui_caption_glyph_mini")
#pragma push_macro("ui_caption_glyph_maxi")
#pragma push_macro("ui_caption_glyph_full")
#pragma push_macro("ui_caption_glyph_quit")

#define ui_caption_glyph_rest ui_glyph_two_joined_squares
#define ui_caption_glyph_menu ui_glyph_trigram_for_heaven
#define ui_caption_glyph_mini ui_glyph_heavy_minus_sign
#define ui_caption_glyph_maxi ui_glyph_white_large_square
#define ui_caption_glyph_full ui_glyph_square_four_corners
#define ui_caption_glyph_quit ui_glyph_n_ary_times_operator

static void ui_caption_toggle_full(void) {
    ui_app.full_screen(!ui_app.is_full_screen);
    ui_caption.view.hidden = ui_app.is_full_screen;
    ui_app.request_layout();
}

static void ui_caption_esc_full_screen(ui_view_t* v, const char utf8[]) {
    swear(v == ui_caption.view.parent);
    // TODO: inside ui_app.c instead of here?
    if (utf8[0] == 033 && ui_app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* unused(b)) {
    ui_app.close();
}

static void ui_caption_mini(ui_button_t* unused(b)) {
    ui_app.show_window(ui.visibility.minimize);
}

static void ui_caption_maximize_or_restore(void) {
    strprintf(ui_caption.maxi.text, "%s",
        ui_app.is_maximized() ?
        ui_caption_glyph_rest : ui_caption_glyph_maxi);
}

static void ui_caption_maxi(ui_button_t* unused(b)) {
    if (!ui_app.is_maximized()) {
        ui_app.show_window(ui.visibility.maximize);
    } else if (ui_app.is_maximized() || ui_app.is_minimized()) {
        ui_app.show_window(ui.visibility.restore);
    }
    ui_caption_maximize_or_restore();
}

static void ui_caption_full(ui_button_t* unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(ui_view_t* v, int32_t x, int32_t y) {
    swear(v == &ui_caption.view);
    ui_point_t pt = { x, y };
    if (ui_app.is_full_screen) {
        return ui.hit_test.client;
    } else if (ui_view.inside(&ui_caption.icon, &pt)) {
        return ui.hit_test.system_menu;
    } else {
        ui_view_for_each(&ui_caption.view, c, {
            bool ignore = c->type == ui_view_container ||
                          c->type == ui_view_spacer ||
                          c->type == ui_view_label;
            if (!ignore && ui_view.inside(c, &pt)) {
                return ui.hit_test.client;
            }
        });
        return ui.hit_test.caption;
    }
}

static ui_color_t ui_caption_color(void) {
    ui_color_t c = ui_app.is_active() ?
        ui_theme.get_color(ui_color_id_active_title) :
        ui_theme.get_color(ui_color_id_inactive_title);
    return c;
}

static void ui_caption_before_measure(ui_view_t* unused(v)) {
    ui_caption.title.hidden = false;
}

static void ui_caption_measured(ui_view_t* v) {
    ui_caption.title.hidden = v->w > ui_app.crc.w;
    v->w = ui_app.crc.w;
}

static void ui_caption_layouted(ui_view_t* v) {
    v->x = 0;
}

static void ui_caption_paint(ui_view_t* v) {
    ui_color_t background = ui_caption_color();
    ui_gdi.fill_with(v->x, v->y, v->w, v->h, background);
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_gaps_t){ 0, 0, 0, 0 };
    ui_caption.view.hidden = false;
    v->parent->character = ui_caption_esc_full_screen; // ESC for full screen
    ui_view.add(&ui_caption.view,
        &ui_caption.icon,
        &ui_caption.menu,
        &ui_caption.title,
        &ui_caption.spacer,
        &ui_caption.mini,
        &ui_caption.maxi,
        &ui_caption.full,
        &ui_caption.quit,
        null);
    ui_caption.view.color_id = ui_color_id_window_text;
    static const ui_gaps_t p = { .left  = 0.25, .top    = 0.25,
                                 .right = 0.25, .bottom = 0.25};
    ui_view_for_each(&ui_caption.view, c, {
        c->fm = &ui_app.fonts.H3;
        c->color_id = ui_caption.view.color_id;
        c->flat = true;
        c->padding = p;
    });
    ui_caption.view.insets = (ui_gaps_t) {
        .left  = 0.75,  .top    = 0.125,
        .right = 0.75,  .bottom = 0.125
    };
    ui_caption.icon.icon  = ui_app.icon;
    ui_caption.view.align = ui.align.left;
    // TODO: this does not help because parent layout will set x and w again
    ui_caption.view.before_measure = ui_caption_before_measure;
    ui_caption.view.measured  = ui_caption_measured;
    ui_caption.view.layouted   = ui_caption_layouted;
    strprintf(ui_caption.view.text, "ui_caption");
    ui_caption_maximize_or_restore();
    ui_caption.view.paint = ui_caption_paint;
}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .fm       = &ui_app.fonts.regular,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .hidden = true
    },
    .icon   = ui_button(ui_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};

#pragma pop_macro("ui_caption_glyph_rest")
#pragma pop_macro("ui_caption_glyph_menu")
#pragma pop_macro("ui_caption_glyph_mini")
#pragma pop_macro("ui_caption_glyph_maxi")
#pragma pop_macro("ui_caption_glyph_full")
#pragma pop_macro("ui_caption_glyph_quit")
