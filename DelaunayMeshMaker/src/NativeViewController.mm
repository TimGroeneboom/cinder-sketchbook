#import "NativeViewController.h"

#import <MobileCoreServices/UTCoreTypes.h>

#include "cinder/app/AppCocoaTouch.h"

@interface NativeViewController ()

@property (nonatomic) UIButton *infoButton;

- (NSArray *)tabBarItems;
- (void)barButtonTapped:(UIBarButtonItem *)sender;
- (void)addEmptyViewControllerToFront;
- (void)infoButtonWasTapped:(id)sender;

@end

@interface MyTableViewController : UITableViewController

@property (nonatomic, weak ) UINavigationController *parentNavigationController;

@end

@implementation NativeViewController

// note: these synthesizers aren't necessary with Clang 3.0 since it will autogenerate the same thing, but it is added for clarity
@synthesize infoButton = _infoButton;
@synthesize infoButtonCallback = _infoButtonCallback;
@synthesize pictureTaken = _pictureTaken;

// -------------------------------------------------------------------------------------------------
#pragma mark - Public: Adding CinderView to Heirarchy

// Note: changing the parent viewcontroller may inhibit the App's orientation related signals.
// To regain signals like willRotate and didRotate, emit them from this view controller

// Get CinderView's parent view controller so we can add it to a our stack, then set some navigation items
- (void)addCinderViewToFront
{
	UIViewController *cinderViewParent = ci::app::getWindow()->getNativeViewController();

	self.viewControllers = @[cinderViewParent];

    
    [self.navigationBar setHidden:YES];
	cinderViewParent.title = @"";
    /*cinderViewParent.
	cinderViewParent.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:self.infoButton];*/
	cinderViewParent.toolbarItems = [self tabBarItems];
}

// Get this app's CinderView and add it as a child in our view heirarchy, in this case the left nav bar button.
// Manually resizing is necessary.
- (void)addCinderViewAsBarButton
{
	[self addEmptyViewControllerToFront];
	UIViewController *front = self.viewControllers[0];
	UIView *cinderView = (__bridge UIView *)ci::app::getWindow()->getNative();
	cinderView.frame = CGRectMake( 0, 0, 60, self.navigationBar.frame.size.height );
	front.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:cinderView];
}

// -------------------------------------------------------------------------------------------------
#pragma mark - UIViewController overridden methods

- (void)viewDidLoad
{
	[super viewDidLoad];

	self.toolbarHidden = NO;
	UIColor *tintColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
	self.navigationBar.tintColor = tintColor;

	self.toolbar.tintColor = tintColor;

	_infoButton = [UIButton buttonWithType:UIButtonTypeInfoLight];
	[_infoButton addTarget:self action:@selector(infoButtonWasTapped:) forControlEvents:UIControlEventTouchUpInside];
    
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	NSLog(@"%@ will rotate", NSStringFromClass([self class]));
	ci::app::AppCocoaTouch::get()->emitWillRotate();
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	ci::app::AppCocoaTouch::get()->emitDidRotate();
}

// pre iOS 6
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
	return YES;
}

// iOS 6+
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
- (NSUInteger)supportedInterfaceOrientations
{
    BOOL deviceIsPad = ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad);
    
    if( deviceIsPad )
        return UIInterfaceOrientationMaskLandscape;
    else
        return UIInterfaceOrientationMaskPortrait;
}
#endif

// -------------------------------------------------------------------------------------------------
#pragma mark - Private UI

- (void)addEmptyViewControllerToFront
{
	UIViewController *emptyViewController = [UIViewController new];
	emptyViewController.title = @"Empty VC";
	emptyViewController.view.backgroundColor = [UIColor clearColor];
    
	self.viewControllers = @[emptyViewController];
	emptyViewController.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:self.infoButton];
	emptyViewController.toolbarItems = [self tabBarItems];
}

- (NSArray *)tabBarItems
{
	UIBarButtonItem *button = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCamera target:self action: @selector(barButtonTapped:)];

	return @[button];
}

