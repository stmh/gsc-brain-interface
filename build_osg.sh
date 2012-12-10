ROOT=${PWD}

cd ../osg/PlatformSpecifics/iOS/

sh build_universal_libs.sh -i -o ${ROOT}/libs -t "osgdb_osc;osgdb_zeroconf;osgdb_p3d;osgdb_avfoundation;osgdb_osg;osgdb_imageio;osgdb_deprecated_osg"
