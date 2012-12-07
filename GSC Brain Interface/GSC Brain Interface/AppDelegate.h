//Created by Thomas Hogarth 2009

//force the link to our desired osgPlugins
#include "osgPlugins.h"

#include "IOSViewer.h"


#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>


@interface AppDelegate : NSObject <UIApplicationDelegate> {
	
	osg::ref_ptr<IOSViewer> _app;
    CADisplayLink* _displayLink;

}

- (void)updateScene;
- (void)removeDisplayLink;


@end

