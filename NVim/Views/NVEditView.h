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
- (void)editView:(NVEditView *)editView redrawInContext:(CGContextRef)ctx;
- (void)editView:(NVEditView *)editView keyDown:(NSEvent *)event;
@end

@interface NVEditView : NVView

@property (nonatomic, readwrite, assign) CGSize contentSize;
@property (nonatomic, readwrite, strong) NSColor *backgroundColor;
@property (nonatomic, nullable, weak) id<NVEditViewDelegate> delegate;

- (void)updateDisplayRect:(CGRect)dirty;
- (void)startContentResize;
- (void)endContentResize;

@end

NS_ASSUME_NONNULL_END
