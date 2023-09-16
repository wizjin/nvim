//
//  NVWindow.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "NVWindow.h"
#import "NVEditView.h"
#import "NVTCPClient.h"

#define kNVWindowDefaultStyleMask    (\
    NSWindowStyleMaskTitled|\
    NSWindowStyleMaskClosable|\
    NSWindowStyleMaskMiniaturizable|\
    NSWindowStyleMaskResizable\
)

@interface NVWindow ()

@property (nonatomic, readonly, strong) NVClient *client;

@end

@implementation NVWindow

- (instancetype)init {
    if (self = [super initWithContentRect:NSMakeRect(0, 0, kNVWindowDefaultWidth, kNVWindowDefaultHeight) styleMask:kNVWindowDefaultStyleMask backing:NSBackingStoreBuffered defer:NO]) {
        self.releasedWhenClosed = NO;
        self.backgroundColor = NSColor.windowBackgroundColor;
        self.titlebarAppearsTransparent = YES;
        self.minSize = NSMakeSize(kNVWindowMinWidth, kNVWindowMinHeight);
        
        _client = [[NVTCPClient alloc] initWithHost:@"127.0.0.1" port:6666];
        NVEditView *editView = [NVEditView new];
        self.contentView = editView;
        @weakify(self);
        dispatch_main_async(^{
            @strongify(self);
            [self.client attachUI];
        });
    }
    return self;
}

- (void)close {
    [self.client detachUI];
    [super close];
}

- (void)cleanup {
    if (self.client != nil) {
        [self.client close];
        _client = nil;
    }
}


@end
