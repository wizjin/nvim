//
//  NVTabView.m
//  NVim
//
//  Created by wizjin on 2023/9/29.
//

#import "NVTabView.h"

@implementation NVTabView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        self.wantsLayer = YES;
    }
    return self;
}

- (void)setBackgroundColor:(NSColor *)backgroundColor {
    if (![self.backgroundColor isEqualTo:backgroundColor]) {
        _backgroundColor = backgroundColor;
        self.layer.backgroundColor = backgroundColor.CGColor;
        [self.layer setNeedsDisplay];
    }
}


@end
