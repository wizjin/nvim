//
//  NVTCPClient.m
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import "NVTCPClient.h"
#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <netdb.h>

@interface NVTCPClient () {
@private
    int skt;
}

@property (nonatomic, readonly, strong) NSString *host;
@property (nonatomic, readonly, assign) int port;

@end

@implementation NVTCPClient

- (instancetype)init {
    if (self = [super init]) {
        skt = INVALID_SOCKET;
        _host = @"localhost";
        _port = 6666;

        [self connect];
    }
    return self;
}

- (void)connect {
    [self close];
    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res = NULL;
    const char *hostname = self.host.cstr;
    char port[32] = {0};
    snprintf(port, sizeof(port), "%d", self.port);
    int err = getaddrinfo(hostname, port, &hints, &res);
    if (err != 0) {
        NVLogW("TCP client lookup failed - %s: %s", self.host.cstr, gai_strerror(err));
    } else {
        for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
            int s = socket(p->ai_family, res->ai_socktype, p->ai_protocol);
            if (s >= 0) {
                switch (p->ai_family) {
                    case AF_INET:
                        ((struct sockaddr_in *)p->ai_addr)->sin_port = htons(self.port);
                    case AF_INET6:
                        ((struct sockaddr_in6 *)p->ai_addr)->sin6_port = htons(self.port);
                }
                if (connect(s, p->ai_addr, p->ai_addrlen) != 0) {
                    close(s);
                } else {
                    skt = s;
                    NVLogI("TCP client connect success - %s:%d", hostname, self.port);
                    break;
                }
            }
        }
    }
    if (res != NULL) {
        freeaddrinfo(res);
    }
    if (skt == INVALID_SOCKET) {
        NVLogW("TCP client connect failed - %s:%d : %s", hostname, self.port, strerror(errno));
    }
}

- (void)close {
    if (skt != INVALID_SOCKET) {
        NVLogD("Close tcp client success - %s:%d", self.host.cstr, self.port);
        close(skt);
        skt = INVALID_SOCKET;
    }
}


@end
