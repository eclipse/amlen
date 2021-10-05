#! /bin/bash

source ../scripts/ISMsetup.sh

java -cp $CLASSPATH com.ibm.ima.test.stat.StatHelper "$@" 
