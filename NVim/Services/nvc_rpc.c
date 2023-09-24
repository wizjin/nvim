//
//  nvc_rpc.c
//  NVim
//
//  Created by wizjin on 2023/9/18.
//

#include "nvc_rpc.h"
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define NVC_RPC_KBYTES_BITS                 10 // 1 << 10 == 1KB
#define NVC_RPC_BUFFER_INIT                 (4   << NVC_RPC_KBYTES_BITS)
#define NVC_RPC_BUFFER_PREREAD              (16  << NVC_RPC_KBYTES_BITS)
#define NVC_RPC_BUFFER_THRESHOLD            (256 << NVC_RPC_KBYTES_BITS)

#define NVC_RPC_TYPE_RESPONSE               1
#define NVC_RPC_TYPE_NOTIFICATION           2

#define nvc_rpc_to_context(_ptr, _member)   nv_member_to_struct(nvc_rpc_context_t, _ptr, _member)

static inline size_t nvc_msgpack_memory_best_size(size_t kept, size_t more) {
    size_t datalen = kept;
    size_t target = kept + more;
    while (datalen < target) {
        datalen *= 2;
    }
    if (datalen > NVC_RPC_BUFFER_THRESHOLD) {
        datalen = kept + ((((more)>>NVC_RPC_KBYTES_BITS) + !!((more)&((1<<NVC_RPC_KBYTES_BITS)-1)))<<NVC_RPC_KBYTES_BITS);
    }
    return datalen;
}

static inline int nvc_unpack_underflow_handler(cw_unpack_context *ptr, size_t more) {
    int res = NVC_RC_ERROR_IN_HANDLER;
    nvc_rpc_context_t *ctx = nvc_rpc_to_context(ptr, cin);
    if (ctx->cin.return_code == CWP_RC_OK) {
        res = NVC_RC_OK;
        size_t used = ctx->cin.current - ctx->cin.start;
        size_t remains = ctx->cin.end - ctx->cin.current;

        size_t ready = 0;
        if (ioctl(ctx->inskt, FIONREAD, &ready) != -1 && ready > more) {
            more = MAX(more, MIN(MIN(ready, NVC_RPC_BUFFER_PREREAD), NVC_RPC_BUFFER_THRESHOLD - remains));
        }

        size_t target = remains + more;
        if (used > 0 && used + target > ctx->inlen) {
            if (remains > 0) {
                memmove(ctx->cin.start, ctx->cin.current, remains);
            }
            if (ctx->inlen > NVC_RPC_BUFFER_THRESHOLD && target < NVC_RPC_BUFFER_THRESHOLD) {
                void *new_indata = realloc(ctx->indata, NVC_RPC_BUFFER_THRESHOLD);
                if (likely(new_indata != NULL)) {
                    ctx->inlen = NVC_RPC_BUFFER_THRESHOLD;
                    ctx->indata = (uint8_t *)new_indata;
                    ctx->cin.start = ctx->indata;
                }
            }
            ctx->cin.current = ctx->cin.start;
            ctx->cin.end = ctx->cin.current + remains;
            used = 0;
        }
        if (target > ctx->inlen) {
            size_t new_inlen = nvc_msgpack_memory_best_size(ctx->inlen, target);
            void *new_indata = realloc(ctx->indata, new_inlen);
            if (unlikely(new_indata == NULL)) {
                res = NVC_RC_BUFFER_UNDERFLOW;
            } else {
                ctx->inlen = new_inlen;
                ctx->indata = (uint8_t *)new_indata;
                ctx->cin.start = ctx->indata;
                ctx->cin.current = ctx->cin.start + used;
                ctx->cin.end = ctx->cin.current + remains;
            }
        }
        if (likely(res == NVC_RC_OK)) {
            while (more > 0) {
                ssize_t n = read(ctx->inskt, ctx->cin.end, more);
                if (n > 0) {
                    ctx->cin.end += n;
                    more -= n;
                    continue;
                }
                if (n == 0) {
                    res = NVC_RC_END_OF_INPUT;
                } else if (n < 0) {
                    if (errno != EBADF) {
                        NVLogW("nvc rpc read data failed: %s", strerror(errno));
                    }
                    ctx->cin.err_no = errno;
                    res = NVC_RC_ERROR_IN_HANDLER;
                }
                break;
            }
        }
    }
    return res;
}

