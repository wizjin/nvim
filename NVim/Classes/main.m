//
//  main.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import <AppKit/AppKit.h>
#import "AppDelegate.h"

int main(int argc, const char * argv[]) {
    int result = 0;
    @autoreleasepool {
        AppDelegate *appDelegate = [AppDelegate new];
        NSApplication.sharedApplication.delegate = appDelegate;
        result = NSApplicationMain(argc, argv);
    }
    return result;
}
