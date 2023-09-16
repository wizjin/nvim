//
//  AppDelegate.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "AppDelegate.h"
#import "NVMainMenu.h"
#import "NVWindow.h"

@interface AppDelegate ()

@property (nonatomic, readonly, strong) NVWindow *window;
@property (nonatomic, readonly, strong) NSStatusItem *statusItem;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSStatusItem *statusItem = [NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength];
    _statusItem = statusItem;
    statusItem.button.image = [NSImage imageNamed:@"StatusLogo"];
    statusItem.button.imagePosition = NSImageLeft;
    
    NSApp.mainMenu = [NVMainMenu new];

    _window = [NVWindow new];
    [self.window center];
    [self.window becomeKeyWindow];
    [self.window orderFrontRegardless];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    [self.window cleanup];
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
