//
//  IdleTimerEventHandler.h
//  GSC Brain Interface
//
//  Created by Stephan Huber on 05.12.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#pragma once

#include <osgGA/GUIEventHandler>


class IdleTimerEventHandler : public osgGA::GUIEventHandler {
public:
    IdleTimerEventHandler(double max_idle_time)
        : osgGA::GUIEventHandler()
        , _maxIdleTime()
    {
        setNewMaxIdleTime(max_idle_time);
    }
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv);
    
    void setNewMaxIdleTime(double max_idle_time);
    
private:
    double _maxIdleTime, _lastEventTimeStamp;
    
};
