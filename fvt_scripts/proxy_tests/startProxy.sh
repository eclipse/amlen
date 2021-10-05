#!/bin/bash +x
#
# 

# Source the ISMsetup file to get access to information for the remote machine
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

file=$1
# destfile=$file  NEED TO STRIP THE PATH from the input filename like when no $1 is passed like below
destfile="${file##*/}"

clientCRL=$2


if [[ "${file}" == "" ]] ; then
  file=../common/proxy.cfg
  destfile=proxy.cfg
fi

if [[ "${clientCRL}" == "" ]] ; then
  clientCRL=tls_certs/revcrl0.crl
fi
set -x
# P1, P2, ....
if [[ $# -ge 4 ]]; then
	PX_TEMP=$4_USER
	PX_USER=$(eval echo \$${PX_TEMP})
	PX_TEMP=$4_HOST
	PX_HOST=$(eval echo \$${PX_TEMP})
	PX_TEMP=$4_PROXYROOT
	PX_PROXYROOT=$(eval echo \$${PX_TEMP})
else  
	PX_USER=${P1_USER}
	PX_HOST=${P1_HOST}
	PX_PROXYROOT=${P1_PROXYROOT}
fi

if [[ "$1" == "?" ]] ; then
  echo "Usage is: "
  echo "     ./startProxy.sh proxy.cfg clientCRL-file [extra-file]"
  echo "   proxy.cfg is the config file to start the proxy with."
  echo "       if no file is specified, a file named 'proxy.cfg' is used." 
  echo "   clientCRL-file if specified will also be copied to the proxy directory and to /tmp/myca.crl."
  echo "        To use DEFAULT CRL, leave off or pass "." and tls_certs/revcrl0.crl will be copied."
  echo "   extra-file if specified will also be copied to the proxy directory. ex. Use for ACL File"
elif [ -f $file ]
then
  if [ ! -d "test" ]; then    
    mkdir test
  fi
  
  cp ../lib/clientproxytest.jar test/
  
  scp -r test ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
  scp -r keystore ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
  scp $clientCRL ${PX_USER}@${PX_HOST}:/tmp/myca.crl
  scp tls_certs/ca_revcrl0.crl ${PX_USER}@${PX_HOST}:/tmp/myica.crl
  scp tls_certs/rca_revcrl0.crl ${PX_USER}@${PX_HOST}:/tmp/myrca.crl
  # imaproxy_config.jar is already in the proxy lib directory. Just use it there.
  # and it is not in the ../lib directory.....
  #  scp ../lib/imaproxy_config.jar ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}/test
  if [[ "$2" != "" ]] ; then
     scp $2 ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
  fi

  if [[ "$3" != "" ]] ; then
     scp $3 ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}
  fi

  # This config_test does not exist anymore as far as I can tell
  # scp -r config_test ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}/config_test
  scp $file ${PX_USER}@${PX_HOST}:${PX_PROXYROOT}/${destfile}
  ssh ${PX_USER}@${PX_HOST} "cd ${PX_PROXYROOT};mkdir -p config"
  ssh ${PX_USER}@${PX_HOST} "cd ${PX_PROXYROOT}; ls -l config; pwd; ulimit -c unlimited; LD_LIBRARY_PATH=/niagara/proxy/lib imaproxy -d $destfile"&
  ssh ${PX_USER}@${PX_HOST} "ps -ef | grep imaproxy"
else
  echo "Cannot find config file '$file'."
fi
set +x
