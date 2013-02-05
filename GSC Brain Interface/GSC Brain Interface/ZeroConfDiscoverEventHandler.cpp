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
    IOSViewer* viewer = dynamic_cast<IOSViewer*>(&aa);
    if (!viewer)
        return false;
    
    // forward all key + user-events to devices
    if ((ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) ||
        (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP) ||
        (ea.getEventType() == osgGA::GUIEventAdapter::USER))
    {
        for(osgViewer::View::Devices::iterator i = viewer->getDevices().begin(); i != viewer->getDevices().end(); ++i)
        {
            if ((*i)->getCapabilities() & osgGA::Device::SEND_EVENTS)
                (*i)->sendEvent(ea);
        }
    }
    
    if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
    {
        // std::cout << "user-event: " << ea.getName() << std::endl;
        
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
                removeAllSendingOSCDevices(viewer);
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
                removeAllSendingOSCDevices(viewer);
                _discoveredDevice = NULL;
            }
        }
    }
    return false;
}

void ZeroConfDiscoverEventHandler::removeAllSendingOSCDevices(IOSViewer* viewer)
{
    osgViewer::View::Devices devices = viewer->getDevices();
    std::vector<osgGA::Device*> to_delete;
    
    for(osgViewer::View::Devices::iterator i = devices.begin(); i != devices.end(); ++i) {
        osgGA::Device* device(*i);
        if (device->getCapabilities() | osgGA::Device::SEND_EVENTS) {
            std::string class_name(device->className());
            if (class_name.find("OSC") != std::string::npos) {
                to_delete.push_back(device);
            }
        }
    }
    
    for(std::vector<osgGA::Device*>::iterator j = to_delete.begin(); j != to_delete.end(); ++j) {
        viewer->removeDevice(*j);
    }
    OSG_ALWAYS << "removed " << to_delete.size() << " sending OSC devices" << std::endl;

}

void ZeroConfDiscoverEventHandler::startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port)
{
    std::ostringstream ss;
    ss << host << ":" << port << ".sender.osc";
    
    if (_discoveredDevice.valid())
    {
        viewer->removeDevice(_discoveredDevice.get());
    }
    osg::ref_ptr<osgDB::Options> options = new osgDB::Options("numMessagesPerEvent=3 delayBetweenSendsInMillisecs=0");
    _discoveredDevice = osgDB::readFile<osgGA::Device>(ss.str(), options);
    if (!_discoveredDevice.valid())
    {
        viewer->setStatusText("could not get osc-device: " + ss.str());
    }
    else {
        std::cout << "sending events to " << ss.str() << std::endl;
        viewer->addDevice(_discoveredDevice.get());
    }
    viewer->sendInit();
}

