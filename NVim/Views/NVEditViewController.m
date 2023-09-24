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

@interface NVEditViewController () <NVClientDelegate>

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
}

- (void)viewDidLoad {
    [super viewDidLoad];
    @weakify(self);
    dispatch_main_async(^{
        @strongify(self);
        [self.client attachUIWithSize:self.editView.bounds.size];
    });
}

- (void)cleanup {
    if (self.client != nil) {
        [self.client close];
        _client = nil;
    }
}

#pragma mark - NSWindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
    [self.client detachUI];
}

- (void)windowDidResize:(NSNotification *)notification {
    [self.client resizeUIWithSize:self.editView.bounds.size];
}

#pragma mark - NVClientDelegate
- (void)clientFlush:(NVClient *)client {
    [self.editView displayIfNeeded];
}

- (void)client:(NVClient *)client updateTitle:(NSString *)title {
    self.title = title != nil ? title : @"";
}

- (void)client:(NVClient *)client updateBackground:(NSColor *)color {
    self.editView.backgroundColor = color;
}

- (void)client:(NVClient *)client updateMouse:(BOOL)enabled {
    self.view.window.ignoresMouseEvents = !enabled;
}


@end
