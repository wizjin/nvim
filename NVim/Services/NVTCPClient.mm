//
//  NVTCPClient.m
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import "NVTCPClient.h"
#import <sys/types.h>
#import <sys/ioctl.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <netdb.h>
#import <pthread.h>
#import <string>
#import <map>
#import "nvc_rpc.h"

#define nvc_rpc_write_const_str(c, str)     nvc_rpc_write_str(c, str, sizeof(str) - 1);
#define nvc_rpc_call_const_begin(c, m, n)   nvc_rpc_call_begin(c, m, sizeof(m) - 1, n)

typedef int (*nv_rpc_action_t)(NVClient *client, nvc_rpc_context_t *ctx, int narg);

@interface NVTCPClient () {
@private
    nvc_rpc_context_t *rpc_ctx;
}

@property (nonatomic, readonly, strong) NSString *host;
@property (nonatomic, readonly, assign) int port;

@end

@implementation NVTCPClient

- (instancetype)initWithHost:(NSString *)host port:(int)port {
    if (self = [super init]) {
        rpc_ctx = NULL;
        _host = host;
        _port = port;

        [self open];
    }
    return self;
}

- (void)open {
    [self close];

    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *infos = NULL;
    const char *hostname = self.host.cstr;
    char port[32] = {0};
    snprintf(port, sizeof(port), "%d", self.port);
    int skt = INVALID_SOCKET;
    int err = getaddrinfo(hostname, port, &hints, &infos);
    if (err != 0) {
        NVLogW("TCP Client lookup failed - %s: %s", self.host.cstr, gai_strerror(err));
    } else {
        for (struct addrinfo *p = infos; p != NULL; p = p->ai_next) {
            int s = nv_config_socket(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
            if (s != INVALID_SOCKET) {
                switch (p->ai_family) {
                    case AF_INET:
                        ((struct sockaddr_in *)p->ai_addr)->sin_port = htons(self.port);
                    case AF_INET6:
                        ((struct sockaddr_in6 *)p->ai_addr)->sin6_port = htons(self.port);
                }
                if (connect(s, p->ai_addr, p->ai_addrlen) != 0) {
                    close(s);
                } else {
                    skt = s;
                    NVLogI("TCP Client connect success - %s:%d", hostname, self.port);
                    break;
                }
            }
        }
    }
    if (infos != NULL) {
        freeaddrinfo(infos);
    }
    if (skt == INVALID_SOCKET) {
        NVLogW("TCP Client create connection failed - %s:%d : %s", hostname, self.port, strerror(errno));
    } else {
        rpc_ctx = nvc_rpc_create(skt, skt, (__bridge void *)self, nvclient_rpc_response_handler, nvclient_rpc_notification_handler);
        if (rpc_ctx == NULL) {
            NVLogE("TCP Client create rpc failed");
        } else {
            NVLogE("TCP Client create rpc success");
        }
        close(skt);
    }
}

- (void)close {
    if (rpc_ctx != NULL) {
        NVLogD("TCP Client close connection success - %s:%d", self.host.cstr, self.port);
        nvc_rpc_destory(&rpc_ctx);
    }
}

- (void)attachUI {
    nvc_rpc_call_const_begin(rpc_ctx, "nvim_ui_attach", 3);
    nvc_rpc_write_unsigned(rpc_ctx, 80);
    nvc_rpc_write_unsigned(rpc_ctx, 40);
    nvc_rpc_write_map_size(rpc_ctx, 2);
    nvc_rpc_write_const_str(rpc_ctx, "override");
    nvc_rpc_write_true(rpc_ctx);
    nvc_rpc_write_const_str(rpc_ctx, "ext_linegrid");
    nvc_rpc_write_true(rpc_ctx);
    nvc_rpc_call_end(rpc_ctx);
}

- (void)detachUI {
    nvc_rpc_call_const_begin(rpc_ctx, "nvim_ui_detach", 0);
    nvc_rpc_call_end(rpc_ctx);
}

#pragma mark - Work Helper
static inline int nv_config_socket_linger(int skt) {
    static const struct linger so_linger = { 1, 1 };
    return setsockopt(skt, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
}

static inline int nv_config_socket_keepalive(int skt) {
    static const int on = 1;
    return setsockopt(skt, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
}

typedef int (*nv_config_socket_handler)(int);

static const nv_config_socket_handler nv_config_socket_handlers[] = {
    nv_config_socket_linger,
    nv_config_socket_keepalive,
};

static inline int nv_config_socket(int skt) {
    if (skt != INVALID_SOCKET) {
        for (int i = 0; i < countof(nv_config_socket_handlers); i++) {
            if (nv_config_socket_handlers[i](skt) != 0) {
                NVLogW("TCP Client config(%d) socket failed: %s", i, strerror(errno));
                close(skt);
                skt = INVALID_SOCKET;
                break;
            }
        }
    }
    return skt;
}

static inline const std::string nvc_rpc_read_str(nvc_rpc_context_t *ctx) {
    uint32_t len = 0;
    auto str = nvc_rpc_read_str(ctx, &len);
    if (likely(str != NULL)) {
        return std::string(str, len);
    }
    return std::string();
}

#pragma mark - Notification Actions
static inline int nvclient_rpc_response_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        uint64_t msgid = nvc_rpc_read_uint64(ctx);
        NVLogI("TCP Client recive response msgid: %lu", msgid);
        if (likely(items-- > 0)) {
            int n = nvc_rpc_read_array_size(ctx);
            if (n >= 2) {
                n -= 2;
                int64_t code = nvc_rpc_read_int64(ctx);
                auto msg = nvc_rpc_read_str(ctx);
                NVLogE("TCP Client match request %d failed(%lu): %s", msgid, code, msg.c_str());
            }
            nvc_rpc_read_skip_items(ctx, n);
        }
        if (likely(items-- > 0)) {
            nvc_rpc_read_skip_items(ctx, nvc_rpc_read_map_size(ctx) * 2);
        }
    }
    return items;
}

static inline int nvclient_rpc_notification_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        auto action = nvc_rpc_read_str(ctx);
        auto p = nv_notification_actions.find(action);
        if (p == nv_notification_actions.end()) {
            NVLogW("TCP Client unknown notification action: %s", action.c_str());
        } else if (likely(items-- > 0)) {
            NVTCPClient *client = (__bridge NVTCPClient *)nvc_rpc_get_userdata(ctx);
            nvc_rpc_read_skip_items(ctx, p->second(client, ctx, nvc_rpc_read_array_size(ctx)));
        }
    }
    return items;
}

