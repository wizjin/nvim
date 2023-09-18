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
extern "C" {
#import "cwpack.h"
}

#define NV_MSGPACK_KBYTES_BITS              10 // 1 << 10 == 1KB
#define NV_MSGPACK_BUFFER_INIT              (4<<NV_MSGPACK_KBYTES_BITS)
#define NV_MSGPACK_BUFFER_PREREAD           (16<<NV_MSGPACK_KBYTES_BITS)
#define NV_MSGPACK_BUFFER_THRESHOLD         (256<<NV_MSGPACK_KBYTES_BITS)
#define cw_pack_const_str(c, str)           cw_pack_str(c, str, sizeof(str) - 1);
#define nv_rpc_call_const_begin(c, m, n)    nv_rpc_call_begin(c, m, sizeof(m) - 1, n)

typedef struct nv_pack_context {
    cw_pack_context cw;
    int skt;
    uint64_t uuid;
    size_t datalen;
    uint8_t *dataptr;
} nv_pack_context_t;

typedef struct nv_unpack_context {
    cw_unpack_context cw;
    int skt;
    size_t datalen;
    uint8_t *dataptr;
} nv_unpack_context_t;

typedef int (*nv_rpc_action_t)(NVClient *client, cw_unpack_context *ctx, int narg);

@interface NVTCPClient () {
@private
    int skt;
    pthread_t worker;
    nv_pack_context_t nv_out_ctx;
}

@property (nonatomic, readonly, strong) NSString *host;
@property (nonatomic, readonly, assign) int port;

@end

@implementation NVTCPClient

- (instancetype)initWithHost:(NSString *)host port:(int)port {
    if (self = [super init]) {
        skt = INVALID_SOCKET;
        worker = NULL;
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
        err = pthread_create(&worker, NULL, nv_start_routine, (__bridge void *)self);
        if (err == 0) {
            nv_pack_context_init(&nv_out_ctx, skt);
        } else {
            NVLogE("TCP Client create worker thread failed: %s", strerror(err));
            worker = NULL;
            [self close];
        }
    }
}

- (void)close {
    nv_pack_context_final(&nv_out_ctx);
    if (skt != INVALID_SOCKET) {
        NVLogD("TCP Client close connection success - %s:%d", self.host.cstr, self.port);
        close(skt);
        skt = INVALID_SOCKET;
    }
    if (worker != NULL) {
        pthread_join(worker, NULL);
        worker = NULL;
    }
}

- (void)attachUI {
    nv_rpc_call_const_begin(&nv_out_ctx, "nvim_ui_attach", 3);
    cw_pack_unsigned(&nv_out_ctx.cw, 80);
    cw_pack_unsigned(&nv_out_ctx.cw, 40);
    cw_pack_map_size(&nv_out_ctx.cw, 2);
    cw_pack_const_str(&nv_out_ctx.cw, "override");
    cw_pack_true(&nv_out_ctx.cw);
    cw_pack_const_str(&nv_out_ctx.cw, "ext_linegrid");
    cw_pack_true(&nv_out_ctx.cw);
    nv_pack_call_end(&nv_out_ctx);
}

- (void)detachUI {
    nv_rpc_call_const_begin(&nv_out_ctx, "nvim_ui_detach", 0);
    nv_pack_call_end(&nv_out_ctx);
}

- (void)runWorkRoutine {
    nv_unpack_context_t ctx;
    if (nv_unpack_context_init(&ctx, skt) == CWP_RC_OK) {
        while (nv_unpack_context_continue(&ctx)) {
            if (cw_look_ahead(&ctx.cw) != CWP_ITEM_ARRAY) {
                cw_skip_items(&ctx.cw, 1);
            } else {
                cw_unpack_next(&ctx.cw);
                switch (ctx.cw.item.as.array.size) {
                    default:
                        cw_skip_items(&ctx.cw, ctx.cw.item.as.array.size);
                        break;
                    case 3:
                        // notify
                        assert(cw_unpack_next_int(&ctx.cw) == 2);
                        nv_call_notification_action(self, cw_unpack_next_str(&ctx.cw), &ctx);
                        break;
                    case 4:
                        assert(cw_unpack_next_int(&ctx.cw) == 1);
                        uint64_t msgid = cw_unpack_next_uint64(&ctx.cw);
                        NVLogI("TCP Client recive response msgid: %lu", msgid);
                        // Show error
                        if (cw_look_ahead(&ctx.cw) != CWP_ITEM_ARRAY) {
                            cw_skip_items(&ctx.cw, 1);
                        } else {
                            cw_unpack_next(&ctx.cw);
                            if (ctx.cw.item.as.array.size != 2) {
                                cw_skip_items(&ctx.cw, ctx.cw.item.as.array.size);
                            } else {
                                int64_t code = cw_unpack_next_int64(&ctx.cw);
                                auto msg = cw_unpack_next_str(&ctx.cw);
                                NVLogE("TCP Client match request %d failed(%lu): %s", msgid, code, msg.c_str());
                            }
                        }
                        // TODO: result
                        if (cw_look_ahead(&ctx.cw) != CWP_ITEM_MAP) {
                            cw_skip_items(&ctx.cw, 1);
                        } else {
                            cw_unpack_next(&ctx.cw);
                            cw_skip_items(&ctx.cw, ctx.cw.item.as.map.size);
                        }
                        break;
                }
            }
        }
        nv_unpack_context_final(&ctx);
    }
}

