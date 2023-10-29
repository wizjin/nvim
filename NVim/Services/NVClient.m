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

@property (nonatomic, readonly, assign) NSInteger pasteboardChangeCount;
@property (nonatomic, readonly, assign) BOOL pasteboardNeedUpdate;

@end

@implementation NVClient

- (instancetype)init {
    if (self = [super init]) {
        ui_ctx = NULL;
        _autoHideMouse = YES;
        _pasteboardChangeCount = 0;
        _pasteboardNeedUpdate = NO;
    }
    return self;
}

- (BOOL)isAttached {
    return nvc_ui_is_attached(ui_ctx);;
}

- (void)openWithRead:(int)read write:(int)write {
    nvc_ui_config_t config = {
        .font_size = kNVDefaultFontSize,
        .scale_factor = NSScreen.mainScreen.backingScaleFactor,
    };
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

- (void)active {
    if (nvc_ui_is_attached(ui_ctx)) {
        NSPasteboard *pasteboard = NSPasteboard.generalPasteboard;
        if (self.pasteboardChangeCount != pasteboard.changeCount) {
            _pasteboardChangeCount = pasteboard.changeCount;
            NSString *str = [pasteboard stringForType:NSPasteboardTypeString];
            if (str.length > 0) {
                NSData *data = [str dataUsingEncoding:NSUTF8StringEncoding];
                nvc_ui_set_pasteboard(ui_ctx, (const char *)data.bytes, (uint32_t)data.length);
            }
        }
    }
}

- (void)deactive {
    if (nvc_ui_is_attached(ui_ctx) && self.pasteboardNeedUpdate) {
        _pasteboardNeedUpdate = NO;
        nvc_ui_update_pasteboard(ui_ctx);
    }
}

- (CGFloat)lineHeight {
    return nvc_ui_get_line_height(ui_ctx);
}

- (CGPoint)cursorPosition {
    return nvc_ui_get_cursor_position(ui_ctx);
}

- (CGSize)attachUIWithSize:(CGSize)size {
    CGSize sz = nvc_ui_attach(ui_ctx, size);
    [self active];
    return sz;
}

- (void)detachUI {
    nvc_ui_detach(ui_ctx);
}

- (void)redrawLayer:(NSInteger)grid context:(CGContextRef)context{
    nvc_ui_redraw(ui_ctx, (int)grid, context);
}

- (CGSize)resizeUIWithSize:(CGSize)size {
    return nvc_ui_resize(ui_ctx, size);
}

- (BOOL)openFiles:(NSArray<NSString *> *)files {
    BOOL res = NO;
    int n = 0;
    for (NSString *file in files) {
        nvc_ui_open_file(ui_ctx, file.cstr, (uint32_t)file.length, res);
        n++;
        res = YES;
    }
    if (n > 1) {
        nvc_ui_tab_next(ui_ctx, 1 - n);
    }
    return res;
}

- (BOOL)keyDown:(NSEvent *)event {
    nvc_ui_key_flags_t flags = nvc_ui_key_flags_init(event.modifierFlags);
    if (!nvc_ui_input_key(ui_ctx, event.keyCode, flags)) {
        NSString *c = event.characters;
        if (c.length > 0) {
            NSData *keys = [c dataUsingEncoding:NSUTF8StringEncoding];
            nvc_ui_input_keystr(ui_ctx, flags, (const char *)keys.bytes, (uint32_t)keys.length);
        }
    }
    return YES;
}

- (BOOL)functionKeyDown:(NSEvent *)event {
    NSString *c = event.characters;
    return (c.length == 1 && nvc_ui_input_function_key(ui_ctx, [c characterAtIndex:0], nvc_ui_key_flags_init(event.modifierFlags)));
}

- (void)inputText:(NSString *)text flags:(NSEventModifierFlags)flags {
    NSData *data = [text dataUsingEncoding:NSUTF8StringEncoding];
    nvc_ui_input_keystr(ui_ctx, nvc_ui_key_flags_init(flags), (const char *)data.bytes, (uint32_t)data.length);
}

- (void)scrollWheel:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_wheel, (event.deltaY > 0 ? nvc_ui_mouse_action_up : nvc_ui_mouse_action_down));
}

- (void)mouseUp:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_left, nvc_ui_mouse_action_release);
}

- (void)mouseDown:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_left, nvc_ui_mouse_action_press);
}

- (void)mouseMoved:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_move, nvc_ui_mouse_action_none);
}

- (void)mouseDragged:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_left, nvc_ui_mouse_action_drag);
}

- (void)rightMouseUp:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_right, nvc_ui_mouse_action_release);
}

- (void)rightMouseDown:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_right, nvc_ui_mouse_action_press);
}

- (void)rightMouseDragged:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_right, nvc_ui_mouse_action_drag);
}

- (void)middleMouseUp:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_middle, nvc_ui_mouse_action_release);
}

- (void)middleMouseDown:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_middle, nvc_ui_mouse_action_press);
}

