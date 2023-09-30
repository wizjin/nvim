//
//  NVSTDClient.m
//  NVim
//
//  Created by wizjin on 2023/9/22.
//

#import "NVSTDClient.h"
#import <pwd.h>

@interface NVSTDClient ()

@property (nonatomic, readonly, strong) NSString *path;
@property (nonatomic, readonly, strong) NSTask *task;

@end

@implementation NVSTDClient

- (instancetype)init {
    return [self initWithPath:@""];
}

- (instancetype)initWithPath:(NSString *)path {
    if (self = [super init]) {
        _path = path;
        [self open];
    }
    return self;
}

- (NSString *)info {
    return self.path.length > 0 ? self.path : @"<embedded nvim>";
}

- (void)open {
    [self close];

    NSString *executablePath = self.path;
    if (![NSFileManager.defaultManager fileExistsAtPath:executablePath]) {
        executablePath = [NSBundle.mainBundle pathForAuxiliaryExecutable:@"bin/nvim"];
    }

    NSError *error = nil;
    NSPipe *inPipe = [NSPipe pipe];
    NSPipe *outPipe = [NSPipe pipe];
    NSTask *task = [NSTask new];
    _task = task;
    task.executableURL = [NSURL fileURLWithPath:executablePath];
    task.arguments = @[@"--embed"];
    task.environment = getEnvironment();
    task.currentDirectoryURL = getWorkDirectoryURL(task.currentDirectoryURL);
    task.standardInput = inPipe;
    task.standardOutput = outPipe;
    task.standardError = nil;
    if (![task launchAndReturnError:&error]) {
        NVLogW("Launch neovim failed: %s", error.description.cstr);
    } else {
        NVLogI("Launch neovim success - %s", self.info.cstr);
        [self openWithRead:nv_std_client_config_fd(outPipe.fileHandleForReading.fileDescriptor)
                     write:nv_std_client_config_fd(inPipe.fileHandleForWriting.fileDescriptor)];
    }
}

- (void)close {
    [super close];
    if (self.task != nil) {
        [self.task terminate];
        [self.task waitUntilExit];
        _task = nil;
    }
}

#pragma mark - Helper
typedef int (*nv_std_client_config_fd_handler)(int);

static inline int nv_std_client_config_fd_nosigpipe(int fd) {
    return fcntl(fd, F_SETNOSIGPIPE, 1);
}

static const nv_std_client_config_fd_handler nv_std_client_config_fd_handlers[] = {
    nv_std_client_config_fd_nosigpipe,
};

static inline int nv_std_client_config_fd(int fd) {
    if (fd != INVALID_SOCKET) {
        for (int i = 0; i < countof(nv_std_client_config_fd_handlers); i++) {
            if (nv_std_client_config_fd_handlers[i](fd) != 0) {
                NVLogW("STD Client config (%d) fd failed: %s", i, strerror(errno));
                close(fd);
                fd = INVALID_SOCKET;
                break;
            }
        }
    }
    return fd;
}

static inline NSString *getHomeDirectory(void) {
    NSString *home = nil;
    struct passwd* pwd = getpwuid(getuid());
    if (pwd != NULL) {
        home = [[NSString alloc] initWithCString:pwd->pw_dir encoding:NSUTF8StringEncoding];
    }
    return home;
}

static inline NSURL *getWorkDirectoryURL(NSURL *current) {
    NSString *home = getHomeDirectory();
    if (home.length > 0) {
        current = [NSURL fileURLWithPath:home];
    }
    return current;
}

static inline NSDictionary<NSString *, NSString *> *getEnvironment(void) {
    NSMutableDictionary<NSString *, NSString *> *environment = [NSMutableDictionary dictionaryWithDictionary:NSProcessInfo.processInfo.environment];
    NSString *home = getHomeDirectory();
    if (home.length > 0) {
        [environment setValue:home forKey:@"HOME"];
    }
    return environment;
}


@end
