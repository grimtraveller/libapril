/************************************************************************************\
 This source file is part of the Awesome Portable Rendering Interface Library         *
 For latest info, see http://libapril.sourceforge.net/                                *
 **************************************************************************************
 Copyright (c) 2010 Ivan Vucica (ivan@vucica.net)                                     *
 *                                                                                    *
 * This program is free software; you can redistribute it and/or modify it under      *
 * the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
 \************************************************************************************/

#import "ApriliOSAppDelegate.h"
#import "main.h"
#import "AprilViewController.h"
#import "EAGLView.h"


@interface AprilDummyViewController : UIViewController
@end
@implementation AprilDummyViewController
- (void)loadView
{
    self.view = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 480, 320)] autorelease];
    self.view.autoresizesSubviews = YES;    
}
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation 
{
    //return (interfaceOrientation == UIInterfaceOrientationLandscapeLeft || interfaceOrientation == UIInterfaceOrientationLandscapeRight);
    return YES;
}
@end



@implementation ApriliOSAppDelegate

@synthesize window;
@synthesize viewController;
@synthesize onPushRegistrationSuccess;
@synthesize onPushRegistrationFailure;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
	
	[[NSFileManager defaultManager] changeCurrentDirectoryPath: [[NSBundle mainBundle] resourcePath]];
    [[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationLandscapeLeft animated:NO];

	// create a window.
	// early creation so Default.png can be displayed while we're waiting for 
	// game initialization
	window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    window.autoresizesSubviews = YES;
    
    // add dummy view controller, so that shouldAutorotate is respected on old devices
    dummyViewController = [[UIViewController alloc] init];
    [window addSubview:dummyViewController.view];
    
	// viewcontroller will automatically add imageview
	viewController = [[[AprilViewController alloc] init] autorelease];
    [dummyViewController presentModalViewController:viewController animated:NO];
    [[UIApplication sharedApplication] setStatusBarOrientation: UIInterfaceOrientationLandscapeLeft animated:NO];

	// set window color
	[window setBackgroundColor:[UIColor blackColor]];

	// display the window
	[window makeKeyAndVisible];

	NSLog(@"Created window");
	
    //////////
	// thanks to Kyle Poole for this trick
    // also used in latest SDL
    // quote:
    // KP: using a selector gets around the "failed to launch application in time" if the startup code takes too long
    // This is easy to see if running with Valgrind
	[self performSelector:@selector(performInit:) withObject:nil afterDelay:0.2f];
    
}

- (void)performInit:(id)object
{
    
    april_init(harray<hstr>());
    if ([[viewController.view subviews] count]) 
    {
        EAGLView* glview = [[viewController.view subviews] objectAtIndex:0];
        [glview startAnimation];
    }
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	NSLog(@"April-based application received memory warning!");
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    if (![[viewController.view subviews] count]) 
    {
        return;
    }
    for (EAGLView *glview in [viewController.view subviews]) 
    {
        if ([glview isKindOfClass:[EAGLView class]]) 
        {
            [glview stopAnimation];
            
            return;
        }
    }
    april_destroy();
}


- (void)applicationWillResignActive:(UIApplication *)application
{
    if (![[viewController.view subviews] count]) 
    {
        return;
    }

    //for (EAGLView *glview in [viewController.view subviews]) 
    {
		EAGLView *glview = viewController.view;
        if ([glview isKindOfClass:[EAGLView class]]) 
        {
            [glview applicationWillResignActive:application];
            [glview stopAnimation];

            return;
        }
    }
}
- (void)applicationDidEnterBackground:(UIApplication *)application
{
	// for our purposes, we don't need to differentiate entering background
	// from resigning activity
	[self applicationWillResignActive:application];
}
- (void)applicationDidBecomeActive:(UIApplication *)application
{
    if (![[viewController.view subviews] count]) 
    {
        return;
    }
    //for (EAGLView *glview in [viewController.view subviews]) 
    {
		EAGLView *glview = viewController.view;
        if ([glview isKindOfClass:[EAGLView class]]) 
        {
            [glview applicationDidBecomeActive:application];
            [glview startAnimation];
            
            return;
        }
    }
}
- (void)applicationWillEnterForeground:(UIApplication *)application
{
	// for our purposes, we don't need to differentiate entering foreground
	// from becoming active
	[self applicationDidBecomeActive:application];
}



///////////////////////////
// utils and handlers for apps 
// that need push notifications
///////////////////////////
#pragma mark Push notifications

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
	if(onPushRegistrationSuccess)
		onPushRegistrationSuccess(deviceToken);
}
- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
	if(onPushRegistrationFailure)
		onPushRegistrationFailure(error);
}


- (void)dealloc
{
	[super dealloc];
	self.window = nil;
}

@end