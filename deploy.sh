#!/bin/sh

VSERSION=zmq-client4cpp
CWD_DIR=`pwd`
TMP_DIR=`date +%Y%m%d_%H%M%S`

#git clone http://gitlab.ztesoft.com/chen.si1/mq-client4cpp $TMP_DIR

DEPLOY_BUILD_HOME=$CWD_DIR

#
# build json
#

make deps

#
# build client
#

make


#
# deploy
#
cd $CWD_DIR
DEPLOY_INSTALL_HOME=$CWD_DIR/install/$VSERSION
mkdir -p $DEPLOY_INSTALL_HOME/lib

cp $DEPLOY_BUILD_HOME/bin/librocketmq64.so $DEPLOY_INSTALL_HOME/lib/
cp $DEPLOY_BUILD_HOME/bin/libaliyunmq64.so $DEPLOY_INSTALL_HOME/lib/

cp -r $DEPLOY_BUILD_HOME/include $DEPLOY_INSTALL_HOME/
cp -r $DEPLOY_BUILD_HOME/example $DEPLOY_INSTALL_HOME/

#cd $CWD_DIR/$install/

#tar cvf ${VSERSION}.tar $VSERSION
#gzip ${VSERSION}.tar
