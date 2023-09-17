//
//  NVEditView.h
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVColorsSet.h"

NS_ASSUME_NONNULL_BEGIN

@interface NVEditView : NSView

@property (nonatomic, readonly, strong) NSFont *font;
@property (nonatomic, strong) NVColorsSet *colorsSet;


@end

NS_ASSUME_NONNULL_END
