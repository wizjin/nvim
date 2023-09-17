//
//  NVColorsSet.h
//  NVim
//
//  Created by wizjin on 2023/9/17.
//

#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface NVColorsSet : NSObject

@property (class, nonatomic, readonly, strong) NVColorsSet *defaultColorsSet;
@property (nonatomic, strong) NSColor *foreground;
@property (nonatomic, strong) NSColor *background;
@property (nonatomic, strong) NSColor *special;


@end

NS_ASSUME_NONNULL_END
