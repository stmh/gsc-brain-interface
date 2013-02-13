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
#include <osgUtil/Optimizer>
#include <stdlib.h>
#include "ZeroConfDiscoverEventHandler.h"
#include "IdleTimerEventHandler.h"

#include <mach/mach.h>
#include <mach/mach_host.h>


#if TESTING
    #include "TestFlight.h"
#endif

static const double MAX_IDLE_TIME = 3*60.0;
static const char* INTERFACE_FILE_NAME = "interface.p3d";



class TestflightNotifyHandler : public osg::NotifyHandler {
public:
    virtual void notify (osg::NotifySeverity severity, const char *message)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        
        std::string msg(message);
        msg = msg.substr(0,msg.length()-1);
        _lastMessages.push_back(msg);
        if(_lastMessages.size() > 60)
            _lastMessages.pop_front();
    
        #if TESTING
            TFLog(@"%s: %s", getSeverity(severity), msg.c_str());
        #else
            std::cout << getSeverity(severity) << msg << std::endl;
        #endif
    }
    
    const char* getSeverity(osg::NotifySeverity severity) {
    
        switch(severity) {
            case osg::ALWAYS:           return "[ALWAYS] ";
            case osg::FATAL:            return "[FATAL] ";
            case osg::WARN:             return "[WARN] ";
            case osg::NOTICE:           return "[NOTICE] ";
            case osg::INFO:             return "[INFO] ";
            case osg::DEBUG_INFO:       return "[DEBUG_INFO] ";
            case osg::DEBUG_FP:         return "[DEBUG_FP] ";
        }
        return "";
    }
    
    std::string getLastMessages()
    {
        std::string result;
        for(std::deque<std::string>::iterator i = _lastMessages.begin(); i != _lastMessages.end(); ++i)
            result += (*i) + '\n';
        
        return result;
    }
    
private:
    std::deque<std::string> _lastMessages;
    OpenThreads::Mutex _mutex;
};



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

class DebugConsoleEventHandler : public osgGA::GUIEventHandler {

public:
    DebugConsoleEventHandler(TestflightNotifyHandler* nh)
        : osgGA::GUIEventHandler()
        , _nh(nh)
        , _visible(false)
    {
        osg::Camera* hudCamera = new osg::Camera;
        hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        hudCamera->setProjectionMatrixAsOrtho2D(0,2*1024,0,2*768);
        hudCamera->setViewMatrix(osg::Matrix::identity());
        hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
        hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
        
         osg::Geode* geode = new osg::Geode();
        hudCamera->addChild(geode);

        _tg = new osgText::Text();
        _tg->setDataVariance(osg::Object::DYNAMIC);
        _tg->setFont("arial.ttf");
        _tg->setPosition(osg::Vec3(20,2*768 - 20,0));
        _tg->setColor(osg::Vec4(1,1,1,1));
        _tg->setText("");
        _tg->setCharacterSize(24.0f);
        _tg->setAxisAlignment(osgText::TextBase::XY_PLANE);
        _tg->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        _tg->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        
        geode->addDrawable(_tg);
        
        _camera = hudCamera;
    }
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if ((ea.getEventType() == osgGA::GUIEventAdapter::RELEASE) && ea.isMultiTouchEvent())
        {
            const osgGA::GUIEventAdapter::TouchData* td = ea.getTouchData();
            if ((td->getNumTouchPoints() == 1) && (td->get(0).tapCount == 3))
            {
                _visible = !_visible;
                osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
                if (viewer) {
                    osg::Group* group = dynamic_cast<osg::Group*>(viewer->getSceneData());
                    if (!group) {
                        group = new osg::Group();
                        group->addChild(viewer->getSceneData());
                        viewer->setSceneData(group);
                    }
                    if (_visible)
                        group->addChild(_camera);
                    else
                        group->removeChild(_camera);
                }
                _tg->setText(_nh->getLastMessages());
            }
        }
        else if ((ea.getEventType() == osgGA::GUIEventAdapter::FRAME) && _visible)
        {
            _tg->setText(_nh->getLastMessages());
        }
        return false;
    }
    
private:
    osg::ref_ptr<TestflightNotifyHandler> _nh;
    osg::ref_ptr<osg::Camera> _camera;
    osg::ref_ptr<osgText::Text> _tg;
    bool _visible;
    
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

