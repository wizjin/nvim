//
//  NSString+Localized.m
//  NVim
//
//  Created by wizjin on 2023/9/10.
//

#import "NSString+Localized.h"
#import <Foundation/Foundation.h>

@implementation NSString (Localized)

- (NSString *)localized {
    return [NSBundle.mainBundle localizedStringForKey:self ?: @"" value:@"" table:nil];
}

- (const char *)cstr {
    const char *p = [self cStringUsingEncoding:NSUTF8StringEncoding];
    return p != NULL ? p : "";
}


@end
