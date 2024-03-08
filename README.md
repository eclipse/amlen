# Amlen

This repo contains the source for the Amlen Message Broker.
It contains (amongst other things) the Amlen Server itself, a WebUI and
a bridge (to MQTT brokers or Kafka). 

A website including documentation can be found at https://eclipse.org/amlen

A community blog can be found at https://amlen.org/

## Building
Requires an x86-64 Linux distribution. Amlen has been built on RHEL7, RHEL8
and recent versions of Fedora. Package names vary slightly between distributions
(Fedora package names are listed here).

Install the prereq packages:
```
sudo dnf install openssl-devel openldap-devel boost-devel CUnit-devel pam-devel curl-devel 
sudo dnf install gcc make vim-common gcc-c++ net-snmp-devel libicu-devel jansson-devel
sudo dnf install ant ant-junit ant-contrib rpm-build icu libxslt dos2unix
sudo dnf install java-1.8.0-openjdk-devel
```
Extract  the source into a directory, put the dependencies (see dependencies 
section of this readme) into another directory and then:
```
export BUILD_LABEL="$(date +%Y%m%d-%H%M)_git_private"
export SROOT=<directory source was extracted into>
export BROOT=${SROOT}
export DEPS_HOME=<directory containing dependencies>
export USE_REAL_TRANSLATIONS=true
export SLESNORPMS=yes
export IMASERVER_BASE_DIR=$BROOT/rpmtree
export JAVA_HOME=<suitable Java 8 SDK e.g. /etc/alternatives/java_sdk>
export PATH=$JAVA_HOME/bin:$PATH

cd $SROOT/server_build
ant -f $SROOT/server_build/build.xml  2>&1 | tee /tmp/ant.log
```
The output (in the form of rpms and tar bundles) is put under `$BROOT/rpms`

# Dependencies
Building the default set of components from this repository requires the following to be downloaded
and placed into a directory (set the $DEP_HOME environment variable to point to this directory)

* openliberty-23.0.0.12.zip
    https://openliberty.io/downloads/
* jackson-annotations-2.16.0.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/core/jackson-annotations/2.16.0/
* jackson-core-2.16.0.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/core/jackson-core/2.16.0/
* jackson-databind-2.16.0.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/core/jackson-databind/2.16.0/
* jackson-jaxrs-base-2.16.0.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/jaxrs/jackson-jaxrs-base/2.16.0/
* jackson-jaxrs-json-provider-2.16.0.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/jaxrs/jackson-jaxrs-json-provider/2.16.0/
* jackson-module-jaxb-annotations-2.16.0.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/module/jackson-module-jaxb-annotations/2.16.0/
* icu4j-74.1.jar
    https://github.com/unicode-org/icu/releases/tag/release-74-1
* dojo-release-1.17.3-src.zip
    https://download.dojotoolkit.org/release-1.17.3/ 
* gridx-1.3.9.zip
    https://github.com/oria/gridx/releases
* d3-3.5.6.zip
    https://github.com/d3/d3/releases/download/v3.5.6/d3.zip (download d3.zip and rename)
* jms.jar
    https://mvnrepository.com/artifact/org.apache.geronimo.specs/geronimo-jms_1.1_spec/1.1.1/

Some of the non-default components (e.g. the bridge to IBM MQ) require other dependencies - see
the next section to see dependencies for individual components

# Building individual components
Some components (like Amlen server) could be packaged by Linux distros. For suggested ways
of building individual components see the [server_build/distrobuild](server_build/distrobuild) sub 
directory.

# Build container
There is a a Docker file and build script that we use to do the build at Eclipse that show downloading
of the dependencies and setting of the environment variables. There can be found in the 
[server_build/buildcontainer](server_build/buildcontainer) directory.

# Releasing
Releasing of Amlen itself is done via the eclipse release process

Releasing the operator (and bundle) is handled seperately see [operator/README.md](operator/README.md)

