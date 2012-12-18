//
//  ZeroConfDiscoverEventHandler.cpp
//  GSC Brain Interface
//
//  Created by Stephan Huber on 05.12.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#include "ZeroConfDiscoverEventHandler.h"
#include <iostream>
#include <sstream>
#include <osgDB/ReadFile>
#include <osg/ValueObject>
#include "IOSViewer.h"


bool ZeroConfDiscoverEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
{
    forwardEvent(ea);
    
    if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
    {
        IOSViewer* viewer = dynamic_cast<IOSViewer*>(&aa);
        if (!viewer)
            return false;
        
        std::cout << "user-event: " << ea.getName() << std::endl;
        
        if (ea.getName() == "/zeroconf/service-added")
        {
            std::string host(""), type("");
            unsigned int port(0);
            
            ea.getUserValue("host", host);
            ea.getUserValue("port", port);
            ea.getUserValue("type", type);
            
            if (type == httpServiceType())
            {
                viewer->readScene(host, port);
            }
            else if (type == oscServiceType() && (!_discoveredDevice.valid()))
            {
                startEventForwarding(viewer, host, port);
            }
        }
        else if (ea.getName() == "/zeroconf/service-removed")
        {
            std::string type("");
            ea.getUserValue("type", type);
            
            if (type == httpServiceType())
                viewer->showMaintenanceScene();
            else if(type == oscServiceType())
            {
                removeDevice(_discoveredDevice);
                _discoveredDevice = NULL;
            }
        }
    }
    return false;
}

void ZeroConfDiscoverEventHandler::startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port)
{
    std::ostringstream ss;
    ss << host << ":" << port << ".sender.osc";
    
    if (_discoveredDevice.valid())
        removeDevice(_discoveredDevice.get());
    
    _discoveredDevice = osgDB::readFile<osgGA::Device>(ss.str());
    if (!_discoveredDevice.valid())
    {
        viewer->setStatusText("could not get osc-device: " + ss.str());
    }
    else {
        std::cout << "sending events to " << ss.str() << std::endl;
        addDevice(_discoveredDevice.get());
    }
    sendInit();
}

void ZeroConfDiscoverEventHandler::forwardEvent(const osgGA::GUIEventAdapter &ea)
{
    for(DeviceList::iterator i = _devices.begin(); i != _devices.end(); ++i)
    {
        (*i)->sendEvent(ea);
    }
}

void ZeroConfDiscoverEventHandler::sendInit()
{
    OSG_NOTICE << "ZeroConfDiscoverEventHandler::sendInit" << std::endl;
    
    // send an resize event
    {
        osg::ref_ptr<osgGA::GUIEventAdapter> ea = new osgGA::GUIEventAdapter();
        ea->setEventType(osgGA::GUIEventAdapter::RESIZE);
        ea->setWindowRectangle(0, 0, 1024, 768);
        forwardEvent(*ea.get());
    }
    // send a keypress + -release of the space-bar
    {
        osg::ref_ptr<osgGA::GUIEventAdapter> ea = new osgGA::GUIEventAdapter();
        ea->setEventType(osgGA::GUIEventAdapter::KEYUP);
        ea->setKey(' ');
        forwardEvent(*ea.get());
        ea->setEventType(osgGA::GUIEventAdapter::KEYDOWN);
        ea->setKey(' ');
        forwardEvent(*ea.get());
    }
}