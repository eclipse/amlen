#!/bin/bash

#------------------------------------------------------------------------------
# gencrts.sh
#
# Creates TLS certificates used in MessageSight performance benchmark testing. 
# This script is designed to create a server trust chain and client trust chain.  
# This script will generate the following certificates:
#   1) a single root CA (self-signed) CA certificate
#   2) an intermediate CA certificate (issued by the root CA) for the server trust chain
#   3) an intermediate CA certificate (issued by the root CA) for the client trust chain
#   3) a messaging server certificate (issued by the server intermediate CA)
#   4) a number of client certificates (issued by the client intermediate CA), by default only one 
#   5) a crl file containing 10K revoked certificates (issued by the client intermediate CA)
#   6) a Java trust store containing the root CA
#   7) a pre-shared key file containing 1M entries
#   8) an MQ server certificate, key database, and stash file
#   9) an LDAP server certificate
#  10) an OAuth server certificate
#------------------------------------------------------------------------------

function usage() {
  echo "usage: $0 [-h] [-s <keysize>]"
  echo "  e.g. $0 -h"
  echo "  e.g. $0 -s 2048"
  echo "- by default this script will use a key size of 2048 bits"
  return 0
}

SIZE=2048
DIR=certs
NUMCLIENTCERTS=1
NUMREVOKED=10000
NUMPSKENTRIES=1000000
MSGDNS=<hostname of mgw server>
CLNTCDP=<hostname of crl server>
CURRDIR=$(pwd)
KEYTOOL=$JAVA_HOME/bin/keytool
GSK8HOME=/usr/local/ibm/gsk8_64
GSK8CMD=$GSK8HOME/bin/gsk8capicmd_64
GSK8LIB=$GSK8HOME/lib64

export LD_LIBRARY_PATH=$GSK8LIB:$LD_LIBRARY_PATH

function genpsk() {
python - <<EOFUNCTION
#!/usr/bin/python
import sys,os,binascii

for i in range(0, $NUMPSKENTRIES):
  print("uid{0},{1}".format(i,binascii.b2a_hex(os.urandom(16))))

EOFUNCTION
}


if [ "$JAVA_HOME" == "" ] ; then 
  echo "java keytool is required to create the Java trust store, set the JAVA_HOME env variable to be location of your java install directory, exiting..."
  exit 1
fi

while getopts ":hs" arg; do
  case $arg in
    s) # key size
        SIZE=${OPTARG}
        ;;
    h|*) # help, print usage info
        usage
        exit 0
        ;;
  esac
done

# cleanup existing certificates in the
rm -rf $DIR
mkdir -p $DIR 
cd $DIR

# Create certificate extensions file for the root CA
cat > rootext.cfg <<EOF
[ req ]
attributes             = req_attributes
distinguished_name     = req_distinguished_name
req_extensions         = v3_ca
prompt                 = no
[ req_attributes ]
[ v3_ca ]
keyUsage               = critical,keyCertSign,cRLSign
basicConstraints       = critical,CA:true,pathlen:1
subjectKeyIdentifier   = hash
authorityKeyIdentifier = keyid,issuer
certificatePolicies    = 1.2.3.4
EOF

# Create certificate extensions file for the intermediate CAs
cat > intcaext.cfg <<EOF
[ req ]
attributes             = req_attributes
distinguished_name     = req_distinguished_name
req_extensions         = v3_ca
prompt                 = no
[ req_attributes ]
[ v3_ca ]
keyUsage               = critical,keyCertSign,cRLSign
basicConstraints       = critical,CA:true,pathlen:0
subjectKeyIdentifier   = hash
authorityKeyIdentifier = keyid,issuer
certificatePolicies    = 1.2.3.4
EOF

# Create certificate extensions file for the messaging server
cat > msgext.cfg <<EOF
[ req ]
attributes              = req_attributes
distinguished_name      = req_distinguished_name
req_extensions          = v3_req
prompt                  = no
[ req_attributes ]
[ v3_req ]
keyUsage                = critical,digitalSignature,keyEncipherment
extendedKeyUsage        = critical,serverAuth
subjectKeyIdentifier    = hash
authorityKeyIdentifier  = keyid,issuer
certificatePolicies     = @polsection
subjectAltName          = critical, @alt_names

