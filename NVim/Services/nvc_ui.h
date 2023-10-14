//
//  nvc_ui.h
//  NVim
//
//  Created by wizjin on 2023/9/20.
//

#ifndef __NVC_UI_H__
#define __NVC_UI_H__

#include <stdint.h>
#include <stdbool.h>
#include <CoreText/CoreText.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t    nvc_ui_color_t;

typedef struct nvc_ui_callback {
    void (*flush)(void *userdata, CGRect dirty);
    void (*update_title)(void *userdata, const char *str, uint32_t len);
    void (*update_background)(void *userdata, nvc_ui_color_t rgb);
    void (*update_tab_background)(void *userdata, nvc_ui_color_t rgb);
    void (*update_tab_list)(void *userdata, bool list_updated);
    void (*mouse_on)(void *userdata);
    void (*mouse_off)(void *userdata);
    void (*font_updated)(void *userdata);
    void (*enable_mouse_autohide)(void *userdata, bool enabled);
    void (*enable_mouse_move)(void *userdata, bool enabled);
    void (*enable_ext_tabline)(void *userdata, bool enabled);
    void (*close)(void *userdata);
} nvc_ui_callback_t;

typedef struct nvc_ui_config {
    CTFontRef   font;
    CGFloat     font_size;
} nvc_ui_config_t;

typedef struct nvc_ui_key_flags {
    bool        shift:      1;
    bool        control:    1;
    bool        option:     1;
    bool        command:    1;
} nvc_ui_key_flags_t;

#define NVC_UI_MOUSE_KEY_LIST       \
    NVC_UI_MOUSE_KEY(wheel)         \
    NVC_UI_MOUSE_KEY(left)          \
    NVC_UI_MOUSE_KEY(right)         \
    NVC_UI_MOUSE_KEY(middle)        \
    NVC_UI_MOUSE_KEY(move)
#define NVC_UI_MOUSE_KEY(_key)      nvc_ui_mouse_key_##_key,
typedef enum nvc_ui_mouse_key {
    NVC_UI_MOUSE_KEY_LIST
} nvc_ui_mouse_key_t;

#define NVC_UI_MOUSE_ACTION_LIST    \
    NVC_UI_MOUSE_ACTION(none)       \
    NVC_UI_MOUSE_ACTION(press)      \
    NVC_UI_MOUSE_ACTION(drag)       \
    NVC_UI_MOUSE_ACTION(release)    \
    NVC_UI_MOUSE_ACTION(up)         \
    NVC_UI_MOUSE_ACTION(down)
#define NVC_UI_MOUSE_ACTION(_key)   nvc_ui_mouse_action_##_key,
typedef enum nvc_ui_mouse_action {
    NVC_UI_MOUSE_ACTION_LIST
} nvc_ui_mouse_action_t;

typedef struct nvc_ui_mouse_info {
    nvc_ui_mouse_key_t      key;
    nvc_ui_mouse_action_t   action;
    CGPoint                 point;
    nvc_ui_key_flags_t      flags;
} nvc_ui_mouse_info_t;

typedef struct nvc_ui_context nvc_ui_context_t;

NVC_API nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_config_t *config, const nvc_ui_callback_t *callback, void *userdata);
NVC_API void nvc_ui_destroy(nvc_ui_context_t *ctx);
NVC_API bool nvc_ui_is_attached(nvc_ui_context_t *ctx);
NVC_API CGSize nvc_ui_attach(nvc_ui_context_t *ctx, CGSize size);
NVC_API void nvc_ui_detach(nvc_ui_context_t *ctx);
NVC_API void nvc_ui_redraw(nvc_ui_context_t *ctx, CGContextRef context);
NVC_API CGSize nvc_ui_resize(nvc_ui_context_t *ctx, CGSize size);
NVC_API CGFloat nvc_ui_get_line_height(nvc_ui_context_t *ctx);
NVC_API CGPoint nvc_ui_get_cursor_position(nvc_ui_context_t *ctx);
NVC_API void nvc_ui_open_file(nvc_ui_context_t *ctx, const char *file, uint32_t len, bool new_tab);
NVC_API void nvc_ui_tab_next(nvc_ui_context_t *ctx, int count);
NVC_API bool nvc_ui_input_key(nvc_ui_context_t *ctx, uint16_t key, nvc_ui_key_flags_t flags);
NVC_API bool nvc_ui_input_function_key(nvc_ui_context_t *ctx, UniChar ch, nvc_ui_key_flags_t flags);
NVC_API void nvc_ui_input_keystr(nvc_ui_context_t *ctx, nvc_ui_key_flags_t flags, const char* keys, uint32_t len);
NVC_API void nvc_ui_input_rawkey(nvc_ui_context_t *ctx, const char* keys, uint32_t len);
NVC_API void nvc_ui_input_mouse(nvc_ui_context_t *ctx, nvc_ui_mouse_info_t mouse);

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_H__ */
