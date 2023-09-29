//
//  NVMainMenu.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "NVMainMenu.h"

@implementation NVMainMenu

- (instancetype)init {
    if (self = [super init]) {
        NSString *name = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
        // App
        NSMenu *app = [self menuWithTitle:name];
        [app addItem:[self itemWithTitle:[NSString stringWithFormat:@"About %@".localized, name] target:NSApp action:@selector(orderFrontStandardAboutPanel:)]];
        [app addItem:NSMenuItem.separatorItem];
        NSMenu *services = [self menuWithTitle:@"Services".localized parent:app];
        NSApp.servicesMenu = services;
        [app addItem:NSMenuItem.separatorItem];
        [app addItem:[self itemWithTitle:[NSString stringWithFormat:@"Hide %@".localized, name] target:NSApp action:@selector(hide:) keyEquivalent:@"h"]];
        [app addItem:[self itemWithTitle:@"Hide Others".localized target:NSApp action:@selector(hideOtherApplications:) keyEquivalent:@"h" eventModifier:NSEventModifierFlagOption]];
        [app addItem:[self itemWithTitle:@"Show All".localized target:NSApp action:@selector(unhideAllApplications:)]];
        [app addItem:NSMenuItem.separatorItem];
        [app addItem:[self itemWithTitle:[NSString stringWithFormat:@"Quit %@".localized, name] target:NSApp action:@selector(terminate:) keyEquivalent:@"q"]];
        // File
        NSMenu *file = [self menuWithTitle:@"File".localized];
        [file addItem:[self itemWithTitle:@"New".localized target:NSApp action:@selector(newDocument:) keyEquivalent:@"n"]];
        [file addItem:[self itemWithTitle:@"Open…".localized target:NSApp action:@selector(openDocument:) keyEquivalent:@"o"]];
        NSMenu *recent = [self menuWithTitle:@"Open Recent".localized parent:file];
        [recent addItem:[self itemWithTitle:@"Clear Menu".localized target:NSApp action:@selector(clearRecentDocuments:)]];
        [file addItem:NSMenuItem.separatorItem];
        [file addItem:[self itemWithTitle:@"Close".localized target:nil action:@selector(performClose:) keyEquivalent:@"w"]];
        [file addItem:[self itemWithTitle:@"Save…".localized target:nil action:@selector(saveDocument:) keyEquivalent:@"s"]];
        [file addItem:[self itemWithTitle:@"Save As…".localized target:nil action:@selector(saveDocumentAs:) keyEquivalent:@"S"]];
        [file addItem:NSMenuItem.separatorItem];
        [file addItem:[self itemWithTitle:@"Page Setup…".localized target:nil action:@selector(runPageLayout:) keyEquivalent:@"P"]];
        [file addItem:[self itemWithTitle:@"Print…".localized target:nil action:@selector(print:) keyEquivalent:@"p"]];
        // Edit
        NSMenu *edit = [self menuWithTitle:@"Edit".localized];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
        [edit addItem:[self itemWithTitle:@"Undo".localized target:nil action:@selector(undo:) keyEquivalent:@"z"]];
        [edit addItem:[self itemWithTitle:@"Redo".localized target:nil action:@selector(redo:) keyEquivalent:@"Z"]];
#pragma clang diagnostic pop
        [edit addItem:NSMenuItem.separatorItem];
        [edit addItem:[self itemWithTitle:@"Cut".localized target:nil action:@selector(cut:) keyEquivalent:@"x"]];
        [edit addItem:[self itemWithTitle:@"Copy".localized target:nil action:@selector(copy:) keyEquivalent:@"c"]];
        [edit addItem:[self itemWithTitle:@"Paste".localized target:nil action:@selector(paste:) keyEquivalent:@"v"]];
        [edit addItem:[self itemWithTitle:@"Delete".localized target:nil action:@selector(delete:)]];
        [edit addItem:[self itemWithTitle:@"Sellect All".localized target:nil action:@selector(selectAll:) keyEquivalent:@"a"]];
        // View
        NSMenu *view = [self menuWithTitle:@"View".localized];
        [view addItem:[self itemWithTitle:@"" target:nil action:@selector(toggleFullScreen:) keyEquivalent:@"f" eventModifier:NSEventModifierFlagControl|NSEventModifierFlagCommand]];
        // Window
        NSMenu *window = [self menuWithTitle:@"Window".localized];
        NSApp.windowsMenu = window;
        [window addItem:[self itemWithTitle:@"Minimize".localized target:nil action:@selector(performMiniaturize:) keyEquivalent:@"m"]];
        [window addItem:[self itemWithTitle:@"Zoom".localized target:nil action:@selector(performZoom:)]];
        [window addItem:NSMenuItem.separatorItem];
        [window addItem:[self itemWithTitle:@"Bring All to Front".localized target:NSApp action:@selector(arrangeInFront:)]];
        // Help
        NSMenu *help = [self menuWithTitle:@"Help".localized];
        NSApp.helpMenu = help;
        [help addItem:[self itemWithTitle:[NSString stringWithFormat:@"%@ Help".localized, name] target:NSApp action:@selector(showHelp:) keyEquivalent:@"?"]];
    }
    return self;
}

#pragma mark - Private Methods
- (NSMenu *)menuWithTitle:(NSString *)title {
    return [self menuWithTitle:title parent:self];
}

- (NSMenu *)menuWithTitle:(NSString *)title parent:(NSMenu *)parent {
    NSMenuItem *item = [NSMenuItem new];
    item.title = title;
    NSMenu *menu = [NSMenu new];
    item.submenu = menu;
    menu.title = title;
    [parent addItem:item];
    return item.submenu;
}

- (NSMenuItem *)itemWithTitle:(NSString *)title target:(nullable id)target action:(SEL)action {
    return [self itemWithTitle:title target:target action:action keyEquivalent:@""];
}

- (NSMenuItem *)itemWithTitle:(NSString *)title target:(nullable id)target action:(SEL)action keyEquivalent:(NSString *)charCode {
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:charCode];
    item.target = target;
    return item;
}

- (NSMenuItem *)itemWithTitle:(NSString *)title target:(nullable id)target action:(SEL)action keyEquivalent:(NSString *)charCode eventModifier:(NSEventModifierFlags)flags {
    NSMenuItem *item = [self itemWithTitle:title target:target action:action keyEquivalent:charCode];
    item.keyEquivalentModifierMask = flags;
    return item;
}


@end
