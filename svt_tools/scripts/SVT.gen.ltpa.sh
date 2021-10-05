#!/bin/bash
#-----------------------------------
# LTPA token generator script.
#
# To generate a million LTPA keys call this script w/ "MILLION" as first argument. This operation will take approx. 6 hours.
# Make sure to go to WAS (mar400) to set the desired expiration time in minutes for the tokens. (for example if you want
# to generate tokens that are already expired for error path testing, then go set minutes to 15 before starting this
# operation. or... if you want tokens that can be used for a long time (1 year) set minutes to 365*24*60 = 525600 )
# 
# Note: if in the future WAS is not mar400(10.10.10.10), then configure that change in this script before running this,
# as it is currently hard coded.
#
# Note: if imasvtest ldap password changes in future (not expected) , then configure that change in this script before running 
# as it is currently hard coded.
#
# Note: The output of this script are db.flat.* files in current working directory. The format of flat files are
#    usernamex<space>ltpatokenx
#    usernamey<space>ltpatokeny
#    usernamez<space>ltpatokenz
#       ...
#-----------------------------------

OPERATION=${1-"DEFAULT"}  ; # DEFAULT or MILLION

svt_get_ltpa_token_bunch(){
    local start=${1-0}
    local count=${2-100}
    local prefix=${3-"u"}
    local password=${4-"imasvtest"};
    local wasserveruri=${5-"https://10.10.10.10:9443/snoop"}
    local tmpout=${6-"./.tmp.ltpa.${start}"}
    local out=${7-`awk 'BEGIN {printf("db.flat.'${prefix}'%07d", '$start'); exit 0;}'`}
    local end;
    let end=($start+$count);

    echo "" | awk ' BEGIN {printf(" . /niagara/test/scripts/commonFunctions.sh\n"); }
                    END { x='$start'; while(x<'$end') {
                    printf ("echo -n \"u%07d \"; svt_get_ltpa_token '$prefix'%07d '$password'
                         '$wasserveruri' '$tmpout' \n ",x, x); x=x+1; } } ' | sh > $out

}

do_default () {
    local adder=${1-0}
    local prefix=${2-"u"}
    . /niagara/test/scripts/commonFunctions.sh
    for v in {0..9}; do
        let x=$v*10000; 
        let x=($x+$adder)
        echo start svt_get_ltpa_token_bunch $x 10000 $prefix ;  
        svt_get_ltpa_token_bunch $x 10000 $prefix &
        waitpid+="$! "; 
    done ; 
    wait $waitpid
}


do_million() {
    local prefix=${1-"u"}
    local v
    for v in {0..9} ; do
    echo run set for $v
        let x=($v*100000);
        do_default $x $prefix
    done
}

if [ "$OPERATION" == "DEFAULT" ] ;then   
    do_default
elif [ "$OPERATION" == "MILLION" ] ;then # default a million users u0000000 u0000001 ....
    do_million
elif [ "$OPERATION" == "MILLION-CAR" ] ;then # generate a million cars c0000001 c0000002 ....
    do_million "c"
elif [ "$OPERATION" == "MILLION-USER" ] ;then
    do_million "u"
else
    echo error
    exit 1;
fi
