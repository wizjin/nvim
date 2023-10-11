//
//  NVClient.h
//  NVim
//
//  Created by wizjin on 2023/9/13.
//

#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class NVClient;

@protocol NVClientDelegate <NSObject>
@optional
- (void)client:(NVClient *)client flush:(CGRect)dirty;
- (void)client:(NVClient *)client updateTitle:(NSString *)title;
- (void)client:(NVClient *)client updateBackground:(NSColor *)color;
- (void)client:(NVClient *)client updateTabBackground:(NSColor *)color;
- (void)client:(NVClient *)client updateTabList:(BOOL)listUpdated;
- (void)client:(NVClient *)client updateMouse:(BOOL)enabled;
- (void)client:(NVClient *)client hideTabline:(BOOL)hidden;
- (void)clientUpdated:(NVClient *)client;
- (void)clientClosed:(NVClient *)client;
@end

@interface NVClient : NSObject

@property (nonatomic, nullable, weak) id<NVClientDelegate> delegate;
@property (nonatomic, readonly, strong) NSString *info;
@property (nonatomic, readonly, assign) CGFloat lineHeight;
@property (nonatomic, readonly, assign) CGPoint cursorPosition;

- (BOOL)isAttached;
- (void)openWithRead:(int)read write:(int)write;
- (void)close;
- (CGSize)attachUIWithSize:(CGSize)size;
- (void)detachUI;
- (void)redrawUI:(CGContextRef)context;
- (CGSize)resizeUIWithSize:(CGSize)size;
- (BOOL)openFiles:(NSArray<NSString *> *)files;
- (void)keyDown:(NSEvent *)event;
- (BOOL)functionKeyDown:(NSEvent *)event;
- (void)inputText:(NSString *)text flags:(NSEventModifierFlags)flags;
- (void)scrollWheel:(NSEvent *)event inView:(NSView *)view;
- (void)mouseUp:(NSEvent *)event inView:(NSView *)view;
- (void)mouseDown:(NSEvent *)event inView:(NSView *)view;
- (void)mouseDragged:(NSEvent *)event inView:(NSView *)view;
- (void)rightMouseUp:(NSEvent *)event inView:(NSView *)view;
- (void)rightMouseDown:(NSEvent *)event inView:(NSView *)view;
- (void)rightMouseDragged:(NSEvent *)event inView:(NSView *)view;
- (void)middleMouseUp:(NSEvent *)event inView:(NSView *)view;
- (void)middleMouseDown:(NSEvent *)event inView:(NSView *)view;
- (void)middleMouseDragged:(NSEvent *)event inView:(NSView *)view;

@end

NS_ASSUME_NONNULL_END
