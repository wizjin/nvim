//
//  NSMenu+NVExt.h
//  NVim
//
//  Created by wizjin on 2023/11/1.
//

#import <AppKit/NSMenu.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSMenu (NVExt)

- (NSMenu *)addSubMenu:(NSString *)title;
- (void)addSeparatorItem;


@end

NS_ASSUME_NONNULL_END
