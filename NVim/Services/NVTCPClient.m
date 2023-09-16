//
//  NVTCPClient.m
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import "NVTCPClient.h"
#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <netdb.h>
#import <pthread.h>
#import "cwpack.h"

#define NV_MSGPACK_BUFFER_MAX               8192
#define cw_pack_const_str(c, str)           cw_pack_str(c, str, sizeof(str) - 1);
#define nv_rpc_call_const_begin(c, m, n)    nv_rpc_call_begin(c, m, sizeof(m) - 1, n)

typedef struct nv_pack_context {
    cw_pack_context cw;
    int skt;
    int uuid;
    uint8_t data[NV_MSGPACK_BUFFER_MAX];
} nv_pack_context_t;

typedef struct nv_unpack_context {
    cw_unpack_context cw;
    int skt;
    uint8_t data[NV_MSGPACK_BUFFER_MAX];
} nv_unpack_context_t;

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
        err = pthread_create(&worker, NULL, start_routine, (__bridge void *)self);
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
    bzero(&ctx, sizeof(ctx));
    nv_unpack_context_init(&ctx, skt);
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
                    cw_unpack_next(&ctx.cw);
                    assert(ctx.cw.item.type == CWP_ITEM_POSITIVE_INTEGER && ctx.cw.item.as.i64 == 2);
                    cw_unpack_next(&ctx.cw);
                    assert(ctx.cw.item.type == CWP_ITEM_STR);
                    notify_routine(self, &ctx);
                    break;
                case 4:
                    cw_unpack_next(&ctx.cw);
                    assert(ctx.cw.item.type == CWP_ITEM_POSITIVE_INTEGER && ctx.cw.item.as.i64 == 1);
                    cw_unpack_next(&ctx.cw);
                    assert(ctx.cw.item.type == CWP_ITEM_POSITIVE_INTEGER);
                    int64_t msgid = ctx.cw.item.as.i64;
                    NVLogI("TCP Client recive response msgid: %d", msgid);
                    cw_unpack_next(&ctx.cw); // error
                    if (ctx.cw.item.type == CWP_ITEM_ARRAY) {
                        if (ctx.cw.item.as.array.size != 2) {
                            cw_skip_items(&ctx.cw, ctx.cw.item.as.array.size);
                        } else {
                            cw_unpack_next(&ctx.cw);
                            assert(ctx.cw.item.type == CWP_ITEM_POSITIVE_INTEGER);
                            int64_t code = ctx.cw.item.as.i64;
                            cw_unpack_next(&ctx.cw);
                            assert(ctx.cw.item.type == CWP_ITEM_STR);
                            NVLogE("TCP Client match request %d failed(%d): %s", msgid, code,
                                   [[NSString alloc] initWithBytes:ctx.cw.item.as.str.start length:ctx.cw.item.as.str.length encoding:NSUTF8StringEncoding].cstr);
                        }
                    }
                    cw_unpack_next(&ctx.cw); // result
                    if (ctx.cw.item.type == CWP_ITEM_MAP) {
                        cw_skip_items(&ctx.cw, ctx.cw.item.as.map.size);
                    }
                    break;
            }
        }
    }
}

#pragma mark - Work Helper
static inline void notify_routine(NVTCPClient *client, nv_unpack_context_t *ctx) {
    NSString *mehod = [[NSString alloc] initWithBytes:ctx->cw.item.as.str.start length:ctx->cw.item.as.str.length encoding:NSASCIIStringEncoding];
    NVLogI("TCP Client notify method: %s", mehod.cstr);
    cw_skip_items(&ctx->cw, 1);
}

