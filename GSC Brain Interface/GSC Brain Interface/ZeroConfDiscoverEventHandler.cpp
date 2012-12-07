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


bool ZeroConfDiscoverEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
{
    if (_oscDevice.valid())
    {
        _oscDevice->sendEvent(ea);
    }
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
            else if (type == oscServiceType() && (!_oscDevice.valid()))
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
                _oscDevice = NULL;
        }
    }
    return false;
}

void ZeroConfDiscoverEventHandler::startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port)
{
    std::ostringstream ss;
    ss << host << ":" << port << ".sender.osc";
    
    _oscDevice = osgDB::readFile<osgGA::Device>(ss.str());
    if (!_oscDevice.valid())
    {
        viewer->setStatusText("could not get osc-device: " + ss.str());
    } else {
        std::cout << "sending events to " << ss.str() << std::endl;
    }
}