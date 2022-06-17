#!/bin/bash
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
# This script is really the guts of the jenkins.uploadcontrib jenkins
# script (but it is hard to embed in that file due to having to escape the script 
# as a groovy string)
#
if [ "${BulkMode}" == "true" ]
then
    BULKFILEDIR="server_build/buildcontainer/bulkcontriblists"
    BULKFILELISTS="${BULKFILEDIR}/*"

    #We'd like to do our normal variable inserts on file list like:
    #python server_build/path_parser.py -m dirreplace -i ${BULKFILEDIR} -o ${BULKFILEDIR}
    #sadly python isn't installed but perl is so we can jury-rig inserting the version:
    VERSION_STRING=`perl -ne '/ISM_VERSION_ID=(.*)/ and print "$1"' server_build/paths.properties`
    perl -pi -e "s/\\\$\\{ISM_VERSION_ID\\}/${VERSION_STRING}/g" ${BULKFILELISTS}
    
    for listfilepath in $BULKFILELISTS
    do
        listfilename=`basename ${listfilepath}`
        echo "Processing ${listfilepath}"
        dldir="dl-${listfilename}"
        mkdir ${dldir}
        wget -nH -np --cut-dirs=1 -i ${listfilepath} -P ${dldir} -B 'https://amlen.org/contribbuilds/'
        ssh -o BatchMode=yes genie.amlen@projects-storage.eclipse.org mkdir -p /home/data/httpd/download.eclipse.org/amlen/${buildpath}/${listfilename}/contrib
        scp -o BatchMode=yes ${dldir}/* genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/${buildpath}/${listfilename}/contrib/
        rm -rf ${dldir}
    done
else
    wget "https://amlen.org/contribbuilds/${contributedpath}"
    ssh -o BatchMode=yes genie.amlen@projects-storage.eclipse.org mkdir -p /home/data/httpd/download.eclipse.org/amlen/${buildpath}/contrib
    scp -o BatchMode=yes ${contributedpath} genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/${buildpath}/contrib/
fi