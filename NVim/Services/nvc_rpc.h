//
//  nvc_rpc.h
//  NVim
//
//  Created by wizjin on 2023/9/18.
//

#ifndef __NVC_RPC_H__
#define __NVC_RPC_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cwpack.h"

typedef struct nvc_rpc_context nvc_rpc_context_t;
typedef int (*nvc_rpc_handler)(nvc_rpc_context_t *ctx, int items);

struct nvc_rpc_context {
    uint64_t            uuid;
    pthread_t           worker;
    void                *userdata;
    nvc_rpc_handler     response_handler;
    nvc_rpc_handler     notification_handler;
    
    cw_unpack_context   cin;
    int                 inskt;
    size_t              inlen;
    uint8_t             *indata;

    cw_pack_context     cout;
    int                 outskt;
    size_t              outlen;
    uint8_t             *outdata;
};

NVC_API int nvc_rpc_init(nvc_rpc_context_t *ctx, int inskt, int outskt, void *userdata, nvc_rpc_handler response_handler, nvc_rpc_handler notification_handler);
NVC_API void nvc_rpc_final(nvc_rpc_context_t *ctx);
NVC_API bool nvc_rpc_read_bool(nvc_rpc_context_t *ctx);
NVC_API int64_t nvc_rpc_read_int64(nvc_rpc_context_t *ctx);
NVC_API const char *nvc_rpc_read_str(nvc_rpc_context_t *ctx, uint32_t *len);
NVC_API int nvc_rpc_read_array_size(nvc_rpc_context_t *ctx);
NVC_API int nvc_rpc_read_map_size(nvc_rpc_context_t *ctx);
NVC_API uint64_t nvc_rpc_call_begin(nvc_rpc_context_t *ctx, const char *method, int method_len, int narg);
NVC_API void nvc_rpc_call_end(nvc_rpc_context_t *ctx);

#define nvc_rpc_get_userdata(_ctx)          ((_ctx)->userdata)
#define nvc_rpc_get_next_uuid(_ctx)         (++(_ctx)->uuid)
#define nvc_rpc_read_ahead(_ctx)            cw_look_ahead(&(_ctx)->cin)

#define nvc_rpc_read_int(_ctx)              ((int)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_uint(_ctx)             ((uint)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_int32(_ctx)            ((int32_t)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_uint32(_ctx)           ((uint32_t)nvc_rpc_read_int64(_ctx))
#define nvc_rpc_read_uint64(_ctx)           ((uint64_t)nvc_rpc_read_int64(_ctx))

#define nvc_rpc_read_skip_items(_ctx, _n)   cw_skip_items(&(_ctx)->cin, _n)
#define nvc_rpc_write_signed(_ctx, _v)      cw_pack_signed(&(_ctx)->cout, _v)
#define nvc_rpc_write_unsigned(_ctx, _v)    cw_pack_signed(&(_ctx)->cout, _v)
#define nvc_rpc_write_array_size(_ctx, _n)  cw_pack_array_size(&(_ctx)->cout, _n)
#define nvc_rpc_write_map_size(_ctx, _n)    cw_pack_map_size(&(_ctx)->cout, _n)
#define nvc_rpc_write_str(_ctx, _v, _l)     cw_pack_str(&(_ctx)->cout, _v, _l)
#define nvc_rpc_write_true(_ctx)            cw_pack_true(&(_ctx)->cout)
#define nvc_rpc_write_flush(_ctx)           cw_pack_flush(&(_ctx)->cout)

#define nvc_rpc_write_const_str(c, str)     nvc_rpc_write_str(c, str, sizeof(str) - 1)
#define nvc_rpc_call_const_begin(c, m, n)   nvc_rpc_call_begin(c, m, sizeof(m) - 1, n)

#define NVC_RPC_ITEM_NIL                    CWP_ITEM_NIL
#define NVC_RPC_ITEM_BOOLEAN                CWP_ITEM_BOOLEAN
#define NVC_RPC_ITEM_POSITIVE_INTEGER       CWP_ITEM_POSITIVE_INTEGER
#define NVC_RPC_ITEM_NEGATIVE_INTEGER       CWP_ITEM_NEGATIVE_INTEGER
#define NVC_RPC_ITEM_FLOAT                  CWP_ITEM_FLOAT
#define NVC_RPC_ITEM_DOUBLE                 CWP_ITEM_DOUBLE
#define NVC_RPC_ITEM_STR                    CWP_ITEM_STR
#define NVC_RPC_ITEM_BIN                    CWP_ITEM_BIN
#define NVC_RPC_ITEM_ARRAY                  CWP_ITEM_ARRAY
#define NVC_RPC_ITEM_MAP                    CWP_ITEM_MAP
#define NVC_RPC_ITEM_EXT                    CWP_ITEM_EXT
#define NVC_RPC_NOT_AN_ITEM                 CWP_NOT_AN_ITEM

#define NVC_RPC_RC_OK                       CWP_RC_OK

#ifdef __cplusplus
}
#endif

#endif /* __NVC_RPC_H__ */
