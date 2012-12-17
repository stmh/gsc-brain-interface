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
#include <osg/ImageStream>
#include <osgGA/Device>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgGA/TrackballManipulator>
#include <osgGA/Device>
#include <osgGA/GUIEventHandler>
#include <stdlib.h>
#include "ZeroConfDiscoverEventHandler.h"
#include "IdleTimerEventHandler.h"


class IgnoreInputTrackballCameraManipulator : public osgGA::TrackballManipulator {
public:
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if ((ea.getEventType() == osgGA::GUIEventAdapter::PUSH)
            || (ea.getEventType() == osgGA::GUIEventAdapter::DRAG)
            || (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE))
            
            return false;
        
        return osgGA::TrackballManipulator::handle(ea, aa);
    }

};


class ScopedNotifyLevel {
public:
    ScopedNotifyLevel(osg::NotifySeverity new_level, const std::string& message = "")
        : _oldLevel(osg::getNotifyLevel())
        , _newLevel(new_level)
        , _message(message)
    {
        osg::setNotifyLevel(new_level);
        osg::notify(new_level) << "begin " << _message << std::endl;
    }
    
    ~ScopedNotifyLevel()
    {
        osg::notify(_newLevel) << "end " << _message << std::endl;
        osg::setNotifyLevel(_oldLevel);
    }
private:
    osg::NotifySeverity _oldLevel, _newLevel;
    std::string _message;
};


static const double MAX_IDLE_TIME = 3*60.0;
static const char* INTERFACE_FILE_NAME = "interface.p3d";

osgDB::Options* createOptions(const osgDB::ReaderWriter::Options* options)
{
    osg::ref_ptr<osgDB::Options> local_options = options ? options->cloneOptions() : 0;
    if (!local_options)
    {
        local_options = osgDB::Registry::instance()->getOptions() ?
                osgDB::Registry::instance()->getOptions()->cloneOptions() :
                new osgDB::Options;
    }

    return local_options.release();
}

osg::ref_ptr<osg::Node> readHoldingSlide(const std::string& filename)
{
    std::string ext = osgDB::getFileExtension(filename);
    if (!osgDB::equalCaseInsensitive(ext,"xml") && 
        !osgDB::equalCaseInsensitive(ext,"p3d")) return 0;

    osg::ref_ptr<osgDB::ReaderWriter::Options> options = createOptions(0);
    options->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_NONE);
    options->setOptionString("preview");

    return osgDB::readRefNodeFile(filename, options.get());
}

osg::ref_ptr<osg::Node> readPresentation(const std::string& filename,const osgDB::ReaderWriter::Options* options)
{
    std::string ext = osgDB::getFileExtension(filename);
    if (!osgDB::equalCaseInsensitive(ext,"xml") &&
        !osgDB::equalCaseInsensitive(ext,"p3d")) return 0;

    osg::ref_ptr<osgDB::Options> local_options = createOptions(options);
    local_options->setOptionString("main");

    return osgDB::readRefNodeFile(filename, local_options.get());
}



void IOSViewer::addDataFolder(const std::string& folder)
{
    osgDB::Registry::instance()->getDataFilePathList().push_front(folder);
    OSG_NOTICE << "add data folder: " << folder << std::endl;
}


void IOSViewer::setStatusText(const std::string& status)
{
    _statusText->setText(status);
    OSG_NOTICE << status << std::endl;
}


void IOSViewer::showMaintenanceScene() {
    setStatusText("waiting for http-server/interface-file");
    setSceneData(_maintenanceScene);
    getEventQueue()->keyPress(' ');
    getEventQueue()->keyRelease(' ');
    
    if (_maintenanceMovie.valid())
        _maintenanceMovie->play();
    _sceneLoaded = _isLocalScene = false;
    checkForLocalFile();
}

void IOSViewer::checkEnvVars()
{
    const char* p3dDevice = getenv("P3D_DEVICE");
    if (p3dDevice)
    {
        osgDB::StringList devices;
        osgDB::split(p3dDevice, devices);
        bool first(true);
        for(osgDB::StringList::iterator i = devices.begin(); i != devices.end(); ++i)
        {
            osg::ref_ptr<osgGA::Device> dev = osgDB::readFile<osgGA::Device>(*i);
            if (dev.valid())
            {
                OSG_INFO << "Adding Device : " << *i << std::endl;
                if (dev->getCapabilities() & osgGA::Device::RECEIVE_EVENTS)
                    addDevice(dev.get());
                
                if (dev->getCapabilities() & osgGA::Device::SEND_EVENTS)
                {
                    if (first)
                    {
                        _zeroconfEventHandler->removeAllDevices();
                        first = false;
                    }
                    _zeroconfEventHandler->addDevice(dev);
                }
                    
            }
            else
            {
                OSG_WARN << "could not open device: " << *i << std::endl;
            }
        }
    }
}

void IOSViewer::readScene(const std::string& host, unsigned int port)
{
    if (_sceneLoaded && !_isLocalScene)
        return;
    
    std::ostringstream ss;
    ss << "http://" << host << ":" << port << "/" << INTERFACE_FILE_NAME;

    OSG_NOTICE << "reading interface from " << ss.str() << std::endl;

    // ScopedNotifyLevel l(osg::DEBUG_INFO, "READ SCENE DATA");
    
    osg::ref_ptr<osg::Node> node = readHoldingSlide(ss.str());
    if (!node)
        setStatusText("could not read interface from " + ss.str());
    else
    {
        setSceneData(node);
        frame();
        node = readPresentation(ss.str(), createOptions(0));
        if(node) {
            setSceneData(node);
            _sceneLoaded = true;
            _isLocalScene = false;
        }
        if (_maintenanceMovie.valid())
            _maintenanceMovie->pause();
    }
    
    checkEnvVars();
}



