//
//  NVLayer.m
//  NVim
//
//  Created by wizjin on 2023/10/29.
//

#import "NVLayer.h"

@implementation NVLayer

- (instancetype)init {
    if (self = [super init]) {
        self.allowsEdgeAntialiasing = NO;
        self.allowsGroupOpacity = NO;
        self.masksToBounds = YES;
        self.doubleSided = NO;
    }
    return self;
}


@end
