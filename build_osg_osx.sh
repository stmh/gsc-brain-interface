
cd ../osg_osx
ROOT=${PWD}

/usr/bin/cmake -G Xcode \
-D OSG_COMPILE_FRAMEWORKS:BOOL=1 \
-D OSG_WINDOWING_SYSTEM:STRING=Cocoa \
-D OSG_BUILD_PLATFORM_IPHONE:BOOL=0 \
-D CMAKE_OSX_ARCHITECTURES:STRING=x86_64 \
-D CMAKE_INSTALL_PREFIX="$ROOT/bin" \
-D OSG_PLUGINS=osgPlugins \
CMAKE_OSX_SYSROOT:STRING=/Developer/SDKs/MacOSX10.7.sdk .

/usr/bin/xcodebuild -configuration "Debug" -target "ALL_BUILD"

