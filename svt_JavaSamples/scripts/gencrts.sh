##!/bin/sh

if [ "$1" == "" -o "$2" == "" -o "$3" == "" ] ; then
  echo "usage: $0 <cert size> <dir to create> <trust chain depth (i.e. number of intermediate CA)> [# of revoked certs]"
  echo "  e.g. $0 2048 certs2Kb 1 (1 intermediate CA)"
  echo "  e.g. $0 2048 certs2Kb 1 10000 (generate 10,000 revoked certs) "
  exit 1
fi

SIZE=$1
DIR=$2
DEPTH=$3
REVCRTS=$4
CURRDIR=$(pwd)

rm -rf $DIR
mkdir -p $DIR ; cd $DIR

# Create certificate extensions file for CA's
cat > ext.cfg <<EOF
[ req ]
attributes             = req_attributes
distinguished_name     = req_distinguished_name
req_extensions         = v3_ca
prompt                 = no
[ req_attributes ]
[ v3_ca ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer
basicConstraints       = CA:true
[ req_distinguished_name ]
countryName                     = US
stateOrProvinceName             = Texas
localityName                    = Austin
organizationalUnitName          = IBM SWG AIM MessageSight Perf Team
commonName                      = IMA perf lab
emailAddress                    = imaperf@us.ibm.com
EOF

# Create certificate extensions file for IMA server
cat > srvext.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
#subjectAltName          = DNS:*.test.example.com
subjectAltName          = DNS:*.softlayer.com
EOF

# Generate RSA keys of specified size
openssl genrsa -out rootCA-key.pem            $SIZE >/dev/null 2>/dev/null
for i in `seq 1 $DEPTH`
do
  openssl genrsa -out intCA$i-server-key.pem      $SIZE >/dev/null 2>/dev/null
  openssl genrsa -out intCA$i-client-key.pem      $SIZE >/dev/null 2>/dev/null
done
openssl genrsa -out imaserver-key.pem         $SIZE >/dev/null 2>/dev/null
openssl genrsa -out perfclient-key.pem        $SIZE >/dev/null 2>/dev/null

## Leval 0 of trust chain
# Create root CA certificate (self-signed)
openssl req -new -x509 -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate" -extensions v3_ca -config ext.cfg -set_serial 1 -key rootCA-key.pem -out rootCA-crt.pem

parentCACrtSrvr=rootCA-crt.pem
parentCAKeySrvr=rootCA-key.pem
parentCACrtClnt=rootCA-crt.pem
parentCAKeyClnt=rootCA-key.pem

## $DEPTH levels of trust chain
for i in `seq 1 $DEPTH`
do 
  # Create certificate request for intermediate CA (server trust chain)
  openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=AIMDEV/CN=IMA Development Team - Int. CA Level $i (server trust chain)" -key intCA${i}-server-key.pem -out intCA${i}-server-crt.csr
  # Create intermediate CA (server trust chain) certificate using the root CA as the issuer
  openssl x509 -days 3650 -in intCA${i}-server-crt.csr -out intCA${i}-server-crt.pem -req -CA $parentCACrtSrvr -CAkey $parentCAKeySrvr -set_serial 1 -extensions v3_ca -extfile ext.cfg 2>/dev/null
  parentCACrtSrvr=intCA${i}-server-crt.pem
  parentCAKeySrvr=intCA${i}-server-key.pem

  # Create certificate request for intermediate CA (client trust chain)
  openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=AIMDEV/CN=IMA Development Team - Int. CA Level $i (client trust chain)" -key intCA${i}-client-key.pem -out intCA${i}-client-crt.csr
  # Create intermediate CA (server trust chain) certificate using the root CA as the issuer
  openssl x509 -days 3650 -in intCA${i}-client-crt.csr -out intCA${i}-client-crt.pem -req -CA $parentCACrtClnt -CAkey $parentCAKeyClnt -set_serial 1 -extensions v3_ca -extfile ext.cfg 2>/dev/null
  parentCACrtClnt=intCA${i}-client-crt.pem
  parentCAKeyClnt=intCA${i}-client-key.pem
done

## Server and client certs
# Create the IMA server certificate request
openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMAPERF/CN=IMA Performance Team(server)" -key imaserver-key.pem -out imaserver-crt.csr
# Create the IMA server certificate using the intermediate CA as the issuer
openssl x509 -days 3650 -in imaserver-crt.csr -out imaserver-crt.pem -req -CA $parentCACrtSrvr -CAkey $parentCAKeySrvr -extensions v3_req -extfile srvext.cfg -set_serial 1 2>/dev/null

# Create the perf client certificate request
openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMAPERF/CN=IMA Performance Team(perfclient)" -key perfclient-key.pem -out perfclient-crt.csr
# Create the perf client certificate using the intermediate CA as the issuer
openssl x509 -days 3650 -in perfclient-crt.csr -out perfclient-crt.pem -req -CA $parentCACrtClnt -CAkey $parentCAKeyClnt -extensions v3_req -set_serial 2 2>/dev/null

# Create "server trust chain" certificate file containing certificates for the entire server trust chain minus the root CA
echo "Creating server-trust-chain.pem file containing the entire server trust chain, minus the root CA"
cat imaserver-crt.pem > server-trust-chain.pem 
for i in `seq 1 $DEPTH`
do
  cat intCA${i}-server-crt.pem >> server-trust-chain.pem
done
cat rootCA-crt.pem >> server-trust-chain.pem

# Create "client trust chain" certificate file containing certificates for the entire client trust chain including the root CA
## NOTE: currently MessageSight does not accept trust chain (i.e. requires individual CA certs)
#echo "Creating client-trust-chain.pem file containing the entire client trust chain, including the root CA"
#cat perfclient-crt.pem > client-trust-chain.pem 
#rm -f client-trust-chain.pem
#touch client-trust-chain.pem
#for i in `seq 1 $DEPTH`
#do
#  cat intCA${i}-client-crt.pem >> client-trust-chain.pem
#done
#cat rootCA-crt.pem >> client-trust-chain.pem

echo -n "Do you want to view the certificates created? [y/N]: "
read response
if [ "$response" == "y" -o "$response" == "Y" ] ; then
  echo "ROOT CA..."
  openssl x509 -in rootCA-crt.pem -text -noout -purpose

  echo "Intermediate CA's.."
  for i in `seq 1 $DEPTH`
  do
    echo "Level $i Intermediate CA (server chain)"
    openssl x509 -in intCA${i}-server-crt.pem -text -noout -purpose
    echo "Level $i Intermediate CA (client chain)"
    openssl x509 -in intCA${i}-client-crt.pem -text -noout -purpose

  done

  echo "MessageSight server certificate..."
  openssl x509 -in imaserver-crt.pem -text -noout -purpose
  echo "Perf client certificate..."
  openssl x509 -in perfclient-crt.pem -text -noout -purpose
fi

# Create PKCS 12 bundle for CA certificates for importing

openssl pkcs12 -export -inkey rootCA-key.pem -in rootCA-crt.pem -out rootCA.p12 -password pass:

# Create a java key store and import the root and intermediate CA certificates into the java key store
IBM_JAVA_HOME=/opt/ibm/java-x86_64-80
ORACLE_JAVA_HOME=/usr/java/jdk1.7.0_02

if [ ! -e $ORACLE_JAVA_HOME ] ; then
  echo "Oracle JVM not installed @ $ORACLE_JAVA_HOME, will not generate Java keystore for ORACLE JVM.  Modify this script to point to ORACLE JAVA HOME"
else
  export EMPTYPASS=password
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCA-crt.pem -keystore oracle.jks -storepass:env EMPTYPASS -noprompt -alias root
fi

if [ ! -e $IBM_JAVA_HOME ] ; then
  echo "IBM JVM not installed @ $IBM_JAVA_HOME, will not generate Java keystore for IBM JVM.  Modify this script to point to IBM JAVA HOME"
else
  export EMPTYPASS=password
  $IBM_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCA-crt.pem -keystore ibm.jks -storepass:env EMPTYPASS -noprompt -alias root
fi

if [ "$REVCRTS" != "" ] ; then
  echo "Generating revoked certificates (`date`)..."
  sudo rm /etc/pki/CA/index.txt
  sudo touch /etc/pki/CA/index.txt
  sudo touch /etc/pki/CA/crlnumber
  sudo chmod 777 /etc/pki/CA/crlnumber
  sudo echo 1000 > /etc/pki/CA/crlnumber

  for i in `seq 1 $(($REVCRTS/2))` ; do
    openssl genrsa -out revoked-$i.pem        $SIZE >/dev/null 2>/dev/null
    openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMAPERF/CN=IMA Performance Team(revoked $i client)" -key revoked-$i.pem -out revoked-$i-crt.csr >/dev/null 2>/dev/null
    openssl x509 -days 3650 -in revoked-$i-crt.csr -out revoked-$i-crt.pem -req -CA $parentCACrtClnt -CAkey $parentCAKeyClnt -extensions v3_req -set_serial $((1000 + $i)) 2>/dev/null
    sudo openssl ca -gencrl -crldays 3650 -keyfile $parentCAKeyClnt -cert $parentCACrtClnt -revoke revoked-$i-crt.pem >/dev/null 2>/dev/null
  done  
  echo "Done generating revoked certificates (`date`)."

  sudo openssl ca -gencrl -keyfile $parentCAKeyClnt -cert $parentCACrtClnt -out ${parentCACrtClnt%%.pem}-crl.pem


  for i in `seq $(($REVCRTS/2))  $REVCRTS` ; do
    openssl genrsa -out revoked-$i.pem        $SIZE >/dev/null 2>/dev/null
    openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMAPERF/CN=IMA Performance Team(revoked $i client)" -key revoked-$i.pem -out revoked-$i-crt.csr >/dev/null 2>/dev/null
    openssl x509 -days 3650 -in revoked-$i-crt.csr -out revoked-$i-crt.pem -req -CA $parentCACrtClnt -CAkey $parentCAKeyClnt -extensions v3_req -set_serial $((1000 + $i)) 2>/dev/null
    sudo openssl ca -gencrl -crldays 3650 -keyfile $parentCAKeyClnt -cert $parentCACrtClnt -revoke revoked-$i-crt.pem >/dev/null 2>/dev/null
  done  
  echo "Done generating revoked certificates (`date`)."

  sudo openssl ca -gencrl -keyfile $parentCAKeyClnt -cert $parentCACrtClnt -out ${parentCACrtClnt%%.pem}-upd-crl.pem


  echo -n "We have our crl file.  Do you want to delete the revoked certificates? [y/N]: "
  read response
  if [ "$response" == "y" -o "$response" == "Y" ] ; then
    rm -f revoked*
  fi
fi



# Clean up files
rm -f *.cfg
rm -f pass.txt
rm -r *.csr
cd $CURRDIR

#end of script