- (void)middleMouseDragged:(NSEvent *)event inView:(NSView *)view {
    nvclient_ui_input_mouse(ui_ctx, event, view, nvc_ui_mouse_key_middle, nvc_ui_mouse_action_drag);
}

- (BOOL)actionCut {
    BOOL res = NO;
    if (nvc_ui_action_cut(ui_ctx)) {
        _pasteboardNeedUpdate = YES;
        res = YES;
    }
    return res;
}

- (BOOL)actionCopy {
    BOOL res = NO;
    if (nvc_ui_action_copy(ui_ctx)) {
        _pasteboardNeedUpdate = YES;
        res = YES;
    }
    return res;
}

- (BOOL)actionPaste {
    NSString *str = [NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
    if (str.length > 0) {
        NSData *data = [str dataUsingEncoding:NSUTF8StringEncoding];
        nvc_ui_action_paste(ui_ctx, (const char *)data.bytes, (uint32_t)data.length);
    }
    return YES;
}

- (BOOL)actionSelectAll {
    nvc_ui_action_select_all(ui_ctx);
    return YES;
}

- (BOOL)actionDelete {
    return nvc_ui_action_delete(ui_ctx);
}

- (BOOL)actionUndo {
    nvc_ui_action_undo(ui_ctx);
    return YES;
}

- (BOOL)actionRedo {
    nvc_ui_action_redo(ui_ctx);
    return YES;
}

static inline void nvclient_ui_input_mouse(nvc_ui_context_t *ctx, NSEvent *event, NSView *view, nvc_ui_mouse_key_t key, nvc_ui_mouse_action_t action) {
    nvc_ui_mouse_info_t mouse = {
        .key        = key,
        .action     = action,
        .point      = [view convertPoint:event.locationInWindow toView:nil],
        .flags      = nvc_ui_key_flags_init(event.modifierFlags),
    };
    nvc_ui_input_mouse(ctx, mouse);
}

#pragma mark - Helper
static inline nvc_ui_key_flags_t nvc_ui_key_flags_init(NSEventModifierFlags flags) {
    return (nvc_ui_key_flags_t) {
        .shift    = flags & NSEventModifierFlagShift,
        .control  = flags & NSEventModifierFlagControl,
        .option   = flags & NSEventModifierFlagOption,
        .command  = flags & NSEventModifierFlagCommand,
    };
}

#pragma mark - Callback
static inline void nvclient_ui_layer_flush(void *userdata, int grid, CGRect dirty) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client grid:grid flush:dirty];
    });
}

static inline void nvclient_ui_layer_resize(void *userdata, int grid, CGRect frame) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client grid:grid resize:frame];
    });
}

static inline void nvclient_ui_layer_close(void *userdata, int grid) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client closeGrid:grid];
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
        [client.delegate client:client enableMouse:YES];
    });
}

static inline void nvclient_ui_mouse_off(void *userdata) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client enableMouse:NO];
    });
}

static inline void nvclient_ui_font_updated(void *userdata) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate clientUpdated:client];
    });
}

static inline void nvclient_ui_enable_mouse_autohide(void *userdata, bool enabled) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        client.autoHideMouse = enabled;
    });
}

static inline void nvclient_ui_enable_mouse_move(void *userdata, bool enabled) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client enableMouseMove:enabled];
    });
}

static inline void nvclient_ui_enable_ext_tabline(void *userdata, bool enabled) {
    NVClient *client = (__bridge NVClient *)userdata;
    @weakify(client);
    dispatch_main_async(^{
        @strongify(client);
        [client.delegate client:client hideTabline:!enabled];
    });
}

static inline void nvclient_ui_update_pasteboard(void *userdata, const char *str, uint32_t len) {
    NVClient *client = (__bridge NVClient *)userdata;
    if (str != NULL && len > 0) {
        NSString *value = [[NSString alloc] initWithBytes:str length:len encoding:NSUTF8StringEncoding];
        @weakify(client);
        dispatch_main_async(^{
            @strongify(client);
            NSPasteboard *pasteBoard = NSPasteboard.generalPasteboard;
            [pasteBoard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
            [pasteBoard setString:value forType:NSPasteboardTypeString];
            client->_pasteboardChangeCount = pasteBoard.changeCount;
        });
    }
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
    NVCLIENT_CALLBACK(layer_flush),
    NVCLIENT_CALLBACK(layer_resize),
    NVCLIENT_CALLBACK(layer_close),
    NVCLIENT_CALLBACK(update_title),
    NVCLIENT_CALLBACK(update_background),
    NVCLIENT_CALLBACK(update_tab_background),
    NVCLIENT_CALLBACK(update_tab_list),
    NVCLIENT_CALLBACK(mouse_on),
    NVCLIENT_CALLBACK(mouse_off),
    NVCLIENT_CALLBACK(font_updated),
    NVCLIENT_CALLBACK(enable_mouse_autohide),
    NVCLIENT_CALLBACK(enable_mouse_move),
    NVCLIENT_CALLBACK(enable_ext_tabline),
    NVCLIENT_CALLBACK(update_pasteboard),
    NVCLIENT_CALLBACK(close),
};


@end
