//Created by Thomas Hogarth 2009

#import "AppDelegate.h"




@implementation AppDelegate

//
//Called once app has finished launching, create the viewer then realize. Can't call viewer->run as will 
//block the final inialization of the windowing system
//
- (void)applicationDidFinishLaunching:(UIApplication *)application 
{
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



- (void)applicationWillResignActive:(UIApplication *)application {
    [_displayLink invalidate];
    // _app->pause();
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    if (_displayLink)
        [_displayLink release];
    
    _displayLink = [application.keyWindow.screen displayLinkWithTarget:self selector:@selector(updateScene)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    // _app->unpause();
}

-(void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    _app->handleMemoryWarning();
}

-(void)applicationWillTerminate:(UIApplication *)application{
    [_displayLink invalidate];
} 



- (void)dealloc {
    [_displayLink release];
    
    _app->cleanup();
	_app = NULL;
    [super dealloc];
}


@end
