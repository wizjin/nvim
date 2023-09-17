//
//  NSColor+NVExt.m
//  NVim
//
//  Created by wizjin on 2023/9/17.
//

#import "NSColor+NVExt.h"

@implementation NSColor (NVExt)

+ (instancetype)colorWithRGB:(uint32_t)rgb {
    const uint8_t *c = (const uint8_t *)&rgb;
    return [NSColor colorWithRed:c[2]/255.0 green:c[1]/255.0 blue:c[0]/255.0 alpha:1.0];
}


@end