- (void)barButtonTapped:(UIBarButtonItem *)sender
{
    [self startCameraControllerFromViewController: self
                                    usingDelegate: self];
}

- (BOOL) startCameraControllerFromViewController: (UIViewController*) controller
                                   usingDelegate: (id <UIImagePickerControllerDelegate,
                                                   UINavigationControllerDelegate>) delegate {
    
    if (([UIImagePickerController isSourceTypeAvailable:
          UIImagePickerControllerSourceTypeCamera] == NO)
        || (delegate == nil)
        || (controller == nil))
        return NO;
    
    
    UIImagePickerController *cameraUI = [[UIImagePickerController alloc] init];
    cameraUI.sourceType = UIImagePickerControllerSourceTypeCamera;
    
    // Displays a control that allows the user to choose picture or
    // movie capture, if both are available:
    /*
    cameraUI.mediaTypes =
    [UIImagePickerController availableMediaTypesForSourceType:
     UIImagePickerControllerSourceTypeCamera];*/
    
    // Hides the controls for moving & scaling pictures, or for
    // trimming movies. To instead show the controls, use YES.
    cameraUI.allowsEditing = NO;
    
    cameraUI.delegate = delegate;
    
    [self presentModalViewController: cameraUI animated: YES];
    
    return YES;
}

- (void) imagePickerController: (UIImagePickerController *) picker
 didFinishPickingMediaWithInfo: (NSDictionary *) info {
    
    NSString *mediaType = [info objectForKey: UIImagePickerControllerMediaType];
    UIImage *originalImage, *editedImage, *imageToSave;
    
    // Handle a still image capture
    if (CFStringCompare ((CFStringRef) mediaType, kUTTypeImage, 0)
        == kCFCompareEqualTo) {
        
        editedImage = (UIImage *) [info objectForKey:
                                   UIImagePickerControllerEditedImage];
        originalImage = (UIImage *) [info objectForKey:
                                     UIImagePickerControllerOriginalImage];
        
        if (editedImage) {
            imageToSave = editedImage;
        } else {
            imageToSave = originalImage;
        }
        
        // Save the new image (original or edited) to the Camera Roll
        //UIImageWriteToSavedPhotosAlbum (imageToSave, nil, nil , nil);
        
        CGRect screenRect = [[UIScreen mainScreen] bounds];
        CGFloat screenWidth = screenRect.size.width;
        CGFloat screenHeight = screenRect.size.height;
        imageToSave = [self imageWithImage:imageToSave scaledToSize:CGSizeMake(screenHeight * 2, screenWidth * 2)];
        
        if( _pictureTaken )
            _pictureTaken( cinder::cocoa::convertUiImage(imageToSave, true) );
        
        [self dismissModalViewControllerAnimated:YES];
    }
}
         
 - (UIImage *)imageWithImage:(UIImage *)image scaledToSize:(CGSize)newSize {
     //UIGraphicsBeginImageContext(newSize);
     UIGraphicsBeginImageContextWithOptions(newSize, NO, 0.0);
     [image drawInRect:CGRectMake(0, 0, newSize.width, newSize.height)];
     UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
     UIGraphicsEndImageContext();
     return newImage;
 }

@end

@implementation MyTableViewController

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *MyIdentifier = @"cell";
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:MyIdentifier];
	if( cell == nil ) {
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:MyIdentifier];
		cell.backgroundView = [UIView new];
		ci::Color color( ci::CM_HSV, indexPath.row / 20.0f, 0.8f, 0.8f );
		cell.backgroundView.backgroundColor = [UIColor colorWithRed:color.r green:color.g blue:color.b alpha:1.0];
		cell.selectionStyle = UITableViewCellSelectionStyleGray;
	}
	cell.textLabel.text = [NSString stringWithFormat:@"%d", indexPath.row];
	return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	return 20;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
	UIColor *color = cell.backgroundView.backgroundColor;

	self.parentNavigationController.navigationBar.tintColor = color;
	self.parentNavigationController.toolbar.tintColor = color;
	[cell setSelected:NO animated:YES];
}

@end