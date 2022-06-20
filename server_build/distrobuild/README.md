# Alternative (OSS Distro) Build
This is an embryonic description of how a build might build/package this software
rather than using the default setup which calls the packaging steps from inside the
build.

The instructions and spec files need a lot more love and testing.

## Set up
To build the any of the rpms, use the following steps to set up an rpm build environment

1) Find a modern Linux distro e.g. centos 8 or Fedora 34
2) Get setup to build rpms:
    ``` 
    sudo dnf install rpmdevtools
    rpmdev-setuptree
    ```

## Dependencies

Some of the rpms have dependencies required to build them (the SDK, the WebUI, the MQ Bridge).

Internally to IBM the dependencies are available here:
https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-local/dependencies/messagegateway-build/

For external users, we'll have to distribute the dependencies or explain to the user where to 
get them from.

## Building the server rpm
3) Zip up the source code:
    ```
    bash server_build/distrobuild/genmgwsrczip.sh <path to source>
    ```
4) Create the source rpm:
    ```
    rpmbuild -bs server_build/distrobuild/imaserver.spec
    ```
5) Build the code and produce the rpm:
    ```
    mock -r fedora-34-x86_64 ~/rpmbuild/SRPMS/AmlenServer-1.1dev-1.fc34.src.rpm
    ```

### Building for CentOS
Some of the packages (ant-contrib) we need to build are not in the main centos repo - they are in a module
so to build for CentOS to need to tell mock about the module:

As root:
```
cd /etc/mock
cp epel-8-x86_64.cfg epel-8_javapkg-x86_64.cfg
Then add to epel-8_javapkg-x86_64.cfg:
 config_opts['module_enable'] = ['javapackages-tools']
```
Then, as your normal user build using your new config:
```
mock -r epel-8_javapkg-x86_64 ~/rpmbuild/SRPMS/AmlenServer-1.1dev-1.fc34.src.rpm 
```

## Building the WebUI
The WebUI requires an extra source zip of dependencies called `imawebui-deps.zip` 
(put into `~/rpmbuild/SOURCES/`) which currently needs to contain a number of files as described
below:

So do the setup steps to get an rpmbuild environment (required for any of the other rpms), then create
the extra deps rpm e.g. by:
```
cd $DEPS_HOME #With the dependencies downloaded from the URL in the dependencies section
zip ~/rpmbuild/SOURCES/imawebui-deps.zip d3.zip  dojo-webui-release-1.17.2-src.zip  gridx-webui-1.3.9.zip openliberty-22.0.0.6.zip icu4j-71_1.jar jackson-*2.13.3*.jar

```
and then do the build:
```
bash server_build/distrobuild/genmgwsrczip.sh <path to source>
rpmbuild -bs server_build/distro/build/imawebui.spec 
mock -r fedora-34-x86_64 -n  ~/rpmbuild/SRPMS/AmlenWebUI-1.1dev-1.fc34.src.rpm 
``` 

## Building MQCBridge
This is a separate rpm to the server (as it depends on the closed source MQClient). 
Ensure you can build the server (above) then:

1. Put both the redestributable MQClient (used at runtime) and the non-redistributable MQClient
   (used at build time) in `~/rpmbuild/SOURCES` (e.g. the files called `9.2.0.5-IBM-MQC-LinuxX64.tar.gz` and `9.2.0.5-IBM-MQC-Redist-LinuxX64.tar.gz`) 
2. Alter the spec file `server_build/distrobuild/mqcbridge.spec` to match the version used in step 1
3. `rpmbuild -bs mqcbridge.spec`
4. `mock -r fedora-34-x86_64 ~/rpmbuild/SRPMS/AmlenMQCBridge-1.1dev-1.fc34.src.rpm`


## Building the IMA Bridge (MQTT and Kafka bridge)

Do the setup steps to get an rpm build env then:

1) Zip up the source code:
```
bash server_build/distrobuild/genmgwsrczip.sh <path to source>
``` 
2) Build the source rpm:
```
rpmbuild -bs server_build/distrobuild/imamqttbridge.spec 
```
3) Build the rpm:
```
mock -r fedora-34-x86_64 ~/rpmbuild/SRPMS/AmlenBridge-1.1dev-1.fc34.src.rpm
```



## Building the Server Protocol Plugin Adaptor

```
bash server_build/distrobuild/genmgwsrczip.sh
rpmbuild -bs server_build/distrobuild/protocolplugin.spec 
mock -r fedora-34-x86_64 ~/rpmbuild/SRPMS/AmlenProtocolPlugin-1.1dev-1.fc34.src.rpm
```

## Build the Client SDK

The SDK consists of the JMS Client, the Protocol plugin SDK and (if all the right 
dependencies exist) the WAS RA as well.

To build the SDK including the WAS RA download dependencies and create the zip file like:
```
zip ~/rpmbuild/SOURCES/imasdk-deps.zip com.ibm.ws.admin.core.jar  com.ibm.ws.runtime.jar  com.ibm.ws.sib.server.jar  j2ee.jar  jms.jar
```
To build without the RA then create the zip file like:
```
zip ~/rpmbuild/SOURCES/imasdk-deps.zip jms.jar
```
Once the dependencies have been created build in the usual way:
```
bash server_build/distrobuild/genmgwsrczip.sh
rpmbuild -bs server_build/distrobuild/imasdk.spec 
mock -r fedora-34-x86_64 ~/rpmbuild/SRPMS/AmlenSDK-1.1dev-1.fc34.src.rpm
```


## Rebranding/Renaming Amlen

By setting `IMA_PATH_PROPERTIES` in a spec file we can control filepaths/Product Names/Versions,
the defaults for which are kept in server_build/paths.properties