osg::Node* IOSViewer::setupHud()
{
    osg::Camera* hudCamera = new osg::Camera;
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setProjectionMatrixAsOrtho2D(0,2*1024,0,2*768);
    hudCamera->setViewMatrix(osg::Matrix::identity());
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    
     osg::Geode* geode = new osg::Geode();
    hudCamera->addChild(geode);
    
    

    osg::ref_ptr<osg::Image> background_image(NULL);
    osg::ref_ptr<osg::Texture> tex(NULL);
    
    background_image = osgDB::readImageFile("background-idle-movie.mov");
    if (background_image)
    {
        _maintenanceMovie = dynamic_cast<osg::ImageStream*>(background_image.get());
        if(_maintenanceMovie.valid())
        {
            // TODO: Fix CoreVideo tex = _maintenanceMovie->createSuitableTexture();
            _maintenanceMovie->setLoopingMode(osg::ImageStream::LOOPING);
            _maintenanceMovie->play();
        }
    }
    else
    {
        background_image = osgDB::readImageFile("Default-Landscape@2x~ipad.png");
    }
    if (background_image)
    {
        osg::Geometry* geometry = osg::createTexturedQuadGeometry(osg::Vec3(0,0,0), osg::Vec3(2048,0,0), osg::Vec3(0,2*768,0), 0, 0, 1, 1);
                
        if (!tex) {
            tex = new osg::Texture2D(background_image);
        }
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
    _statusText->setFont("arial.ttf");
    _statusText->setPosition(osg::Vec3(20,20,0));
    _statusText->setColor(osg::Vec4(1,1,1,1));
    _statusText->setText("waiting for http-server/interface-file");
    _statusText->setCharacterSize(24.0f);
    _statusText->setAxisAlignment(osgText::TextBase::XY_PLANE);
    _statusText->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    _statusText->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    geode->addDrawable(_statusText);
    return hudCamera;
}

void IOSViewer::checkForLocalFile()
{
    if (_sceneLoaded) return;
    std::string local_scene_file = osgDB::findDataFile(INTERFACE_FILE_NAME);
    
    if (!local_scene_file.empty())
    {
        osg::Node* node = osgDB::readNodeFile(local_scene_file);
        if (node) {
            setSceneData(node);
            checkEnvVars();
            _sceneLoaded = _isLocalScene = true;
        }
        else
            setStatusText("could not read scene from local file "+ local_scene_file);
    }
}


void IOSViewer::realize()
{
    setThreadingModel(SingleThreaded);
    getEventQueue()->setFirstTouchEmulatesMouse(true);
    setenv("OSG_GL_ERROR_CHECKING", "ON", 1);
    
    osg::DisplaySettings* settings = osg::DisplaySettings::instance();
    settings->setNumMultiSamples(4);
    
    // setup scene
    osg::Group* group = new osg::Group();
    group->addChild(setupHud());
    
    setCameraManipulator(new IgnoreInputTrackballCameraManipulator());
    
    // seup event handler
    _zeroconfEventHandler = new ZeroConfDiscoverEventHandler();
    addEventHandler(new IdleTimerEventHandler(MAX_IDLE_TIME));
    addEventHandler(_zeroconfEventHandler);
    
    
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
    
    
    _maintenanceScene = group;
    
    setSceneData(group);
    
    checkForLocalFile();
    
    osgViewer::Viewer::realize();
}

void IOSViewer::setSceneData(osg::Node *node)
{
    osgViewer::Viewer::setSceneData(node);
    
    /* does not work
    osg::BoundingSphere bs = node->getBound();

    double dist = osg::DisplaySettings::instance()->getScreenDistance();
    
    
    double screenWidth = 0.2; //osg::DisplaySettings::instance()->getScreenWidth();
    double screenHeight = 0.15; //osg::DisplaySettings::instance()->getScreenHeight();
    double screenDistance = 0.3; //osg::DisplaySettings::instance()->getScreenDistance();

    double vfov = atan2(screenHeight/2.0,screenDistance)*2.0;
    double hfov = atan2(screenWidth/2.0,screenDistance)*2.0;
    double viewAngle = vfov<hfov ? vfov : hfov;

    dist = 10 * bs.radius();
    


    osg::Vec3 center = bs.center();
    osg::Vec3 eye = center - osg::Vec3d(0.0, dist, 0.0);
    osg::Vec3 up = osg::Vec3d(0.0, 0.0, 1.0);
    
    osg::Matrixd matrix;
    matrix.makeLookAt(eye, center, up);

    getCamera()->setViewMatrix( matrix );
    getCamera()->setProjectionMatrixAsPerspective( viewAngle, screenWidth/screenHeight, 0.1, 1000.0);

    */
}


void IOSViewer::cleanup()
{

}

void IOSViewer::wakeUp()
{
    checkForLocalFile();
    _zeroconfEventHandler->sendInit();
}


void IOSViewer::handleMemoryWarning()
{

}