IOSViewer::IOSViewer()
    : osgViewer::Viewer()
    , _maintenanceScene(NULL)
    , _statusText(NULL)
    , _maintenanceMovie(NULL)
    , _sceneLoaded(false)
    , _isLocalScene(false)
{
    osg::setNotifyLevel(osg::NOTICE);
    
#if TESTING
    TestflightNotifyHandler* nh =new TestflightNotifyHandler();
    osg::setNotifyHandler(nh);
    
    addEventHandler(new DebugConsoleEventHandler(nh));
#endif
    
    osg::ref_ptr<osgDB::ReaderWriter::Options> cacheAllOption = new osgDB::ReaderWriter::Options;
    cacheAllOption->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_IMAGES);
    osgDB::Registry::instance()->setOptions(cacheAllOption.get());
}

void IOSViewer::addDataFolder(const std::string& folder)
{
    osgDB::Registry::instance()->getDataFilePathList().push_front(folder);
    OSG_INFO << "add data folder: " << folder << std::endl;
}


void IOSViewer::setStatusText(const std::string& status)
{
    _statusText->setText(status);
    OSG_NOTICE << status << std::endl;
}


void IOSViewer::showMaintenanceScene() {
    setStatusText("waiting for http-server/interface-file ...");
    setSceneData(_maintenanceScene);
    getEventQueue()->keyPress(' ');
    getEventQueue()->keyRelease(' ');
    
    putenv("P3D_DEVICE=");
    
    if (_maintenanceMovie.valid())
        _maintenanceMovie->play();
    _sceneLoaded = _isLocalScene = false;
    checkForLocalFile();
}

void IOSViewer::checkEnvVars()
{
    
    {
        const char* p3dDevice = getenv("P3D_DEVICE");
        if (p3dDevice)
        {
            osgDB::StringList devices;
            osgDB::split(p3dDevice, devices);
            for(osgDB::StringList::iterator i = devices.begin(); i != devices.end(); ++i)
            {
                osg::ref_ptr<osgGA::Device> dev = osgDB::readFile<osgGA::Device>(*i);
                if (dev.valid())
                {
                    OSG_NOTICE << "Adding Device : " << *i << std::endl;
                    addDevice(dev.get());
                }
                else
                {
                    OSG_WARN << "could not open device: " << *i << std::endl;
                }
            }
        }
    }
    {
        const char* p3dTimeOut = getenv("P3D_TIMEOUT");
        if(p3dTimeOut)
        {
            unsigned int new_max_idle_time = atoi(p3dTimeOut);
            if (_idleTimerEventHandler.valid() && new_max_idle_time > 0)
            {
                _idleTimerEventHandler->setNewMaxIdleTime(new_max_idle_time);
            }
        }
    }
    
    {
        char* OSGNOTIFYLEVEL=getenv("OSG_NOTIFY_LEVEL");
        if (!OSGNOTIFYLEVEL) OSGNOTIFYLEVEL=getenv("OSGNOTIFYLEVEL");
        if(OSGNOTIFYLEVEL)
        {
            osg::NotifySeverity notifyLevel = osg::NOTICE;
            std::string stringOSGNOTIFYLEVEL(OSGNOTIFYLEVEL);

            // Convert to upper case
            for(std::string::iterator i=stringOSGNOTIFYLEVEL.begin();
                i!=stringOSGNOTIFYLEVEL.end();
                ++i)
            {
                *i=toupper(*i);
            }

            if(stringOSGNOTIFYLEVEL.find("ALWAYS")!=std::string::npos)          notifyLevel=osg::ALWAYS;
            else if(stringOSGNOTIFYLEVEL.find("FATAL")!=std::string::npos)      notifyLevel=osg::FATAL;
            else if(stringOSGNOTIFYLEVEL.find("WARN")!=std::string::npos)       notifyLevel=osg::WARN;
            else if(stringOSGNOTIFYLEVEL.find("NOTICE")!=std::string::npos)     notifyLevel=osg::NOTICE;
            else if(stringOSGNOTIFYLEVEL.find("DEBUG_INFO")!=std::string::npos) notifyLevel=osg::DEBUG_INFO;
            else if(stringOSGNOTIFYLEVEL.find("DEBUG_FP")!=std::string::npos)   notifyLevel=osg::DEBUG_FP;
            else if(stringOSGNOTIFYLEVEL.find("DEBUG")!=std::string::npos)      notifyLevel=osg::DEBUG_INFO;
            else if(stringOSGNOTIFYLEVEL.find("INFO")!=std::string::npos)       notifyLevel=osg::INFO;
            else std::cout << "Warning: invalid OSG_NOTIFY_LEVEL set ("<<stringOSGNOTIFYLEVEL<<")"<<std::endl;
            
            osg::setNotifyLevel(notifyLevel);

        }
    }
}