#pragma mark - Work Helper
static inline void *nv_start_routine(void *ptr) {
    [(__bridge NVTCPClient *)ptr runWorkRoutine];
    return NULL;
}

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

static inline size_t nv_msgpack_memory_best_size(size_t kept, size_t more) {
    size_t datalen = kept;
    size_t target = kept + more;
    while (datalen < target) {
        datalen *= 2;
    }
    if (datalen > NV_MSGPACK_BUFFER_THRESHOLD) {
        datalen = kept + ((((more)>>NV_MSGPACK_KBYTES_BITS) + !!((more)&((1<<NV_MSGPACK_KBYTES_BITS)-1)))<<NV_MSGPACK_KBYTES_BITS);
    }
    return datalen;
}

#pragma mark - MsgPack Pack
static inline int nv_pack_context_init(nv_pack_context_t *ctx, int skt) {
    int result = CWP_RC_MALLOC_ERROR;
    bzero(ctx, sizeof(nv_pack_context_t));
    ctx->datalen = NV_MSGPACK_BUFFER_INIT;
    ctx->dataptr = (uint8_t *)malloc(ctx->datalen);
    if (ctx->dataptr != NULL) {
        result = cw_pack_context_init(&ctx->cw, ctx->dataptr, ctx->datalen, nv_pack_overflow_handler);
        if (result != CWP_RC_OK) {
            nv_pack_context_final(ctx);
        } else {
            cw_pack_set_flush_handler(&ctx->cw, nv_pack_flush_handler);
            ctx->skt = skt;
            ctx->uuid = 1;
        }
    }
    return result;
}

static inline void nv_pack_context_final(nv_pack_context_t *ctx) {
    if (ctx->dataptr != NULL) {
        free(ctx->dataptr);
    }
    bzero(ctx, sizeof(nv_pack_context_t));
    ctx->skt = INVALID_SOCKET;
}

static inline int nv_pack_overflow_handler(cw_pack_context *ptr, size_t more) {
    int result = nv_pack_flush_handler(ptr);
    if (result == CWP_RC_OK) {
        nv_pack_context_t *ctx = (nv_pack_context_t *)ptr;
        size_t kept = ctx->cw.current - ctx->cw.start;
        size_t target = kept + more;
        if (ctx->datalen > NV_MSGPACK_BUFFER_THRESHOLD && target < NV_MSGPACK_BUFFER_THRESHOLD) {
            void *new_dateptr = realloc(ctx->dataptr, NV_MSGPACK_BUFFER_THRESHOLD);
            if (new_dateptr != NULL) {
                ctx->datalen = NV_MSGPACK_BUFFER_THRESHOLD;
                ctx->dataptr = (uint8_t *)new_dateptr;
                ctx->cw.start = ctx->dataptr;
                ctx->cw.current = ctx->cw.start + kept;
                ctx->cw.end = ctx->cw.start + ctx->datalen;
            }
        }
        if (target > ctx->datalen) {
            size_t new_datalen = nv_msgpack_memory_best_size(ctx->datalen, more);
            void *new_dateptr = realloc(ctx->dataptr, new_datalen);
            if (new_dateptr == NULL) {
                result = CWP_RC_BUFFER_OVERFLOW;
            } else {
                ctx->datalen = new_datalen;
                ctx->dataptr = (uint8_t *)new_dateptr;
                ctx->cw.start = ctx->dataptr;
                ctx->cw.current = ctx->cw.start + kept;
                ctx->cw.end = ctx->cw.start + ctx->datalen;
            }
        }
    }
    return result;
}

