//
//  NVClient.m
//  NVim
//
//  Created by wizjin on 2023/9/13.
//

#import "NVClient.h"
#import "cwpack.h"

@implementation NVClient

- (instancetype)init {
    if (self = [super init]) {
        uint8_t buffer[20];
        cw_pack_context pc;
        cw_pack_context_init (&pc, buffer, sizeof(buffer), 0);
        cw_pack_unsigned(&pc, 12345);

        cw_unpack_context uc;
        cw_unpack_context_init(&uc, pc.start, pc.current - pc.start, 0);
        cw_unpack_next(&uc);
        NVLogI("CWPack unpack value = %lld", uc.item.as.u64);
    }
    return self;
}


@end