[ alt_names ]
DNS.1                   = ${MSGDNS}

[ polsection ]
policyIdentifier        = 1.2.3.4
CPS.1                   = http://w3-03.ibm.com/transform/sas/as-web.nsf/ContentDocsByTitle/Information+Technology+Security+Standards
userNotice.1            = @notices

[ notices ] 
explicitText        = "Use of this certificate should adhere to IBM Corporate standards ITCS104 and ITCS300"
EOF

# Create certificate extensions file for the OAuth and LDAP servers
cat > authext.cfg <<EOF
[ req ]
attributes              = req_attributes
distinguished_name      = req_distinguished_name
req_extensions          = v3_req
prompt                  = no
[ req_attributes ]
[ v3_req ]
keyUsage                = critical,digitalSignature,keyEncipherment
extendedKeyUsage        = critical,serverAuth
subjectKeyIdentifier    = hash
authorityKeyIdentifier  = keyid,issuer
certificatePolicies     = @polsection

[ polsection ]
policyIdentifier        = 1.2.3.4
CPS.1                   = http://w3-03.ibm.com/transform/sas/as-web.nsf/ContentDocsByTitle/Information+Technology+Security+Standards
userNotice.1            = @notices

[ notices ] 
explicitText        = "Use of this certificate should adhere to IBM Corporate standards ITCS104 and ITCS300"
EOF

# Create certificate extensions file for clients
cat > clientext.cfg <<EOF
[ req ]
attributes              = req_attributes
distinguished_name      = req_distinguished_name
req_extensions          = v3_req
prompt                  = no
[ v3_req ]
keyUsage                = critical,digitalSignature,keyEncipherment
extendedKeyUsage        = critical,clientAuth
subjectKeyIdentifier    = hash
authorityKeyIdentifier  = keyid,issuer
certificatePolicies     = @polsection
crlDistributionPoints   = URI:https://${CLNTCDP}/crl/cert-intCA-crl.pem

[ polsection ]
CPS.1                   = http://w3-03.ibm.com/transform/sas/as-web.nsf/ContentDocsByTitle/Information+Technology+Security+Standards
userNotice.1            = @usernotice

[ usernotice ] 
explicitText        = "Use of this certificate should adhere to IBM Corporate standards ITCS104 and ITCS300"
EOF

# We need to add some uniqueness to the subject name of the certificates to avoid a hash collision
rand1=$RANDOM 
rand2=$RANDOM

echo "Creating server and client trust chains"

# Generate RSA keys of specified size
openssl genrsa -out key-rootCA.pem            $SIZE >/dev/null 2>/dev/null
openssl genrsa -out key-server-intCA.pem      $SIZE >/dev/null 2>/dev/null
openssl genrsa -out key-client-intCA.pem      $SIZE >/dev/null 2>/dev/null
openssl genrsa -out key-server.pem            $SIZE >/dev/null 2>/dev/null
openssl genrsa -out key-oauth.pem             $SIZE >/dev/null 2>/dev/null
openssl genrsa -out key-ldap.pem              $SIZE >/dev/null 2>/dev/null
openssl genrsa -out key-mq.pem                $SIZE >/dev/null 2>/dev/null

# Create the root CA certificate request (self-signed)
openssl req -new -sha256 -x509 -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate(r1-${rand1} r2-${rand2})/CN=IBM Corporate(root)" -key key-rootCA.pem -out rootCA-crt.csr
# Create the root CA certificate using the root CA as the issuer
openssl x509 -days 3560 -in rootCA-crt.csr -out cert-rootCA.pem -extensions v3_ca -extfile rootext.cfg -set_serial "1"

# Create the server intermediate CA certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=WIoTP MessageSight Org(server intermediate)" -key key-server-intCA.pem -out intCA-server-crt.csr
# Create the server intermediate CA certificate using the root CA as the issuer
openssl x509 -days 3560 -in intCA-server-crt.csr -out cert-server-intCA.pem -req -sha256 -CA cert-rootCA.pem -CAkey key-rootCA.pem -extensions v3_ca -extfile intcaext.cfg -set_serial "11" 2>/dev/null