static inline int nv_pack_flush_handler(cw_pack_context *ptr) {
    nv_pack_context_t *ctx = (nv_pack_context_t *)ptr;
    int result = CWP_RC_ERROR_IN_HANDLER;
    if (ctx->cw.return_code == 0) {
        result = CWP_RC_OK;
        size_t contains = ctx->cw.current - ctx->cw.start;
        if (contains > 0) {
            size_t i = 0;
            while (contains > 0) {
                ssize_t n = write(ctx->skt, ctx->cw.start + i, contains);
                if (n > 0) {
                    i += n;
                    contains -= n;
                    continue;
                }
                if (n == 0) {
                    result = CWP_RC_END_OF_INPUT;
                } else {
                    if (errno != EBADF) {
                        NVLogW("TCP Client write data failed: %s", strerror(errno));
                    }
                    ctx->cw.err_no = errno;
                    result = CWP_RC_ERROR_IN_HANDLER;
                }
                break;
            }
            if (contains > 0) {
                memmove(ctx->cw.start, ctx->cw.start + i, contains);
            }
            ctx->cw.current = ctx->cw.start + contains;
            ctx->cw.end = ctx->cw.start + ctx->datalen;
        }
    }
    return result;
}

#pragma mark - MsgPack Unpack
static inline int nv_unpack_context_init(nv_unpack_context_t *ctx, int skt) {
    int result = CWP_RC_MALLOC_ERROR;
    bzero(ctx, sizeof(nv_unpack_context_t));
    ctx->datalen = NV_MSGPACK_BUFFER_INIT;
    ctx->dataptr = (uint8_t *)malloc(ctx->datalen);
    if (ctx->dataptr != NULL) {
        result = cw_unpack_context_init(&ctx->cw, ctx->dataptr, 0, nv_unpack_underflow_handler);
        if (result != CWP_RC_OK) {
            nv_unpack_context_final(ctx);
        } else {
            ctx->skt = skt;
        }
    }
    return result;
}

static inline void nv_unpack_context_final(nv_unpack_context_t *ctx) {
    if (ctx->dataptr != NULL) {
        free(ctx->dataptr);
    }
    bzero(ctx, sizeof(nv_unpack_context_t));
    ctx->skt = INVALID_SOCKET;
}

static inline bool nv_unpack_context_continue(nv_unpack_context_t *ctx) {
    return ctx->cw.return_code == 0;
}

static inline int nv_unpack_underflow_handler(cw_unpack_context *ptr, size_t more) {
    nv_unpack_context_t *ctx = (nv_unpack_context_t *)ptr;
    int result = CWP_RC_ERROR_IN_HANDLER;
    if (ctx->cw.return_code == 0) {
        result = CWP_RC_OK;
        size_t used = ctx->cw.current - ctx->cw.start;
        size_t remains = ctx->cw.end - ctx->cw.current;

        size_t ready = 0;
        if (ioctl(ctx->skt, FIONREAD, &ready) != -1 && ready > more) {
            more = MAX(more, MIN(MIN(ready, NV_MSGPACK_BUFFER_PREREAD), NV_MSGPACK_BUFFER_THRESHOLD - remains));
        }

        size_t target = remains + more;
        if (used > 0 && used + target > ctx->datalen) {
            if (remains > 0) {
                memmove(ctx->cw.start, ctx->cw.current, remains);
            }
            if (ctx->datalen > NV_MSGPACK_BUFFER_THRESHOLD && target < NV_MSGPACK_BUFFER_THRESHOLD) {
                void *new_dataptr = realloc(ctx->dataptr, NV_MSGPACK_BUFFER_THRESHOLD);
                if (new_dataptr != NULL) {
                    ctx->datalen = NV_MSGPACK_BUFFER_THRESHOLD;
                    ctx->dataptr = (uint8_t *)new_dataptr;
                    ctx->cw.start = ctx->dataptr;
                }
            }
            ctx->cw.current = ctx->cw.start;
            ctx->cw.end = ctx->cw.current + remains;
            used = 0;
        }
        if (target > ctx->datalen) {
            size_t new_datalen = nv_msgpack_memory_best_size(ctx->datalen, target);
            void *new_dataptr = realloc(ctx->dataptr, new_datalen);
            if (new_dataptr == NULL) {
                result = CWP_RC_BUFFER_UNDERFLOW;
            } else {
                ctx->datalen = new_datalen;
                ctx->dataptr = (uint8_t *)new_dataptr;
                ctx->cw.start = ctx->dataptr;
                ctx->cw.current = ctx->cw.start + used;
                ctx->cw.end = ctx->cw.current + remains;
            }
        }
        if (result == CWP_RC_OK) {
            while (more > 0) {
                ssize_t n = read(ctx->skt, ctx->cw.end, more);
                if (n > 0) {
                    ctx->cw.end += n;
                    more -= n;
                    continue;
                }
                if (n == 0) {
                    result = CWP_RC_END_OF_INPUT;
                } else if (n < 0) {
                    if (errno != EBADF) {
                        NVLogW("TCP Client read data failed: %s", strerror(errno));
                    }
                    ctx->cw.err_no = errno;
                    result = CWP_RC_ERROR_IN_HANDLER;
                }
                break;
            }
        }
    }
    return result;
}

