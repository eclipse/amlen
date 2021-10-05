#!/bin/bash

#---------------------------------------------
# Script pulls LTPA token cache from mar400 to initialize the client side for use
#---------------------------------------------

svt_init_data_cache(){
#   local dataserver=${1-10.10.10.10} # mar400 has /svt.data master copy
    local dataserver=${1-198.23.126.150} # eric-svtpvt-seed-1 has /svt.data master copy at /seed/svtauto/cmqtt-data
    local tardir="/svt.data.tar"
    local outdir="/svt.data"
    local dosync=0;
    local numfiles;
    local tarcount;
    local numfilemagic=2003012; # expect at least this many files for a valid data cache.
    local neededspacemagic=$((7418408+1525944+1525944+1000000)); # how much space is needed to try to install this
                                    # 7418408 is svt.data after extraction
                                    # 1525944 is svt.data.tar x2
                                    # 1000000 is buffer
    local mydir
    local myvar=0;
    local iteration=0;
    local diskdir="/niagara/"; #default unless found that / has more.

    local root_avail=`df / | grep % | grep -v Use | grep [0-9] | awk '{print $3}'`
    local niagara_avail=`df /niagara | grep % | grep -v Use | grep [0-9] | awk '{print $3}'`

    #if ! xargs --version 2>/dev/null | grep -i GNU > /dev/null 2>/dev/null ;then
        #echo "-----------------------------------------------------------------"
        #echo "WARNING: This setup script currently requires a GNU version of xargs. "
        #echo " if you need svt data cache installed on this system please install the GNU version of xargs "
        #echo "-----------------------------------------------------------------"
        #return 1;
    #fi

    if [ -e /svt.data/failed.setup ] ;then
        echo "-----------------------------------------------------------------"
        echo "WARNING: It may not be possible to setup svt data cache on this particular client system."
        echo "WARNING: Skipping setup at this time."
        echo "WARNING: If you want to try again then rm -rf /svt.data/failed.setup. "
        echo "-----------------------------------------------------------------"
        return 1;
    fi
    if ! [ -e /svt.data/complete ] ;then
        rm -rf /svt.data
        rm -rf /svt.data.tar
        rm -rf /niagara/svt.data
        rm -rf /niagara/svt.data.tar
    fi

    if [ -L /svt.data -a -d ${diskdir}svt.data ] ; then
        diskdir="/niagara/"
        echo "`date` A Using $diskdir for storage of svt.data cache"
    elif [ -d /svt.data ] ; then
        diskdir="/"
        echo "`date` B Using $diskdir for storage of svt.data cache"
#   elif (($root_avail>$niagara_avail)); then
#       diskdir="/"
#       echo "`date` C Using $diskdir for storage of svt.data cache - first time intialization "
#       if (($root_avail<$neededspacemagic)); then
#           if ! [ -e ${outdir}/complete ] ; then
#               echo "-----------------------------------------------------------------"
#               echo "WARNING: There may not be enough space to complete this operation"
#               echo "WARNING: You can savely disregard this warning if you are not using the files in /svt.data for your testing"
#               echo "-----------------------------------------------------------------"
#           fi
#       fi
    else
        diskdir="/niagara/"
        echo "`date` D Using $diskdir for storage of svt.data cache - first time initialization "
        if (($niagara_avail<$neededspacemagic)); then
            if ! [ -e ${outdir}/complete ] ; then
                echo "-----------------------------------------------------------------"
                echo "WARNING: There may not be enough space to complete this operation"
                echo "WARNING: You can savely disregard this warning if you are not using the files in /svt.data for your testing"
                echo "-----------------------------------------------------------------"
            fi
        fi
    fi

    for mydir in svt.data svt.data.tar ; do
        if ! [ -e ${diskdir}$mydir ] ;then
            echo "`date` Rebuilding directory structure ${diskdir}$mydir "
            mkdir -p ${diskdir}$mydir
            chmod 777 ${diskdir}$mydir
            if [ "$diskdir" == "/niagara/" ] ; then
                echo "`date` Linking directory structure to actual storage in ${diskdir}$mydir "
                rm -rf /$mydir
                ln -s /niagara/$mydir /$mydir
            fi
        fi
    done

    echo "`date` check sum "
#   rsync -r --perms $dataserver:/svt.data/sum /svt.data.tar/sum
    rsync -r --perms $dataserver:/seed/svtauto/cmqtt-data/sum /svt.data.tar/sum

    echo "`date` check numfiles "
    numfiles=`find ${outdir}/ |wc -l`
    if (($numfiles>=$numfilemagic)); then  # note this is the expected number of files currently  , update for additional checking
        echo "`date` Looking good so far $numfiles found in ${outdir}"
    else
        echo "`date` Could not find expected number of files $numfiles in ${outdir}"
        rm -rf ${outdir}/sum
        rm -rf ${outdir}/complete
    fi

    if ! [ -e ${outdir}/sum ] ;then
        echo "`date` rsync needed a - /svt.data directory not valid"
        dosync=1;
    elif ! [ -e ${outdir}/complete ] ;then
        echo "`date` rsync needed /svt.data/complete file missing"
        dosync=1;
    else
        if ! diff ${outdir}/sum ${tardir}/sum > /dev/null 2>/dev/null ;then
            echo "`date` rsync needed b - sum files differ"
            echo "`date` rsync outdir/sum:"
            cat ${outdir}/sum
            echo "`date` rsync tardir/sum:"
            cat ${tardir}/sum
            dosync=1;
        fi
    fi

    if [ "$dosync" == "1" ] ; then
        echo "`date` Deleting ${outdir}/complete to mark the directory as not complete. "
        rm -rf ${outdir}/complete
        echo "`date` preparing for sync"
        echo "`date` Start rsync "
#       rsync -r --perms $dataserver:/svt.data /svt.data.tar
        rsync -r --perms $dataserver:/seed/svtauto/cmqtt-data/ /svt.data.tar/svt.data
        echo "`date` Completed rsync "
    fi

    if ! [ -e ${outdir}/complete ] ;then
        rm -rf ${outdir}/*
        echo "`date` copying local files"
        cp -p -f -v -r ${tardir}/svt.data/* ${outdir}/ 2>/dev/null 1>/dev/null
        echo "`date` Untar local files to ${outdir}/ "
        # -----------------------------------------
        # The next super ugly command in the while loop is
        # going to untar each .tgz file found and expects to find 2001 or 1002 files
        # in each output directory, which if it does it will delete the input .tgz
        # otherwise it does not delete the input .tgz and prints a warning.
        # Outstanding .tgz files should continue to be handled if they still exist.
        #
        # Note: mar456 does not seem to be working...
        # -----------------------------------------
        let iteration=0
        tarcount=`find ${outdir}/ | grep \.tgz | wc -l `
        while (($tarcount>0)); do
                echo "`date` Untar local files : iteration $iteration. - Note this doesn't always succeed on first pass, thus the iterations."
                echo "`date` Untar local files : iteration $iteration. - Currently $tarcount files left to untar"
                #-----------------------------------------------
                # TODO: Maybe switch to this so mar499 will work.
                #-----------------------------------------------
                find ${outdir}/ | grep tgz |  while read line ; do
                    tar -C `dirname $line` -xzvf $line 1>/dev/null 2>/dev/null
                    myvar1=$(dirname $line);
                    myvar2=$(basename $line);
                    myvar3=$(echo ${myvar2} | sed "s/.tgz//");
                    #echo myvar1 is ${myvar1};
                    #echo myvar2 is ${myvar2};
                    #echo myvar3 is ${myvar3};
                    myvar=$(find ${myvar1}/${myvar3} |wc -l );
                    #echo myvar is ${myvar};
                    if ((${myvar}!=2001)); then
                        if ((${myvar}!=1002)); then
                            echo WARNING: unexpected number of files found : $line ;
                        else
                            rm -rf $line ;
                        fi;
                    else
                        rm -rf $line ;
                    fi
                done

                #-----------------------------------------
                # This way does not work on Apple system mar499
                #-----------------------------------------
                #find ${outdir}/ | grep tgz | xargs -I repstr echo "tar -C \`dirname repstr\` -xzvf repstr;  \
                        #myvar1=\$(dirname repstr); \
                        #myvar2=\$(basename repstr); \
                        #myvar3=\$(echo \${myvar2} | sed \"s/.tgz//\"); \
                        #myvar=\$(find \${myvar1}/\${myvar3} |wc -l ); \
                        #echo myvar1 is \${myvar1}; \
                        #echo myvar2 is \${myvar2}; \
                        #echo myvar3 is \${myvar3}; \
                        #if ((\${myvar}!=2001)); then \
                            #if ((\${myvar}!=1002)); then \
                                #echo WARNING: unexpected number of files found : repstr ;  \
                            #else \
                                #rm -rf repstr ; \
                            #fi; \
                        #else \
                                #rm -rf repstr ; \
                        #fi ;" |sh 2> /dev/null 1> /dev/null
            let iteration=$iteration+1
            if (($iteration>3)); then
                echo "WARNING: breaking after 3 iterations. If it is not there by now it probably is not working"
                break;
            fi
            tarcount=`find ${outdir}/ | grep \.tgz | wc -l`
        done
        echo "`date` Cleanup local tmp files"
        find ${tardir}/ | grep tgz | xargs -I repstr echo "rm -rf repstr" |sh 2> /dev/null 1> /dev/null
        echo "`date` SVT data Summary:"
        numfiles=`find ${outdir}/ |wc -l`
        echo $numfiles
        if (($numfiles>=$numfilemagic)); then  # note this is the expected number of files currently  , update for additional checking
            echo "`date` Update sum and complete"
            date> ${outdir}/complete
        else
            echo "WARNING: some files appear to be missing. Things do not seem setup correctly. Not marking the data cache as complete"
            date> ${outdir}/failed.setup
        fi
    fi

    echo "`date` SVT data cache initialized"

}


svt_init_data_cache

sleep 5

echo "Done."
