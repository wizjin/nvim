//
//  AppDelegate.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "AppDelegate.h"

@interface AppDelegate ()

@property (nonatomic, strong) IBOutlet NSWindow *window;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}


@end
