#import "ViewController.h"
#import "SDKWrapper.h"

@interface ViewController ()
@property (strong, nonatomic) SDKWrapper *sdkWrapper;
@property (strong, nonatomic) UIScrollView *scrollView;
@property (strong, nonatomic) UIStackView *stackView;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.title = @"SDK Test App";
    self.view.backgroundColor = [UIColor systemBackgroundColor];
    
    self.sdkWrapper = [[SDKWrapper alloc] init];
    
    [self setupUI];
}

- (void)setupUI {
    // 创建滚动视图
    self.scrollView = [[UIScrollView alloc] init];
    self.scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:self.scrollView];
    
    // 创建堆栈视图
    self.stackView = [[UIStackView alloc] init];
    self.stackView.axis = UILayoutConstraintAxisVertical;
    self.stackView.spacing = 16;
    self.stackView.alignment = UIStackViewAlignmentFill;
    self.stackView.distribution = UIStackViewDistributionFill;
    self.stackView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.scrollView addSubview:self.stackView];
    
    // 创建日志文本视图
    self.logTextView = [[UITextView alloc] init];
    self.logTextView.backgroundColor = [UIColor systemGray6Color];
    self.logTextView.font = [UIFont fontWithName:@"Menlo" size:12];
    self.logTextView.editable = NO;
    self.logTextView.text = @"SDK Test App Ready\n";
    self.logTextView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.stackView addArrangedSubview:self.logTextView];
    
    // 创建按钮
    self.initButton = [self createButtonWithTitle:@"Initialize SDK" action:@selector(initSDK:)];
    [self.stackView addArrangedSubview:self.initButton];
    
    self.testThreadPoolButton = [self createButtonWithTitle:@"Test Thread Pool" action:@selector(testThreadPool:)];
    self.testThreadPoolButton.enabled = NO;
    [self.stackView addArrangedSubview:self.testThreadPoolButton];
    
    self.testHttpButton = [self createButtonWithTitle:@"Test HTTP Client" action:@selector(testHttp:)];
    self.testHttpButton.enabled = NO;
    [self.stackView addArrangedSubview:self.testHttpButton];
    
    self.testLogButton = [self createButtonWithTitle:@"Test Logging" action:@selector(testLog:)];
    self.testLogButton.enabled = NO;
    [self.stackView addArrangedSubview:self.testLogButton];
    
    self.shutdownButton = [self createButtonWithTitle:@"Shutdown SDK" action:@selector(shutdownSDK:)];
    self.shutdownButton.enabled = NO;
    [self.stackView addArrangedSubview:self.shutdownButton];
    
    // 设置约束
    [NSLayoutConstraint activateConstraints:@[
        [self.scrollView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [self.scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:16],
        [self.scrollView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-16],
        [self.scrollView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
        
        [self.stackView.topAnchor constraintEqualToAnchor:self.scrollView.topAnchor],
        [self.stackView.leadingAnchor constraintEqualToAnchor:self.scrollView.leadingAnchor],
        [self.stackView.trailingAnchor constraintEqualToAnchor:self.scrollView.trailingAnchor],
        [self.stackView.bottomAnchor constraintEqualToAnchor:self.scrollView.bottomAnchor],
        [self.stackView.widthAnchor constraintEqualToAnchor:self.scrollView.widthAnchor],
        
        [self.logTextView.heightAnchor constraintEqualToConstant:200]
    ]];
}

- (UIButton *)createButtonWithTitle:(NSString *)title action:(SEL)action {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    button.backgroundColor = [UIColor systemBlueColor];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    button.layer.cornerRadius = 8;
    button.titleLabel.font = [UIFont systemFontOfSize:16 weight:UIFontWeightMedium];
    [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
    
    [button.heightAnchor constraintEqualToConstant:44].active = YES;
    
    return button;
}

- (void)appendLog:(NSString *)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString *timestamp = [NSDateFormatter localizedStringFromDate:[NSDate date]
                                                             dateStyle:NSDateFormatterNoStyle
                                                             timeStyle:NSDateFormatterMediumStyle];
        NSString *logEntry = [NSString stringWithFormat:@"[%@] %@\n", timestamp, message];
        self.logTextView.text = [self.logTextView.text stringByAppendingString:logEntry];
        
        // 滚动到底部
        NSRange bottom = NSMakeRange(self.logTextView.text.length - 1, 1);
        [self.logTextView scrollRangeToVisible:bottom];
    });
}

- (IBAction)initSDK:(id)sender {
    [self appendLog:@"Initializing SDK..."];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        BOOL success = [self.sdkWrapper initializeSDK];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (success) {
                [self appendLog:@"SDK initialized successfully"];
                self.initButton.enabled = NO;
                self.testThreadPoolButton.enabled = YES;
                self.testHttpButton.enabled = YES;
                self.testLogButton.enabled = YES;
                self.shutdownButton.enabled = YES;
                
                NSString *version = [self.sdkWrapper getSDKVersion];
                NSString *platform = [self.sdkWrapper getPlatformInfo];
                [self appendLog:[NSString stringWithFormat:@"SDK Version: %@", version]];
                [self appendLog:[NSString stringWithFormat:@"Platform: %@", platform]];
            } else {
                [self appendLog:@"SDK initialization failed"];
            }
        });
    });
}

- (IBAction)testThreadPool:(id)sender {
    [self appendLog:@"Testing thread pool..."];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self.sdkWrapper testThreadPool:^(NSString *result) {
            [self appendLog:result];
        }];
    });
}

- (IBAction)testHttp:(id)sender {
    [self appendLog:@"Testing HTTP client..."];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self.sdkWrapper testHttpClient:^(NSString *result) {
            [self appendLog:result];
        }];
    });
}

- (IBAction)testLog:(id)sender {
    [self appendLog:@"Testing logging system..."];
    
    [self.sdkWrapper testLogging:^(NSString *result) {
        [self appendLog:result];
    }];
}

- (IBAction)shutdownSDK:(id)sender {
    [self appendLog:@"Shutting down SDK..."];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self.sdkWrapper shutdownSDK];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [self appendLog:@"SDK shutdown completed"];
            self.initButton.enabled = YES;
            self.testThreadPoolButton.enabled = NO;
            self.testHttpButton.enabled = NO;
            self.testLogButton.enabled = NO;
            self.shutdownButton.enabled = NO;
        });
    });
}

@end
