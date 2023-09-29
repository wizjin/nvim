//
//  nvc_ui.cpp
//  NVim
//
//  Created by wizjin on 2023/9/20.
//

#include "nvc_ui.h"
#include <CoreText/CoreText.h>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include "nvc_rpc.h"

#define nvc_ui_to_context(_ptr, _member)    nv_member_to_struct(nvc_ui_context_t, _ptr, _member)
#define nvc_ui_get_userdata(_ctx)           nvc_rpc_get_userdata(&(_ctx)->rpc)
#define nvc_lock_guard_t                    std::lock_guard<std::mutex>

#pragma mark - NVC RPC Helper
static const std::string nvc_rpc_empty_str = "";
static inline const std::string nvc_rpc_read_str(nvc_rpc_context_t *ctx) {
    uint32_t len = 0;
    auto str = nvc_rpc_read_str(ctx, &len);
    return likely(str != nullptr) ? std::string(str, len) : nvc_rpc_empty_str;
}

#pragma mark - NVC UI Struct
typedef int (*nvc_ui_action_t)(nvc_ui_context_t *ctx, int narg);

#define NVC_UI_COLOR_CODE_LIST              \
    NVC_UI_COLOR_CODE(foreground),          \
    NVC_UI_COLOR_CODE(background),          \
    NVC_UI_COLOR_CODE(special),             \
    NVC_UI_COLOR_CODE(reverse),             \
    NVC_UI_COLOR_CODE(italic),              \
    NVC_UI_COLOR_CODE(bold),                \
    NVC_UI_COLOR_CODE(nocombine),           \
    NVC_UI_COLOR_CODE(strikethrough),       \
    NVC_UI_COLOR_CODE(underline),           \
    NVC_UI_COLOR_CODE(undercurl),           \
    NVC_UI_COLOR_CODE(underdouble),         \
    NVC_UI_COLOR_CODE(underdotted),         \
    NVC_UI_COLOR_CODE(underdashed),         \
    NVC_UI_COLOR_CODE(altfont),             \
    NVC_UI_COLOR_CODE(blend)

#define NVC_UI_COLOR_CODE(_code)            nvc_ui_color_code_##_code
typedef enum nvc_ui_color_code { NVC_UI_COLOR_CODE_LIST } nvc_ui_color_code_t;

#undef NVC_UI_COLOR_CODE
#define NVC_UI_COLOR_CODE(_code)            { #_code, nvc_ui_color_code_##_code }
static const std::map<const std::string, nvc_ui_color_code_t> nvc_ui_color_name_table = { NVC_UI_COLOR_CODE_LIST };

typedef struct nvc_ui_cell_size {
    uint32_t width;
    uint32_t height;
} nvc_ui_cell_size_t;

typedef struct mode_info {
    std::string name;
    std::string short_name;
    std::string cursor_shape;
    std::string mouse_shape;
    int         cell_percentage;
    int         blinkwait;
    int         blinkon;
    int         blinkoff;
    int         attr_id;
    int         attr_id_lm;
} mode_info_t;

typedef void (*nvc_ui_set_mode_info)(mode_info_t &info, nvc_rpc_context_t *ctx);

