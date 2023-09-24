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

typedef struct nvc_ui_callback {
    void (*flush)(void *userdata);
    void (*update_title)(void *userdata, const char *str, uint32_t len);
    void (*update_background)(void *userdata, uint32_t rgb);
} nvc_ui_callback_t;

typedef struct nvc_ui_config {
    const char  *familyName;
    CGFloat     fontSize;
} nvc_ui_config_t;

typedef struct nvc_ui_context nvc_ui_context_t;

NVC_API nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_config_t *config, const nvc_ui_callback_t *callback, void *userdata);
NVC_API void nvc_ui_destory(nvc_ui_context_t *ctx);
NVC_API void nvc_ui_attach(nvc_ui_context_t *ctx, CGContextRef context, CGSize size);
NVC_API void nvc_ui_detach(nvc_ui_context_t *ctx);
NVC_API void nvc_ui_resize(nvc_ui_context_t *ctx, CGSize size);


#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_H__ */
