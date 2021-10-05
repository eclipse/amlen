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

if [ "$1" == "" -o "$2" == "" ] ; then
  echo "usage: $0 <cert size> <dir to create>"
  echo "  e.g. $0 2048 certs2Kb"
  exit 1
fi

SIZE=$1
DIR=$2
CURRDIR=$(pwd)

rm -rf $DIR
mkdir -p $DIR ; cd $DIR

# Create certificate extensions file for CA's
cat > ext.cfg <<EOF
[ req ]
attributes             = req_attributes
req_extensions         = v3_ca
prompt                 = no
[ req_attributes ]
[ v3_ca ]
basicConstraints       = CA:true
EOF

# Create certificate extensions file for IMA server
cat > srvext.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName          = DNS:*.softlayer.com
EOF

# Generate RSA keys of specified size
openssl genrsa -out rootCAa-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out rootCAb-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaCAa-key.pem      $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaCAb-key.pem      $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaCAc-key.pem      $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaservera-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaserverb-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaserverc-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaserverd-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaclienta-key.pem $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaclientb-key.pem $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaclientc-key.pem $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaclientd-key.pem $SIZE >/dev/null 2>/dev/null

# Create root CA A certificate (self-signed)
openssl req -new -x509 -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate A" -extensions v3_ca -set_serial 1 -key rootCAa-key.pem -out rootCAa-crt.pem

# Create root CA B certificate (self-signed)
openssl req -new -x509 -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate B" -extensions v3_ca -set_serial 1 -key rootCAb-key.pem -out rootCAb-crt.pem

# Create certificate request for the intermediate CA A
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=AIMDEV/CN=IMA Development Team A" -key imaCAa-key.pem -out imaCAa-crt.csr
# Create the intermediate CA A certificate using the root CA A as the issuer
openssl x509 -days 1800 -in imaCAa-crt.csr -out imaCAa-crt.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -set_serial 3 -extensions v3_ca -extfile ext.cfg 2>/dev/null

# Create certificate request for the intermediate CA B
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=AIMDEV/CN=IMA Development Team B" -key imaCAb-key.pem -out imaCAb-crt.csr
# Create the intermediate CA B certificate using the intermediate CA A as the issuer
openssl x509 -days 1800 -in imaCAb-crt.csr -out imaCAb-crt.pem -req -CA imaCAa-crt.pem -CAkey imaCAa-key.pem -set_serial 3 -extensions v3_ca -extfile ext.cfg 2>/dev/null

# Create certificate request for the intermediate CA C
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=AIMDEV/CN=IMA Development Team C" -key imaCAc-key.pem -out imaCAc-crt.csr
# Create the intermediate CA C certificate using the root CA B as the issuer
openssl x509 -days 1800 -in imaCAc-crt.csr -out imaCAc-crt.pem -req -CA rootCAb-crt.pem -CAkey rootCAb-key.pem -set_serial 1 -extensions v3_ca -extfile ext.cfg 2>/dev/null

# Create the IMA server A certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverA)" -key imaservera-key.pem -out imaservera-crt.csr
# Create the IMA server A certificate using the root CA A as the issuer
openssl x509 -days 1800 -in imaservera-crt.csr -out imaservera-crt.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 1 2>/dev/null

# Create the IMA server B certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverB)" -key imaserverb-key.pem -out imaserverb-crt.csr
# Create the IMA server B certificate using the intermediate CA A as the issuer
openssl x509 -days 1800 -in imaserverb-crt.csr -out imaserverb-crt.pem -req -CA imaCAa-crt.pem -CAkey imaCAa-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 1 2>/dev/null

# Create the IMA server C certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverC)" -key imaserverc-key.pem -out imaserverc-crt.csr
# Create the IMA server C certificate using the intermediate CA B as the issuer
openssl x509 -days 1800 -in imaserverc-crt.csr -out imaserverc-crt.pem -req -CA imaCAb-crt.pem -CAkey imaCAb-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 1 2>/dev/null

# Create the IMA server D certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverD)" -key imaserverd-key.pem -out imaserverd-crt.csr
# Create the IMA server D certificate using the intermediate CA C as the issuer
openssl x509 -days 1800 -in imaserverd-crt.csr -out imaserverd-crt.pem -req -CA imaCAc-crt.pem -CAkey imaCAc-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 1 2>/dev/null

# Create the ima client a certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclientA)" -key imaclienta-key.pem -out imaclienta-crt.csr
# Create the ima client a certificate using the root CA A as the issuer
openssl x509 -days 1800 -in imaclienta-crt.csr -out imaclienta-crt.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -set_serial 2 2>/dev/null

