//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"

@implementation NVEditView

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
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!self.inLiveResize) {
        CGContextRef context = NSGraphicsContext.currentContext.CGContext;
        CGContextTranslateCTM(context, 0, self.contentSize.height);
        CGContextScaleCTM(context, 1, -1);
        [self.delegate redrawEditView:self inContext:context dirty:dirtyRect];
        CGContextFlush(context);
    }
}


@end
