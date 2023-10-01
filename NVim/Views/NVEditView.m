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
        
        [self registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
    }
    return self;
}

- (void)dealloc {
    [self unregisterDraggedTypes];
}

- (void)viewWillStartLiveResize {
    [self startContentResize];
    [super viewWillStartLiveResize];
}

- (void)viewDidEndLiveResize {
    [super viewDidEndLiveResize];
    [self endContentResize];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    if ([sender.draggingPasteboard.types containsObject:NSPasteboardTypeFileURL]) {
        NSDragOperation operation = sender.draggingSourceOperationMask;
        if (operation & NSDragOperationLink) {
            return NSDragOperationLink;
        } else if (operation & NSDragOperationCopy) {
            return NSDragOperationCopy;
        }
    }
    return NSDragOperationGeneric;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender {
    BOOL res = NO;
    NSPasteboard *pasteboard = sender.draggingPasteboard;
    if ([pasteboard.types containsObject:NSPasteboardTypeFileURL]) {
        NSFileManager *fm = NSFileManager.defaultManager;
        NSMutableArray<NSString *> *files = [NSMutableArray new];
        for (NSURL *url in [pasteboard readObjectsForClasses:@[NSURL.class] options:nil]) {
            NSString *file = url.path;
            if ([fm fileExistsAtPath:file]) {
                [files addObject:file];
            }
        }
        res = [self.client openFiles:files];
    }
    return res;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    [self.client keyDown:event];
}

- (void)scrollWheel:(NSEvent *)event {
    [self.client scrollWheel:event inView:self];
}

- (void)mouseUp:(NSEvent *)event {
    [self.client mouseUp:event inView:self];
}

- (void)mouseDown:(NSEvent *)event {
    [self.client mouseDown:event inView:self];
}

- (void)mouseDragged:(NSEvent *)event {
    [self.client mouseDragged:event inView:self];
}

- (void)rightMouseUp:(NSEvent *)event {
    [self.client rightMouseUp:event inView:self];
}

- (void)rightMouseDown:(NSEvent *)event {
    [self.client rightMouseDown:event inView:self];
}

- (void)rightMouseDragged:(NSEvent *)event {
    [self.client rightMouseDragged:event inView:self];
}

- (void)otherMouseUp:(NSEvent *)event {
    [self.client middleMouseUp:event inView:self];
}

- (void)otherMouseDown:(NSEvent *)event {
    [self.client middleMouseDown:event inView:self];
}

- (void)otherMouseDragged:(NSEvent *)event {
    [self.client middleMouseDragged:event inView:self];
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
    static const CGAffineTransform matrix = { 1, 0, 0, -1, 0, 0 };
    CGContextSetTextMatrix(context, matrix);
    CGContextSetShouldAntialias(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetTextDrawingMode(context, kCGTextFill);
    [self.client redrawUI:context];
    CGContextFlush(context);
}


@end
