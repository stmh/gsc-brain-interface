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
    typedef std::vector<osg::ref_ptr< osgGA::Device > > DeviceList;
    
    ZeroConfDiscoverEventHandler() : osgGA::GUIEventHandler() {}
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv);
    
    void startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port);
    
    static const char* httpServiceType() { return "_p3d_http._tcp"; }
    static const char* oscServiceType() { return "_p3d_osc._udp"; }
    void addDevice(osgGA::Device* device)
    {
        _devices.push_back(device);
    }
    
    void removeDevice(osgGA::Device* device)
    {
        DeviceList::iterator itr = std::find(_devices.begin(), _devices.end(), device);
        if (itr != _devices.end())
            _devices.erase(itr);
    }
    
    void removeAllDevices() { _devices.clear(); }
    
private:
    osg::ref_ptr<osgGA::Device> _discoveredDevice;
    DeviceList _devices;
};