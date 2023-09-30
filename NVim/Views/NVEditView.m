//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"

@interface NVEditView () <CALayerDelegate>

@property (nonatomic, readonly, strong) CALayer *drawLayer;

@end

@implementation NVEditView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        _contentSize = CGSizeZero;
        self.wantsLayer = YES;
        CALayer *drawLayer = [CALayer new];
        [self.layer addSublayer:(_drawLayer = drawLayer)];
        drawLayer.contentsScale = self.layer.contentsScale;
        drawLayer.allowsEdgeAntialiasing = NO;
        drawLayer.allowsGroupOpacity = NO;
        drawLayer.masksToBounds = YES;
        drawLayer.doubleSided = NO;
        drawLayer.delegate = self;
    }
    return self;
}

- (void)viewWillStartLiveResize {
    [self startContentResize];
    [super viewWillStartLiveResize];
}

- (void)viewDidEndLiveResize {
    [super viewDidEndLiveResize];
    [self endContentResize];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    [self.delegate editView:self keyDown:event];
}

- (void)mouseUp:(NSEvent *)event {
    [self.delegate editView:self mouseUp:event];
}

- (void)mouseDown:(NSEvent *)event {
    [self.delegate editView:self mouseDown:event];
}

- (void)mouseDragged:(NSEvent *)event {
    [self.delegate editView:self mouseDragged:event];
}

- (void)scrollWheel:(NSEvent *)event {
    [self.delegate editView:self scrollWheel:event];
}

#pragma mark - Public Methods
- (void)setContentSize:(CGSize)contentSize {
    if (!CGSizeEqualToSize(self.contentSize, contentSize)) {
        _contentSize = contentSize;
        self.drawLayer.frame = CGRectMake(0, 0, contentSize.width, contentSize.height);
    }
}

- (void)setBackgroundColor:(NSColor *)backgroundColor {
    if (![self.backgroundColor isEqualTo:backgroundColor]) {
        _backgroundColor = backgroundColor;
        self.layer.backgroundColor = backgroundColor.CGColor;
        self.drawLayer.backgroundColor = self.layer.backgroundColor;
    }
}

- (void)updateDisplayRect:(CGRect)dirty {
    [self.drawLayer setNeedsDisplayInRect:dirty];
}

- (void)startContentResize {
    [self.drawLayer setHidden:YES];
}

- (void)endContentResize {
    @weakify(self);
    dispatch_main_async(^{
        @strongify(self);
        [self.drawLayer setHidden:NO];
    });
}

#pragma mark - CALayerDelegate
- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)context {
    static const CGAffineTransform matrix = {1, 0, 0, -1, 0, 0 };
    CGContextSetTextMatrix(context, matrix);
    CGContextSetShouldAntialias(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetTextDrawingMode(context, kCGTextFill);
    [self.delegate editView:self redrawInContext:context];
    CGContextFlush(context);
}


@end
