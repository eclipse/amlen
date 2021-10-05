#!/bin/bash

# ----------------------------------------------------------------------------------------
# This script is intended to speed up debugging by allowing easy replacement for privately
# built binaries to be easily replaced within a container without having to remove or 
# rebuild the container.
# ----------------------------------------------------------------------------------------

function usage {
  echo "usage: $0 -n <container name> -v <host volume directory> -t <mapped directory inside container> -s <server_ship directory>"
  echo " e.g.: $0 -n IMA -v /home/messagesight -t /var/messagesight -s /root/server_ship"
}

while getopts ":n:v:t:s:" opt ; do
  case $opt in
        n)
                CONTNAME=$OPTARG
                docker inspect $CONTNAME >/dev/null 2>/dev/null ; rc=$?
                if [ "$rc" != "0" ] ; then echo "Container $CONTNAME does not exist, exiting..." ; exit 1; fi
                ;;
        v)
                HOSTVOL=$OPTARG
                if [ ! -e $HOSTVOL ] ; then echo "directory $HOSTVOL does not exist, exiting..." ; exit 1; fi
                ;;
        t)
                CONTDIR=$OPTARG
                ;;
        s)
                SRVRSHIP=$OPTARG
                if [ ! -e $SRVRSHIP ] ; then echo "directory $SRVRSHIP does not exist, exiting..." ; exit 1; fi
                ;;
        \?|h)
                usage
                exit 0
                ;;
  esac
done

if [ "$CONTNAME" == "" -o "$HOSTVOL" == "" -o "$CONTDIR" == "" -o "$SRVRSHIP" == "" ] ; then
  usage
  exit 1
fi

docker exec -it $CONTNAME ls $CONTDIR >/dev/null 2>/dev/null ; rc=$?
if [ "$rc" != "0" ] ; then echo "Container directory $CONTDIR does not exist, exiting..." ; exit 1; fi

unalias rm mv cp /dev/null 2>/dev/null

mkdir -p $HOSTVOL/current
cd $HOSTVOL/current
if [ "$HOSTVOL/$(basename $SRVRSHIP)" != "$SRVRSHIP" ] ; then
  cp -rf $SRVRSHIP $HOSTVOL
fi
rm -f bin lib debug
ln -s ../$(basename $SRVRSHIP)/bin bin
ln -s ../$(basename $SRVRSHIP)/lib lib
ln -s ../$(basename $SRVRSHIP)/debug debug

tee $HOSTVOL/updatelinks.sh << EOF >/dev/null
#!/bin/bash
unalias rm mv cp >/dev/null 2>/dev/null
cd /opt/ibm/imaserver/lib64
for lib in \`ls *.so\` ; do
  mv -f \$lib \$lib.orig ; rm -f \$lib.orig
  ln -s $CONTDIR/current/lib/\$lib .
done
cd /opt/ibm/imaserver/bin
mv -f imaserver imaserver.orig ;  rm -f imaserver.orig
ln -s $CONTDIR/current/bin/imaserver .
orig=\`file /opt/ibm/imaserver/bin/startDockerMSServer.sh | grep "shell script" | wc -l\`
if [ "\$orig" == "1" ] ; then
  mv -f startDockerMSServer.sh $CONTDIR/current/bin
  ln -sf $CONTDIR/current/bin/startDockerMSServer.sh /opt/ibm/imaserver/bin/startDockerMSServer.sh
fi
cd /opt/ibm/imaserver/debug/lib
for lib in \`ls *.so\` ; do
  mv -f \$lib \$lib.orig ; rm -f \$lib.orig
  ln -s $CONTDIR/current/debug/lib/\$lib .
done
cd /opt/ibm/imaserver/debug/bin
mv -f imaserver imaserver.orig ; rm -f imaserver.orig
ln -s $CONTDIR/current/debug/bin/imaserver .
EOF

chmod +x $HOSTVOL/updatelinks.sh
chmod +x $HOSTVOL/current/bin/*

docker exec -it $CONTNAME $CONTDIR/updatelinks.sh

rm -f $HOSTVOL/updatelinks.sh
	
