# Use Centos7 base image
FROM quay.io/centos/centos:centos7 
#### Set the base image to alma8 (RHEL8 clone)
###FROM quay.io/almalinux/almalinux:8 

#File author / Maintainer
MAINTAINER ISM Build

WORKDIR ${IMA_SVR_INSTALL_PATH}/bin

# Add RPM in a temporary directory inside the container
# Make sure that imaserver.rpm is available in the directory where "docker build" command
# is being executed
ADD ./imaserver.rpm /tmp/

# Update packages
RUN yum -y upgrade && \
    echo 'group=root' >/etc/messagesight-user.cfg && \
    echo 'groupaccess=fullwrite' >>/etc/messagesight-user.cfg && \
    yum -y install /tmp/imaserver.rpm && \
    rm -f /tmp/imaserver.rpm && \
    yum -y clean all

ENTRYPOINT [ "${IMA_SVR_INSTALL_PATH}/bin/startServer.sh" ]
