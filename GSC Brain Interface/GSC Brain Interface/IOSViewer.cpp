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


static double MAX_IDLE_TIME = 3*60.0;



class ZeroConfDiscoverEventHandler : public osgGA::GUIEventHandler {
public:
    ZeroConfDiscoverEventHandler() : osgGA::GUIEventHandler() {}
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
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
                else if (type == oscServiceType())
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
    
    void startEventForwarding(IOSViewer* viewer, const std::string& host, unsigned int port)
    {
        std::ostringstream ss;
        ss << host << ":" << port << ".sender.osc";
        std::cout << "sending events to " << ss.str() << std::endl;
        _oscDevice = osgDB::readFile<osgGA::Device>(ss.str());
        if (!_oscDevice.valid()) {
            viewer->setStatusText("could not get osc-device: " + ss.str());
        }
    }

    static const char* httpServiceType() { return "_p3d_http._tcp"; }
    static const char* oscServiceType() { return "_p3d_osc._udp"; }
    
private:
    osg::ref_ptr<osgGA::Device> _oscDevice;
};




class IdleTimerEventHandler : public osgGA::GUIEventHandler {
public:
    IdleTimerEventHandler(double max_idle_time)
        : osgGA::GUIEventHandler()
        , _maxIdleTime(max_idle_time)
    {
    }
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
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
                OSG_ALWAYS << "resetting scene ..., idle timeout" << std::endl;
                
                view->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_Home);
                view->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_Home);
                
                view->getEventQueue()->keyPress(' ');
                view->getEventQueue()->keyRelease(' ');
            }
            
        }
        
        return false;
    }
private:
    double _maxIdleTime, _lastEventTimeStamp;
    
};



void IOSViewer::setDataFolder(const std::string& folder)
{
    osgDB::Registry::instance()->getDataFilePathList().push_front(folder);
}

void IOSViewer::setStatusText(const std::string& status)
{
    _statusText->setText(status);
}


void IOSViewer::showMaintenanceScene() {
    setStatusText("waiting for http-server/interface-file");
    setSceneData(_maintenanceScene);
    getEventQueue()->keyPress(' ');
    getEventQueue()->keyRelease(' ');
}

void IOSViewer::readScene(const std::string& host, unsigned int port)
{
    std::ostringstream ss;
    ss << "http://" << host << ":" << port << "/interface.p3d";

    std::cout << "reading interface from " << ss.str() << std::endl;


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
    
    
    
    // seup event handler
    addEventHandler(new IdleTimerEventHandler(MAX_IDLE_TIME));
    addEventHandler(new ZeroConfDiscoverEventHandler());
    
    
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

        }
        else
            setStatusText("could not get zeroconf-device: " + (*i));
    }
    
    
    
    // setup scene
    osg::Group* group = new osg::Group();
    group->addChild(setupHud());
    
    _maintenanceScene = group;
    
    setSceneData(group);
    
    osgViewer::Viewer::realize();
}


void IOSViewer::cleanup()
{

}


void IOSViewer::handleMemoryWarning()
{

}