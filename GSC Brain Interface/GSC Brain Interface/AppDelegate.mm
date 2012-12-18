//Created by Thomas Hogarth 2009

#import "AppDelegate.h"
#ifdef TESTING
    #import "Testflight.h"
#endif


@implementation AppDelegate

//
//Called once app has finished launching, create the viewer then realize. Can't call viewer->run as will 
//block the final inialization of the windowing system
//
- (void)applicationDidFinishLaunching:(UIApplication *)application 
{
    #ifdef TESTING
    
	[TestFlight takeOff:@"8d28e8c9dd4f4488134a230b2f66c940_MTYwODIxMjAxMi0xMi0wNyAxMjozMjoxNy40MDM5ODU"];
    [TestFlight setDeviceIdentifier:[[UIDevice currentDevice] uniqueIdentifier]];
    
#endif
    
    NSString* doc_folder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    
	_app = new IOSViewer();
    _app->addDataFolder([doc_folder UTF8String]);
    _app->addDataFolder([[[NSBundle mainBundle] bundlePath] UTF8String]);
    _app->realize();
    _app->frame();
    
    _displayLink = NULL;
}


//
//Timer called function to update our scene and render the viewer
//
- (void)updateScene {
	_app->frame();
}


- (void)removeDisplayLink
{
    if (_displayLink)
        [_displayLink invalidate];
    _displayLink = NULL;

}

- (void)applicationWillResignActive:(UIApplication *)application
{
    [self removeDisplayLink];
    // _app->pause();
}


- (void)applicationDidBecomeActive:(UIApplication *)application
{    
    [self removeDisplayLink];
    
    _displayLink = [application.keyWindow.screen displayLinkWithTarget:self selector:@selector(updateScene)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    
    _app->wakeUp();
}

-(void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    _app->handleMemoryWarning();
}

-(void)applicationWillTerminate:(UIApplication *)application
{
    [self removeDisplayLink];
} 



- (void)dealloc
{
    [self removeDisplayLink];
    
    _app->cleanup();
	_app = NULL;
    [super dealloc];
}


@end
