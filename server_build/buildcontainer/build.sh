#!/bin/bash
#This assumes we're cd'd into the server_build dir of
#the source code

export BUILD_LABEL="$(date +%Y%m%d-%H%M)_eclipsebuild"
export SROOT=$(realpath `pwd`/..)
export BROOT=${SROOT}
export DEPS_HOME=/deps
export USE_REAL_TRANSLATIONS=true
export SLESNORPMS=yes
export IMASERVER_BASE_DIR=$BROOT/rpmtree
export JAVA_HOME=/etc/alternatives/java_sdk_1.8.0

#Set up a home directory we can write to (for rpmbuild)
if [ "${HOME}" == "/" ]; then 
    unset HOME
fi
    
if [ -z "${HOME}" ]; then
    export HOME=$BROOT/home
    mkdir $HOME
fi

echo "HOME is $HOME"
echo "free mem:"
free -g
echo "Num processors according to nproc:"
nproc

#We're getting out of mem errors in Eclipse infrastructure
export AMLEN_MAX_BUILD_JOBS=3


ant -f $SROOT/server_build/build.xml  2>&1 | tee $BROOT/ant.log
