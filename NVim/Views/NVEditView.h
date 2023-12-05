//
//  NVEditView.h
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVView.h"
#import "NVClient.h"

NS_ASSUME_NONNULL_BEGIN

@class NVEditView;

@interface NVEditView : NVView

@property (nonatomic, readwrite, strong) NVClient *client;
@property (nonatomic, readwrite, assign) CGSize contentSize;
@property (nonatomic, readwrite, strong) NSColor *backgroundColor;

- (void)layer:(NSInteger)grid flush:(CGRect)dirty;
- (void)layer:(NSInteger)grid resize:(CGRect)frame;
- (void)closeLayer:(NSInteger)grid;
- (void)startContentResize;
- (void)endContentResize;

@end

NS_ASSUME_NONNULL_END
