//
//  NVUtils.h
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#ifndef __NVUTILS_H__
#define __NVUTILS_H__

#if defined(__cplusplus)
#define NVC_API                 extern "C"
#else
#define NVC_API                 extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_SOCKET          -1
#define countof(array)          (sizeof(array) / sizeof(array[0]))

#ifdef __OBJC__

#pragma mark - Weakify & Strongify Macros
#if __has_feature(objc_arc)

#if DEBUG
#   define ext_keywordify       autoreleasepool {}
#else
#   define ext_keywordify       try {} @catch (...) {}
#endif

#define weakify(_x)                                     \
    ext_keywordify                                      \
    _Pragma("clang diagnostic push")                    \
    _Pragma("clang diagnostic ignored \"-Wshadow\"")    \
    __weak __typeof__(_x) __weak_##_x##__ = _x;         \
    _Pragma("clang diagnostic pop")

#define strongify(_x)                                   \
    ext_keywordify                                      \
    _Pragma("clang diagnostic push")                    \
    _Pragma("clang diagnostic ignored \"-Wshadow\"")    \
    __strong __typeof__(_x) _x = __weak_##_x##__;       \
    _Pragma("clang diagnostic pop")

#endif /* objc_arc */

#pragma mark - Dispatch Helper
#include <dispatch/dispatch.h>

dispatch_queue_t dispatch_queue_create_for(id obj, dispatch_queue_attr_t attr);

static inline void dispatch_main_async(dispatch_block_t block) {
    dispatch_async(dispatch_get_main_queue(), block);
}

static inline void dispatch_main_after(double delta, dispatch_block_t block) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delta * NSEC_PER_SEC)), dispatch_get_main_queue(), block);
}

#endif /* __OBJC__ */

#if !defined(MIN)
#   define MIN(A,B)             ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#endif

#if !defined(MAX)
#   define MAX(A,B)             ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })
#endif

#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)

#define nv_member_to_struct(_struct, _ptr, _member) (_struct *)((intptr_t)(_ptr) - __offsetof(_struct, _member))

#ifdef __cplusplus
}
#endif


#endif /* __NVUTILS_H__ */