#define NVC_UI_SET_MODE_INFO_STR(_action)   { #_action, [](mode_info_t &info, nvc_rpc_context_t *ctx) { info._action = nvc_rpc_read_str(ctx); } }
#define NVC_UI_SET_MODE_INFO_INT(_action)   { #_action, [](mode_info_t &info, nvc_rpc_context_t *ctx) { info._action = nvc_rpc_read_int(ctx); } }
#define NVC_UI_SET_MODE_INFO_NULL(_action)  { #_action, nullptr }
static const std::map<const std::string, nvc_ui_set_mode_info> nvc_ui_set_mode_info_actions = {
    NVC_UI_SET_MODE_INFO_STR(name),
    NVC_UI_SET_MODE_INFO_STR(short_name),
    NVC_UI_SET_MODE_INFO_STR(cursor_shape),
    NVC_UI_SET_MODE_INFO_STR(mouse_shape),
    NVC_UI_SET_MODE_INFO_INT(cell_percentage),
    NVC_UI_SET_MODE_INFO_INT(blinkwait),
    NVC_UI_SET_MODE_INFO_INT(blinkon),
    NVC_UI_SET_MODE_INFO_INT(blinkoff),
    NVC_UI_SET_MODE_INFO_NULL(hl_id),
    NVC_UI_SET_MODE_INFO_NULL(id_lm),
    NVC_UI_SET_MODE_INFO_INT(attr_id),
    NVC_UI_SET_MODE_INFO_INT(attr_id_lm),
};

typedef std::map<nvc_ui_color_code_t, nvc_ui_color_t>   nvc_ui_colors_set_t;
typedef std::map<int, nvc_ui_colors_set_t>              nvc_ui_colors_set_map_t;
typedef std::map<std::string, int>                      nvc_ui_group_map_t;
typedef std::map<int, CGLayerRef, std::less<int>>       nvc_ui_layer_map_t;
typedef std::vector<mode_info_t>                        mode_info_list_t;

struct nvc_ui_context {
    nvc_rpc_context_t       rpc;
    nvc_ui_callback_t       cb;
    bool                    attached;
    CTFontRef               font;
    CGFloat                 font_size;
    std::string             mode;
    int                     mode_idx;
    CGSize                  cell_size;
    CGFloat                 font_offset;
    nvc_ui_cell_size_t      window_size;
    nvc_ui_colors_set_t     default_colors;
    nvc_ui_colors_set_map_t hl_attrs;
    nvc_ui_group_map_t      hl_groups;
    mode_info_list_t        mode_infos;
    nvc_ui_layer_map_t      layers;
    std::mutex              locker;
};

static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items);

static const nvc_ui_cell_size_t nvc_ui_cell_size_zero = { 0, 0 };

#pragma mark - NVC UI Helper
static inline int nvc_ui_init_font(nvc_ui_context_t *ctx, const nvc_ui_config_t *config) {
    int res = NVC_RC_ILLEGAL_CALL;
    CFStringRef name = CFStringCreateWithCString(nullptr, config->family_name, CFStringGetSystemEncoding());
    if (name == nullptr) {
        res = NVC_RC_MALLOC_ERROR;
    } else {
        ctx->font_size = config->font_size;
        ctx->font = CTFontCreateWithName(name, config->font_size, nullptr);
        if (unlikely(ctx->font == nullptr)) {
            NVLogW("nvc ui create font failed: %s (%lf)", config->family_name, config->font_size);
        } else {
            CGFloat ascent = CTFontGetAscent(ctx->font);
            CGFloat descent = CTFontGetDescent(ctx->font);
            CGFloat leading = CTFontGetLeading(ctx->font);
            CGFloat height = ceil(ascent + descent + leading);
            CGGlyph glyph = (CGGlyph) 0;
            UniChar capitalM = (UniChar) 0x004D;
            CGSize advancement = CGSizeZero;
            CTFontGetGlyphsForCharacters(ctx->font, &capitalM, &glyph, 1);
            CTFontGetAdvancesForGlyphs(ctx->font, kCTFontOrientationHorizontal, &glyph, &advancement, 1);
            CGFloat width = ceil(advancement.width);
            ctx->cell_size = CGSizeMake(width, height);
            ctx->font_offset = descent;
            res = NVC_RC_OK;
        }
        CFRelease(name);
    }
    return res;
}

static inline nvc_ui_cell_size_t nvc_ui_size2cell(nvc_ui_context_t *ctx, CGSize size) {
    nvc_ui_cell_size_t cell;
    cell.width = floor(size.width/ctx->cell_size.width);
    cell.height = floor(size.height/ctx->cell_size.height);
    return cell;
}

