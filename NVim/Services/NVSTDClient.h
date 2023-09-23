//
//  NVSTDClient.h
//  NVim
//
//  Created by wizjin on 2023/9/22.
//

#import "NVClient.h"

NS_ASSUME_NONNULL_BEGIN

@interface NVSTDClient : NVClient

- (instancetype)init;
- (instancetype)initWithPath:(NSString *)path;


@end

NS_ASSUME_NONNULL_END
