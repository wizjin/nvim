//
//  NSString+NVExt.m
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import "NSString+NVExt.h"

@implementation NSString (NVExt)

- (const char *)cstr {
    const char *p = [self cStringUsingEncoding:NSUTF8StringEncoding];
    return p != NULL ? p : "";
}


@end
