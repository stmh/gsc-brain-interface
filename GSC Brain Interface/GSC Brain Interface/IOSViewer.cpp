//
//  IOSViewer.cpp
//  Present3D
//
//  Created by Stephan Huber on 22.11.12.
//  Copyright (c) 2012 OpenSceneGraph. All rights reserved.
//

#include "IOSViewer.h"
#include <osgDB/Registry>
#include <osg/Texture2D>
#include <osg/ValueObject>
#include <osgGA/Device>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/Device>
#include <osgGA/GUIEventHandler>
#include <stdlib.h>

class ZeroConfDiscoverEventHandler : public osgGA::GUIEventHandler {
public:
    ZeroConfDiscoverEventHandler(const std::string& type) : osgGA::GUIEventHandler(), _type(type) {}
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
    {
        if (_oscDevice.valid())
        {
            _oscDevice->sendEvent(ea);
            return false;
        }
        if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
        {
            if (ea.getName() == "/zeroconf/service-added")
            {
                
                IOSViewer* viewer = dynamic_cast<IOSViewer*>(&aa);
                if (!viewer)
                    return false;
                    
                std::string host("");
                unsigned int port(0);
                
                ea.getUserValue("host", host);
                ea.getUserValue("port", port);
                
                if (_type == httpServiceType())
                {
                    viewer->readScene(host, port);
                }
                else if (_type == oscServiceType())
                {
                    startEventForwarding(viewer, host, port);
                }
                
            }
        }
        return false;
    }
    
    void startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port)
    {
        std::ostringstream ss;
        ss << host << ":" << port << ".sender.osc";
    
        _oscDevice = osgDB::readFile<osgGA::Device>(ss.str());
        if (!_oscDevice.valid()) {
            viewer->setStatusText("could not get osc-device: " + ss.str());
        }
    }

    static const char* httpServiceType() { return "_present3dhttp._tcp"; }
    static const char* oscServiceType() { return "_present3dosc._udp"; }
    
private:
    const std::string _type;
    osg::ref_ptr<osgGA::Device> _oscDevice;
};

void IOSViewer::setDataFolder(const std::string& folder)
{
    osgDB::Registry::instance()->getDataFilePathList().push_front(folder);
}

void IOSViewer::setStatusText(const std::string& status)
{
    _statusText->setText(status);
}


void IOSViewer::readScene(const std::string& host, unsigned int port)
{
    std::ostringstream ss;
    ss << host << ":" << port << "/interface.p3d";
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(ss.str());
    if (!node)
        setStatusText("could not read interface from " + ss.str());
    else
        setSceneData(node);
    
}



osg::Node* IOSViewer::setupHud()
{
    osg::Camera* hudCamera = new osg::Camera;
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setProjectionMatrixAsOrtho2D(0,1024,0,786);
    hudCamera->setViewMatrix(osg::Matrix::identity());
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    
     osg::Geode* geode = new osg::Geode();
    hudCamera->addChild(geode);


    osg::Image* background_image = osgDB::readImageFile("Default-Landscape@2x~ipad.png");
    if (background_image)
    {
        osg::Geometry* geometry = osg::createTexturedQuadGeometry(osg::Vec3(0,0,0), osg::Vec3(1024,0,0), osg::Vec3(0,786,0), 0, 0, 1, 1);
        
        /*
        osg::Vec4Array* colors = new osg::Vec4Array();
        colors->push_back(osg::Vec4(1,1,1,1));
        colors->push_back(osg::Vec4(1,1,1,1));
        colors->push_back(osg::Vec4(1,1,1,1));
        colors->push_back(osg::Vec4(1,1,1,1));
        geometry->setColorArray(colors);
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
        geometry->setNormalBinding(osg::Geometry::BIND_OFF);
        */
        
        osg::Texture2D* tex = new osg::Texture2D();
        tex->setImage(background_image);
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        tex->setResizeNonPowerOfTwoHint(false);
        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        
        geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
        geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        
        geode->addDrawable(geometry);

    }
    
    _statusText = new osgText::Text();
    _statusText->setDataVariance(osg::Object::DYNAMIC);
    _statusText->setFont("Arial.ttf");
    _statusText->setPosition(osg::Vec3(20,20,0));
    _statusText->setColor(osg::Vec4(1,1,1,1));
    _statusText->setText("waiting for http-server/interface-file");
    _statusText->setCharacterSize(10.0f);
    _statusText->setAxisAlignment(osgText::TextBase::XY_PLANE);
    _statusText->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _statusText->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    geode->addDrawable(_statusText);
    return hudCamera;
}




void IOSViewer::realize()
{
    setThreadingModel(SingleThreaded);
    getEventQueue()->setFirstTouchEmulatesMouse(true);
    setenv("OSG_GL_ERROR_CHECKING", "ON", 1);
    
    setCameraManipulator(new osgGA::TrackballManipulator());
    
    // setup zeroconf
    
    std::vector<std::string> service_types;
    service_types.push_back(ZeroConfDiscoverEventHandler::httpServiceType());
    service_types.push_back(ZeroConfDiscoverEventHandler::oscServiceType());
    
    for(std::vector<std::string>::iterator i = service_types.begin(); i != service_types.end(); ++i)
    {
        osgGA::Device* device = osgDB::readFile<osgGA::Device>((*i)+".discover.zeroconf");
        if (device)
        {
            addDevice(device);
            addEventHandler(new ZeroConfDiscoverEventHandler(*i));
        }
        else
            setStatusText("could not get zeroconf-device: " + (*i));
    }
    
    
    
    // setup scene
    osg::Group* group = new osg::Group();
    
    group->addChild(setupHud());
    
    setSceneData(group);
    
    osgViewer::Viewer::realize();
}


void IOSViewer::cleanup()
{

}


void IOSViewer::handleMemoryWarning()
{

}