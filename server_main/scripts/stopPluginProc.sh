#!/bin/bash
# Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#set -x

DATE=`date`
echo "------------------- $DATE -------------------"


s=`ps -ef -w -w |grep java | grep imaPlugin.jar` 
if [ "$s" != "" ]; then
jpid=`echo $s | awk '{print $2}' `
echo "Killing Plugin process with pid = $jpid" 
kill -9 $jpid
else
echo "Plugin process is not running"
fi
exit 0

