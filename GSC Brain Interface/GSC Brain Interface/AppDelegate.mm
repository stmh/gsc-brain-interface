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
    _app->setDataFolder([doc_folder UTF8String]);
    _app->realize();
    _app->frame();
	
	_displayLink = [application.keyWindow.screen displayLinkWithTarget:self selector:@selector(updateScene)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}


//
//Timer called function to update our scene and render the viewer
//
- (void)updateScene {
	_app->frame();
}



- (void)applicationWillResignActive:(UIApplication *)application {

}


- (void)applicationDidBecomeActive:(UIApplication *)application {

}

-(void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    _app->handleMemoryWarning();
}

-(void)applicationWillTerminate:(UIApplication *)application{
    [_displayLink invalidate];
} 



- (void)dealloc {
    _app->cleanup();
	_app = NULL;
    [super dealloc];
}


@end
