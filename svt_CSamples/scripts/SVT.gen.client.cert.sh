#!/bin/bash

#-------------------------------------------------------
# RTC : 39862 : Create client cert with unique common name
#-------------------------------------------------------

CA=${1-""}
CAkey=${2-""}
CN=${3-1}

usage(){
    echo "-------------------------------------------------------"
    echo "Usage for $0: SVT Client Certificate Generator (RTC 39862)"
    echo ""
    echo "    $0 <rootca-crt.pem> <rootca-key.pem> <number>"
    echo ""
    echo "    Note:<number> will be postfixed and padded with 0s to form CommonName"
    echo "         so for example 2 will become CN0000002 "
    echo ""
    echo "               or "
    echo ""
    echo "         if <number> == "MILLION" then a loop will be started creating 1 million client certs"
    echo "-------------------------------------------------------"
    echo ""
    echo "Example 1: Create a single client certificate for CN0000002"
    echo "$0  /niagara/test/common/rootCA-crt.pem  /niagara/test/common/rootCA-key.pem 2"
    echo ""
    echo "Example 2: Generating 1 million unique certs" 
    echo "$0  /niagara/test/common/rootCA-crt.pem  /niagara/test/common/rootCA-key.pem MILLION"
    echo ""

}

if [ -z $CA -o -z $CAkey ] ;then 
    usage
    exit 1;
elif [ "$CN" == "MILLION" ] ;then 
    echo "Creating a Million client certificates"
    rm -rf file.srl #start from scratch
    rm -rf client.cnf # start clean.

    rm -rf certificates
    mkdir certificates
    
    let v=0; while (($v<1000000)); do  
        curdir=`pwd`
        rm -rf CN0*.key
        rm -rf CN0*.pem
        mkdir certificates/$v; 
        let k=0;
        echo -n "$v "
        while(($k<1000)); do
            let cn=($v+$k)
            echo -n "."
            $0 $CA $CAkey $cn 2> /dev/null 1>/dev/null;
            let k=$k+1;
        done
        mv -f CN0*.key certificates/$v
        mv -f CN0*.pem certificates/$v
        cd certificates
        tar -czvf $v.tgz $v
        rm -rf $v
        cd $curdir
        let v=$v+1000; 
    done

    exit 0;
fi

if [ ! -e client.cnf ] ;then
	echo "[dir_sect]

keyUsage=digitalSignature,keyEncipherment

subjectKeyIdentifier=hash

authorityKeyIdentifier=keyid,issuer

subjectAltName=otherName:1.3.6.1.4.1.311.20.2.3;UTF8:someuser@some.company.com, email:someuser@some.company.com" > client.cnf

fi

if [ ! -e file.srl ] ;then
	echo "0000000000" > file.srl
fi

CN=`echo $CN | awk '{printf("CN%07d", $1);}'`
echo "Generating client certificate for CN $CN"

rm -rf ${CN}.key
rm -rf client.req
rm -rf ${CN}.pem
openssl genrsa -out ${CN}.key 1024
openssl req -days 3650 -key ${CN}.key -new -out client.req  -subj "/C=US/O=IBM/OU=SVT/CN=${CN}"
openssl x509 -req -days 3650 -in client.req  -CA $CA -CAkey $CAkey -CAserial file.srl -extfile client.cnf -extensions dir_sect -out $CN.pem

