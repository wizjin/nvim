//
//  NVClient.m
//  NVim
//
//  Created by wizjin on 2023/9/13.
//

#import "NVClient.h"
#import "nvc_ui.h"

@interface NVClient () {
    nvc_ui_context_t *ui_ctx;
}

@end

@implementation NVClient

- (instancetype)init {
    if (self = [super init]) {
        ui_ctx = NULL;
    }
    return self;
}

- (void)openWithRead:(int)read write:(int)write {
    ui_ctx = nvc_ui_create(read, write, &nvclient_ui_callbacks, (__bridge void *)self);
}

- (void)close {
    if (ui_ctx != NULL) {
        NVLogD("TCP Client close connect success - %s", self.info.cstr);
        nvc_ui_destory(ui_ctx);
        ui_ctx = NULL;
    }
}

- (void)attachUI {
    nvc_ui_attach(ui_ctx);
}

- (void)detachUI {
    nvc_ui_detach(ui_ctx);
}

static inline void nvclient_ui_flush(void *userdata) {
    NVClient *client = (__bridge NVClient *)userdata;
    dispatch_main_async(^{
        [client.delegate clientFlush:client];
    });
}

static inline void nvclient_ui_update_title(void *userdata, const char *str, uint32_t len) {
    NVClient *client = (__bridge NVClient *)userdata;
    NSString *title = [[NSString alloc] initWithBytes:str length:len encoding:NSUTF8StringEncoding];
    dispatch_main_async(^{
        [client.delegate client:client updateTitle:title];
    });
}

static inline void nvclient_ui_update_background(void *userdata, uint32_t rgb) {
    NVClient *client = (__bridge NVClient *)userdata;
    NSColor *color = [NSColor colorWithRGB:rgb];
    dispatch_main_async(^{
        [client.delegate client:client updateBackground:color];
    });
}

#define NVCLIENT_CALLBACK(_func)    ._func = nvclient_ui_##_func
static const nvc_ui_callback_t nvclient_ui_callbacks = {
    NVCLIENT_CALLBACK(flush),
    NVCLIENT_CALLBACK(update_title),
    NVCLIENT_CALLBACK(update_background),
};


@end