static inline const std::string cw_unpack_as_str(cw_unpack_context *ctx) {
    return std::string((const char *)ctx->item.as.str.start, ctx->item.as.str.length);
}

static inline const std::string cw_unpack_next_str(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_STR);
    return cw_unpack_as_str(ctx);
}

static inline NSString *cw_unpack_as_nsstr(cw_unpack_context *ctx) {
    return [[NSString alloc] initWithBytes:ctx->item.as.str.start length:ctx->item.as.str.length encoding:NSUTF8StringEncoding];
}

static inline NSString *cw_unpack_next_nsstr(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_STR);
    return cw_unpack_as_nsstr(ctx);
}

static inline bool cw_unpack_next_bool(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_BOOLEAN);
    return ctx->item.as.boolean;
}

static inline int cw_unpack_next_int(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_POSITIVE_INTEGER || ctx->item.type == CWP_ITEM_NEGATIVE_INTEGER);
    return (int)ctx->item.as.i64;
}

static inline uint32_t cw_unpack_next_uint32(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_POSITIVE_INTEGER || ctx->item.type == CWP_ITEM_NEGATIVE_INTEGER);
    return (uint32_t)ctx->item.as.i64;
}

static inline int64_t cw_unpack_next_int64(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_POSITIVE_INTEGER || ctx->item.type == CWP_ITEM_NEGATIVE_INTEGER);
    return ctx->item.as.i64;
}

static inline uint64_t cw_unpack_next_uint64(cw_unpack_context *ctx) {
    cw_unpack_next(ctx);
    assert(ctx->item.type == CWP_ITEM_POSITIVE_INTEGER);
    return ctx->item.as.u64;
}

static inline int cw_unpack_next_array(cw_unpack_context *ctx) {
    int items = 0;
    if (cw_look_ahead(ctx) != CWP_ITEM_ARRAY) {
        cw_skip_items(ctx, 1);
    } else {
        cw_unpack_next(ctx);
        items = ctx->item.as.array.size;
    }
    return items;
}

#pragma mark - Notification Actions
static inline int nv_notification_action_redraw(NVClient *client, cw_unpack_context *ctx, int count) {
    //NVLogD("TCP Client redraw: %d", count);
    while (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        if (narg-- > 0) {
            auto action = cw_unpack_next_str(ctx);
            auto p = nv_redraw_actions.find(action);
            if (p == nv_redraw_actions.end()) {
                NVLogW("TCP Client unknown redraw action: %s", action.c_str());
            } else if (p->second != NULL) {
                narg = p->second(client, ctx, narg);
            }
            cw_skip_items(ctx, narg);
        }
    }
    return count;
}

#define NV_NOTIFICATION_ACTION(action)   { #action, nv_notification_action_##action}
static const std::map<const std::string, nv_rpc_action_t> nv_notification_actions = {
    NV_NOTIFICATION_ACTION(redraw),
};

static inline void nv_call_notification_action(NVTCPClient *client, const std::string& action, nv_unpack_context_t *ctx) {
    auto p = nv_notification_actions.find(action);
    if (p == nv_notification_actions.end()) {
        NVLogW("TCP Client unknown notification action: %s", action.c_str());
        cw_skip_items(&ctx->cw, 1);
    } else {
        int need_skip = p->second(client, &ctx->cw, cw_unpack_next_array(&ctx->cw));
        cw_skip_items(&ctx->cw, need_skip);
    }
}

#pragma mark - Redraw Actions
static inline int nv_redraw_action_set_title(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int items = cw_unpack_next_array(ctx);
        if (items-- > 0) {
            NSString *title = cw_unpack_next_nsstr(ctx);
            dispatch_main_async(^{
                [client.delegate client:client updateTitle:title];
            });
        }
        cw_skip_items(ctx, items);
    }
    return count;
}

