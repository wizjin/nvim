//
//  NSMenu+NVExt.m
//  NVim
//
//  Created by wizjin on 2023/11/1.
//

#import "NSMenu+NVExt.h"

@implementation NSMenu (NVExt)

- (NSMenu *)addSubMenu:(NSString *)title {
    NSMenuItem *item = [NSMenuItem new];
    item.title = title;
    NSMenu *menu = [NSMenu new];
    item.submenu = menu;
    menu.title = title;
    [self addItem:item];
    return item.submenu;
}

- (void)addSeparatorItem {
    [self addItem:NSMenuItem.separatorItem];
}


@end
