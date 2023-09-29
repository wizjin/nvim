//
//  NVTCPClient.m
//  NVim
//
//  Created by wizjin on 2023/9/16.
//

#import "NVTCPClient.h"
#import <sys/types.h>
#import <sys/ioctl.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <netdb.h>
#import "nvc_ui.h"

@interface NVTCPClient ()

@property (nonatomic, readonly, strong) NSString *host;
@property (nonatomic, readonly, assign) int port;

@end

@implementation NVTCPClient

- (instancetype)initWithHost:(NSString *)host port:(int)port {
    if (self = [super init]) {
        _host = host;
        _port = port;
        [self open];
    }
    return self;
}

- (NSString *)info {
    return [self.host stringByAppendingFormat:@":%d", self.port];
}

- (void)open {
    [self close];

    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *infos = NULL;
    const char *hostname = self.host.cstr;
    char port[32] = {0};
    snprintf(port, sizeof(port), "%d", self.port);
    int skt = INVALID_SOCKET;
    int err = getaddrinfo(hostname, port, &hints, &infos);
    if (err != 0) {
        NVLogW("TCP Client lookup failed - %s: %s", self.host.cstr, gai_strerror(err));
    } else {
        for (struct addrinfo *p = infos; p != NULL; p = p->ai_next) {
            int s = nv_tcp_client_config_socket(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
            if (s != INVALID_SOCKET) {
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
                    NVLogI("TCP Client connect success - %s", self.info.cstr);
                    break;
                }
            }
        }
    }
    if (infos != NULL) {
        freeaddrinfo(infos);
    }
    if (skt == INVALID_SOCKET) {
        NVLogW("TCP Client create connection failed - %s : %s", self.info.cstr, strerror(errno));
    } else {
        [self openWithRead:skt write:skt];
        close(skt);
    }
}

#pragma mark - Socket Helper
static inline int nv_tcp_client_config_socket_linger(int skt) {
    static const struct linger so_linger = { 1, 1 };
    return setsockopt(skt, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
}

static inline int nv_tcp_client_config_socket_nosigpipe(int skt) {
    static const int on = 1;
    return setsockopt(skt, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
}

static inline int nv_tcp_client_config_socket_keepalive(int skt) {
    static const int on = 1;
    return setsockopt(skt, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
}

static inline int nv_tcp_client_config_socket_tcp_nodelay(int skt) {
    static const int on = 1;
    return setsockopt(skt, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
}

typedef int (*nv_tcp_client_config_socket_handler)(int);

static const nv_tcp_client_config_socket_handler nv_tcp_client_config_socket_handlers[] = {
    nv_tcp_client_config_socket_linger,
    nv_tcp_client_config_socket_nosigpipe,
    nv_tcp_client_config_socket_keepalive,
    nv_tcp_client_config_socket_tcp_nodelay,
};

static inline int nv_tcp_client_config_socket(int skt) {
    if (skt != INVALID_SOCKET) {
        for (int i = 0; i < countof(nv_tcp_client_config_socket_handlers); i++) {
            if (nv_tcp_client_config_socket_handlers[i](skt) != 0) {
                NVLogW("TCP Client config (%d) socket failed: %s", i, strerror(errno));
                close(skt);
                skt = INVALID_SOCKET;
                break;
            }
        }
    }
    return skt;
}


@end
