//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"

@interface NVEditView ()

@property (nonatomic, readonly, strong) NSString *text;

@end

@implementation NVEditView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        _font = [NSFont monospacedSystemFontOfSize:20 weight:NSFontWeightRegular];
        _backgroundColor = NSColor.clearColor;
        _text = @"You can draw the entire text frame directly into the current graphic context. \n\nThe frame object contains an array of line objects that can be retrieved for individual rendering or to get glyph information.";
    }
    return self;
}

- (void)dealloc {
}

- (void)setBackgroundColor:(NSColor *)backgroundColor {
    if (![_backgroundColor isEqualTo:backgroundColor]) {
        _backgroundColor = backgroundColor;
        [self setNeedsDisplayInRect:self.bounds];
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = NSGraphicsContext.currentContext.CGContext;
    CGContextSetFillColorWithColor(ctx, self.backgroundColor.CGColor);
    CGContextFillRect(ctx, dirtyRect);
}


@end
