ROOT=${PWD}
OSC_PORT=8000
HTTP_PORT=9000

export OSG_WINDOW="100 100 800 600"
cd /Users/stephan/Documents/osg/build/bin
./present3Dd \
  --device 0.0.0.0:${HTTP_PORT}${ROOT}/gsc_ipad.resthttp \
  --device _p3d_http._tcp:${HTTP_PORT}.advertise.zeroconf \
  --device _p3d_osc._udp:${OSC_PORT}.advertise.zeroconf \
  /Users/stephan/Documents/Projekte/cefix/3rdParty/OpenSceneGraph-Data/cow.osg
