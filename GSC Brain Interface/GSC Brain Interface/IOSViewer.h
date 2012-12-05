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

class IOSViewer : public osgViewer::Viewer {
public:
    IOSViewer()
        : osgViewer::Viewer()
        , _maintenanceScene(NULL)
        , _statusText(NULL)
        , _maintenanceMovie(NULL)
    {
    }
    void addDataFolder(const std::string& folder);
    void realize();
    void cleanup();
    void handleMemoryWarning();
    
    void readScene(const std::string& host, unsigned int port);
    void setStatusText(const std::string& status);
    
    void showMaintenanceScene();
    
protected:
    osg::Node* setupHud();
    
    
    
private:
    osg::ref_ptr<osg::Node>     _maintenanceScene;
    osg::ref_ptr<osgText::Text> _statusText;
    osg::ref_ptr<osg::ImageStream> _maintenanceMovie;

};