static inline CGSize nvc_ui_cell2size(nvc_ui_context_t *ctx, int width, int height) {
    return CGSizeMake(ctx->cell_size.width * width, ctx->cell_size.height * height);
}

static inline bool nvc_ui_size_equals(const nvc_ui_cell_size_t& s1, const nvc_ui_cell_size_t& s2) {
    return s1.width == s2.width && s1.height == s2.height;
}

static inline bool nvc_ui_update_size(nvc_ui_context_t *ctx, CGSize size) {
    bool res = false;
    nvc_ui_cell_size_t wnd_size = nvc_ui_size2cell(ctx, size);
    if (!nvc_ui_size_equals(ctx->window_size, wnd_size)) {
        ctx->window_size = wnd_size;
        res = true;
    }
    return res;
}

#pragma mark - NVC UI API
nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_config_t *config, const nvc_ui_callback_t *callback, void *userdata) {
    nvc_ui_context_t *ctx = nullptr;
    if (inskt != INVALID_SOCKET && outskt != INVALID_SOCKET && config != NULL && callback != NULL) {
        ctx = new nvc_ui_context_t();
        if (unlikely(ctx == nullptr)) {
            NVLogE("nvc ui create context failed");
        } else {
            ctx->attached = false;
            ctx->cb = *callback;
            ctx->font = nullptr;
            ctx->mode_idx = 0;
            ctx->cell_size = CGSizeZero;
            ctx->window_size = nvc_ui_cell_size_zero;
            int res = nvc_rpc_init(&ctx->rpc, inskt, outskt, userdata, nvc_ui_response_handler, nvc_ui_notification_handler);
            if (res == NVC_RC_OK) {
                res = nvc_ui_init_font(ctx, config);
            }
            if (res != NVC_RC_OK) {
                nvc_ui_destroy(ctx);
                NVLogE("nvc ui init context failed");
                free(ctx);
                ctx = nullptr;
            }
        }
    }
    return ctx;
}

void nvc_ui_destroy(nvc_ui_context_t *ctx) {
    if (ctx != nullptr) {
        nvc_rpc_final(&ctx->rpc);
        for (const auto& p : ctx->layers) {
            CGLayerRelease(p.second);
        }
        ctx->layers.clear();
        if (ctx->font != nullptr) {
            CFRelease(ctx->font);
            ctx->font = nullptr;
        }
        delete ctx;
    }
}

bool nvc_ui_is_attached(nvc_ui_context_t *ctx) {
    bool res = false;
    if (likely(ctx != nullptr)) {
        res = ctx->attached;
    }
    return res;
}

CGSize nvc_ui_attach(nvc_ui_context_t *ctx, CGSize size) {
    if (likely(ctx != nullptr && !ctx->attached)) {
        ctx->attached = true;
        nvc_ui_update_size(ctx, size);
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_attach", 3);
        nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.width);
        nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.height);
        nvc_rpc_write_map_size(&ctx->rpc, 2);
        nvc_rpc_write_const_str(&ctx->rpc, "override");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_write_const_str(&ctx->rpc, "ext_linegrid");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_call_end(&ctx->rpc);
        size = nvc_ui_cell2size(ctx, ctx->window_size.width, ctx->window_size.height);
    }
    return size;
}

void nvc_ui_detach(nvc_ui_context_t *ctx) {
    if (likely(ctx != nullptr && ctx->attached)) {
        ctx->attached = false;
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_detach", 0);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

void nvc_ui_redraw(nvc_ui_context_t *ctx, CGContextRef context) {
    if (likely(ctx != nullptr && ctx->attached && context != nullptr)) {
        nvc_lock_guard_t guard(ctx->locker);
        for (const auto& [key, layer] : ctx->layers) {
            CGContextDrawLayerAtPoint(context, CGPointZero, layer);
        }
        CGContextFlush(context);
    }
}

CGSize nvc_ui_resize(nvc_ui_context_t *ctx, CGSize size) {
    if (likely(ctx != nullptr && ctx->attached)) {
        if (nvc_ui_update_size(ctx, size)) {
            nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_try_resize", 2);
            nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.width);
            nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.height);
            nvc_rpc_call_end(&ctx->rpc);
            size = nvc_ui_cell2size(ctx, ctx->window_size.width, ctx->window_size.height);
        }
    }
    return size;
}

