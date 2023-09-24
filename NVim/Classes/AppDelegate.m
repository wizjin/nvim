//
//  AppDelegate.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "AppDelegate.h"
#import "NVMainMenu.h"
#import "NVEditViewController.h"

#define kNVWindowDefaultStyleMask    (\
    NSWindowStyleMaskTitled|\
    NSWindowStyleMaskClosable|\
    NSWindowStyleMaskMiniaturizable|\
    NSWindowStyleMaskResizable\
)

@interface AppDelegate ()

@property (nonatomic, readonly, strong) NSWindow *window;
@property (nonatomic, readonly, strong) NSStatusItem *statusItem;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSStatusItem *statusItem = [NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength];
    _statusItem = statusItem;
    statusItem.button.image = [NSImage imageNamed:@"StatusLogo"];
    statusItem.button.imagePosition = NSImageLeft;
    
    NSApp.mainMenu = [NVMainMenu new];

    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSZeroRect styleMask:kNVWindowDefaultStyleMask backing:NSBackingStoreBuffered defer:NO];
    _window = window;

    NVEditViewController *controller = [NVEditViewController new];
    window.releasedWhenClosed = NO;
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = NSColor.windowBackgroundColor;
    window.contentViewController = controller;
    window.delegate = controller;
    window.minSize = NSMakeSize(kNVWindowMinWidth, kNVWindowMinHeight);
    [window setContentSize:NSMakeSize(kNVWindowDefaultWidth, kNVWindowDefaultHeight)];
    [window center];
    [window becomeKeyWindow];
    [window orderFrontRegardless];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    [(NVEditViewController *)self.window.contentViewController cleanup];
    if (self.statusItem != nil) {
        [NSStatusBar.systemStatusBar removeStatusItem:self.statusItem];
        _statusItem = nil;
    }
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}


@end
