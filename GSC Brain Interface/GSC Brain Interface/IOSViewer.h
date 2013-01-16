//
//  IOSViewer.h
//  Present3D
//
//  Created by Stephan Huber on 22.11.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#pragma once


#include <osgViewer/Viewer>
#include <osgText/Text>
#include <osg/ImageStream>
#include "ZeroConfDiscoverEventHandler.h"
#include "IdleTimerEventHandler.h"

#include "TargetConditionals.h"

#if TARGET_OS_IPHONE
#define TESTING 1
#endif 

class IOSViewer : public osgViewer::Viewer {
public:
    IOSViewer();
    void addDataFolder(const std::string& folder);
    void realize();
    void cleanup();
    void handleMemoryWarning();
    
    void checkForLocalFile();
    
    void readScene(const std::string& host, unsigned int port);
    void setStatusText(const std::string& status);
    
    void showMaintenanceScene();
    
    virtual void setSceneData(osg::Node* node);
    
    void wakeUp();
    void sendInit();
    
    virtual void frame (double simulationTime=USE_REFERENCE_TIME) {
        showMem("before frame");
        osgViewer::Viewer::frame(simulationTime);
        showMem("after frame");
    }
    
protected:
    void showMem(const std::string& msg);
    osg::Node* setupHud();
    void checkEnvVars();
    
    
    
private:
    osg::ref_ptr<osg::Node>     _maintenanceScene;
    osg::ref_ptr<osgText::Text> _statusText;
    osg::ref_ptr<osg::ImageStream> _maintenanceMovie;
    bool _sceneLoaded, _isLocalScene;
    osg::ref_ptr<ZeroConfDiscoverEventHandler> _zeroconfEventHandler;
    osg::ref_ptr<IdleTimerEventHandler> _idleTimerEventHandler;

};