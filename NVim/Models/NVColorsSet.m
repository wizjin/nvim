//
//  NVColorsSet.m
//  NVim
//
//  Created by wizjin on 2023/9/17.
//

#import "NVColorsSet.h"

@implementation NVColorsSet

+ (NVColorsSet *)defaultColorsSet {
    static NVColorsSet *colorsSet;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        colorsSet = [NVColorsSet new];
        colorsSet.foreground = NSColor.textColor;
        colorsSet.background = NSColor.textBackgroundColor;
        colorsSet.special = NSColor.selectedTextColor;
    });
    return colorsSet;
}


@end
