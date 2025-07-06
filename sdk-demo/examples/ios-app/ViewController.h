#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

@property (weak, nonatomic) IBOutlet UITextView *logTextView;
@property (weak, nonatomic) IBOutlet UIButton *initButton;
@property (weak, nonatomic) IBOutlet UIButton *testThreadPoolButton;
@property (weak, nonatomic) IBOutlet UIButton *testHttpButton;
@property (weak, nonatomic) IBOutlet UIButton *testLogButton;
@property (weak, nonatomic) IBOutlet UIButton *shutdownButton;

- (IBAction)initSDK:(id)sender;
- (IBAction)testThreadPool:(id)sender;
- (IBAction)testHttp:(id)sender;
- (IBAction)testLog:(id)sender;
- (IBAction)shutdownSDK:(id)sender;

@end
