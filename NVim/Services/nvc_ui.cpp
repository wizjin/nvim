//
//  nvc_ui.cpp
//  NVim
//
//  Created by wizjin on 2023/9/20.
//

#include "nvc_ui.h"
#include <CoreText/CoreText.h>
#include <string>
#include <map>
#include "nvc_rpc.h"

#define nvc_ui_to_context(_ptr, _member)    nv_member_to_struct(nvc_ui_context_t, _ptr, _member)
#define nvc_ui_get_userdata(_ctx)           nvc_rpc_get_userdata(&(_ctx)->rpc)

typedef int (*nvc_ui_action_t)(nvc_ui_context_t *ctx, int narg);

typedef enum nvc_ui_color_code {
    nvc_ui_color_code_foreground    = 1,
    nvc_ui_color_code_background    = 2,
    nvc_ui_color_code_special       = 3,
    nvc_ui_color_code_reverse       = 4,
    nvc_ui_color_code_italic        = 5,
    nvc_ui_color_code_bold          = 6,
    nvc_ui_color_code_strikethrough = 7,
    nvc_ui_color_code_underline     = 8,
    nvc_ui_color_code_undercurl     = 9,
    nvc_ui_color_code_underdouble   = 10,
    nvc_ui_color_code_underdotted   = 11,
    nvc_ui_color_code_underdashed   = 12,
    nvc_ui_color_code_altfont       = 13,
    nvc_ui_color_code_blend         = 14,
} nvc_ui_color_code_t;

#define NVC_UI_COLOR_NAME(_code) { #_code, nvc_ui_color_code_##_code }
static const std::map<const std::string, nvc_ui_color_code_t> nvc_ui_color_name_table = {
    NVC_UI_COLOR_NAME(foreground),
    NVC_UI_COLOR_NAME(background),
    NVC_UI_COLOR_NAME(special),
    NVC_UI_COLOR_NAME(reverse),
    NVC_UI_COLOR_NAME(italic),
    NVC_UI_COLOR_NAME(bold),
    NVC_UI_COLOR_NAME(strikethrough),
    NVC_UI_COLOR_NAME(underline),
    NVC_UI_COLOR_NAME(undercurl),
    NVC_UI_COLOR_NAME(underdouble),
    NVC_UI_COLOR_NAME(underdotted),
    NVC_UI_COLOR_NAME(underdashed),
    NVC_UI_COLOR_NAME(altfont),
    NVC_UI_COLOR_NAME(blend),
};

typedef struct nvc_ui_cell_size {
    uint32_t width;
    uint32_t height;
} nvc_ui_cell_size_t;

class NVCUIGrid;

typedef std::map<nvc_ui_color_code_t, uint32_t> nvc_ui_colors_set_t;
typedef std::map<int, nvc_ui_colors_set_t>      nvc_ui_colors_set_map_t;
typedef std::map<std::string, int>              nvc_ui_group_map_t;
typedef std::map<int, NVCUIGrid*>               nvc_ui_grid_map_t;

struct nvc_ui_context {
    nvc_rpc_context_t       rpc;
    nvc_ui_callback_t       cb;
    bool                    attached;
    CTFontRef               font;
    CGLayerRef              layer;
    CGSize                  cell_size;
    nvc_ui_cell_size_t      window_size;
    nvc_ui_colors_set_t     default_colors;
    nvc_ui_colors_set_map_t hl_attrs;
    nvc_ui_group_map_t      hl_groups;
    nvc_ui_grid_map_t       grids;
};

static inline CGLayerRef nvc_ui_create_layer(nvc_ui_context_t *ctx, CGContextRef context, int width ,int height) {
    if (context == nullptr && ctx->layer != nullptr) {
        context = CGLayerGetContext(ctx->layer);
    }
    CGLayerRef layer = CGLayerCreateWithContext(context, CGSizeMake(ctx->cell_size.width * width, ctx->cell_size.height * height), nullptr);
    if (layer != nullptr) {
        CGContextRef context = CGLayerGetContext(layer);
        CGContextSetShouldAntialias(context, true);
        CGContextSetShouldSmoothFonts(context, true);
    }
    return layer;
}

