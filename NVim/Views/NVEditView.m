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
        _colorsSet = NVColorsSet.defaultColorsSet;
        _text = @"You can draw the entire text frame directly into the current graphic context. \n\nThe frame object contains an array of line objects that can be retrieved for individual rendering or to get glyph information.";
    }
    return self;
}

- (void)dealloc {
}


- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = NSGraphicsContext.currentContext.CGContext;
    CGContextSetFillColorWithColor(ctx, self.colorsSet.background.CGColor);
    CGContextFillRect(ctx, dirtyRect);
}


@end
