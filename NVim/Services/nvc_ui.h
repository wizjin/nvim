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
#include <CoreGraphics/CoreGraphics.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t    nvc_ui_color_t;

typedef struct nvc_ui_callback {
    void (*flush)(void *userdata);
    void (*update_title)(void *userdata, const char *str, uint32_t len);
    void (*update_background)(void *userdata, nvc_ui_color_t rgb);
    void (*mouse_on)(void *userdata);
    void (*mouse_off)(void *userdata);
} nvc_ui_callback_t;

typedef struct nvc_ui_config {
    const char  *family_name;
    CGFloat     font_size;
} nvc_ui_config_t;

typedef struct nvc_ui_context nvc_ui_context_t;

NVC_API nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_config_t *config, const nvc_ui_callback_t *callback, void *userdata);
NVC_API void nvc_ui_destroy(nvc_ui_context_t *ctx);
NVC_API bool nvc_ui_is_attached(nvc_ui_context_t *ctx);
NVC_API CGSize nvc_ui_attach(nvc_ui_context_t *ctx, CGSize size);
NVC_API void nvc_ui_detach(nvc_ui_context_t *ctx);
NVC_API void nvc_ui_redraw(nvc_ui_context_t *ctx, CGContextRef context);
NVC_API CGSize nvc_ui_resize(nvc_ui_context_t *ctx, CGSize size);

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_H__ */
