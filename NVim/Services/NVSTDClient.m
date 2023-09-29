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
        [self openWithRead:outPipe.fileHandleForReading.fileDescriptor write:inPipe.fileHandleForWriting.fileDescriptor];
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
