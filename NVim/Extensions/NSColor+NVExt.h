//
//  NSColor+NVExt.h
//  NVim
//
//  Created by wizjin on 2023/9/17.
//

#import <AppKit/NSColor.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSColor (NVExt)

+ (instancetype)colorWithRGB:(uint32_t)rgb;


@end

NS_ASSUME_NONNULL_END
