//
//  NVEditView.h
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface NVEditView : NSView

@property (nonatomic, readonly, strong) NSFont *font;
@property (nonatomic, readwrite, strong) NSColor *backgroundColor;


@end

NS_ASSUME_NONNULL_END
