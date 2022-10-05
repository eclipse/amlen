# Copyright (c) 2015-2022 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

#Choose a base image that matches the type of build we are using...
#By default we use: centos7
FROM quay.io/centos/centos:7
# For a RHEL8/CentOS8 distro, we use AlmaLinux:
#FROM quay.io/almalinux/almalinux:8
# For Fedora, the verion changes very often due to their short life
#FROM quay.io/fedora/fedora:35-x86_64

#File author / Maintainer
MAINTAINER ISM Build

# Update packages
RUN yum -y upgrade 

RUN yum install -y python3

ADD ./requirements.txt /tmp/

RUN pip3 install --upgrade -r /tmp/requirements.txt && \
    rm /tmp/requirements.txt

# scripts contains the scripts necessary for the readiness probe
ADD ./scripts/* /usr/share/bin/
# add settings to allow HA to work in a virtual environment more robustly

#Comment out the line below to run as root user
USER ima:ima

ENTRYPOINT [ "/usr/share/bin/monitor.py" ]
