//
//  IdleTimerEventHandler.cpp
//  GSC Brain Interface
//
//  Created by Stephan Huber on 05.12.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#include "IdleTimerEventHandler.h"
#include <osgViewer/View>


bool IdleTimerEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
{
    if (ea.getEventType() != osgGA::GUIEventAdapter::FRAME)
    {
        _lastEventTimeStamp = ea.getTime();
    }
    
    if (ea.getTime() > _lastEventTimeStamp + _maxIdleTime)
    {
        osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
        if (view)
        {
            OSG_ALWAYS << "resetting scene ... idle timeout" << std::endl;
            
            view->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_Home);
            view->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_Home);
            
            view->getEventQueue()->keyPress(' ');
            view->getEventQueue()->keyRelease(' ');
        }
        
    }
    
    return false;
}