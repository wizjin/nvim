//
//  NVClient.h
//  NVim
//
//  Created by wizjin on 2023/9/13.
//

#import "NVColorsSet.h"

NS_ASSUME_NONNULL_BEGIN

@class NVClient;

@protocol NVClientDelegate <NSObject>
@optional
- (void)client:(NVClient *)client updateTitle:(NSString *)title;
- (void)client:(NVClient *)client updateColorsSet:(NVColorsSet *)colorsSet;
- (void)clientFlush:(NVClient *)client;
@end

@interface NVClient : NSObject

@property (nonatomic, nullable, weak) id<NVClientDelegate> delegate;

- (void)attachUI;
- (void)detachUI;
- (void)close;


@end

NS_ASSUME_NONNULL_END
