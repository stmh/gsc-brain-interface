//
//  main.cpp
//  gsc brain interface osx
//
//  Created by Stephan Huber on 07.12.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#include <iostream>
#include <stdlib.h>
#include "IOSViewer.h"
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[])
{
     
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    
    //osg::ref_ptr<osg::Image> img = osgDB::readImageFile("/Users/stephan/Desktop/present3DdScreenSnapz002.png");
    //osgDB::writeImageFile(*img, "/Users/stephan/Desktop/tetsttt.png");
    
    IOSViewer* viewer = new IOSViewer();
    viewer->addDataFolder("/Users/stephan/Documents/gsc-brain-ios-interface/GSC Brain Interface");
    viewer->addDataFolder("/Users/stephan/Documents/gsc-brain-ios-interface/GSC Brain Interface/assets");
    
    viewer->realize();
    viewer->run();
  	[pool release];
    
    return 0;
}

