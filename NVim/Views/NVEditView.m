//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"

@interface NVEditView ()

@property (nonatomic, readonly, assign) CTFramesetterRef frameSetter;
@property (nonatomic, readonly, assign) CTFrameRef textFrame;

@end

@implementation NVEditView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
        paragraphStyle.lineBreakMode = NSLineBreakByClipping;
        NSAttributedString *text = [[NSAttributedString alloc] initWithString:@"You can draw the entire text frame directly into the current graphic context. \n\nThe frame object contains an array of line objects that can be retrieved for individual rendering or to get glyph information." attributes:@{
            NSFontAttributeName: [NSFont monospacedSystemFontOfSize:20 weight:NSFontWeightRegular],
            NSForegroundColorAttributeName: NSColor.blackColor,
            NSParagraphStyleAttributeName: paragraphStyle,
        }];
        _frameSetter = CTFramesetterCreateWithAttributedString((__bridge CFAttributedStringRef)text);
    }
    return self;
}

- (void)dealloc {
    if (self.textFrame != nil) {
        CFRelease(self.textFrame);
        _textFrame = nil;
    }
    if (self.frameSetter != nil) {
        CFRelease(self.frameSetter);
        _frameSetter = nil;
    }
}

- (void)layout {
    [super layout];
    if (self.textFrame != nil) {
        CGRect box = CGPathGetBoundingBox(CTFrameGetPath(self.textFrame));
        if (!CGRectEqualToRect(self.bounds, box)) {
            CFRelease(self.textFrame);
            _textFrame = nil;
        }
    }
    if (self.textFrame == nil) {
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathAddRect(path, NULL, self.bounds);
        _textFrame = CTFramesetterCreateFrame(self.frameSetter, CFRangeMake(0, 0), path, NULL);
        CGPathRelease(path);
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = NSGraphicsContext.currentContext.CGContext;
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, dirtyRect);
    CTFrameDraw(self.textFrame, ctx);
}


@end