static inline int nvc_pack_flush_handler(cw_pack_context *ptr) {
    int res = NVC_RC_ERROR_IN_HANDLER;
    nvc_rpc_context_t *ctx = nvc_rpc_to_context(ptr, cout);
    if (ctx->cout.return_code == NVC_RC_OK) {
        res = NVC_RC_OK;
        size_t contains = ctx->cout.current - ctx->cout.start;
        if (contains > 0) {
            size_t i = 0;
            while (contains > 0) {
                ssize_t n = write(ctx->outskt, ctx->cout.start + i, contains);
                if (n > 0) {
                    i += n;
                    contains -= n;
                    continue;
                }
                if (n == 0) {
                    res = NVC_RC_END_OF_INPUT;
                } else {
                    if (errno != EBADF) {
                        NVLogW("nvc rpc write data failed: %s", strerror(errno));
                    }
                    ctx->cout.err_no = errno;
                    res = NVC_RC_ERROR_IN_HANDLER;
                }
                break;
            }
            if (contains > 0) {
                memmove(ctx->cout.start, ctx->cout.start + i, contains);
            }
            ctx->cout.current = ctx->cout.start + contains;
            ctx->cout.end = ctx->cout.start + ctx->outlen;
        }
    }
    return res;
}

static inline int nvc_pack_overflow_handler(cw_pack_context *ptr, size_t more) {
    int res = nvc_pack_flush_handler(ptr);
    if (res == NVC_RC_OK) {
        nvc_rpc_context_t *ctx = nvc_rpc_to_context(ptr, cout);
        size_t kept = ctx->cout.current - ctx->cout.start;
        size_t target = kept + more;
        if (ctx->outlen > NVC_RPC_BUFFER_THRESHOLD && target < NVC_RPC_BUFFER_THRESHOLD) {
            void *new_outdata = realloc(ctx->outdata, NVC_RPC_BUFFER_THRESHOLD);
            if (likely(new_outdata != NULL)) {
                ctx->outlen = NVC_RPC_BUFFER_THRESHOLD;
                ctx->outdata = (uint8_t *)new_outdata;
                ctx->cout.start = ctx->outdata;
                ctx->cout.current = ctx->cout.start + kept;
                ctx->cout.end = ctx->cout.start + ctx->outlen;
            }
        }
        if (target > ctx->outlen) {
            size_t new_datalen = nvc_msgpack_memory_best_size(ctx->outlen, more);
            void *new_outdata = realloc(ctx->outdata, new_datalen);
            if (unlikely(new_outdata == NULL)) {
                res = NVC_RC_BUFFER_OVERFLOW;
            } else {
                ctx->outlen = new_datalen;
                ctx->outdata = (uint8_t *)new_outdata;
                ctx->cout.start = ctx->outdata;
                ctx->cout.current = ctx->cout.start + kept;
                ctx->cout.end = ctx->cout.start + ctx->outlen;
            }
        }
    }
    return res;
}

static void *nvc_rpc_routine(void *ptr) {
    nvc_rpc_context_t *ctx = (nvc_rpc_context_t *)ptr;
    while (likely(ctx->cin.return_code == NVC_RC_OK)) {
        int items = nvc_rpc_read_array_size(ctx);
        if (likely(items-- > 0)) {
            int rpc_type = nvc_rpc_read_int(ctx);
            switch (rpc_type) {
                case NVC_RPC_TYPE_RESPONSE:
                    items = ctx->response_handler(ctx, items);
                    break;
                case NVC_RPC_TYPE_NOTIFICATION:
                    items = ctx->notification_handler(ctx, items);
                    break;
                default:
                    NVLogW("nvc rpc recv unknown type: %d", rpc_type);
                    break;
            }
            nvc_rpc_read_skip_items(ctx, items);
        }
    }
    return NULL;
}

int nvc_rpc_init(nvc_rpc_context_t *ctx, int inskt, int outskt, void *userdata, nvc_rpc_handler response_handler, nvc_rpc_handler notification_handler) {
    int res = NVC_RC_ILLEGAL_CALL;
    if (ctx != NULL &&inskt != INVALID_SOCKET && outskt != INVALID_SOCKET && response_handler != NULL && notification_handler != NULL) {
        bzero(ctx, sizeof(nvc_rpc_context_t));
        ctx->uuid = 0;
        ctx->userdata = userdata;
        ctx->response_handler = response_handler;
        ctx->notification_handler = notification_handler;
        ctx->inskt = dup(inskt);
        ctx->outskt = dup(outskt);
        ctx->inlen = NVC_RPC_BUFFER_INIT;
        ctx->outlen = NVC_RPC_BUFFER_INIT;
        ctx->indata = (uint8_t *)malloc(ctx->inlen);
        ctx->outdata = (uint8_t *)malloc(ctx->outlen);
        res = NVC_RC_OK;
        if (unlikely(ctx->indata == NULL || ctx->outdata == NULL)) {
            res = NVC_RC_MALLOC_ERROR;
        }
        if (likely(res == CWP_RC_OK)) {
            res = cw_unpack_context_init(&ctx->cin, ctx->indata, 0, nvc_unpack_underflow_handler);
        }
        if (likely(res == CWP_RC_OK)) {
            res = cw_pack_context_init(&ctx->cout, ctx->outdata, ctx->outlen, nvc_pack_overflow_handler);
            cw_pack_set_flush_handler(&ctx->cout, nvc_pack_flush_handler);
        }
        if (likely(res == CWP_RC_OK)) {
            res = pthread_create(&ctx->worker, NULL, nvc_rpc_routine, ctx);
            if (unlikely(res != 0)) {
                NVLogE("nvc rpc create worker thread failed: %s", strerror(res));
                ctx->worker = NULL;
            }
        }
        if (res != NVC_RC_OK) {
            nvc_rpc_final(ctx);
        }
    }
    return res;
}