#pragma mark - NVC UI Redraw Actions
static inline int nvc_ui_redraw_action_set_title(nvc_ui_context_t *ctx, int count) {
    if (likely(count-- > 0)) {
        int items = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(items-- > 0)) {
            uint32_t len = 0;
            ctx->cb.update_title(nvc_ui_get_userdata(ctx), nvc_rpc_read_str(&ctx->rpc, &len), len);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, items);
    }
    return count;
}

static inline int nvc_ui_redraw_action_mode_info_set(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            bool enabled = nvc_rpc_read_bool(&ctx->rpc);
            mode_info_list_t mode_infos;
            for (int n = nvc_rpc_read_array_size(&ctx->rpc); n > 0; n--) {
                mode_info_t info;
                for (int mn = nvc_rpc_read_map_size(&ctx->rpc); mn > 0; mn--) {
                    auto name = nvc_rpc_read_str(&ctx->rpc);
                    auto p = nvc_ui_set_mode_info_actions.find(name);
                    if (unlikely(p == nvc_ui_set_mode_info_actions.end())) {
                        nvc_rpc_read_skip_items(&ctx->rpc, 1);
                        NVLogW("nvc ui unknown mode info: %s", name.c_str());
                    } else {
                        if (p->second != nullptr) {
                            p->second(info, &ctx->rpc);
                        } else {
                            nvc_rpc_read_skip_items(&ctx->rpc, 1);
                        }
                    }
                }
                mode_infos.push_back(info);
            }
            ctx->mode_infos = mode_infos;
            NVLogD("nvc ui mode info set: enabled=%d", (int)enabled);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_option_set(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            std::string value;
            auto key = nvc_rpc_read_str(&ctx->rpc);
            if (likely(narg > 0)) {
                switch (nvc_rpc_read_ahead(&ctx->rpc)) {
                    case NVC_RPC_ITEM_STR:
                        value = nvc_rpc_read_str(&ctx->rpc);
                        narg--;
                        break;
                    case NVC_RPC_ITEM_BOOLEAN:
                        value = nvc_rpc_read_bool(&ctx->rpc) ? "true" : "false";
                        narg--;
                        break;
                    case NVC_RPC_ITEM_POSITIVE_INTEGER:
                    case NVC_RPC_ITEM_NEGATIVE_INTEGER:
                        value = std::to_string(nvc_rpc_read_int(&ctx->rpc));
                        narg--;
                        break;
                    default:
                        NVLogW("nvc ui find unknown option value type: %s", key.c_str());
                }
            }
            //NVLogD("Set option: %s = %s", key.c_str(), value.c_str());
            nvc_rpc_read_skip_items(&ctx->rpc, narg);
        }
    }
    return items;
}