static inline int nv_redraw_action_option_set(NVClient *client, cw_unpack_context *ctx, int count) {
    while (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        if (narg-- > 0) {
            std::string value;
            auto key = cw_unpack_next_str(ctx);
            if (narg > 0) {
                switch (cw_look_ahead(ctx)) {
                    case CWP_ITEM_STR:
                        value = cw_unpack_next_str(ctx);
                        narg--;
                        break;
                    case CWP_ITEM_BOOLEAN:
                        value = cw_unpack_next_bool(ctx) ? "true" : "false";
                        narg--;
                        break;
                    case CWP_ITEM_POSITIVE_INTEGER:
                    case CWP_ITEM_NEGATIVE_INTEGER:
                        value = std::to_string(cw_unpack_next_int(ctx));
                        narg--;
                        break;
                    default:
                        NVLogW("Unknown option value type: %s", key.c_str());
                }
            }
            // TODO: Apply options
            //NVLogD("Set option: %s = %s", key.c_str(), value.c_str());
        }
        cw_skip_items(ctx, narg);
    }
    return count;
}

static inline int nv_redraw_action_flush(NVClient *client, cw_unpack_context *ctx, int count) {
    dispatch_main_async(^{
        [client.delegate clientFlush:client];
    });
    return count;
}

static inline int nv_redraw_action_grid_resize(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        if (narg >= 3) {
            narg -= 3;
            int grid = cw_unpack_next_int(ctx);
            int width = cw_unpack_next_int(ctx);
            int height = cw_unpack_next_int(ctx);
            // TODO: Resize grid
            NVLogI("Grid resize %d - %dx%d", grid, width, height);
        }
        cw_skip_items(ctx, narg);
    }
    return count;
}

static inline int nv_redraw_action_default_colors_set(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        if (narg >= 3) {
            narg -= 3;
            NVColorsSet *colorsSet = [NVColorsSet new];
            colorsSet.foreground = [NSColor colorWithRGB:cw_unpack_next_uint32(ctx)];
            colorsSet.background = [NSColor colorWithRGB:cw_unpack_next_uint32(ctx)];
            colorsSet.special = [NSColor colorWithRGB:cw_unpack_next_uint32(ctx)];
            dispatch_main_async(^{
                [client.delegate client:client updateColorsSet:colorsSet];
            });
        }
        cw_skip_items(ctx, narg);
    }
    return count;
}

static inline int nv_redraw_action_hl_attr_define(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        // TODO: update highlight colors
        cw_skip_items(ctx, narg);
    }
    return count;
}

static inline int nv_redraw_action_hl_group_set(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        // TODO: update highlight group
        cw_skip_items(ctx, narg);
    }
    return count;
}

static inline int nv_redraw_action_grid_line(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        if (narg-- > 4) {
            narg -= 4;
            int grid = cw_unpack_next_int(ctx);
            int row = cw_unpack_next_int(ctx);
            int col_start = cw_unpack_next_int(ctx);
            int cells = cw_unpack_next_array(ctx);
            std::string output;
            while (cells-- > 0) {
                int cnum = cw_unpack_next_array(ctx);
                if (cnum-- > 0) {
                    auto text = cw_unpack_next_str(ctx);
                    output += text;
                    if (cnum >= 2) {
                        cnum -= 2;
                        cw_unpack_next_int(ctx); // hl
                        int repeat = cw_unpack_next_int(ctx);
                        while (--repeat > 0) {
                            output += text;
                        }
                    }
                }
                cw_skip_items(ctx, cnum);
            }
            cw_skip_items(ctx, cells);
            // TODO: Update grid lines
            NVLogI("Grid line %d row = %d, col_start = %d", grid, row, col_start);
            //NVLogD("Grid line: %s", output.c_str());
        }
        cw_skip_items(ctx, narg);
    }
    return count;
}

static inline int nv_redraw_action_grid_clear(NVClient *client, cw_unpack_context *ctx, int count) {
    if (count-- > 0) {
        int narg = cw_unpack_next_array(ctx);
        if (narg-- > 0) {
            int grid = cw_unpack_next_int(ctx);
            // TODO: Clear grid
            NVLogI("Grid clear %d", grid);
        }
        cw_skip_items(ctx, narg);
    }
    return count;
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
static inline uint64_t nv_rpc_call_begin(nv_pack_context_t *ctx, const char *method, int method_len, int narg) {
    uint64_t uuid = ctx->uuid++;
    cw_pack_array_size(&ctx->cw, 4);
    cw_pack_signed(&ctx->cw, 0);
    cw_pack_unsigned(&ctx->cw, uuid);
    cw_pack_str(&ctx->cw, method, method_len);
    cw_pack_array_size(&ctx->cw, narg);
    return uuid;
}

static inline void nv_pack_call_end(nv_pack_context_t *ctx) {
    cw_pack_flush(&ctx->cw);
}


@end
