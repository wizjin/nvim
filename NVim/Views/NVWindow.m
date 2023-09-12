//
//  NVWindow.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "NVWindow.h"
#import "NVEditView.h"

#define kNVWindowDefaultStyleMask    (\
    NSWindowStyleMaskTitled|\
    NSWindowStyleMaskClosable|\
    NSWindowStyleMaskMiniaturizable|\
    NSWindowStyleMaskResizable\
)

@implementation NVWindow

- (instancetype)init {
    if (self = [super initWithContentRect:NSMakeRect(0, 0, kNVWindowDefaultWidth, kNVWindowDefaultHeight) styleMask:kNVWindowDefaultStyleMask backing:NSBackingStoreBuffered defer:NO]) {
        self.releasedWhenClosed = NO;
        self.backgroundColor = NSColor.windowBackgroundColor;
        self.titlebarAppearsTransparent = YES;
        self.minSize = NSMakeSize(kNVWindowMinWidth, kNVWindowMinHeight);
        self.contentView = [NVEditView new];
    }
    return self;
}


@end
