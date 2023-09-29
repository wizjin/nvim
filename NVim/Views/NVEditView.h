//
//  NVEditView.h
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVView.h"

NS_ASSUME_NONNULL_BEGIN

@class NVEditView;

@protocol NVEditViewDelegate <NSObject>
- (void)redrawEditView:(NVEditView *)editView inContext:(CGContextRef)ctx dirty:(CGRect)dirty;
@end

@interface NVEditView : NVView

@property (nonatomic, readwrite, assign) CGSize contentSize;
@property (nonatomic, readwrite, strong) NSColor *backgroundColor;
@property (nonatomic, nullable, weak) id<NVEditViewDelegate> delegate;


@end

NS_ASSUME_NONNULL_END
