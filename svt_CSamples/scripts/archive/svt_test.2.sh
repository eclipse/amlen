#!/bin/bash

CLIENT_PASSWORD=gr8wh1te

do_must_gather() {
    local server=${1-""}
    local client=${2-`hostname -i`}
    local name=${3-"$server"}
    local CLIENT_BASE_DIR=${4-"/mg"}
    local outdir=$CLIENT_BASE_DIR/`date +%s`
    local RTC_MAX_SIZE=52428800 ; # 50MB the max size of file that can be uploaded to RTC.
    local origsize
    local newsize
    local regex;
    local line;
    local data;
    local var;
    local squeezing_needed;

    if [ "$server" == "" ] ; then echo "ERROR: no server specified." ; return 1; fi
    if [ "$client" == "" ] ; then echo "ERROR: no client specified." ; return 1; fi
    if [ "$name" == "" ] ; then echo "ERROR: no name specified." ; return 1; fi

    echo "------------------------------------"
    echo "`date` : Doing Must Gather . Output directory is $outdir"
    echo "------------------------------------"
    rm -rf $outdir
    mkdir -p $outdir/tmp
    rm -rf $CLIENT_BASE_DIR/previous
    rm -rf $CLIENT_BASE_DIR/previous.$name
    mv -f $CLIENT_BASE_DIR/current $CLIENT_BASE_DIR/previous
    mv -f $CLIENT_BASE_DIR/current.$name $CLIENT_BASE_DIR/previous.$name
    rm -rf $CLIENT_BASE_DIR/current
    rm -rf $CLIENT_BASE_DIR/current.$name

    echo "------------------------------------"
    echo "`date` : Files can be accessed via symbolic link at `hostname -i`:$CLIENT_BASE_DIR/current"
    echo "`date` : Files can be accessed via symbolic link at `hostname -i`:$CLIENT_BASE_DIR/current.$name"
    echo "------------------------------------"
    ln -s $outdir $CLIENT_BASE_DIR/current 
    ln -s $outdir $CLIENT_BASE_DIR/current.$name


    echo "------------------------------------"
    echo "`date` : Doing trace flush"
    echo "------------------------------------"
    ssh -nx admin@$server imaserver trace flush

    echo "------------------------------------"
    echo "`date` : Starting must gather on appliance $server mg.$name.tgz"
    echo "------------------------------------"
    ssh -nx admin@$server platform must-gather mg.$name.tgz

    echo "------------------------------------"
    echo "`date` : Transfer file mg.$name.tgz back to client"
    echo "------------------------------------"
    #ssh -nx admin@$server file put mg.$name.tgz scp://root@$client:$outdir ### Stopped working on 3.14.13 ... not sure why...
    echo "getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f mg.$name.tgz -d $outdir -t \"file put\""
    getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f mg.$name.tgz -d $outdir -t "file put"

    echo "------------------------------------"
    echo "`date` : Transfer core files to client (if necessary)"
    echo "------------------------------------"
    data=`ssh -nx  admin@$server advanced-pd-options list`
    for var in $data ;do
        #line=`echo "$var" | grep -oE 'bt.*|.*_core.*' |awk '{print $1}' `
        #if echo "$line" |grep -E 'bt.*|.*_core.*' >/dev/null ; then
        line=`echo "$var" | grep -oE 'bt.*' |awk '{print $1}' `
        if echo "$line" |grep -E 'bt.*' >/dev/null ; then
            #echo ssh -nx admin@$server advanced-pd-options export $line scp://root@$client:$outdir ### Stopped working on 3.14.13 ... not sure why...
            #ssh -nx admin@$server advanced-pd-options export $line scp://root@$client:$outdir ### Stopped working on 3.14.13 ... not sure why...
            echo "getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $outdir  -t \"advanced-pd-options export\""
            getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $outdir  -t "advanced-pd-options export"
            rc=$?
            echo ssh rc is $rc
        fi
    done

    origsize=`stat -c%s $outdir/mg.$name.tgz`
    echo "------------------------------------"
    echo "`date` : size of file  mg.$name.tgz : $origsize"
    echo "------------------------------------"

    echo "------------------------------------"
    echo "`date` : extract tar file"
    echo "------------------------------------"
    tar --directory=$outdir/tmp -xzvf $outdir/mg.$name.tgz

    echo "------------------------------------"
    echo "`date` : deobfuscate and clear .otx files"
    echo "------------------------------------"
    find $outdir/tmp | grep .otx$ | xargs -i echo "cat {} | /niagara/deobfuscate.pl | base64 -d > {}.deob" |sh
    find $outdir/tmp -name "*.otx" -exec rm -f {} \;

    squeezing_needed=1;
    while [ $squeezing_needed == "1" ] ; do
        echo "------------------------------------"
        echo "`date` : reconstruct tar , and squeeze it to fit into RTC < $RTC_MAX_SIZE "
        echo "------------------------------------"
        rm -rf $outdir/mg.$name.tgz
        tar -czvf $outdir/mg.$name.tgz $outdir/tmp/*
    
        newsize=`stat -c%s $outdir/mg.$name.tgz`
        echo "------------------------------------"
        echo -n "`date` :new size of file: mg.$name.tgz : $newsize . Percent reduction from original($origsize) = "
        echo " $newsize $origsize " | awk '{printf("%f\n", ($1/$2)*100.0);}' 
        echo "------------------------------------"
    
        if(($newsize>=RTC_MAX_SIZE)); then
            find $outdir/* -type f -printf "%s %p\n" |sort -nr | grep -v \.tgz | awk '{print $2}' | while read line; do
                regex= "AMQERR"                
                if [[ $line =~ $regex ]] ;then
                    echo "------------------------------------"
                    echo "`date` : removing file to allow data to upload into RTC: `ls -tlra $line`"
                    echo "------------------------------------"
                    sleep 1;
                    ls -tlra $line >> $outdir/README.removed.files
                    rm -rf $line; 
                    break;
                fi
                #223842676 /mg/1362416687/ima/cores/imatrace_prev.log.formatted.otx.deob
                regex= "_prev.log"                
                if [[ $line =~ $regex ]] ;then
                    echo "------------------------------------"
                    echo "`date` : removing file to allow data to upload into RTC: `ls -tlra $line`"
                    echo "------------------------------------"
                    sleep 1;
                    ls -tlra $line >> $outdir/README.removed.files
                    rm -rf $line; 
                    break;
                fi
            done
        else
            squeezing_needed=0;
        fi
    done

    rm -rf $outdir/tmp

    echo "------------------------------------"
    echo "`date` : Complete."
    echo "------------------------------------"

    echo "scp root@`hostname -i`:$outdir/*.tgz .   #`date` FFDC : $server"
 
}

