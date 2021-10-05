#!/bin/bash

source cluster.cfg

i=$1

command() {
echo curl -X POST -d '{ "SecurityProfile":{ "SVTSecProf2":{"MinimumProtocolMethod":"SSLv3","UseClientCertificate":false,"Ciphers":"Fast","UsePasswordAuthentication":true,"CertificateProfile":"SVTCertProf"}} }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
curl -X POST -d '{ "SecurityProfile":{ "SVTSecProf2":{"MinimumProtocolMethod":"SSLv3","UseClientCertificate":false,"Ciphers":"Fast","UsePasswordAuthentication":true,"CertificateProfile":"SVTCertProf"}} }' http://${SERVER[$i]}:${PORT}/ima/v1/configuration
}


if [ -z $i ]; then
  for i in $(eval echo {1..${#SERVER[@]}}); do
    command
  done
else 
    command
fi


