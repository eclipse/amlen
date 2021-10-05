#!/bin/bash

#echo ./fixvmprofile.sh
#./fixvmprofile.sh

#echo ./dockerrestart.sh
#./dockerrestart.sh

echo ./disablecluster.sh
./disablecluster.sh

echo ./cleanstore.sh
./cleanstore.sh

echo sleep 10s
sleep 10s

echo ./waitproduction.sh
./waitproduction.sh

echo ./enablecluster.sh 
./enablecluster.sh 

echo ./rmtrace.sh
./rmtrace.sh

echo ./restart.sh
./restart.sh

echo sleep 5s
sleep 5s

echo ./waitproduction.sh
./waitproduction.sh
