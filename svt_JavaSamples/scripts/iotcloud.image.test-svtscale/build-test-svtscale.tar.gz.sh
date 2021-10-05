#!/bin/bash 
declare  external_sdk_iotcloud_dir="/mnt/mar145/BUILD_TOOLS_FROM_RTC/applications"
declare  historian_bundleContent_v1_0="/iotcloud.dependencies.rtc/historian.bundleContent.v1.0"
declare  rest_client_lib_v1_0="/iotcloud.dependencies.rtc/rest.client.lib.v1.0"
declare  glassfishJersey_api_dir="/org.glassfish.jersey/api"
declare  glassfishJersey_ext_dir="/org.glassfish.jersey/ext"
declare  glassfishJersey_v2_12_dir="/org.glassfish.jersey/v2.12"

TEST_DIR="/niagara"
DEST_DIR="${TEST_DIR}/iotcloud"
DEST_FILE="test-svtscale.tar.gz"

cd ${TEST_DIR}/
./svt-getISMProdBuild.sh -c true

echo " "
echo "Prepare the destination directory to build file:  ${DEST_DIR}/${DEST_FILE}"
rm    -fr ${DEST_DIR}
mkdir -p  ${DEST_DIR}
cd    ${DEST_DIR}

echo "Copy JARs and LIBs needed for test case in test-svtscale Docker Container"
cp  ${TEST_DIR}/test/lib/javasample.jar .
cp  ${TEST_DIR}/application/client_ship/lib/org.eclipse.paho.client.mqttv3.jar .
cp  ${TEST_DIR}/test/svt_ssl/lon02-1.iot.jks .
cp  ${TEST_DIR}/test/svt_ssl/PRODUCTION.iot.jks .

# USED By IOTHistorian Class
cp  "${external_sdk_iotcloud_dir}${historian_bundleContent_v1_0}/jackson-annotations-2.3.3.jar" .
cp  "${external_sdk_iotcloud_dir}${historian_bundleContent_v1_0}/jackson-core-2.3.3.jar" .
cp  "${external_sdk_iotcloud_dir}${historian_bundleContent_v1_0}/jackson-databind-2.3.3.jar" .
cp  "${external_sdk_iotcloud_dir}${historian_bundleContent_v1_0}/json-simple-1.1.1.jar" .
   	
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/commons-codec-1.6.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/commons-lang-2.4.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/commons-logging-1.1.1.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/commons-net-2.0.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/fluent-hc-4.2.3.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/httpclient-4.2.3.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/httpclient-cache-4.2.3.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/httpcore-4.2.2.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/httpmime-4.2.3.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/jackson-all-1.9.11.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/slf4j-api-1.6.1.jar" .
cp  "${external_sdk_iotcloud_dir}${rest_client_lib_v1_0}/slf4j-simple-1.6.1.jar" .

cp  "${external_sdk_iotcloud_dir}${glassfishJersey_api_dir}/javax.ws.rs-api-2.0.1.jar" .

cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/aopalliance-repackaged-2.3.0-b10.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/asm-debug-all-5.0.2.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/hk2-api-2.3.0-b10.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/hk2-locator-2.3.0-b10.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/hk2-utils-2.3.0-b10.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/javassist-3.18.1-GA.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/javax.annotation-api-1.2.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/javax.inject-2.3.0-b10.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/javax.servlet-api-3.0.1.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/jaxb-api-2.2.7.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/jersey-guava-2.12.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/org.osgi.core-4.2.0.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/osgi-resource-locator-1.0.1.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/persistence-api-1.0.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_ext_dir}/validation-api-1.1.0.Final.jar" .

cp  "${external_sdk_iotcloud_dir}${glassfishJersey_v2_12_dir}/jersey-client.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_v2_12_dir}/jersey-common.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_v2_12_dir}/jersey-container-servlet-core.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_v2_12_dir}/jersey-container-servlet.jar" .
cp  "${external_sdk_iotcloud_dir}${glassfishJersey_v2_12_dir}/jersey-server.jar" .

tar -zpcf ${DEST_FILE}  *.jar *.jks

echo "Verify the contents of ${DEST_FILE}"
tar -vtf  ${DEST_FILE}

echo " "
echo " Now go check-in ${DEST_DIR}/${DEST_FILE} to iotcoud's RTC Project: iotcloud.image.test-svtscale/binaries "
