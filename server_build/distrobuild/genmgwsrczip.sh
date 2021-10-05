# Copyright (c) 2020-2021 Contributors to the Eclipse Foundation
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

tmpdir=/var/tmp
finaldest=~/rpmbuild/SOURCES/

sourcelocation=$1
sourcename=$2

if [ -z "$sourcename" ]; then
  sourcename=messagegateway-buildoss
fi

if [ -z "$sourcelocation" ]; then
  if [ ! -z "$SROOT" ]; then
    sourcelocation=$SROOT
  else
    sourcelocation=~/ghe/messagegateway
  fi
fi

echo "Creating $sourcename.zip contain $sourcename dir containing contents of $sourcelocation"

tmppath=$tmpdir/$sourcename

rm $tmppath
ln -s $sourcelocation $tmppath

cd $tmpdir
zipname=${sourcename}.zip
zip -r "$zipname" "$sourcename" '-x*/.git/*' '-x*/target/*' '-x*/svt_*' '-x*/fvt_*' '-x*/testTools_*' '-x*/Documentation/doc_infocenter/*' '-x*/perftools/*'

echo "Moving $zipname to $finaldest"
mv "$zipname" $finaldest/

rm $tmppath