static inline int nv_notification_action_redraw(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    NVLogD("TCP Client redraw: %d", items);
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx);
        if (narg-- > 0) {
            auto action = nvc_rpc_read_str(ctx);
            auto p = nv_redraw_actions.find(action);
            if (p == nv_redraw_actions.end()) {
                NVLogW("TCP Client unknown redraw action: %s", action.c_str());
            } else if (p->second != NULL) {
                narg = p->second(client, ctx, narg);
            }
            nvc_rpc_read_skip_items(ctx, narg);
        }
    }
    return items;
}

#define NV_NOTIFICATION_ACTION(action)   { #action, nv_notification_action_##action}
static const std::map<const std::string, nv_rpc_action_t> nv_notification_actions = {
    NV_NOTIFICATION_ACTION(redraw),
};

#pragma mark - Redraw Actions
static inline int nv_redraw_action_set_title(NVClient *client, nvc_rpc_context_t *ctx, int count) {
    if (likely(count-- > 0)) {
        int items = nvc_rpc_read_array_size(ctx);
        if (likely(items-- > 0)) {
            uint32_t len = 0;
            const char *str = nvc_rpc_read_str(ctx, &len);
            if (str != NULL) {
                NSString *title = [[NSString alloc] initWithBytes:str length:len encoding:NSUTF8StringEncoding];
                dispatch_main_async(^{
                    [client.delegate client:client updateTitle:title];
                });
            }
        }
        nvc_rpc_read_skip_items(ctx, items);
    }
    return count;
}

static inline int nv_redraw_action_option_set(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx);
        if (likely(narg-- > 0)) {
            std::string value;
            auto key = nvc_rpc_read_str(ctx);
            if (narg > 0) {
                switch (nvc_rpc_read_ahead(ctx)) {
                    case NVC_RPC_ITEM_STR:
                        value = nvc_rpc_read_str(ctx);
                        narg--;
                        break;
                    case NVC_RPC_ITEM_BOOLEAN:
                        value = nvc_rpc_read_bool(ctx) ? "true" : "false";
                        narg--;
                        break;
                    case NVC_RPC_ITEM_POSITIVE_INTEGER:
                    case NVC_RPC_ITEM_NEGATIVE_INTEGER:
                        value = std::to_string(nvc_rpc_read_int(ctx));
                        narg--;
                        break;
                    default:
                        NVLogW("Unknown option value type: %s", key.c_str());
                }
            }
            // TODO: Apply options
            //NVLogD("Set option: %s = %s", key.c_str(), value.c_str());
            nvc_rpc_read_skip_items(ctx, narg);
        }
    }
    return items;
}

