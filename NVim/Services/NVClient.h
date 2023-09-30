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
- (void)clientClosed:(NVClient *)client;
@end

@interface NVClient : NSObject

@property (nonatomic, nullable, weak) id<NVClientDelegate> delegate;
@property (nonatomic, readonly, strong) NSString *info;

- (BOOL)isAttached;
- (void)openWithRead:(int)read write:(int)write;
- (void)close;
- (CGSize)attachUIWithSize:(CGSize)size;
- (void)detachUI;
- (void)redrawUI:(CGContextRef)ctx dirty:(CGRect)dirty;
- (CGSize)resizeUIWithSize:(CGSize)size;
- (void)keyDown:(NSEvent *)event;


@end

NS_ASSUME_NONNULL_END
