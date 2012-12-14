ROOT=${PWD}

cd ../osg/PlatformSpecifics/iOS/

sh build_universal_libs.sh  -o ${ROOT}/libs -t "osg;osgDB;osgGA;osgUtil;osgViewer;osgManipulator;osgText;osgdb_zeroconf;osgdb_curl;osgdb_imageio;osgdb_osg;osgdb_deprecated_osg;curl;OpenThreads;osgdb_p3d;osgPresentation;osgVolume;osgFX;osgdb_osc;osgdb_serializers_osg;osgdb_freetype;osgdb_avfoundation"

