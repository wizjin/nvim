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
@property (nonatomic, readonly, strong) NSPipe *inPipe;
@property (nonatomic, readonly, strong) NSPipe *outPipe;

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
    return self.path;
}

- (void)open {
    [self close];
    
    NSString *executablePath = self.path;
    if (![NSFileManager.defaultManager fileExistsAtPath:executablePath]) {
        executablePath = [NSBundle.mainBundle pathForAuxiliaryExecutable:@"bin/nvim"];
    }
    
    NSError *error = nil;
    _inPipe = [NSPipe pipe];
    _outPipe = [NSPipe pipe];
    _task = [NSTask new];
    self.task.executableURL = [NSURL fileURLWithPath:executablePath];
    self.task.arguments = @[@"--embed"];
    self.task.environment = getEnvironment();
    self.task.standardInput = self.inPipe;
    self.task.standardOutput = self.outPipe;
    if (![self.task launchAndReturnError:&error]) {
        NVLogW("Launch neovim failed: %s", error.description.cstr);
    } else {
        NVLogI("Launch neovim success: %s", self.path.cstr);
        [self openWithRead:self.outPipe.fileHandleForReading.fileDescriptor write:self.inPipe.fileHandleForWriting.fileDescriptor];
    }
}

- (void)close {
    [super close];
    if (self.task != nil) {
        [self.task terminate];
        [self.task waitUntilExit];
        _task = nil;
    }
    _inPipe = nil;
    _outPipe = nil;
}

static inline NSDictionary<NSString *, NSString *> *getEnvironment(void) {
    NSMutableDictionary<NSString *, NSString *> *environment = [NSMutableDictionary dictionaryWithDictionary:NSProcessInfo.processInfo.environment];
    struct passwd* pwd = getpwuid(getuid());
    if (pwd != NULL) {
        [environment setValue:[[NSString alloc] initWithCString:pwd->pw_dir encoding:NSUTF8StringEncoding] forKey:@"HOME"];
    }
    return environment;
}


@end
