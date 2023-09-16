//
//  NSString+NVExt.h
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import <Foundation/NSString.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSString (NVExt)

@property (nonatomic, readonly, assign) const char *cstr;


@end

NS_ASSUME_NONNULL_END
