//
//  NVEditViewController.m
//  NVim
//
//  Created by wizjin on 2023/9/24.
//

#import "NVEditViewController.h"
#import "NVEditView.h"
#import "NVTabView.h"
#import "NVTCPClient.h"
#import "NVSTDClient.h"

@interface NVEditViewController () <NVClientDelegate>

@property (nonatomic, readonly, strong) NVClient *client;
@property (nonatomic, readonly, strong) NVTabView *tabView;
@property (nonatomic, readonly, strong) NVEditView *editView;

@end

@implementation NVEditViewController

- (void)loadView {
    self.view = [NVView new];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    _client = [NVSTDClient new];
    //_client = [[NVSTDClient alloc] initWithPath:@"/usr/local/bin/nvim"];
    //_client = [[NVTCPClient alloc] initWithHost:@"127.0.0.1" port:6666];
    self.client.delegate = self;
    
    NVTabView *tabView = [[NVTabView alloc] initWithFrame:NSMakeRect(0, 0, NSWidth(self.view.bounds), kNVTabViewDefaultHeight)];
    [self.view addSubview:(_tabView = tabView)];
    NVEditView *editView = [NVEditView new];
    [self.view addSubview:(_editView = editView)];
    editView.client = self.client;
}

- (void)viewDidAppear {
    [super viewDidAppear];
    if (!self.client.isAttached) {
        [self updateContentSize:[self.client attachUIWithSize:self.contentUISize]];
    }
}

- (void)viewDidLayout {
    [super viewDidLayout];
    CGRect bounds = self.view.bounds;
    CGFloat offset = 0;
    CGFloat width = NSWidth(bounds) - kNVContentMarginWidth;
    if (!self.tabView.isHidden) {
        offset = NSHeight(self.tabView.bounds);
        self.tabView.frame = CGRectMake(kNVContentMarginWidth/2, 0, width, offset);
    }
    self.editView.frame = CGRectMake(kNVContentMarginWidth/2, offset, width, NSHeight(bounds) - offset);
}

- (void)cleanup {
    if (self.client != nil) {
        [self.client close];
        _client = nil;
    }
}

- (void)updateContentSize:(CGSize)size {
    self.editView.contentSize = size;
}

#pragma mark - NSWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    [self.client detachUI];
}

- (void)windowDidEndLiveResize:(NSNotification *)notification {
    [self updateContentSize:[self.client resizeUIWithSize:self.contentUISize]];
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
    [self.editView startContentResize];
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification {
    [self.editView endContentResize];
}

- (void)windowWillExitFullScreen:(NSNotification *)notification {
    [self.editView startContentResize];
}

- (void)windowDidExitFullScreen:(NSNotification *)notification {
    [self.editView endContentResize];
}

#pragma mark - NVClientDelegate
- (void)client:(NVClient *)client flush:(CGRect)dirty {
    [self.editView updateDisplayRect:dirty];
}

- (void)client:(NVClient *)client updateTitle:(NSString *)title {
    self.title = (title != nil ? title : @"");
}

- (void)client:(NVClient *)client updateBackground:(NSColor *)color {
    self.editView.backgroundColor = color;
}

- (void)client:(NVClient *)client updateTabBackground:(NSColor *)color {
    self.tabView.backgroundColor = color;
}

- (void)client:(NVClient *)client updateTabList:(BOOL)listUpdated {
    // TODO: Notify tab changed
}

- (void)client:(NVClient *)client enableMouse:(BOOL)enabled {
    self.view.window.ignoresMouseEvents = !enabled;
}

- (void)client:(NVClient *)client enableMouseMove:(BOOL)enabled {
    self.view.window.acceptsMouseMovedEvents = enabled;
}

- (void)client:(NVClient *)client hideTabline:(BOOL)hidden {
    if (self.tabView.isHidden != hidden) {
        self.tabView.hidden = hidden;
        [self contentLayout];
    }
}

- (void)clientUpdated:(NVClient *)client {
    [self contentLayout];
}

- (void)clientClosed:(NVClient *)client {
    [self.view.window close];
}

#pragma mark - Helper
- (void)contentLayout {
    [self.view setNeedsLayout:YES];
    [self updateContentSize:[self.client resizeUIWithSize:self.contentUISize]];
}

- (CGSize)contentUISize {
    CGSize size = self.view.bounds.size;
    size.width -= kNVContentMarginWidth;
    size.height -= kNVContentMarginHeight;
    if (!self.tabView.isHidden) {
        size.height -= NSHeight(self.tabView.bounds);
    }
    return size;
}


@end