static inline int nvc_ui_redraw_action_mode_change(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            ctx->mode = nvc_rpc_read_str(&ctx->rpc);
            ctx->mode_idx = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui change mode: %s %d", ctx->mode.c_str(), ctx->mode_idx);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}


static inline int nvc_ui_redraw_action_mouse_on(nvc_ui_context_t *ctx, int items) {
    if (likely(items > 0)) {
        ctx->cb.mouse_on(nvc_ui_get_userdata(ctx));
    }
    return items;
}

static inline int nvc_ui_redraw_action_mouse_off(nvc_ui_context_t *ctx, int items) {
    if (likely(items > 0)) {
        ctx->cb.mouse_off(nvc_ui_get_userdata(ctx));
    }
    return items;
}

static inline int nvc_ui_redraw_action_flush(nvc_ui_context_t *ctx, int items) {
    ctx->cb.flush(nvc_ui_get_userdata(ctx));
    return items;
}

static inline int nvc_ui_redraw_action_grid_resize(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int width = nvc_rpc_read_int(&ctx->rpc);
            int height = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid resize %d - %dx%d", grid_id, width, height);
            CGSize size = nvc_ui_cell2size(ctx, width, height);
            CGLayerRef layer = CGLayerCreateWithContext(nullptr, size, nullptr);
            if (layer != nullptr) {
                CGContextRef context = CGLayerGetContext(layer);
                CGContextSetShouldAntialias(context, true);
                CGContextSetShouldSmoothFonts(context, false);
                CGContextSetTextDrawingMode(context, kCGTextFill);
            }
            nvc_lock_guard_t guard(ctx->locker);
            auto p = ctx->layers.find(grid_id);
            if (p != ctx->layers.end()) {
                CGLayerRef layer = p->second;
                ctx->layers.erase(p);
                CGLayerRelease(layer);
            }
            if (layer != nullptr) {
                ctx->layers[grid_id] = layer;
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_default_colors_set(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 3)) {
            narg -= 3;
            ctx->default_colors[nvc_ui_color_code_foreground] = nvc_rpc_read_uint32(&ctx->rpc);
            ctx->default_colors[nvc_ui_color_code_background] = nvc_rpc_read_uint32(&ctx->rpc);
            ctx->default_colors[nvc_ui_color_code_special] = nvc_rpc_read_uint32(&ctx->rpc);
            ctx->cb.update_background(nvc_ui_get_userdata(ctx), ctx->default_colors[nvc_ui_color_code_background]);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_hl_attr_define(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            int hl_id = nvc_rpc_read_int(&ctx->rpc);
            nvc_ui_colors_set_t colors;
            for (int mn = nvc_rpc_read_map_size(&ctx->rpc); mn > 0; mn--) {
                auto key = nvc_rpc_read_str(&ctx->rpc);
                auto p = nvc_ui_color_name_table.find(key);
                if (unlikely(p != nvc_ui_color_name_table.end())) {
                    colors[p->second] = nvc_rpc_read_uint32(&ctx->rpc);
                } else {
                    NVLogW("nvc ui invalid hl attr %s", key.c_str());
                    nvc_rpc_read_skip_items(&ctx->rpc, 1);
                }
            }
            ctx->hl_attrs[hl_id] = colors;
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_hl_group_set(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            auto name = nvc_rpc_read_str(&ctx->rpc);
            ctx->hl_groups[name] = nvc_rpc_read_int(&ctx->rpc);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline UniChar nvc_ui_utf82unicode(const uint8_t *str, uint8_t len) {
    UniChar unicode = 0;
    switch (len) {
        case 1:
            unicode = str[0];
            break;
        case 2:
            if (str[1] == 0x80) {
                unicode = ((str[0] << 6) + (str[1]&0x3F))
                        | (((str[0] >> 2)&0x07) << 8);
            }
            break;
        case 3:
            if (((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80)) {
                unicode = ((str[1] << 6) + (str[2]&0x3F))
                        | (((str[0] << 4) + ((str[1] >> 2)&0x0F)) << 8);
            }
            break;
        case 4:
            if (((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80) && ((str[3]&0xC0) == 0x80)) {
                unicode = ((str[2] << 6) + (str[3]&0x3F))
                        | (((str[1] << 4) + ((str[2] >> 2)&0x0F)) << 8)
                        | ((((str[0] << 2)&0x1C) + ((str[1] >> 4)&0x03)) << 16);
            }
            break;
        default:
            NVLogW("nvc ui invalid utf8 %d", len);
            break;
    }
    return unicode;
}

static inline void nvc_ui_set_fill_color(CGContextRef context, uint32_t rgb) {
    const uint8_t *c = (const uint8_t *)&rgb;
    CGContextSetRGBFillColor(context, c[2]/255.0, c[1]/255.0, c[0]/255.0, 1.0);
}

static inline uint32_t nvc_ui_get_default_color(nvc_ui_context_t *ctx, nvc_ui_color_code_t code) {
    uint32_t c = 0;
    auto p = ctx->default_colors.find(code);
    if (p != ctx->default_colors.end()) {
        c = p->second;
    }
    return c;
}

static inline uint32_t nvc_ui_find_hl_color(nvc_ui_context_t *ctx, int hl, nvc_ui_color_code_t code) {
    auto p = ctx->hl_attrs.find(hl);
    if (p != ctx->hl_attrs.end()) {
        auto q = p->second.find(code);
        if (q != p->second.end()) {
            return q->second;
        }
    }
    return nvc_ui_get_default_color(ctx, code);
}

static inline int nvc_ui_redraw_action_grid_line(nvc_ui_context_t *ctx, int items) {
    CGContextRef context = nullptr;
    int last_grid_id = -1;
    nvc_lock_guard_t guard(ctx->locker);
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg > 4)) {
            narg -= 4;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int row = nvc_rpc_read_int(&ctx->rpc);
            int col_start = nvc_rpc_read_int(&ctx->rpc);
            int cells = nvc_rpc_read_array_size(&ctx->rpc);
            if (last_grid_id != grid_id || context == nullptr) {
                last_grid_id = grid_id;
                if (context != nullptr) {
                    CGContextFlush(context);
                    context = nullptr;
                }
                auto p = ctx->layers.find(grid_id);
                if (likely(p != ctx->layers.end())) {
                    CGLayerRef layer = p->second;
                    context = CGLayerGetContext(layer);
                }
            }
            if (likely(context != nullptr)) {
                CGPoint pt = CGPointMake(col_start * ctx->cell_size.width, (ctx->window_size.height - row) * ctx->cell_size.height);
                int last_hl_id = 0;
                uint32_t fg = nvc_ui_get_default_color(ctx, nvc_ui_color_code_foreground);
                uint32_t bg = nvc_ui_get_default_color(ctx, nvc_ui_color_code_background);
                while (cells-- > 0) {
                    int cnum = nvc_rpc_read_array_size(&ctx->rpc);
                    if (likely(cnum-- > 0)) {
                        uint32_t len = 0;
                        const uint8_t *str = (const uint8_t *)nvc_rpc_read_str(&ctx->rpc, &len);
                        UniChar ch = nvc_ui_utf82unicode(str, len);
                        int repeat = 1;
                        if (cnum-- > 0) {
                            int hl_id = nvc_rpc_read_int(&ctx->rpc);
                            if (hl_id != last_hl_id) {
                                last_hl_id = hl_id;
                                fg = nvc_ui_find_hl_color(ctx, hl_id, nvc_ui_color_code_foreground);
                                bg = nvc_ui_find_hl_color(ctx, hl_id, nvc_ui_color_code_background);
                            }
                        }
                        if (cnum-- > 0) {
                            repeat = nvc_rpc_read_int(&ctx->rpc);
                        }
                        nvc_ui_set_fill_color(context, bg);
                        CGContextFillRect(context, CGRectMake(pt.x, pt.y - ctx->font_offset, ceil(ctx->cell_size.width * repeat), ctx->cell_size.height));
                        if (likely(ch != 0) && ch != 0x0020) {
                            CGGlyph glyph = 0;
                            if (likely(CTFontGetGlyphsForCharacters(ctx->font, &ch, &glyph, 1))) {
                                nvc_ui_set_fill_color(context, fg);
                                while (repeat-- > 0) {
                                    CTFontDrawGlyphs(ctx->font, &glyph, &pt, 1, context);
                                    pt.x += ctx->cell_size.width;
                                }
                            }
                        }
                        if (repeat > 0) {
                            pt.x += ctx->cell_size.width * repeat;
                        }
                    }
                    nvc_rpc_read_skip_items(&ctx->rpc, cnum);
                }
            }
            nvc_rpc_read_skip_items(&ctx->rpc, cells);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    if (context != nullptr) {
        CGContextFlush(context);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_clear(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid %d clear", grid_id);
            nvc_lock_guard_t guard(ctx->locker);
            auto p = ctx->layers.find(grid_id);
            if (p != ctx->layers.end()) {
                CGRect bounds = CGRectZero;
                CGLayerRef layer = p->second;
                bounds.size = CGLayerGetSize(layer);
                CGContextRef context = CGLayerGetContext(layer);
                CGContextClearRect(context, bounds);
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_destroy(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid %d destroy", grid_id);
            nvc_lock_guard_t guard(ctx->locker);
            auto p = ctx->layers.find(grid_id);
            if (p != ctx->layers.end()) {
                CGLayerRef layer = p->second;
                ctx->layers.erase(p);
                CGLayerRelease(layer);
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_cursor_goto(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int width = nvc_rpc_read_int(&ctx->rpc);
            int height = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid cursor goto %d - %dx%d", grid_id, width, height);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_scroll(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid scroll %d", grid_id);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_tabline_update(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        NVLogD("nvc ui update tabline");
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

#define NVC_REDRAW_ACTION(action)           { #action, nvc_ui_redraw_action_##action }
#define NVC_REDRAW_ACTION_IGNORE(action)    { #action, nullptr }
// NOTE: https://neovim.io/doc/user/ui.html
static const std::map<const std::string, nvc_ui_action_t> nvc_ui_redraw_actions = {
    // Global Events
    NVC_REDRAW_ACTION_IGNORE(set_icon),
    NVC_REDRAW_ACTION(set_title),
    NVC_REDRAW_ACTION(mode_info_set),
    NVC_REDRAW_ACTION(option_set),
    NVC_REDRAW_ACTION(mode_change),
    NVC_REDRAW_ACTION(mouse_on),
    NVC_REDRAW_ACTION(mouse_off),
    NVC_REDRAW_ACTION_IGNORE(busy_start),
    NVC_REDRAW_ACTION_IGNORE(busy_stop),
    NVC_REDRAW_ACTION_IGNORE(suspend),
    NVC_REDRAW_ACTION_IGNORE(update_menu),
    NVC_REDRAW_ACTION_IGNORE(bell),
    NVC_REDRAW_ACTION_IGNORE(visual_bell),
    NVC_REDRAW_ACTION(flush),
    // Grid Events (line-based)
    NVC_REDRAW_ACTION(grid_resize),
    NVC_REDRAW_ACTION(default_colors_set),
    NVC_REDRAW_ACTION(hl_attr_define),
    NVC_REDRAW_ACTION(hl_group_set),
    NVC_REDRAW_ACTION(grid_line),
    NVC_REDRAW_ACTION(grid_clear),
    NVC_REDRAW_ACTION(grid_destroy),
    NVC_REDRAW_ACTION(grid_cursor_goto),
    NVC_REDRAW_ACTION(grid_scroll),
    // Multigrid Events
    NVC_REDRAW_ACTION_IGNORE(win_pos),
    NVC_REDRAW_ACTION_IGNORE(win_float_pos),
    NVC_REDRAW_ACTION_IGNORE(win_external_pos),
    NVC_REDRAW_ACTION_IGNORE(win_hide),
    NVC_REDRAW_ACTION_IGNORE(win_close),
    NVC_REDRAW_ACTION_IGNORE(msg_set_pos),
    NVC_REDRAW_ACTION_IGNORE(win_viewport),
    NVC_REDRAW_ACTION_IGNORE(win_extmark),
    // Popupmenu Events
    NVC_REDRAW_ACTION_IGNORE(popupmenu_show),
    NVC_REDRAW_ACTION_IGNORE(popupmenu_select),
    NVC_REDRAW_ACTION_IGNORE(popupmenu_hide),
    // Tabline Events
    NVC_REDRAW_ACTION(tabline_update),
    // Cmdline Events
    NVC_REDRAW_ACTION_IGNORE(cmdline_show),
    NVC_REDRAW_ACTION_IGNORE(cmdline_pos),
    NVC_REDRAW_ACTION_IGNORE(cmdline_special_char),
    NVC_REDRAW_ACTION_IGNORE(cmdline_hide),
    NVC_REDRAW_ACTION_IGNORE(cmdline_block_show),
    NVC_REDRAW_ACTION_IGNORE(cmdline_block_append),
    NVC_REDRAW_ACTION_IGNORE(cmdline_block_hide),
    // Message/Dialog Events
    NVC_REDRAW_ACTION_IGNORE(msg_show),
    NVC_REDRAW_ACTION_IGNORE(msg_clear),
    NVC_REDRAW_ACTION_IGNORE(msg_showmode),
    NVC_REDRAW_ACTION_IGNORE(msg_showcmd),
    NVC_REDRAW_ACTION_IGNORE(msg_ruler),
    NVC_REDRAW_ACTION_IGNORE(msg_history_show),
    NVC_REDRAW_ACTION_IGNORE(msg_history_clear),
};

#pragma mark - NVC UI Notification Actions
static inline int nvc_ui_notification_action_redraw(nvc_ui_context_t *ctx, int items) {
    NVLogD("nvc ui redraw: %d", items);
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            auto action = nvc_rpc_read_str(&ctx->rpc);
            auto p = nvc_ui_redraw_actions.find(action);
            if (unlikely(p == nvc_ui_redraw_actions.end())) {
                NVLogW("nvc ui unknown redraw action: %s", action.c_str());
            } else if (p->second != nullptr) {
                //NVLogW("nvc ui redraw action: %s", action.c_str());
                narg = p->second(ctx, narg);
            }
            nvc_rpc_read_skip_items(&ctx->rpc, narg);
        }
    }
    return items;
}

#define NVC_NOTIFICATION_ACTION(action) { #action, nvc_ui_notification_action_##action}
static const std::map<const std::string, nvc_ui_action_t> nvc_ui_notification_actions = {
    NVC_NOTIFICATION_ACTION(redraw),
};

#pragma mark - NVC UI Helper
static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        uint64_t msgid = nvc_rpc_read_uint64(ctx);
        NVLogD("nvc ui recive response msgid: %lu", msgid);
        if (likely(items-- > 0)) {
            int n = nvc_rpc_read_array_size(ctx);
            if (n >= 2) {
                n -= 2;
                int64_t code = nvc_rpc_read_int64(ctx);
                auto msg = nvc_rpc_read_str(ctx);
                NVLogE("nvc ui match request %d failed(%lu): %s", msgid, code, msg.c_str());
            }
            nvc_rpc_read_skip_items(ctx, n);
        }
        if (likely(items-- > 0)) {
            int n = nvc_rpc_read_map_size(ctx);
            if (n > 0) {
                nvc_rpc_read_skip_items(ctx, n * 2);
            }
        }
    }
    return items;
}

static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        auto action = nvc_rpc_read_str(ctx);
        auto p = nvc_ui_notification_actions.find(action);
        if (unlikely(p == nvc_ui_notification_actions.end())) {
            NVLogW("nvc ui unknown notification action: %s", action.c_str());
        } else if (likely(items-- > 0)) {
            nvc_rpc_read_skip_items(ctx, p->second(nvc_ui_to_context(ctx, rpc), nvc_rpc_read_array_size(ctx)));
        }
    }
    return items;
}
