//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"

@implementation NVEditView

- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    CGContextTranslateCTM(context, CGRectGetMinX(self.contentRect), CGRectGetHeight(self.contentRect));
    CGContextScaleCTM(context, 1, -1);
    [self.delegate redrawEditView:self inContext:context];
    CGContextFlush(context);

}


@end
