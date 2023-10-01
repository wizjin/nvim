//
//  NVView.m
//  NVim
//
//  Created by wizjin on 2023/9/29.
//

#import "NVView.h"

@implementation NVView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        self.clipsToBounds = YES;
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}


@end
