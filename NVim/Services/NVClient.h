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
- (void)clientFlush:(NVClient *)client;
- (void)client:(NVClient *)client updateTitle:(NSString *)title;
- (void)client:(NVClient *)client updateBackground:(NSColor *)color;
- (void)client:(NVClient *)client updateMouse:(BOOL)enabled;

@end

@interface NVClient : NSObject

@property (nonatomic, nullable, weak) id<NVClientDelegate> delegate;
@property (nonatomic, readonly, strong) NSString *info;

- (BOOL)isAttached;
- (void)openWithRead:(int)read write:(int)write;
- (CGSize)attachUIWithSize:(CGSize)size;
- (void)detachUI;
- (void)redrawUI;
- (CGSize)resizeUIWithSize:(CGSize)size;
- (void)close;


@end

NS_ASSUME_NONNULL_END
