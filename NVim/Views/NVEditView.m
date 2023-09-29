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

- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!self.inLiveResize) {
        CGContextRef context = NSGraphicsContext.currentContext.CGContext;
        CGContextTranslateCTM(context, CGRectGetMinX(self.contentRect), CGRectGetHeight(self.contentRect));
        CGContextScaleCTM(context, 1, -1);
        [self.delegate redrawEditView:self inContext:context dirty:dirtyRect];
        CGContextFlush(context);
    }
}


@end
