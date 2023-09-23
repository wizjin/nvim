//
//  nvc_ui.cpp
//  NVim
//
//  Created by wizjin on 2023/9/20.
//

#include "nvc_ui.h"
#import <string>
#import <map>
#include "nvc_rpc.h"

#define nvc_ui_to_context(_ptr, _member)    nv_member_to_struct(nvc_ui_context_t, _ptr, _member)
#define nvc_ui_get_userdata(_ctx)           nvc_rpc_get_userdata(&(_ctx)->rpc)

typedef int (*nvc_ui_action_t)(nvc_ui_context_t *ctx, int narg);

struct nvc_ui_context {
    nvc_rpc_context_t   rpc;
    nvc_ui_callback_t   cb;
};

static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items);

static inline const std::string nvc_rpc_read_str(nvc_rpc_context_t *ctx) {
    uint32_t len = 0;
    auto str = nvc_rpc_read_str(ctx, &len);
    if (likely(str != NULL)) {
        return std::string(str, len);
    }
    return std::string();
}

#pragma mark - NVC UI API
nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_callback_t *callback, void *userdata) {
    nvc_ui_context_t *ctx = NULL;
    if (inskt != INVALID_SOCKET && outskt != INVALID_SOCKET && callback != NULL) {
        ctx = (nvc_ui_context_t *)malloc(sizeof(nvc_ui_context_t));
        if (unlikely(ctx == NULL)) {
            NVLogE("nvc ui create context failed");
        } else {
            bzero(ctx, sizeof(nvc_ui_context_t));
            int res = nvc_rpc_init(&ctx->rpc, inskt, outskt, userdata, nvc_ui_response_handler, nvc_ui_notification_handler);
            if (res == NVC_RPC_RC_OK) {
                ctx->cb = *callback;
            } else {
                NVLogE("nvc ui init context failed");
                free(ctx);
                ctx = NULL;
            }
        }
    }
    return ctx;
}

void nvc_ui_destory(nvc_ui_context_t *ctx) {
    if (ctx != NULL) {
        nvc_rpc_final(&ctx->rpc);
    }
}

void nvc_ui_attach(nvc_ui_context_t *ctx) {
    if (likely(ctx != NULL)) {
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_attach", 3);
        nvc_rpc_write_unsigned(&ctx->rpc, 80);
        nvc_rpc_write_unsigned(&ctx->rpc, 40);
        nvc_rpc_write_map_size(&ctx->rpc, 2);
        nvc_rpc_write_const_str(&ctx->rpc, "override");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_write_const_str(&ctx->rpc, "ext_linegrid");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

void nvc_ui_detach(nvc_ui_context_t *ctx) {
    if (likely(ctx != NULL)) {
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_detach", 0);
        nvc_rpc_call_end(&ctx->rpc);
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
            int grid = nvc_rpc_read_int(&ctx->rpc);
            int width = nvc_rpc_read_int(&ctx->rpc);
            int height = nvc_rpc_read_int(&ctx->rpc);
            // TODO: Resize grid
            NVLogI("nvc ui grid resize %d - %dx%d", grid, width, height);
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
            uint32_t foreground = nvc_rpc_read_uint32(&ctx->rpc);
            foreground++;
            uint32_t background = nvc_rpc_read_uint32(&ctx->rpc);
            uint32_t special = nvc_rpc_read_uint32(&ctx->rpc);
            special++;
            ctx->cb.update_background(nvc_ui_get_userdata(ctx), background);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_hl_attr_define(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        // TODO: update highlight colors
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_hl_group_set(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        // TODO: update highlight group
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
            int grid = nvc_rpc_read_int(&ctx->rpc);
            // TODO: Clear grid
            NVLogI("nvc ui grid clear %d", grid);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

#define NV_REDRAW_ACTION(action)        { #action, nvc_ui_redraw_action_##action }
#define NV_REDRAW_ACTION_IGNORE(action) { #action, NULL }
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
    NV_REDRAW_ACTION_IGNORE(grid_destroy),
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
