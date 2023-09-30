//
//  NVClient.m
//  NVim
//
//  Created by wizjin on 2023/9/13.
//

#import "NVClient.h"
#import "nvc_ui.h"

@interface NVClient () {
@private
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

- (BOOL)isAttached {
    return nvc_ui_is_attached(ui_ctx);;
}

- (void)openWithRead:(int)read write:(int)write {
    nvc_ui_config_t config;
    bzero(&config, sizeof(config));
    config.family_name = kNVDefaultFamilyName;
    config.font_size = kNVDefaultFontSize;
    ui_ctx = nvc_ui_create(read, write, &config, &nvclient_ui_callbacks, (__bridge void *)self);
    if (ui_ctx != nil) {
        NVLogI("NV Client open connect success - %s", self.info.cstr);
    }
}

- (void)close {
    if (ui_ctx != NULL) {
        NVLogI("NV Client close connect success - %s", self.info.cstr);
        nvc_ui_destroy(ui_ctx);
        ui_ctx = NULL;
    }
}

- (CGSize)attachUIWithSize:(CGSize)size {
    return nvc_ui_attach(ui_ctx, size);
}

- (void)detachUI {
    nvc_ui_detach(ui_ctx);
}

- (void)redrawUI:(CGContextRef)ctx dirty:(CGRect)dirty {
    nvc_ui_redraw(ui_ctx, ctx, dirty);
}

- (CGSize)resizeUIWithSize:(CGSize)size {
    return nvc_ui_resize(ui_ctx, size);
}

- (void)keyDown:(NSEvent *)event {
    NSEventModifierFlags flags = event.modifierFlags;
    nvc_ui_key_info_t key = {
        .code       = event.keyCode,
        .shift      = flags & NSEventModifierFlagShift,
        .control    = flags & NSEventModifierFlagControl,
        .option     = flags & NSEventModifierFlagOption,
        .command    = flags & NSEventModifierFlagCommand,
    };
    if (!nvc_ui_input_key(ui_ctx, key)) {
        NSString *c = event.characters;
        if (c.length > 0) {
            NSData *keys = [c dataUsingEncoding:NSUTF8StringEncoding];
            nvc_ui_input_keystr(ui_ctx, (const char *)keys.bytes, (uint32_t)keys.length);
        }
    }
}

static inline void nvclient_ui_flush(void *userdata, CGRect dirty) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client flush:dirty];
    });
}

static inline void nvclient_ui_update_title(void *userdata, const char *str, uint32_t len) {
    NVClient *client = (__bridge NVClient *)userdata;
    NSString *title = [[NSString alloc] initWithBytes:str length:len encoding:NSUTF8StringEncoding];
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client updateTitle:title];
    });
}

static inline void nvclient_ui_update_background(void *userdata, nvc_ui_color_t rgb) {
    NVClient *client = (__bridge NVClient *)userdata;
    NSColor *color = [NSColor colorWithRGB:rgb];
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client updateBackground:color];
    });
}

static inline void nvclient_ui_update_tab_background(void *userdata, nvc_ui_color_t rgb) {
    NVClient *client = (__bridge NVClient *)userdata;
    NSColor *color = [NSColor colorWithRGB:rgb];
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client updateTabBackground:color];
    });
}

static inline void nvclient_ui_update_tab_list(void *userdata, bool list_updated) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client updateTabList:list_updated];
    });
}

static inline void nvclient_ui_mouse_on(void *userdata) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client updateMouse:YES];
    });
}

static inline void nvclient_ui_mouse_off(void *userdata) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client updateMouse:NO];
    });
}

static inline void nvclient_ui_close(void *userdata) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate clientClosed:client];
    });
}

#define NVCLIENT_CALLBACK(_func)    ._func = nvclient_ui_##_func
static const nvc_ui_callback_t nvclient_ui_callbacks = {
    NVCLIENT_CALLBACK(flush),
    NVCLIENT_CALLBACK(update_title),
    NVCLIENT_CALLBACK(update_background),
    NVCLIENT_CALLBACK(update_tab_background),
    NVCLIENT_CALLBACK(update_tab_list),
    NVCLIENT_CALLBACK(mouse_on),
    NVCLIENT_CALLBACK(mouse_off),
    NVCLIENT_CALLBACK(close),
};


@end
