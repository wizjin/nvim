//
//  NVEditView.h
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class NVEditView;

@protocol NVEditViewDelegate <NSObject>
- (void)redrawEditView:(NVEditView *)editView inContext:(CGContextRef)ctx;
@end

@interface NVEditView : NSView

@property (nonatomic, readwrite, assign) CGRect contentRect;

@property (nonatomic, nullable, weak) id<NVEditViewDelegate> delegate;


@end

NS_ASSUME_NONNULL_END
