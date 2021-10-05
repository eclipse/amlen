#!/bin/sh
#------------------------------------------------------------------------------
# gencrts.sh
#
# Create various ssl certificates for testing purposes.
#
#
# rootCA A - self signed root certificate
#   imaservera - server certificate signed by rootCA A - serial 1
#   imaclienta - client certificate signed by rootCA A - serial 2
#
#   imaCA A - intermediate CA signed by rootCA A - serial 3
#     imaserverb - server certificate signed by imaCA A - serial 1
#     imaclientb - client certificate signed by imaCA A - serial 2
#
#     imaCA B - intermediate CA signed by imaCA A - serial 3
#       imaserverc - server certificate signed by imaCA B - serial 1
#       imaclientc - client certificate signed by imaCA B - serial 2
#
# rootCA B - self signed root certificate
#   imaCA C - intermediate CA signed by rootCA B - serial 1
#     imaserverd - server certificate signed by imaCA C - serial 1
#     imaclientd - client certificate signed by imaCA C - serial 2
#
# The certificates checked into automation expire  September 5 14:49:27 2023 GMT
#------------------------------------------------------------------------------

#if [ "$1" == "" -o "$2" == "" ] ; then
#  echo "usage: $0 <cert size> <dir to create>"
#  echo "  e.g. $0 2048 certs2Kb"
#  exit 1
#fi

SIZE=2048
DIR="A3"
CURRDIR=$(pwd)

mkdir -p "${DIR}"
cd $DIR || exit 2

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
organizationalUnitName          = Corporate
commonName                      = IBM Corporate
EOF
# Create certificate extensions file for CA's
#cat > ext.cfg <<EOF
#[ req ]
#attributes             = req_attributes
#req_extensions         = v3_ca
#prompt                 = no
#[ req_attributes ]
#[ v3_ca ]
#basicConstraints       = CA:true
#EOF

# Create certificate extensions file for IMA server
cat > srvext.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName=IP:${A3_IPv4_1}
EOF

# Generate RSA keys of specified size
openssl genrsa -out rootCA-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaCA-key.pem      $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaserver-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaclient-key.pem $SIZE >/dev/null 2>/dev/null

# Create root CA A certificate (self-signed)
openssl req -new -x509 -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate" -extensions v3_ca -config ext.cfg -set_serial 1 -key rootCA-key.pem -out rootCA-crt.pem

# Create certificate request for the intermediate CA A
openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=AIMDEV/CN=IMA Development Team" -key imaCA-key.pem -out imaCA-crt.csr
# Create the intermediate CA A certificate using the root CA A as the issuer
openssl x509 -days 3650 -in imaCA-crt.csr -out imaCA-crt.pem -req -CA rootCA-crt.pem -CAkey rootCA-key.pem -set_serial 3 -extensions v3_ca -extfile ext.cfg 2>/dev/null

# Create the IMA server A certificate request
openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(server)" -key imaserver-key.pem -out imaserver-crt.csr
# Create the IMA server A certificate using the root CA A as the issuer
openssl x509 -days 3650 -in imaserver-crt.csr -out imaserver-crt.pem -req -CA rootCA-crt.pem -CAkey rootCA-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 1 2>/dev/null

# Create the ima client a certificate request
openssl req -new -days 3650 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclient)" -key imaclient-key.pem -out imaclient-crt.csr
# Create the ima client a certificate using the root CA A as the issuer
openssl x509 -days 3650 -in imaclient-crt.csr -out imaclient-crt.pem -req -CA rootCA-crt.pem -CAkey rootCA-key.pem -extensions v3_req -set_serial 2 2>/dev/null

# Add nothing to imaservera-crt.pem as its signer is root

# Add nothing to imaclienta-crt.pem as its signer is root

# Create CAfile.pem for server truststore
openssl x509 -in rootCA-crt.pem -text >> CAfile.pem
openssl x509 -in imaCA-crt.pem -text >> CAfile.pem
openssl x509 -in imaserver-crt.pem -text >> CAfile.pem

# Create PKCS 12 bundle for client certificate A
openssl pkcs12 -export -inkey imaclient-key.pem -in imaclient-crt.pem -out imaclient.p12 -password pass:password

# Create a java key store and import the root and ima CA certificates into the java key store
IBM_JAVA_HOME=/opt/ibm/ibm-java-x86_64-80

if [ ! -e $IBM_JAVA_HOME ] ; then
  echo "IBM JVM not installed @ $IBM_JAVA_HOME, will not generate Java keystore for IBM JVM.  Modify this script to point to IBM JAVA HOME"
else
  # Create jks for client A
  keytool -import -trustcacerts -file imaCA-crt.pem -keystore ibm.jks -storepass password -noprompt -alias ima
  keytool -import -trustcacerts -file rootCA-crt.pem -keystore ibm.jks -storepass password -noprompt -alias root
  keytool -importkeystore -srckeystore imaclient.p12 -destkeystore ibm.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
fi

# Clean up files
#rm -f *.cfg
#rm -f pass.txt
#rm -r *.csr
cd $CURRDIR

#end of script
