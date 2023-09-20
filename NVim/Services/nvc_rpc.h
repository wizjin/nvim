//
//  nvc_rpc.h
//  NVim
//
//  Created by wizjin on 2023/9/18.
//

#ifndef __NVC_RPC_H__
#define nvc_rpc_h

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nvc_rpc_context nvc_rpc_context_t;
typedef int (*nvc_rpc_handler)(nvc_rpc_context_t *ctx, int items);

NVC_API nvc_rpc_context_t *nvc_rpc_create(int inskt, int outskt, void *userdata, nvc_rpc_handler response_handler, nvc_rpc_handler notification_handler);
NVC_API void nvc_rpc_destory(nvc_rpc_context_t **pctx);
NVC_API void *nvc_rpc_get_userdata(nvc_rpc_context_t *ctx);
NVC_API uint64_t nvc_rpc_get_next_uuid(nvc_rpc_context_t *ctx);
NVC_API int nvc_rpc_read_ahead(nvc_rpc_context_t *ctx);
NVC_API bool nvc_rpc_read_bool(nvc_rpc_context_t *ctx);
NVC_API int64_t nvc_rpc_read_int64(nvc_rpc_context_t *ctx);
NVC_API const char *nvc_rpc_read_str(nvc_rpc_context_t *ctx, uint32_t *len);
NVC_API int nvc_rpc_read_array_size(nvc_rpc_context_t *ctx);
NVC_API int nvc_rpc_read_map_size(nvc_rpc_context_t *ctx);
NVC_API void nvc_rpc_read_skip_items(nvc_rpc_context_t *ctx, int items);
NVC_API void nvc_rpc_write_signed(nvc_rpc_context_t *ctx, int64_t value);
NVC_API void nvc_rpc_write_unsigned(nvc_rpc_context_t *ctx, int64_t value);
NVC_API void nvc_rpc_write_array_size(nvc_rpc_context_t *ctx, uint32_t n);
NVC_API void nvc_rpc_write_map_size(nvc_rpc_context_t *ctx, uint32_t n);
NVC_API void nvc_rpc_write_str(nvc_rpc_context_t *ctx, const char *v, uint32_t l);
NVC_API void nvc_rpc_write_true(nvc_rpc_context_t *ctx);
NVC_API void nvc_rpc_write_flush(nvc_rpc_context_t *ctx);

#define nvc_rpc_read_int(_ctx)          ((int)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_uint(_ctx)         ((uint)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_int32(_ctx)        ((int32_t)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_uint32(_ctx)       ((uint32_t)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_uint64(_ctx)       ((uint64_t)nvc_rpc_read_int64(_ctx))

#define NVC_RPC_ITEM_NIL                300
#define NVC_RPC_ITEM_BOOLEAN            301
#define NVC_RPC_ITEM_POSITIVE_INTEGER   302
#define NVC_RPC_ITEM_NEGATIVE_INTEGER   303
#define NVC_RPC_ITEM_FLOAT              304
#define NVC_RPC_ITEM_DOUBLE             305
#define NVC_RPC_ITEM_STR                306
#define NVC_RPC_ITEM_BIN                307
#define NVC_RPC_ITEM_ARRAY              308
#define NVC_RPC_ITEM_MAP                309
#define NVC_RPC_ITEM_EXT                310
#define NVC_RPC_NOT_AN_ITEM             999

#ifdef __cplusplus
}
#endif

#endif /* __NVC_RPC_H__ */