void nvc_rpc_final(nvc_rpc_context_t *ctx) {
    if (ctx != NULL) {
        if (ctx->inskt != INVALID_SOCKET) {
            close(ctx->inskt);
            ctx->inskt = INVALID_SOCKET;
        }
        if (ctx->outskt != INVALID_SOCKET) {
            close(ctx->outskt);
            ctx->outskt = INVALID_SOCKET;
        }
        if (ctx->worker != NULL) {
            pthread_join(ctx->worker, NULL);
            ctx->worker = NULL;
        }
        if (ctx->indata != NULL) {
            free(ctx->indata);
            ctx->indata = NULL;
            ctx->inlen = 0;
        }
        if (ctx->outdata != NULL) {
            free(ctx->outdata);
            ctx->outdata = NULL;
            ctx->outlen = 0;
        }
    }
}

bool nvc_rpc_read_bool(nvc_rpc_context_t *ctx) {
    bool res = false;
    if (unlikely(cw_look_ahead(&ctx->cin) != CWP_ITEM_BOOLEAN)) {
        cw_skip_items(&ctx->cin, 1);
    } else {
        cw_unpack_next(&ctx->cin);
        res = ctx->cin.item.as.boolean;
    }
    return res;
}

int64_t nvc_rpc_read_int64(nvc_rpc_context_t *ctx) {
    int64_t res = 0;
    cwpack_item_types type = cw_look_ahead(&ctx->cin);
    if (unlikely(type != CWP_ITEM_POSITIVE_INTEGER && type != CWP_ITEM_NEGATIVE_INTEGER)) {
        cw_skip_items(&ctx->cin, 1);
    } else {
        cw_unpack_next(&ctx->cin);
        res = ctx->cin.item.as.i64;
    }
    return res;
}

const char *nvc_rpc_read_str(nvc_rpc_context_t *ctx, uint32_t *len) {
    const char *res = NULL;
    uint32_t nlen = 0;
    if (unlikely(cw_look_ahead(&ctx->cin) != CWP_ITEM_STR)) {
        cw_skip_items(&ctx->cin, 1);
    } else {
        cw_unpack_next(&ctx->cin);
        nlen = ctx->cin.item.as.str.length;
        if (nlen > 0) {
            res = ctx->cin.item.as.str.start;
        }
    }
    if (likely(len != NULL)) {
        *len = nlen;
    }
    return res;
}

int nvc_rpc_read_array_size(nvc_rpc_context_t *ctx) {
    int items = 0;
    if (unlikely(cw_look_ahead(&ctx->cin) != CWP_ITEM_ARRAY)) {
        cw_skip_items(&ctx->cin, 1);
    } else {
        cw_unpack_next(&ctx->cin);
        items = ctx->cin.item.as.array.size;
    }
    return items;
}

int nvc_rpc_read_map_size(nvc_rpc_context_t *ctx) {
    int items = 0;
    if (unlikely(cw_look_ahead(&ctx->cin) != CWP_ITEM_MAP)) {
        cw_skip_items(&ctx->cin, 1);
    } else {
        cw_unpack_next(&ctx->cin);
        items = ctx->cin.item.as.map.size;
    }
    return items;
}

uint64_t nvc_rpc_call_begin(nvc_rpc_context_t *ctx, const char *method, int method_len, int narg) {
    uint64_t uuid = nvc_rpc_get_next_uuid(ctx);
    nvc_rpc_write_array_size(ctx, 4);
    nvc_rpc_write_signed(ctx, 0);
    nvc_rpc_write_unsigned(ctx, uuid);
    nvc_rpc_write_str(ctx, method, method_len);
    nvc_rpc_write_array_size(ctx, narg);
    return uuid;
}

void nvc_rpc_call_end(nvc_rpc_context_t *ctx) {
    nvc_rpc_write_flush(ctx);
}