static inline int nv_redraw_action_flush(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    dispatch_main_async(^{
        [client.delegate clientFlush:client];
    });
    return items;
}

static inline int nv_redraw_action_grid_resize(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx);
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid = nvc_rpc_read_int(ctx);
            int width = nvc_rpc_read_int(ctx);
            int height = nvc_rpc_read_int(ctx);
            // TODO: Resize grid
            NVLogI("Grid resize %d - %dx%d", grid, width, height);
        }
        nvc_rpc_read_skip_items(ctx, narg);
    }
    return items;
}

static inline int nv_redraw_action_default_colors_set(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx);
        if (likely(narg >= 3)) {
            narg -= 3;
            NVColorsSet *colorsSet = [NVColorsSet new];
            colorsSet.foreground = [NSColor colorWithRGB:nvc_rpc_read_uint32(ctx)];
            colorsSet.background = [NSColor colorWithRGB:nvc_rpc_read_uint32(ctx)];
            colorsSet.special = [NSColor colorWithRGB:nvc_rpc_read_uint32(ctx)];
            dispatch_main_async(^{
                [client.delegate client:client updateColorsSet:colorsSet];
            });
        }
        nvc_rpc_read_skip_items(ctx, narg);
    }
    return items;
}

static inline int nv_redraw_action_hl_attr_define(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx);
        // TODO: update highlight colors
        nvc_rpc_read_skip_items(ctx, narg);
    }
    return items;
}

static inline int nv_redraw_action_hl_group_set(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx);
        // TODO: update highlight group
        nvc_rpc_read_skip_items(ctx, narg);
    }
    return items;
}

static inline int nv_redraw_action_grid_line(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    if (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx);
        if (likely(narg > 4)) {
            narg -= 4;
            int grid = nvc_rpc_read_int(ctx);
            int row = nvc_rpc_read_int(ctx);
            int col_start = nvc_rpc_read_int(ctx);
            int cells = nvc_rpc_read_array_size(ctx);
            std::string output;
            while (cells-- > 0) {
                int cnum = nvc_rpc_read_array_size(ctx);
                if (likely(cnum-- > 0)) {
                    auto text = nvc_rpc_read_str(ctx);
                    output += text;
                    if (cnum >= 2) {
                        cnum -= 2;
                        nvc_rpc_read_int(ctx); // hl
                        int repeat = nvc_rpc_read_int(ctx);
                        while (--repeat > 0) {
                            output += text;
                        }
                    }
                }
                nvc_rpc_read_skip_items(ctx, cnum);
            }
            nvc_rpc_read_skip_items(ctx, cells);
            // TODO: Update grid lines
            NVLogI("Grid line %d row = %d, col_start = %d", grid, row, col_start);
            //NVLogD("Grid line: %s", output.c_str());
        }
        nvc_rpc_read_skip_items(ctx, narg);
    }
    return items;
}

static inline int nv_redraw_action_grid_clear(NVClient *client, nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx);
        if (likely(narg-- > 0)) {
            int grid = nvc_rpc_read_int(ctx);
            // TODO: Clear grid
            NVLogI("Grid clear %d", grid);
        }
        nvc_rpc_read_skip_items(ctx, narg);
    }
    return items;
}

#define NV_REDRAW_ACTION(action)        { #action, nv_redraw_action_##action }
#define NV_REDRAW_ACTION_IGNORE(action) { #action, NULL }
// NOTE: https://neovim.io/doc/user/ui.html
static const std::map<const std::string, nv_rpc_action_t> nv_redraw_actions = {
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

#pragma mark - API Helper
static inline uint64_t nvc_rpc_call_begin(nvc_rpc_context_t *ctx, const char *method, int method_len, int narg) {
    uint64_t uuid = nvc_rpc_get_next_uuid(ctx);
    nvc_rpc_write_array_size(ctx, 4);
    nvc_rpc_write_signed(ctx, 0);
    nvc_rpc_write_unsigned(ctx, uuid);
    nvc_rpc_write_str(ctx, method, method_len);
    nvc_rpc_write_array_size(ctx, narg);
    return uuid;
}

static inline void nvc_rpc_call_end(nvc_rpc_context_t *ctx) {
    nvc_rpc_write_flush(ctx);
}


@end
