//
//  NVSTDClient.m
//  NVim
//
//  Created by wizjin on 2023/9/22.
//

#import "NVSTDClient.h"

@interface NVSTDClient ()

@property (nonatomic, readonly, strong) NSString *path;

@end

@implementation NVSTDClient

- (instancetype)initWithPath:(NSString *)path {
    if (self = [super init]) {
        _path = path;
        [self open];
    }
    return self;
}

- (NSString *)info {
    return self.path;
}

- (void)open {
    [self close];
}


@end