# Create the ima client b certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclientB)" -key imaclientb-key.pem -out imaclientb-crt.csr
# Create the ima client b certificate using the intermediate CA A as the issuer
openssl x509 -days 1800 -in imaclientb-crt.csr -out imaclientb-crt.pem -req -CA imaCAa-crt.pem -CAkey imaCAa-key.pem -extensions v3_req -set_serial 2 2>/dev/null

# Create the ima client c certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclientC)" -key imaclientc-key.pem -out imaclientc-crt.csr
# Create the ima client c certificate using the intermediate CA B as the issuer
openssl x509 -days 1800 -in imaclientc-crt.csr -out imaclientc-crt.pem -req -CA imaCAb-crt.pem -CAkey imaCAb-key.pem -extensions v3_req -set_serial 2 2>/dev/null

# Create the ima client d certificate request
openssl req -new -days 1800 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclientD)" -key imaclientd-key.pem -out imaclientd-crt.csr
# Create the ima client d certificate using the intermediate CA C as the issuer
openssl x509 -days 1800 -in imaclientd-crt.csr -out imaclientd-crt.pem -req -CA imaCAc-crt.pem -CAkey imaCAc-key.pem -extensions v3_req -set_serial 2 2>/dev/null

echo -n "Do you want to view the certificates created? [y/N]: "
read response
if [ "$response" == "y" -o "$response" == "Y" ] ; then
  echo "ROOT CA A..."
  openssl x509 -in rootCAa-crt.pem -text -noout -purpose
  echo "ROOT CA B..."
  openssl x509 -in rootCAb-crt.pem -text -noout -purpose
  echo "Intermediate CA A..."
  openssl x509 -in imaCAa-crt.pem -text -noout -purpose
  echo "Intermediate CA B..."
  openssl x509 -in imaCAb-crt.pem -text -noout -purpose
  echo "Intermediate CA C..."
  openssl x509 -in imaCAc-crt.pem -text -noout -purpose
  echo "IMA server certificate A..."
  openssl x509 -in imaservera-crt.pem -text -noout -purpose
  echo "IMA server certificate B..."
  openssl x509 -in imaserverb-crt.pem -text -noout -purpose
  echo "IMA server certificate C..."
  openssl x509 -in imaserverc-crt.pem -text -noout -purpose
  echo "IMA server certificate D..."
  openssl x509 -in imaserverd-crt.pem -text -noout -purpose
  echo "IMA client certificate A..."
  openssl x509 -in imaclienta-crt.pem -text -noout -purpose
  echo "IMA client certificate B..."
  openssl x509 -in imaclientb-crt.pem -text -noout -purpose
  echo "IMA client certificate C..."
  openssl x509 -in imaclientc-crt.pem -text -noout -purpose
  echo "IMA client certificate D..."
  openssl x509 -in imaclientd-crt.pem -text -noout -purpose
fi

# Add nothing to imaservera-crt.pem as its signer is root

# Add imaCAa-crt.pem to imaserverb-crt.pem
cat imaCAa-crt.pem >> imaserverb-crt.pem

# Add imaCAb-crt.pem and imaCAa-crt.pem to imaserverc-crt.pem
cat imaCAb-crt.pem >> imaserverc-crt.pem
cat imaCAa-crt.pem >> imaserverc-crt.pem

# Add imaCAc-crt.pem to imaserverd-crt.pem
cat imaCAc-crt.pem >> imaserverd-crt.pem

# Add nothing to imaclienta-crt.pem as its signer is root

# Add imaCAa-crt.pem to imaclientb-crt.pem
cat imaCAa-crt.pem >> imaclientb-crt.pem

# Add imaCAb-crt.pem and imaCAa-crt.pem to imaclientc-crt.pem
cat imaCAb-crt.pem >> imaclientc-crt.pem
cat imaCAa-crt.pem >> imaclientc-crt.pem

# Add imaCAc-crt.pem to imaclientd-crt.pem
cat imaCAc-crt.pem >> imaclientd-crt.pem

# Create CAfile.pem for server truststore
openssl x509 -in rootCAa-crt.pem -text >> CAfile.pem
openssl x509 -in rootCAb-crt.pem -text >> CAfile.pem

# Create PKCS 12 bundle for client certificate A
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt.pem -out imaclienta.p12 -password pass:password

# Create PKCS 12 bundle for client certificate B
openssl pkcs12 -export -inkey imaclientb-key.pem -in imaclientb-crt.pem -out imaclientb.p12 -password pass:password

# Create PKCS 12 bundle for client certificate C
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt.pem -out imaclientc.p12 -password pass:password

