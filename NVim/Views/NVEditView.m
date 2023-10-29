//
//  NVEditView.m
//  NVim
//
//  Created by wizjin on 2023/9/12.
//

#import "NVEditView.h"
#import "NVLayer.h"

#define kNSEventModifierMask    (NSEventModifierFlagControl|NSEventModifierFlagOption|NSEventModifierFlagCommand)
#define kVK_Delete              0x33

@interface NVEditView () <CALayerDelegate, NSTextInputClient>

@property (nonatomic, readonly, strong) NSMutableDictionary<NSNumber *, NVLayer *> *drawLayers;
@property (nonatomic, readonly, strong) NSTextInputContext *textInputContext;
@property (nonatomic, readonly, assign) NSEventModifierFlags flags;
@property (nonatomic, readonly, strong) NSString *markedText;

@end

@implementation NVEditView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        _contentSize = CGSizeZero;
        _textInputContext = [[NSTextInputContext alloc] initWithClient:self];
        _flags = 0;
        _markedText = nil;
        _drawLayers = [NSMutableDictionary new];

        self.wantsLayer = YES;
        
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
    if (self.client.autoHideMouse) {
        [NSCursor setHiddenUntilMouseMoves:YES];
    }
    if (![self.client functionKeyDown:event] && ![self.textInputContext handleEvent:event]) {
        [self.client keyDown:event];
    }
}

- (void)flagsChanged:(NSEvent *)event {
    _flags = event.modifierFlags;
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

- (void)mouseMoved:(NSEvent *)event {
    [self.client mouseMoved:event inView:self];
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
    }
}

- (void)setBackgroundColor:(NSColor *)backgroundColor {
    if (![self.backgroundColor isEqualTo:backgroundColor]) {
        _backgroundColor = backgroundColor;
        self.layer.backgroundColor = backgroundColor.CGColor;
    }
}

- (void)layer:(NSInteger)grid flush:(CGRect)dirty {
    [[self findLayer:@(grid)] setNeedsDisplayInRect:dirty];
}

- (void)layer:(NSInteger)grid resize:(CGRect)frame {
    [[self findLayer:@(grid)] setFrame:frame];
}

- (void)closeLayer:(NSInteger)grid {
    NSNumber *gid = @(grid);
    NVLayer *layer = [self.drawLayers objectForKey:gid];
    if (layer != nil) {
        [layer removeFromSuperlayer];
        [self.drawLayers removeObjectForKey:gid];
    }
}

- (void)startContentResize {
    for (NVLayer *layer in self.drawLayers.allValues) {
        [layer setHidden:YES];
    }
}

- (void)endContentResize {
    @weakify(self);
    dispatch_main_async(^{
        @strongify(self);
        for (NVLayer *layer in self.drawLayers.allValues) {
            [layer setHidden:NO];
        }
    });
}

#pragma mark - CALayerDelegate
- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)context {
    static const CGAffineTransform matrix = { 1, 0, 0, -1, 0, 0 };
    CGContextSetTextMatrix(context, matrix);
    CGContextSetShouldAntialias(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetTextDrawingMode(context, kCGTextFill);
    [self.client redrawLayer:[(NVLayer *)layer grid] context:context];
    CGContextFlush(context);
}

#pragma mark - NSTextInputClient
- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    NSString *text = get_string(string);
    [self.client inputText:text flags:self.flags];
    if (self.hasMarkedText) {
        [self unmarkText];
    }
}

- (void)doCommandBySelector:(SEL)selector {

}

- (BOOL)performKeyEquivalent:(NSEvent *)event {
    BOOL res = NO;
    if ((event.modifierFlags&kNSEventModifierMask) == NSEventModifierFlagCommand) {
        if (event.keyCode == kVK_Delete) {
            if ((event.modifierFlags&NSEventModifierFlagShift) == 0) {
                res = [self.client actionDelete];
            }
        } else {
            NSString *cs = event.charactersIgnoringModifiers;
            if (cs.length > 0) {
                unichar c = [cs characterAtIndex:0];
                if ((event.modifierFlags&NSEventModifierFlagShift) == NSEventModifierFlagShift) c = toupper(c);
                switch (c) {
                    case 'x':
                        res = [self.client actionCut];
                        break;
                    case 'c':
                        res = [self.client actionCopy];
                        break;
                    case 'v':
                        res = [self.client actionPaste];
                        break;
                    case 'a':
                        res = [self.client actionSelectAll];
                        break;
                    case 'z':
                        res = [self.client actionUndo];
                        break;
                    case 'Z':
                        res = [self.client actionRedo];
                        break;
                    default:
                        break;
                }
            }
        }
    }
    if (!res) {
        res = [self.client keyDown:event];
    }
    return res;
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    _markedText = get_string(string);
}

- (void)unmarkText {
    _markedText = nil;
}

- (NSRange)selectedRange {
    return NSMakeRange(0, 0);
}

- (NSRange)markedRange {
    return NSMakeRange(0, 0);
}

- (BOOL)hasMarkedText {
    return self.markedText != nil;
}

- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {
    NSAttributedString *text = nil;
    if (actualRange != nil && actualRange->location != NSNotFound && self.hasMarkedText) {
        text = [[NSAttributedString alloc] initWithString:self.markedText];
    }
    return text;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {
    NSRect rc = {
        .origin = self.client.cursorPosition,
        .size = CGSizeMake(0, self.client.lineHeight),
    };
    return [self.window convertRectToScreen:[self convertRect:rc toView:nil]];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return 0;
}

#pragma mark - Helper
- (NVLayer *)findLayer:(NSNumber *)grid {
    NVLayer *layer = [self.drawLayers objectForKey:grid];
    if (layer == nil) {
        layer = [NVLayer new];
        [self.drawLayers setObject:layer forKey:grid];
        layer.contentsScale = self.layer.contentsScale;
        layer.grid = grid.integerValue;
        layer.delegate = self;
        [self.layer addSublayer:layer];
    }
    return layer;
}

static inline NSString *get_string(id val) {
    if ([val isKindOfClass:NSString.class]) {
        return val;
    }
    if ([val isKindOfClass:NSAttributedString.class]) {
        return [val string];
    }
    return @"";
}


@end
