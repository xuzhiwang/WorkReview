#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void(^SDKTestCallback)(NSString *result);

@interface SDKWrapper : NSObject

- (BOOL)initializeSDK;
- (void)shutdownSDK;
- (NSString *)getSDKVersion;
- (NSString *)getPlatformInfo;

- (void)testThreadPool:(SDKTestCallback)callback;
- (void)testHttpClient:(SDKTestCallback)callback;
- (void)testLogging:(SDKTestCallback)callback;

@end

NS_ASSUME_NONNULL_END