# Create PKCS 12 bundle for client certificate D
openssl pkcs12 -export -inkey imaclientd-key.pem -in imaclientd-crt.pem -out imaclientd.p12 -password pass:password

# Create a java key store and import the root and ima CA certificates into the java key store
IBM_JAVA_HOME=/opt/ibm/java-x86_64-70
ORACLE_JAVA_HOME=/usr/java/jdk1.7.0_02

if [ ! -e $ORACLE_JAVA_HOME ] ; then
  echo "Oracle JVM not installed @ $ORACLE_JAVA_HOME, will not generate Java keystore for ORACLE JVM.  Modify this script to point to ORACLE JAVA HOME"
else
  # Create jks for client A
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAa-crt.pem -keystore oraclea.jks -storepass password -noprompt -alias rootA
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAb-crt.pem -keystore oraclea.jks -storepass password -noprompt -alias rootB
  $ORACLE_JAVA_HOME/bin/keytool -importkeystore -srckeystore imaclienta.p12 -destkeystore oraclea.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client B
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAa-crt.pem -keystore oracleb.jks -storepass password -noprompt -alias rootA
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAb-crt.pem -keystore oracleb.jks -storepass password -noprompt -alias rootB
  $ORACLE_JAVA_HOME/bin/keytool -importkeystore -srckeystore imaclientb.p12 -destkeystore oracleb.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client C
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAa-crt.pem -keystore oraclec.jks -storepass password -noprompt -alias rootA
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAb-crt.pem -keystore oraclec.jks -storepass password -noprompt -alias rootB
  $ORACLE_JAVA_HOME/bin/keytool -importkeystore -srckeystore imaclientc.p12 -destkeystore oraclec.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client D
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAa-crt.pem -keystore oracled.jks -storepass password -noprompt -alias rootA
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file rootCAb-crt.pem -keystore oracled.jks -storepass password -noprompt -alias rootB
  $ORACLE_JAVA_HOME/bin/keytool -importkeystore -srckeystore imaclientd.p12 -destkeystore oracled.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client B with just intermediate CA A for truststore
  $ORACLE_JAVA_HOME/bin/keytool -import -trustcacerts -file imaCAa-crt.pem -keystore oraclee.jks -storepass password -noprompt -alias imaA
  $ORACLE_JAVA_HOME/bin/keytool -importkeystore -srckeystore imaclientb.p12 -destkeystore oracled.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
fi

if [ ! -e $IBM_JAVA_HOME ] ; then
  echo "IBM JVM not installed @ $IBM_JAVA_HOME, will not generate Java keystore for IBM JVM.  Modify this script to point to IBM JAVA HOME"
else
  # Create jks for client A
  keytool -import -trustcacerts -file rootCAa-crt.pem -keystore ibma.jks -storepass password -noprompt -alias rootA
  keytool -import -trustcacerts -file rootCAb-crt.pem -keystore ibma.jks -storepass password -noprompt -alias rootB
  keytool -importkeystore -srckeystore imaclienta.p12 -destkeystore ibma.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client B
  keytool -import -trustcacerts -file rootCAa-crt.pem -keystore ibmb.jks -storepass password -noprompt -alias rootA
  keytool -import -trustcacerts -file rootCAb-crt.pem -keystore ibmb.jks -storepass password -noprompt -alias rootB
  keytool -importkeystore -srckeystore imaclientb.p12 -destkeystore ibmb.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client C
  keytool -import -trustcacerts -file rootCAa-crt.pem -keystore ibmc.jks -storepass password -noprompt -alias rootA
  keytool -import -trustcacerts -file rootCAb-crt.pem -keystore ibmc.jks -storepass password -noprompt -alias rootB
  keytool -importkeystore -srckeystore imaclientc.p12 -destkeystore ibmc.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client D
  keytool -import -trustcacerts -file rootCAa-crt.pem -keystore ibmd.jks -storepass password -noprompt -alias rootA
  keytool -import -trustcacerts -file rootCAb-crt.pem -keystore ibmd.jks -storepass password -noprompt -alias rootB
  keytool -importkeystore -srckeystore imaclientd.p12 -destkeystore ibmd.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password

  # Create jks for client D
  keytool -import -trustcacerts -file imaCAa-crt.pem -keystore ibme.jks -storepass password -noprompt -alias imaA
  keytool -importkeystore -srckeystore imaclientb.p12 -destkeystore ibmd.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
fi

# Clean up files
rm -f *.cfg
rm -f pass.txt
rm -r *.csr
cd $CURRDIR

#end of script
