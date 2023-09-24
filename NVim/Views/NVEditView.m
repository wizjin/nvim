//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"

@interface NVEditView ()

@end

@implementation NVEditView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        _font = [NSFont monospacedSystemFontOfSize:20 weight:NSFontWeightRegular];
        _backgroundColor = NSColor.clearColor;
    }
    return self;
}

- (void)setBackgroundColor:(NSColor *)backgroundColor {
    if ([self.backgroundColor isNotEqualTo:backgroundColor]) {
        _backgroundColor = backgroundColor;
        self.layer.backgroundColor = self.backgroundColor.CGColor;
    }
}


@end