# Create the client intermediate CA certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=WIoTP MessageSight Org(client intermediate)" -key key-client-intCA.pem -out intCA-client-crt.csr
# Create the client intermediate CA certificate using the root CA as the issuer
openssl x509 -days 3560 -in intCA-client-crt.csr -out cert-client-intCA.pem -req -sha256 -CA cert-rootCA.pem -CAkey key-rootCA.pem -extensions v3_ca -extfile intcaext.cfg -set_serial "12" 2>/dev/null

# Create the Messaging Server certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=${MSGDNS}" -key key-server.pem -out server-crt.csr
# Create the Messaging Server certificate, using the server intermediate CA as the issuer
openssl x509 -days 3560 -in server-crt.csr -out cert-server.pem -req -sha256 -CA cert-server-intCA.pem -CAkey key-server-intCA.pem -extensions v3_req -extfile msgext.cfg -set_serial "111" 2>/dev/null

# Create the OAuth Server certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=WIoTP MessageSight OAuth Server" -key key-oauth.pem -out oauth-crt.csr
# Create the OAuth Server certificate, using the server intermediate CA as the issuer
openssl x509 -days 3560 -in oauth-crt.csr -out cert-oauth.pem -req -sha256 -CA cert-server-intCA.pem -CAkey key-server-intCA.pem -extensions v3_req -extfile authext.cfg -set_serial "112" 2>/dev/null
# Create the OAuth Server certificate Combined with the private key required for MessageSight OAuth Profile.
cat key-oauth.pem cert-oauth.pem > certandkey-oauth.pem

# Create the LDAP Server certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=WIoTP MessageSight LDAP Server" -key key-ldap.pem -out ldap-crt.csr
# Create the LDAP Server certificate, using the server intermediate CA as the issuer
openssl x509 -days 3560 -in ldap-crt.csr -out cert-ldap.pem -req -sha256 -CA cert-server-intCA.pem -CAkey key-server-intCA.pem -extensions v3_req -extfile authext.cfg -set_serial "113" 2>/dev/null

# Create the MQ Server certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=WIoTP MessageSight MQ Server" -key key-mq.pem -out mq-crt.csr
# Create the MQ Server certificate, using the server intermediate CA as the issuer
openssl x509 -days 3560 -in mq-crt.csr -out cert-mq.pem -req -sha256 -CA cert-server-intCA.pem -CAkey key-server-intCA.pem -extensions v3_req -extfile authext.cfg -set_serial "114" 2>/dev/null


# Client certificate loop
for i in `seq 1 $NUMCLIENTCERTS` ; do
  # Generate RSA keys of specified size
  openssl genrsa -out key-client-${i}.pem            $SIZE >/dev/null 2>/dev/null
  # Create the client certificate request
  openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=perfclient${i}" -key key-client-${i}.pem -out client-crt-${i}.csr
  # Create the client certificate, using the client intermediate CA as the issuer
  openssl x509 -days 3560 -in client-crt-${i}.csr -out cert-client-${i}.pem -req -sha256 -CA cert-client-intCA.pem -CAkey key-client-intCA.pem -extensions v3_req -extfile clientext.cfg -set_serial "12${i}" 2>/dev/null
  i=$((i+1))
done

mv cert-client-1.pem cert-client.pem
mv key-client-1.pem key-client.pem

# Revoked client certificate loop
STARTINGSERIAL=999999
echo "Generating $NUMREVOKED revoked certificates, this can take an hour (`date`)..."
# default OpenSSL CA dir assumes (CentOS or RedHat)
OPENSSLCADIR=/etc/pki/CA
grep -qi ubuntu /etc/issue
rc=$?
if [ "$rc" = "0" ] ; then
  OPENSSLCADIR=./demoCA
fi