#pragma mark - Helper
static inline void *start_routine(void *ptr) {
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

#pragma mark - MsgPack Pack
static inline void nv_pack_context_init(nv_pack_context_t *ctx, int skt) {
    bzero(ctx, sizeof(nv_pack_context_t));
    cw_pack_context_init(&ctx->cw, ctx->data, NV_MSGPACK_BUFFER_MAX, nv_pack_overflow_handler);
    cw_pack_set_flush_handler(&ctx->cw, nv_pack_flush_handler);
    ctx->skt = skt;
    ctx->uuid = 1;
}

static inline void nv_pack_context_final(nv_pack_context_t *ctx) {
    bzero(ctx, sizeof(nv_pack_context_t));
    ctx->skt = INVALID_SOCKET;
}

static inline int nv_pack_overflow_handler(cw_pack_context *ptr, size_t more) {
    int result = nv_pack_flush_handler(ptr);
    if (result == CWP_RC_OK) {
        if (more > NV_MSGPACK_BUFFER_MAX) {
            result = CWP_RC_BUFFER_OVERFLOW;
        } else {
            nv_pack_context_t *ctx = (nv_pack_context_t *)ptr;
            ctx->cw.end = ctx->cw.start + NV_MSGPACK_BUFFER_MAX;
            result = CWP_RC_OK;
        }
    }
    return result;
}

static inline int nv_pack_flush_handler(cw_pack_context *ptr) {
    nv_pack_context_t *ctx = (nv_pack_context_t *)ptr;
    int result = CWP_RC_ERROR_IN_HANDLER;
    if (ctx->skt != INVALID_SOCKET) {
        result = CWP_RC_OK;
        size_t contains = ctx->cw.current - ctx->cw.start;
        if (contains > 0) {
            size_t i = 0;
            while (contains > 0) {
                ssize_t n = write(ctx->skt, ctx->cw.start + i, contains);
                if (n <= 0) {
                    if (errno != EBADF) {
                        NVLogW("TCP Client write data failed: %s", strerror(errno));
                    }
                    ctx->skt = INVALID_SOCKET;
                    result = CWP_RC_END_OF_INPUT;
                    break;
                }
                i += n;
                contains -= n;
            }
            ctx->cw.current = ctx->cw.start;
        }
    }
    return result;
}

#pragma mark - MsgPack Unpack
static inline void nv_unpack_context_init(nv_unpack_context_t *ctx, int skt) {
    cw_unpack_context_init(&ctx->cw, ctx->data, 0, nv_unpack_underflow_handler);
    ctx->skt = skt;
}

static inline bool nv_unpack_context_continue(nv_unpack_context_t *ctx) {
    return ctx->skt != INVALID_SOCKET;
}

static inline int nv_unpack_underflow_handler(cw_unpack_context *ptr, size_t more) {
    nv_unpack_context_t *ctx = (nv_unpack_context_t *)ptr;
    int result = CWP_RC_ERROR_IN_HANDLER;
    if (ctx->skt != INVALID_SOCKET) {
        size_t used = ctx->cw.current - ctx->cw.start;
        size_t remains = ctx->cw.end - ctx->cw.current;
        if (used + remains + more > NV_MSGPACK_BUFFER_MAX) {
            if (remains > 0) {
                memmove(ctx->cw.start, ctx->cw.current, remains);
            }
            ctx->cw.current = ctx->cw.start;
            ctx->cw.end = ctx->cw.start + remains;
        }
        if (remains + more > NV_MSGPACK_BUFFER_MAX) {
            result = CWP_RC_BUFFER_UNDERFLOW;
        } else {
            result = CWP_RC_OK;
            while (more > 0) {
                ssize_t n = read(ctx->skt, ctx->cw.end, more);
                if (n <= 0) {
                    if (errno != EBADF) {
                        NVLogW("TCP Client read data failed: %s", strerror(errno));
                    }
                    ctx->skt = INVALID_SOCKET;
                    result = CWP_RC_END_OF_INPUT;
                    break;
                }
                ctx->cw.end += n;
                more -= n;
            }
        }
    }
    return result;
}

#pragma mark - API Helper
static inline void nv_rpc_call_begin(nv_pack_context_t *ctx, const char *method, int method_len, int narg) {
    cw_pack_array_size(&ctx->cw, 4);
    cw_pack_signed(&ctx->cw, 0);
    cw_pack_unsigned(&ctx->cw, ctx->uuid++);
    cw_pack_str(&ctx->cw, method, method_len);
    cw_pack_array_size(&ctx->cw, narg);
}

static inline void nv_pack_call_end(nv_pack_context_t *ctx) {
    nv_pack_flush_handler(&ctx->cw);
}

@end