void IOSViewer::readScene(const std::string& host, unsigned int port)
{
    if (_sceneLoaded && !_isLocalScene)
        return;
    osgDB::Registry::instance()->clearObjectCache();
    std::ostringstream ss;
    ss << "http://" << host << ":" << port << "/" << INTERFACE_FILE_NAME;

    OSG_NOTICE << "reading interface from " << ss.str() << std::endl;

    // ScopedNotifyLevel l(osg::DEBUG_INFO, "READ SCENE DATA");
    
    osg::ref_ptr<osg::Node> node = readHoldingSlide(ss.str());
    checkEnvVars();
    if (node) {
        setSceneData(node);
        frame();
    }
    
    node = readPresentation(ss.str(), createOptions(0));
    if(node) {
        setSceneData(node);
        sendInit();
        _sceneLoaded = true;
        _isLocalScene = false;
    
        if (_maintenanceMovie.valid())
            _maintenanceMovie->pause();
        
    } else {
        setStatusText("could not read interface from " + ss.str());
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
    // _idleTimerEventHandler = new IdleTimerEventHandler(MAX_IDLE_TIME);
    // addEventHandler(_idleTimerEventHandler);
    addEventHandler(_zeroconfEventHandler);

    
    
    _maintenanceScene = group;
    
    setSceneData(group);
    
    checkForLocalFile();
    
    reloadDevices();
    
    osgViewer::Viewer::realize();
}

void IOSViewer::reloadDevices() {
    
    if (_sceneLoaded)
        return;
    
    osgViewer::View::Devices devices = getDevices();
    std::vector<osgGA::Device*> to_delete;
    
    for(osgViewer::View::Devices::iterator i = devices.begin(); i != devices.end(); ++i) {
        osgGA::Device* device(*i);
        if (device->getCapabilities() | osgGA::Device::SEND_EVENTS) {
            to_delete.push_back(device);
        }
    }
    
    for(std::vector<osgGA::Device*>::iterator j = to_delete.begin(); j != to_delete.end(); ++j) {
        removeDevice(*j);
    }
    
    OSG_ALWAYS << "removed " << to_delete.size() << " devices" << std::endl;
    
    setStatusText("Reloading zeroconf devices, waiting for http-server/interface-file ...");
    
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
    
}

void IOSViewer::setSceneData(osg::Node *node)
{
    osgUtil::Optimizer o;
    o.optimize(node);
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
    sendInit();
}

bool getSystemMemoryUsage(unsigned long &free_mem, unsigned long &used_mem, unsigned long &mem_size)
{
	mach_port_t host_port;
	mach_msg_type_number_t host_size;
	vm_size_t pagesize;
	
	host_port = mach_host_self();
	host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
	host_page_size(host_port, &pagesize);
	
	vm_statistics_data_t vm_stat;
	
	if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS)
		return false;
	
	/* Stats in bytes */
	natural_t mem_used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pagesize;
	natural_t mem_free = vm_stat.free_count * pagesize;
	natural_t mem_total = mem_used + mem_free;
	used_mem = round(mem_used);
	free_mem = round(mem_free);
	mem_size = round(mem_total);
	return true;
}


void IOSViewer::showMem(const std::string& msg) {
    unsigned long freemem, used_mem, mem_size;
    getSystemMemoryUsage(freemem,used_mem, mem_size);
    std::cout << msg << "  free: " << freemem << " used: " << used_mem << std::endl;
    
}

void IOSViewer::sendInit()
{
    // send a keypress + -release of the space-bar
    {
        getEventQueue()->keyPress(' ');
        getEventQueue()->keyRelease(' ');
        getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_Home);
        getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_Home);
    }
}

void IOSViewer::handleMemoryWarning()
{
    osgDB::Registry::instance()->clearObjectCache();
}