sudo rm $OPENSSLCADIR/index.txt 2>/dev/null
sudo mkdir -p $OPENSSLCADIR
sudo touch $OPENSSLCADIR/index.txt
sudo touch $OPENSSLCADIR/crlnumber
sudo chmod 777 $OPENSSLCADIR/crlnumber
sudo echo $STARTINGSERIAL > $OPENSSLCADIR/crlnumber

# Generate the RSA key for the revoked certificates
openssl genrsa -out key-revoked.pem     $SIZE >/dev/null 2>/dev/null

# Create the revoked certificate request
openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=WIoTP MessageSight Org(r1-${rand1} r2-${rand2})/CN=revoked client" -key key-revoked.pem -out revoked-crt.csr >/dev/null 2>/dev/null

revcount=0
for i in `seq 0 $NUMREVOKED` ; do
  # Create the client certificate
  openssl x509 -days 3650 -in revoked-crt.csr -out cert-revoked-$i.pem -req -CA cert-client-intCA.pem -CAkey key-client-intCA.pem -extensions v3_req -set_serial $((STARTINGSERIAL + RANDOM*i)) 2>/dev/null
  
  # Revoke the client certificate
  openssl ca -gencrl -crldays 3650 -keyfile key-client-intCA.pem -cert cert-client-intCA.pem -revoke cert-revoked-$i.pem >/dev/null 2>/dev/null
  
  # Cleanup revoked certificate (required info is already in $OPENSSLCADIR/index.txt)
  if [ $((revcount % 1000)) == 0 ] ; then
    rm -f cert-revoked-*.pem
  fi
  
  revcount=$((revcount + 1))
done

# Generate the CRL file
echo "Generating CRL file from revoked certificates"
openssl ca -gencrl -crldays 3650 -keyfile key-client-intCA.pem -cert cert-client-intCA.pem -out cert-client-intCA-crl.pem

echo "Completed generating crl file (`date`)"
echo "Compressing CRL file"
gzip cert-client-intCA-crl.pem

# Cleanup revoked cert artifacts
rm -f revoked-crt.csr
rm -f cert-revoked-*.pem
if [ "${OPENSSLCADIR:0:1}" = "." ] ; then
  rm -rf $OPENSSLCADIR
fi

# Build the server trust chain
cat cert-server.pem             >   server-trust-chain.pem
cat cert-server-intCA.pem       >>  server-trust-chain.pem
cat cert-rootCA.pem             >>  server-trust-chain.pem

# Build the client trust chain
cat cert-client.pem             >   client-trust-chain.pem
cat cert-client-intCA.pem       >>  client-trust-chain.pem
cat cert-rootCA.pem             >>  client-trust-chain.pem

# Create the Java trust store
echo "Creating Java trust store"
export EMPTYPASS=password
$KEYTOOL -import -trustcacerts -file cert-rootCA.pem -keystore truststore.jks -storepass:env EMPTYPASS -noprompt -alias root

# Create the oauth2 key store
echo "Creating oauth2 key store: create temporary file testkeystore.p12"
openssl pkcs12 -export -in cert-oauth.pem -inkey key-oauth.pem -certfile cert-oauth.pem -out testkeystore.p12 -password env:EMPTYPASS
echo "Creating oauth2 key store : oauth2.jks"
$KEYTOOL -importkeystore -srckeystore testkeystore.p12 -srcstoretype pkcs12 -destkeystore oauth2.jks -deststoretype JKS -srcstorepass:env EMPTYPASS -deststorepass:env EMPTYPASS -noprompt
echo "Creating oauth2 key store : removing temporary file testkeystore.p12"
rm -rf testkeystore.p12


# Generate MQ KeyDB and Stash file
if [ -e $GSK8CMD ] ; then
    echo "Create new MQ Key Database"
    $GSK8CMD -keydb -create -db mq.kdb -pw $EMPTYPASS -type cms -expire 3650 -stash
    $GSK8CMD -cert -add -db mq.kdb -pw $EMPTYPASS -format binary -trust enable -file cert-mq.pem -label mqserver
fi

# Generate PSK file
echo "Generating PSK File"
genpsk > psk.csv
echo "Compressing generated PSK File"
gzip psk.csv

# Cleanup
rm -f *.cfg
rm -f *.csr




