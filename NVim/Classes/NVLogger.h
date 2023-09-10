//
//  NVLogger.h
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#ifndef __NVLOGGER_H__
#define __NVLOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#if DEBUG
    void NVLogOutput(char lvl, const char *format, ...);
#else
#   define NVLogOutput(...)  ((void *)0)
#endif

#define NVLogE(...)             NVLogOutput('E', ##__VA_ARGS__)
#define NVLogI(...)             NVLogOutput('I', ##__VA_ARGS__)
#define NVLogW(...)             NVLogOutput('W', ##__VA_ARGS__)
#define NVLogD(...)             NVLogOutput('D', ##__VA_ARGS__)
#define NVLogT(...)             NVLogOutput('T', ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif /* __NVLOGGER_H__ */