class NVCUIGrid {
private:
    CGLayerRef  m_layer;
public:
    NVCUIGrid(nvc_ui_context_t *ctx, int width, int height) {
        m_layer = nvc_ui_create_layer(ctx, nullptr, width, height);
    }

    virtual ~NVCUIGrid() {
        if (m_layer != nullptr) {
            CFRelease(m_layer);
            m_layer = nullptr;
        }
    }

    inline void clear() {
        if (m_layer != nullptr) {
            CGRect bounds = CGRectZero;
            CGContextRef context = CGLayerGetContext(m_layer);
            bounds.size = CGLayerGetSize(m_layer);
            CGContextClearRect(context, bounds);
        }
    }

    inline void resize(nvc_ui_context_t *ctx, int width, int height) {
        
    }
};

static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items);

static const std::string nvc_rpc_empty_str = "";
static const nvc_ui_cell_size_t nvc_ui_cell_size_zero = { 0, 0 };

#pragma mark - NVC RPC Helper
static inline const std::string nvc_rpc_read_str(nvc_rpc_context_t *ctx) {
    uint32_t len = 0;
    auto str = nvc_rpc_read_str(ctx, &len);
    return likely(str != nullptr) ? std::string(str, len) : nvc_rpc_empty_str;
}

#pragma mark - NVC UI Helper
static inline int nvc_ui_init_font(nvc_ui_context_t *ctx, const nvc_ui_config_t *config) {
    int res = NVC_RC_ILLEGAL_CALL;
    CFStringRef name = CFStringCreateWithCString(nullptr, config->familyName, CFStringGetSystemEncoding());
    if (name == nullptr) {
        res = NVC_RC_MALLOC_ERROR;
    } else {
        ctx->font = CTFontCreateWithName(name, config->fontSize, nullptr);
        if (ctx->font == nullptr) {
            NVLogW("nvc ui create font failed: %s (%lf)", config->familyName, config->fontSize);
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
            CGFloat width = advancement.width;
            ctx->cell_size = CGSizeMake(width, height);
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

static inline bool nvc_ui_size_equals(const nvc_ui_cell_size_t& s1, const nvc_ui_cell_size_t& s2) {
    return s1.width == s2.width && s1.height == s2.height;
}

static inline bool nvc_ui_update_size(nvc_ui_context_t *ctx, CGContextRef context, CGSize size) {
    bool res = false;
    nvc_ui_cell_size_t wnd_size = nvc_ui_size2cell(ctx, size);
    if (!nvc_ui_size_equals(ctx->window_size, wnd_size)) {
        ctx->window_size = wnd_size;
        CGLayer *layer = nvc_ui_create_layer(ctx, context, ctx->window_size.width, ctx->window_size.height);
        if (ctx->layer != nullptr) {
            CFRelease(ctx->layer);
        }
        ctx->layer = layer;
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
            ctx->layer = nullptr;
            ctx->cell_size = CGSizeZero;
            ctx->window_size = nvc_ui_cell_size_zero;
            int res = nvc_rpc_init(&ctx->rpc, inskt, outskt, userdata, nvc_ui_response_handler, nvc_ui_notification_handler);
            if (res == NVC_RC_OK) {
                res = nvc_ui_init_font(ctx, config);
            }
            if (res != NVC_RC_OK) {
                nvc_ui_destory(ctx);
                NVLogE("nvc ui init context failed");
                free(ctx);
                ctx = nullptr;
            }
        }
    }
    return ctx;
}

void nvc_ui_destory(nvc_ui_context_t *ctx) {
    if (ctx != nullptr) {
        nvc_rpc_final(&ctx->rpc);
        for (auto p = ctx->grids.begin(); p != ctx->grids.end(); p++) {
            delete p->second;
        }
        ctx->grids.clear();
        if (ctx->layer != nullptr) {
            CFRelease(ctx->layer);
            ctx->layer = nullptr;
        }
        if (ctx->font != nullptr) {
            CFRelease(ctx->font);
            ctx->font = nullptr;
        }
        delete ctx;
    }
}

void nvc_ui_attach(nvc_ui_context_t *ctx, CGContextRef context, CGSize size) {
    if (likely(ctx != nullptr && !ctx->attached)) {
        ctx->attached = true;
        nvc_ui_update_size(ctx, context, size);

        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_attach", 3);
        nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.width);
        nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.height);
        nvc_rpc_write_map_size(&ctx->rpc, 2);
        nvc_rpc_write_const_str(&ctx->rpc, "override");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_write_const_str(&ctx->rpc, "ext_linegrid");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

void nvc_ui_detach(nvc_ui_context_t *ctx) {
    if (likely(ctx != nullptr && ctx->attached)) {
        ctx->attached = false;
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_detach", 0);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

void nvc_ui_resize(nvc_ui_context_t *ctx, CGSize size) {
    if (likely(ctx != nullptr && ctx->attached)) {
        if (nvc_ui_update_size(ctx, nullptr, size)) {
            nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_try_resize", 2);
            nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.width);
            nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.height);
            nvc_rpc_call_end(&ctx->rpc);
        }
    }
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
            // TODO: Apply options
            //NVLogD("Set option: %s = %s", key.c_str(), value.c_str());
            nvc_rpc_read_skip_items(&ctx->rpc, narg);
        }
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
            auto p = ctx->grids.find(grid_id);
            if (p != ctx->grids.end()) {
                p->second->resize(ctx, width, height);
            } else {
                NVCUIGrid *grid = new NVCUIGrid(ctx, width, height);;
                if (likely(grid)) {
                    ctx->grids[grid_id] = grid;
                }
            }
            NVLogI("nvc ui grid resize %d - %dx%d", grid_id, width, height);
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
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            int hl_id = nvc_rpc_read_int(&ctx->rpc);
            nvc_ui_colors_set_t colors;
            int items = nvc_rpc_read_map_size(&ctx->rpc);
            for (int i = 0; i < items; i++) {
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
    if (likely(items-- > 0)) {
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

static inline int nvc_ui_redraw_action_grid_line(nvc_ui_context_t *ctx, int items) {
    if (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg > 4)) {
            narg -= 4;
            int grid = nvc_rpc_read_int(&ctx->rpc);
            int row = nvc_rpc_read_int(&ctx->rpc);
            int col_start = nvc_rpc_read_int(&ctx->rpc);
            int cells = nvc_rpc_read_array_size(&ctx->rpc);
            std::string output;
            while (cells-- > 0) {
                int cnum = nvc_rpc_read_array_size(&ctx->rpc);
                if (likely(cnum-- > 0)) {
                    auto text = nvc_rpc_read_str(&ctx->rpc);
                    output += text;
                    if (cnum >= 2) {
                        cnum -= 2;
                        nvc_rpc_read_int(&ctx->rpc); // hl
                        int repeat = nvc_rpc_read_int(&ctx->rpc);
                        while (--repeat > 0) {
                            output += text;
                        }
                    }
                }
                nvc_rpc_read_skip_items(&ctx->rpc, cnum);
            }
            nvc_rpc_read_skip_items(&ctx->rpc, cells);
            // TODO: Update grid lines
            NVLogI("nvc ui grid line %d row = %d, col_start = %d", grid, row, col_start);
            //NVLogD("Grid line: %s", output.c_str());
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_clear(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            auto p = ctx->grids.find(grid_id);
            if (p != ctx->grids.end()) {
                p->second->clear();
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
            auto p = ctx->grids.find(grid_id);
            if (p != ctx->grids.end()) {
                ctx->grids.erase(p);
                delete p->second;
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

#define NV_REDRAW_ACTION(action)        { #action, nvc_ui_redraw_action_##action }
#define NV_REDRAW_ACTION_IGNORE(action) { #action, nullptr }
// NOTE: https://neovim.io/doc/user/ui.html
static const std::map<const std::string, nvc_ui_action_t> nvc_ui_redraw_actions = {
    // Global Events
    NV_REDRAW_ACTION_IGNORE(set_icon),
    NV_REDRAW_ACTION(set_title),
    NV_REDRAW_ACTION_IGNORE(mode_info_set),
    NV_REDRAW_ACTION(option_set),
    NV_REDRAW_ACTION_IGNORE(mode_change),
    NV_REDRAW_ACTION_IGNORE(mouse_on),
    NV_REDRAW_ACTION_IGNORE(mouse_off),
    NV_REDRAW_ACTION_IGNORE(busy_start),
    NV_REDRAW_ACTION_IGNORE(busy_stop),
    NV_REDRAW_ACTION_IGNORE(suspend),
    NV_REDRAW_ACTION_IGNORE(update_menu),
    NV_REDRAW_ACTION_IGNORE(bell),
    NV_REDRAW_ACTION_IGNORE(visual_bell),
    NV_REDRAW_ACTION(flush),
    // Grid Events (line-based)
    NV_REDRAW_ACTION(grid_resize),
    NV_REDRAW_ACTION(default_colors_set),
    NV_REDRAW_ACTION(hl_attr_define),
    NV_REDRAW_ACTION(hl_group_set),
    NV_REDRAW_ACTION(grid_line),
    NV_REDRAW_ACTION(grid_clear),
    NV_REDRAW_ACTION(grid_destroy),
    NV_REDRAW_ACTION_IGNORE(grid_cursor_goto),
    NV_REDRAW_ACTION_IGNORE(grid_scroll),
    // Multigrid Events
    NV_REDRAW_ACTION_IGNORE(win_pos),
    NV_REDRAW_ACTION_IGNORE(win_float_pos),
    NV_REDRAW_ACTION_IGNORE(win_external_pos),
    NV_REDRAW_ACTION_IGNORE(win_hide),
    NV_REDRAW_ACTION_IGNORE(win_close),
    NV_REDRAW_ACTION_IGNORE(msg_set_pos),
    NV_REDRAW_ACTION_IGNORE(win_viewport),
    NV_REDRAW_ACTION_IGNORE(win_extmark),
    // Popupmenu Events
    NV_REDRAW_ACTION_IGNORE(popupmenu_show),
    NV_REDRAW_ACTION_IGNORE(popupmenu_select),
    NV_REDRAW_ACTION_IGNORE(popupmenu_hide),
    // Tabline Events
    NV_REDRAW_ACTION_IGNORE(tabline_update),
    // Cmdline Events
    NV_REDRAW_ACTION_IGNORE(cmdline_show),
    NV_REDRAW_ACTION_IGNORE(cmdline_pos),
    NV_REDRAW_ACTION_IGNORE(cmdline_special_char),
    NV_REDRAW_ACTION_IGNORE(cmdline_hide),
    NV_REDRAW_ACTION_IGNORE(cmdline_block_show),
    NV_REDRAW_ACTION_IGNORE(cmdline_block_append),
    NV_REDRAW_ACTION_IGNORE(cmdline_block_hide),
    // Message/Dialog Events
    NV_REDRAW_ACTION_IGNORE(msg_show),
    NV_REDRAW_ACTION_IGNORE(msg_clear),
    NV_REDRAW_ACTION_IGNORE(msg_showmode),
    NV_REDRAW_ACTION_IGNORE(msg_showcmd),
    NV_REDRAW_ACTION_IGNORE(msg_ruler),
    NV_REDRAW_ACTION_IGNORE(msg_history_show),
    NV_REDRAW_ACTION_IGNORE(msg_history_clear),
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
            } else if (p->second != NULL) {
                //NVLogW("nvc ui redraw action: %s", action.c_str());
                narg = p->second(ctx, narg);
            }
            nvc_rpc_read_skip_items(&ctx->rpc, narg);
        }
    }
    return items;
}

#define NV_NOTIFICATION_ACTION(action)   { #action, nvc_ui_notification_action_##action}
static const std::map<const std::string, nvc_ui_action_t> nvc_ui_notification_actions = {
    NV_NOTIFICATION_ACTION(redraw),
};

#pragma mark - NVC UI Helper
static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        uint64_t msgid = nvc_rpc_read_uint64(ctx);
        NVLogI("nvc ui recive response msgid: %lu", msgid);
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
            nvc_rpc_read_skip_items(ctx, nvc_rpc_read_map_size(ctx) * 2);
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
