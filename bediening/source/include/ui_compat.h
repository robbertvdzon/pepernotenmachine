#pragma once

// LVGL 9.1 .. 9.6 compatibility for object flags. LVGL 9.6 deprecates
// lv_obj_add_flag()/lv_obj_remove_flag() in favour of per-flag setters, which
// do not exist in 9.1. These wrappers use whichever the installed version has.

#include <lvgl.h>

#if (LVGL_VERSION_MAJOR > 9) || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR >= 6)
#define UI_LVGL_PER_FLAG_SETTERS 1
#else
#define UI_LVGL_PER_FLAG_SETTERS 0
#endif

static inline void ui_set_scrollable(lv_obj_t* o, bool en) {
#if UI_LVGL_PER_FLAG_SETTERS
    lv_obj_set_scrollable(o, en);
#else
    if (en) lv_obj_add_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
#endif
}

static inline void ui_set_clickable(lv_obj_t* o, bool en) {
#if UI_LVGL_PER_FLAG_SETTERS
    lv_obj_set_clickable(o, en);
#else
    if (en) lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
#endif
}

static inline void ui_set_hidden(lv_obj_t* o, bool hidden) {
#if UI_LVGL_PER_FLAG_SETTERS
    lv_obj_set_hidden(o, hidden);
#else
    if (hidden) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
#endif
}

static inline void ui_set_scroll_on_focus(lv_obj_t* o, bool en) {
#if UI_LVGL_PER_FLAG_SETTERS
    lv_obj_set_scroll_on_focus(o, en);
#else
    if (en) lv_obj_add_flag(o, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
#endif
}

static inline bool ui_is_hidden(const lv_obj_t* o) {
#if UI_LVGL_PER_FLAG_SETTERS
    return lv_obj_is_hidden(o);
#else
    return lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN);
#endif
}
