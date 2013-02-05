//
//  ZeroConfDiscoverEventHandler.h
//  GSC Brain Interface
//
//  Created by Stephan Huber on 05.12.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#pragma once

#include <osgGA/GUIEventHandler>
#include <osgGA/Device>
#include <algorithm>

class IOSViewer;

class ZeroConfDiscoverEventHandler : public osgGA::GUIEventHandler {
public:    
    ZeroConfDiscoverEventHandler() : osgGA::GUIEventHandler() {}
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv);
    
    void startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port);
    
    void removeAllSendingOSCDevices(IOSViewer* viewer);
    
    static const char* httpServiceType() { return "_p3d_http._tcp"; }
    static const char* oscServiceType() { return "_p3d_osc._udp"; }
            
private:
    
    osg::ref_ptr<osgGA::Device> _discoveredDevice;
};