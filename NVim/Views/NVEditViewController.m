//
//  NVEditViewController.m
//  NVim
//
//  Created by wizjin on 2023/9/24.
//

#import "NVEditViewController.h"
#import "NVEditView.h"
#import "NVTCPClient.h"
#import "NVSTDClient.h"

@interface NVEditViewController () <NVClientDelegate, NVEditViewDelegate>

@property (nonatomic, readonly, strong) NVClient *client;
@property (nonatomic, readonly, strong) NVEditView *editView;

@end

@implementation NVEditViewController

- (instancetype)init {
    if (self = [super initWithNibName:nil bundle:nil]) {
        _client = [NVSTDClient new];
        //_client = [[NVSTDClient alloc] initWithPath:@"/usr/local/bin/nvim"];
        //_client = [[NVTCPClient alloc] initWithHost:@"127.0.0.1" port:6666];
        self.client.delegate = self;
    }
    return self;
}

- (void)loadView {
    _editView = [NVEditView new];
    self.view = self.editView;
    self.editView.delegate = self;
}

- (void)viewDidAppear {
    [super viewDidAppear];
    if (!self.client.isAttached) {
        [self updateContentSize:[self.client attachUIWithSize:convertToUISize(self.editView.bounds.size)]];
    }
}

- (void)cleanup {
    if (self.client != nil) {
        [self.client close];
        _client = nil;
    }
}

- (void)updateContentSize:(CGSize)size {
    self.editView.contentRect = NSMakeRect(kNVContentMarginWidth/2, 0, size.width, size.height);
}

#pragma mark - NSWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    [self.client detachUI];
}

- (void)windowDidEndLiveResize:(NSNotification *)notification {
    [self updateContentSize:[self.client resizeUIWithSize:convertToUISize(self.editView.bounds.size)]];
}

#pragma mark - NVClientDelegate
- (void)client:(NVClient *)client flush:(CGRect)dirty {
    [self.editView setNeedsDisplayInRect:dirty];
}

- (void)client:(NVClient *)client updateTitle:(NSString *)title {
    self.title = (title != nil ? title : @"");
}

- (void)client:(NVClient *)client updateBackground:(NSColor *)color {
    self.editView.layer.backgroundColor = color.CGColor;
}

- (void)client:(NVClient *)client updateMouse:(BOOL)enabled {
    self.editView.window.ignoresMouseEvents = !enabled;
}

#pragma mark - NVEditViewDelegate
- (void)redrawEditView:(NVEditView *)editView inContext:(CGContextRef)ctx dirty:(CGRect)dirty {
    [self.client redrawUI:ctx dirty:dirty];
}

#pragma mark - Helper
static inline CGSize convertToUISize(const CGSize size) {
    return CGSizeMake(size.width - kNVContentMarginWidth, size.height - kNVContentMarginHeight);
}


@end
