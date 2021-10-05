#!/bin/sh
# create hash links on all files in the specified directory
#

if [ -z "$1" ] ; then
        truststore="../truststore"
else
        truststore=$1
fi

process_tenant()
{
	target=$1

        echo "Processing truststore for $target"
        suffix="_capath"
        dest=$target$suffix
        mkdir $truststore/$dest
        cwd=$PWD
        cp $target/*.crt $truststore/$dest/
        cd $truststore/$dest

	# create new links
	for i in * ; do
		if [ -r $i -a ! -d $i ] ; then
	    		hash=$(openssl x509 -hash -noout -in $i 2>/dev/null)
	    		if [ -z "$hash" ] ; then
	    		    continue
	    		fi
	    		suffix=0
	    		while [ -e $hash.$suffix ] ; do
	    		    suffix=$((suffix + 1))
	    		done
	    		ln -sf $i $hash.$suffix
		fi
	done
        cd ..
        suffix="_cafile.pem"
        ln -s $dest/imaCA.crt $target$suffix
        cd $cwd
} 



rm -rf $truststore
mkdir $truststore
for i in * ; do
	if [ -d $i ] ; then
		process_tenant $i
	fi
done



