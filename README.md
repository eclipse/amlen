# Amlen

This repo contains the source for the Amlen Message Broker.
It contains (amongst other things) the Amlen Server itself, a WebUI and
a bridge (to MQTT brokers or Kafka)

## Building
Requires an x86-64 Linux distribution. Amlen has been built on RHEL7, RHEL8
and recent versions of Fedora. Package names vary slightly between distributions
(Fedora package names are listed here).

Install the prereq packages:
```
sudo dnf install openssl-devel openldap-devel boost-devel CUnit-devel pam-devel curl-devel 
sudo dnf install gcc make vim-common gcc-c++ net-snmp-devel libicu-devel
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

cd $SROOT/server_build
ant -f $SROOT/server_build/build.xml  2>&1 | tee /tmp/ant.log
```
# Using Amlen
Amlen has evolved from an IBM Product called "IBM WIoTP Message Gateway" and since usage of Amlen
is very similar, the best source of information is the Message Gateway documentation at:
https://ibm.biz/messagegateway_v50_docs

(We plan shortly to create "Getting Started" and "Migration" guides and host our own copy of the docs)

# Dependencies
Building the default set of components from this repository requires the following to be downloaded
and placed into a directory (set the $DEP_HOME environment variable to point to this directory)

* d3.zip
    https://d3js.org/
* openliberty-21.0.0.2.zip
    https://openliberty.io/downloads/
* jackson-annotations-2.12.3.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/core/jackson-annotations/2.12.3/
* jackson-core-2.12.3.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/core/jackson-core/2.12.3/
* jackson-databind-2.12.3.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/core/jackson-databind/2.12.3/
* jackson-jaxrs-base-2.12.3.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/jaxrs/jackson-jaxrs-base/2.12.3/
* jackson-jaxrs-json-provider-2.12.3.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/jaxrs/jackson-jaxrs-json-provider/2.12.3/
* jackson-module-jaxb-annotations-2.12.3.jar
    https://repo1.maven.org/maven2/com/fasterxml/jackson/module/jackson-module-jaxb-annotations/2.12.3/
* dojo-webui-release-1.16.3-src.zip
    https://download.dojotoolkit.org/release-1.16.3/ (download and rename src.zip_)
* gridx-webui-1.3.9.zip
    https://github.com/oria/gridx/releases (download and rename)
* icu4j-69_1.jar
    https://github.com/unicode-org/icu/releases/tag/release-69-1
* fscontext.jar
* jms.jar
* providerutil.jar
* ibm-java-jre-8.0-6.15-linux-x86_64.tgz 
    https://www.ibm.com/support/pages/java-sdk-downloads-version-80
    Download the AMD64 simple unzip with license, run it to extract and then create tgz with
    the correct name (we will streamline our handling of Java JRE's before first release)

Some of the non-default components (e.g. the bridge to IBM MQ) require other dependencies - see
the next section to see dependencies for individual components

# Building individual components
Some components (like Amlen server) could be packaged by Linux distros. For suggested ways
of building individual components see the [server_build/distrobuild](server_build/distrobuild) sub 
directory.
