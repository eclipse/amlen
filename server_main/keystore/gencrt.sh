#!/bin/sh

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
distinguished_name     = req_distinguished_name
req_extensions         = v3_ca
prompt                 = no
[ req_attributes ]
[ v3_ca ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer
basicConstraints       = CA:false
[ req_distinguished_name ]
countryName                     = US
organizationalUnitName          = IBM IoT MessageSight development team
commonName                      = IBM IoT MessageSight
EOF

# Generate RSA keys of specified size
openssl genrsa -out MessageSightKey.pem            $SIZE >/dev/null 2>/dev/null

# Create self-signed certificate
openssl req -new -x509 -sha256 -days $((365 * 20)) -subj "/C=US/O=IBM/OU=IoT/CN=IBM IoT MessageSight" -extensions v3_ca -config ext.cfg -set_serial $(date +%s) -key MessageSightKey.pem -out MessageSightCert.pem

# Create a java key store and import the certificate into the java key store
if [ -z $IBM_JAVA_HOME ] ; then
  IBM_JAVA_HOME=/opt/ibm/java-x86_64-80
fi

if [ ! -e $IBM_JAVA_HOME ] ; then
  echo "IBM JVM not installed @ $IBM_JAVA_HOME, will not generate Java keystore for IBM JVM.  Set en var IBM_JAVA_HOME to point to IBM JAVA HOME"
else
  echo "IBM_JAVA_HOME=$IBM_JAVA_HOME"
  export EMPTYPASS=ima15adev
  openssl pkcs12 -export -password env:EMPTYPASS -in MessageSightCert.pem -inkey MessageSightKey.pem -out key.p12 -name default
  $IBM_JAVA_HOME/bin/keytool -importkeystore -deststorepass $EMPTYPASS -destkeystore key.jks -srckeystore key.p12 -srcstoretype PKCS12 -srcstorepass $EMPTYPASS -alias default
fi


# Clean up files
rm -f *.cfg
rm -f pass.txt
rm -f *.csr
rm -f *.p12
cd $CURRDIR

#end of script
