#pragma once

#import <UIKit/UIKit.h>

#include "cinder/Function.h"
#include "cinder/Surface.h"

@interface NativeViewController : UINavigationController <UIImagePickerControllerDelegate,
    UINavigationControllerDelegate>

- (void)addCinderViewToFront;
- (void)addCinderViewAsBarButton;
- (UIImage *)imageWithImage:(UIImage *)image scaledToSize:(CGSize)newSize;

@property (nonatomic) std::function<void()> infoButtonCallback;
@property (nonatomic) std::function<void(ci::Surface)> pictureTaken;
@end
