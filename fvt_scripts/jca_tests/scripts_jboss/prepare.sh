#!/bin/bash
## prepare.sh
## This script will do things no script should do.

set -x

IP=`echo ${WASIP} | cut -d: -f1`

# Create and variable replace tests.properties
cp tests.uninitialized.properties tests.properties
sed -i -e s/M1_NAME/${hostname}/g tests.properties
sed -i -e s/APPSERVER/JBOSS/g tests.properties
sed -i -e s/A1_IPv4_1/${A1_IPv4_1}/g config/test.config
sed -i -e s/A2_IPv4_1/${A2_IPv4_1}/g config/test.config

# Unpack testTools_JCAtests.ear and variable replace
unzip ../lib/testTools_JCAtests.ear -d ear/
unzip ear/testTools_JCAtests.jar -d jar/
rm ear/testTools_JCAtests.jar
rm ../lib/testTools_JCAtests.ear
rm jar/META-INF/ejb-jar.xml
cp config/ejb-jar.xml jar/META-INF/ejb-jar.xml
sed -i -e s/A1_IPv4_1/${A1_IPv4_1}/g jar/META-INF/ejb-jar.xml
sed -i -e s/A2_IPv4_1/${A2_IPv4_1}/g jar/META-INF/ejb-jar.xml
cd jar/
zip -r ../ear/testTools_JCAtests.jar *
cd ../ear/
zip -r ../../lib/testTools_JCAtests.ear *
cd ../
chmod +x ../lib/testTools_JCAtests.ear
rm -rf jar/ ear/

cp ../lib/testTools_JCAtests.ear /tmp/.

# Create env.file on App Server machine
echo "echo ${A1_IPv4_1} > /WAS/env.file" | ssh ${IP}
echo "echo ${A2_IPv4_2} >> /WAS/env.file" | ssh ${IP}
