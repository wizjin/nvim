//
//  NVTCPClient.h
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import "NVClient.h"

NS_ASSUME_NONNULL_BEGIN

@interface NVTCPClient : NVClient

- (instancetype)initWithHost:(NSString *)host port:(int)port;


@end

NS_ASSUME_NONNULL_END
