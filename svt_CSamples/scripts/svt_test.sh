#!/bin/bash

if [[ -f "/niagara/test/testEnv.sh" ]] ;then
    source /niagara/test/testEnv.sh
elif [[ -f "/niagara/test/scripts/ISMsetup.sh" ]] ;then
    source /niagara/test/scripts/ISMsetup.sh
fi

CLIENT_PASSWORD=tun4r0ll

. /niagara/test/scripts/commonFunctions.sh

SVTPP="svt_post_process"

do_xyHMS_plot () {
    local term="svg"
    local inputfile=${1-""}
    local outputfile=${2-""}
    local title=${3-""}
    local xlabel=${4-""}
    local ylabel=${5-""}

    if ! echo $outputfile | grep .${term} > /dev/null ;then
        echo "WARNING: Outputfile $outputfile was not in proper format. Added .${term} suffix ."
        outputfile=${outputfile}.${term}
    fi
    
    echo "
#--------------------------
# Create Simple xy plot for data input in format such as below, in output of .${term} file format
#
# 05:21:50 115
# 05:21:51 3672
# 05:21:52 11669
# 05:21:53 14668
# 05:21:54 18184
# 05:21:55 17152
# 05:21:56 43389
# 05:21:57 52629
#
#--------------------------

set terminal $term
set title \"$title\"
set xlabel \"$xlabel\"
set ylabel \"$ylabel\"
set xdata time
set timefmt \"%H:%M:%S\"
set format x \"%H:%M:%S\"
#set term png
set output \"$outputfile\"
set xtics rotate by -30
plot \"$inputfile\" using 1:2 with linespoints " > tmp.plot

    gnuplot -e 'load "tmp.plot" '

}

do_stats(){
    { while read line ; do echo "$line" ; done ; } |
    awk '
    NR == 1 {sum=0; max=$1 ; min=$1} 
    $1 >= max {max = $1} 
    $1 <= min {min = $1} 
    { sum+=$1; array[NR]=$1; } 
    
    END {
    for(x=1;x<=NR;x++){
        sumsq+=((array[x]-(sum/NR))^2);
    }
    if (NR>0){
        print "Min: "min," Max: "max " Sum: "sum, " Number of lines: "NR, " Average: "sum/NR " Standard Deviation: "sqrt(sumsq/NR)
    } else {
        print "No data. "
    }
    
    }' ;
}


do_find_x_dot_interface() {
    local server=$1
    local x_dot=$2
    local adminserver
    local data
    local iface
    local output
    if [ -n "$SVT_ADMIN_IP" ] ;then
        adminserver=$SVT_ADMIN_IP
    else
        adminserver=$server
    fi
    data=`ssh admin@$adminserver list ethernet-interface `
    for iface in $data; do
        #echo testing iface $iface > /dev/null; 
        output=`ssh admin@$adminserver show ethernet-interface $iface`; 
        #echo "testing iface for ${x_dot} $iface $output"
        if echo "$output" | grep address | grep "\"${x_dot}\." > /dev/null; then 
            echo $iface ;
            return 0;
        fi
    done
    return 1;
}
do_find_10_dot_interface() {
	 local server=$1
	local rc
	do_find_x_dot_interface $server "10\.10"
	rc=$?
}

do_get_log_prefix(){
    local flag=${1-""} ; # for minumum pass in MININUM
    local logp
    #--------------------------------------------------
    # log prefix (logp) the prefix for all log files.
    #--------------------------------------------------
    logp="log"
    if [ -n "$AF_AUTOMATION_TEST" ] ;then
        #--------------------------------------------------
        # log prefix (logp) the prefix for all log files. (enhanced for automation)
        #--------------------------------------------------
	if [ -n "$flag" ] ; then # MINIMUM
       		logp="log.${AF_AUTOMATION_TEST}."
	else
        	logp="log.${AF_AUTOMATION_TEST}.`hostname`"
	fi
    fi
    echo -n "$logp"
}


do_stack_gather() {
    local server=${1-""}
    local client=${2-`hostname -i`}
    local name=${3-"$server"}
    local CLIENT_BASE_DIR=${4-`pwd`}

    echo "------------------------------------"
    echo "`date` : Do stack snapshot "
    echo "------------------------------------"
    ssh -nx admin@$server advanced-pd-options dumpstack imaserver

    echo "------------------------------------"
    echo "`date` : Transfer stack files to client"
    echo "------------------------------------"
    data=`ssh -nx  admin@$server advanced-pd-options list | awk '{print $1}' |sort -u |grep stack`
    for var in $data ;do
        echo "var is $var"
        line=`echo "$var" | grep -oE '.*_stack.*' |awk '{print $1}' | sort -u `
        if echo "$line" |grep -E '_stack' >/dev/null ; then
            #echo "Start Running ./getimafile.exp command"
            echo "Start Running ./appliance.exp command"
            #echo "./getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $CLIENT_BASE_DIR -t \"advanced-pd-options export\""
            #./getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $CLIENT_BASE_DIR -t "advanced-pd-options export"
            echo "/niagara/test/svt_cmqtt/appliance.exp -a export -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $CLIENT_BASE_DIR "
            /niagara/test/svt_cmqtt/appliance.exp -a export -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $CLIENT_BASE_DIR 
            echo "Done Running ./getimafile.exp command"
            rc=$?
            echo ssh rc is $rc
        fi
    done

    echo "------------------------------------"
    echo "`date` : Deobfuscate the stack files"
    echo "------------------------------------"

    find $CLIENT_BASE_DIR | grep _stack  | grep .otx$ | xargs -i echo "cat {} | /niagara/deobfuscate.pl | base64 -d > {}.deob" |sh -x


    echo "------------------------------------"
    echo "`date` : Done"
    echo "------------------------------------"
}


#-----------------------------------------------------------
# TODO: Add an automatic dump core on imaserver for tough problems -e.g. defect 37001 ,
#       Note: it would be cool if it stored it after doing standard dump in /mg/xxxxxx.core/ then it is only there
#       if needed, and does not slow the gather of the first round of data.
#-----------------------------------------------------------
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
    echo "`date` : Do stack snapshot "
    echo "------------------------------------"
    ssh -nx admin@$server advanced-pd-options dumpstack imaserver

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
    echo "`date` : Do stack snapshot "
    echo "------------------------------------"
    ssh -nx admin@$server advanced-pd-options dumpstack imaserver

    echo "------------------------------------"
    echo "`date` : Transfer file mg.$name.enc back to client"
    echo "------------------------------------"
    echo "/niagara/test/svt_cmqtt/appliance.exp -a export -s $server -c $client -u root -p $CLIENT_PASSWORD -f mg.$name.tgz -d $outdir "
    /niagara/test/svt_cmqtt/appliance.exp -a export -s $server -c $client -u root -p $CLIENT_PASSWORD -f mg.$name.tgz -d $outdir 

    echo "------------------------------------"
    echo "`date` : Do stack snapshot "
    echo "------------------------------------"
    ssh -nx admin@$server advanced-pd-options dumpstack imaserver

    echo "------------------------------------"
    echo "`date` : Transfer core files to client (if necessary)"
    echo "------------------------------------"
    data=`ssh -nx  admin@$server advanced-pd-options list | awk '{print $1}' |sort -u`
    for var in $data ;do
        echo "var is $var"
        #line=`echo "$var" | grep -oE 'bt.*|.*_core.*' |awk '{print $1}' `
        #if echo "$line" |grep -E 'bt.*|.*_core.*' >/dev/null ; then
        line=`echo "$var" | grep -oE 'bt.*|.*_stack.*' |awk '{print $1}' | sort -u `
        if echo "$line" |grep -E 'bt.*|_stack' >/dev/null ; then
            #echo ssh -nx admin@$server advanced-pd-options export $line scp://root@$client:$outdir ### Stopped working on 3.14.13 ... not sure why...
            #ssh -nx admin@$server advanced-pd-options export $line scp://root@$client:$outdir ### Stopped working on 3.14.13 ... not sure why...
            echo "Start Running ./appliance.exp command"
           # echo "./getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $outdir  -t \"advanced-pd-options export\""
           # ./getimafile.exp  -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $outdir  -t "advanced-pd-options export"
            echo "/niagara/test/svt_cmqtt/appliance.exp -a export -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $outdir "
            /niagara/test/svt_cmqtt/appliance.exp -a export -s $server -c $client -u root -p $CLIENT_PASSWORD -f $line -d $outdir 
            echo "Done Running ./appliance.exp command"
            rc=$?
            echo ssh rc is $rc
        fi
    done

if [ -e $outdir/mg.$name.tgz ] ;then #### 2014.01.28 - nolonger used code. now .enc files will be transferred
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
    find $outdir/ | grep .otx$ | xargs -i echo "cat {} | /niagara/deobfuscate.pl | base64 -d > {}.deob" |sh
    find $outdir/  -name "*.otx" -exec rm -f {} \;

    echo "------------------------------------"
    echo "`date` : Fix .tgz.otx.deob files by moving them to .otx.deob.tgz files"
    echo "------------------------------------"
    find $outdir -type f | grep tgz.otx.deob$ | sed "s/.tgz.otx.deob//g" | xargs -i echo "mv {}.tgz.otx.deob {}.otx.deob.tgz" |sh

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
  
        echo "------------------------------------"
        echo "`date` : skip squeezing - 7.17.2013" 
        echo "------------------------------------"
        break;
          
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
elif [ -e $outdir/mg.$name.tgz.enc ] ;then
    echo "------------------------------------"
    echo "`date` : Before Decrypt .enc file now if needed"
    echo "------------------------------------"
    ls -tlra $outdir/*
    echo "------------------------------------"
    echo "`date` : Decrypting"
    echo "------------------------------------"
     find $outdir/*.enc | xargs -i /niagara/process_file.sh {}
    echo "------------------------------------"
    echo "`date` : After Decrypt .enc files"
    echo "------------------------------------"
    ls -tlra $outdir/*

fi


    echo "------------------------------------"
    echo "`date` : Complete."
    echo "------------------------------------"

    echo "------------------------------------"
    echo " FFDC (Mustgather) from $server on `date`  " 
    echo ""
    echo "Please use the standard \"test password\" to scp the files."
    echo "If you do not know the \"test password\", go to this URL:"
    echo " https://w3-connections.ibm.com/wikis/home?lang=en-us#!/wiki/FSSArea/page/1.7%20Test%20admin%20passwords "
    echo ""
    echo "------------------------------------"
    echo "scp root@`hostname -i`:$outdir/* .   #`date` FFDC : $server"
 
}


svt_check_processes_still_active () {
    local now_time=${1}
    local qos=${2}
    local process_name=${3-mqttsample_array}
    local num_clients=${4-0}
    local data_tmp
    local data

    #sleep 1;
    {
    data=`ps -ef  | grep ${process_name} | grep -E "${qos}.*.${now_time}" | grep -v grep | wc -l`
    if (($num_clients>0)); then
        if (($data==$num_clients)); then
            return 0;
        else
            echo "----------------------------------"
            echo "----------------------------------" ; # I would rather this not ever print out...
            echo "WARNING: ps -ef  | grep $process_name | grep -E "$qos.*.$now_time" | grep -v grep |wc -l"
            echo "WARNING: The number of prcesses : $data , with process name $process_name ,time $now_time did not == to the expected number of processes: $num_clients for qos $qos"
            echo "----------------------------------"
            echo "----------------------------------"
            return 1;

        fi
    fi
    if (($data>0)); then
        return 0;
    else
        echo "----------------------------------"
        echo "----------------------------------" ; # I would rather this not ever print out...
        echo "WARNING: ps -ef  | grep $process_name | grep -E "$qos.*.$now_time" | grep -v grep |wc -l"
        echo "WARNING: no processes with process name $process_name and time $now_time active for qos $qos"
        echo "----------------------------------"
        echo "----------------------------------"
        return 1;
    fi
    } 1>> /dev/stderr 2>>/dev/stderr

    return 0;

}
svt_check_processes_still_active2 () {
    local now_time=${1}
    local qos=${2}
    local process_name=${3-mqttsample_array}
    local num_clients=${4-0}
    local data_tmp
    local data

    #sleep 1;
    {
    data=`ps -ef  | grep ${process_name} | grep -v grep | wc -l`
    if (($num_clients>0)); then
        if (($data==$num_clients)); then
            return 0;
        else
            echo "----------------------------------"
            echo "----------------------------------" ; # I would rather this not ever print out...
            echo "WARNING: ps -ef  | grep $process_name | grep -v grep |wc -l"
            echo "WARNING: The number of prcesses : $data , with process name $process_name ,time $now_time did not == to the expected number of processes: $num_clients for qos $qos"
            echo "----------------------------------"
            echo "----------------------------------"
            return 1;

        fi
    fi
    if (($data>0)); then
        return 0;
    else
        echo "----------------------------------"
        echo "----------------------------------" ; # I would rather this not ever print out...
        echo "WARNING: ps -ef  | grep $process_name  | grep -v grep |wc -l"
        echo "WARNING: no processes with process name $process_name and time $now_time active for qos $qos"
        echo "----------------------------------"
        echo "----------------------------------"
        return 1;
    fi
    } 1>> /dev/stderr 2>>/dev/stderr

    return 0;

}



svt_verify_condition() {
    local condition=$1
    local count=$2
    local file_pattern=$3
    local num_clients=${4-0} ; # if passed in non-zero it will do extra verification that exactly this many clients is running.
    local test_app=${5-"mqttsample_array"} ;
    local timeout=${6-0} ; #
    local num_logs=${7-0} ; # if passed in it will do extra verification that exactly this many logs are there
    local starttime=0;
    local endtime=0;
    local curtime=0;
    local myregex="[0-9]+"
    local checkrc
    local checkrc2
    local countlogs

    echo "inside svt_verify_condition : File pattern is $file_pattern"

    if (($timeout>0)); then
        starttime=`date +%s`
        let endtime=($starttime+$timeout);
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION: Timeout set for this operation. Operation will timeout at $endtime ( timeout: $timeout seconds)"
        echo "-----------------------------------------"
    fi 

    if ! [[ "$count" =~ $myregex ]] ; then
        echo "ERROR: count non-numeric"
        return 1;
    fi
    if (($num_logs>0)); then
        countlogs=`find $file_pattern | wc -l`
        while(($countlogs<$num_logs)); do
            echo "`date`: waiting for number of logs :$countlogs to match expected: $num_logs "
            if (($timeout>0)); then
                curtime=`date +%s`
                if (($curtime>$endtime)); then
                    echo "-----------------------------------------"
                    echo " `date` SVT AUTOMATION: WARNING: Operation timed out - condition $condition was not achieved as expected"
                    echo "-----------------------------------------"
                    return 1;
                fi
            fi
            sleep 1;
            countlogs=`find $file_pattern | wc -l`
        done
        echo "-----------------------------------------"
        echo " `date` Verified that expected number of logs $num_logs == $countlogs were found matching $file_pattern"
        echo "-----------------------------------------"
    fi
    k=0;
    while (($k<$count)); do
        echo  svt_check_processes_still_active ${now} ${qos} ${test_app}

        svt_check_processes_still_active ${now} ${qos} ${test_app}
        checkrc=$?
        if (($num_clients>0)); then
            svt_check_processes_still_active ${now} ${qos} ${test_app}
            checkrc2=$?
        fi
    
        w=`grep "WARNING" $file_pattern 2>/dev/null |wc -l`
        e=`grep "ERROR" $file_pattern 2>/dev/null  |wc -l`
        c=`ps -ef | grep $test_app 2>/dev/null | grep -v grep 2>/dev/null  | wc -l`
        k=`grep "$condition" $file_pattern 2>/dev/null |wc -l`
        echo "Waiting for condition $condition to occur $count times(current: $k) warnings($w) errors:($e) clients:($c)"
        date;
    	if (($k>=$count)); then
            echo "Success: Condition has been met k = $k , count = $count"
            break;
	    fi
        if [ "$checkrc" != "0" ] ;then
        	if [ "$SVT_AT_VARIATION_SKIP_PREMATURE_BREAK" == "true" ] ;then
                echo "`date` WARNING: breaking prematurely skipped due to SVT_AT_VARIATION_SKIP_PREMATURE_BREAK setting ... "
			    echo "WARNING: no processes are still active. k = $k , count = $count. Will not break yet due to overide setting. "
		    else
           		echo "`date` WARNING: breaking prematurely because no processes are still active. k = $k , count = $count. "
           		return 1;
		    fi	
        fi

        if (($timeout>0)); then
            curtime=`date +%s`
            if (($curtime>$endtime)); then
                echo "-----------------------------------------"
                echo " `date` SVT AUTOMATION: WARNING: Operation timed out - condition $condition was not achieved as expected"
                echo "-----------------------------------------"
                return 1;
            fi
        fi
        sleep 2;
    done
    echo "Success. Condition $condition has been met."
    return 0;
}

svt_verify_condition2() {
    local condition=$1
    local count=$2
    local file_pattern=$3
    local now=$4 
    local qos=$5 
    local test_app=${6-"mqttsample_array"} ;
    local num_clients=${7-0} ; # if passed in it will do extra verification that exactly this many clients is running.
    local timeout=${6-0} ;
    local starttime=0;
    local endtime=0;
    local curtime=0;
    local myregex="[0-9]+"
    local checkrc
    local checkrc2

    echo "File pattern is $file_pattern"

    if (($timeout>0)); then
        starttime=`date +%s`
        let endtime=($starttime+$timeout);
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION: Timeout set for this operation. Operation will timeout at $endtime ( timeout: $timeout seconds)"
        echo "-----------------------------------------"
    fi 

    if ! [[ "$count" =~ $myregex ]] ; then
        echo "ERROR: count non-numeric"
        return 1;
    fi
    k=0;
    while (($k<$count)); do
        svt_check_processes_still_active2 ${now} ${qos} ${test_app}
        checkrc=$?
        if (($num_clients>0)); then
            svt_check_processes_still_active2 ${now} ${qos} ${test_app}
            checkrc2=$?
        fi
    
        w=`grep "WARNING" $file_pattern 2>/dev/null |wc -l`
        e=`grep "ERROR" $file_pattern 2>/dev/null  |wc -l`
        c=`ps -ef | grep $test_app 2>/dev/null | grep -v grep 2>/dev/null  | wc -l`
        k=`grep "$condition" $file_pattern 2>/dev/null |wc -l`
        echo "Waiting for condition $condition to occur $count times(current: $k) warnings($w) errors:($e) clients:($c)"
        date;
    	if (($k>=$count)); then
            echo "Success: Condition has been met k = $k , count = $count"
            break;
	    fi
        if [ "$checkrc" != "0" ] ;then
            	if [ "$SVT_AT_VARIATION_SKIP_PREMATURE_BREAK" == "true" ] ;then
            		echo "`date` WARNING: breaking prematurely skipped due to SVT_AT_VARIATION_SKIP_PREMATURE_BREAK setting ... "
        			echo "WARNING: no processes are still active. k = $k , count = $count. Will not break yet due to overide setting. "
		else
            		echo "`date` WARNING: breaking prematurely because no processes are still active. k = $k , count = $count. "
            		return 1;
		fi	
        fi

        if (($timeout>0)); then
            curtime=`date +%s`
            if (($curtime>$endtime)); then
                echo "-----------------------------------------"
                echo " `date` SVT AUTOMATION: WARNING: Operation timed out - condition $condition was not achieved as expected"
                echo "-----------------------------------------"
                return 1;
            fi
        fi
        sleep 2;
    done
    echo "Success. Condition $condition has been met."
    return 0;
}

svt_check_log_files_for_complete(){
    local now_time=${1}
    local qos=${2}
    local num_clients=${3-0}
    local completion_string=${4-"SVT_TEST_RESULT:"}
    local data
    
    data=`cat log.*${qos}.*${now_time} 2>/dev/null | grep "$completion_string" | wc -l`
    echo "`date` cat log.*${qos}.*${now_time} 2>/dev/null | grep \"$completion_string\" | wc -l"
    echo "$data $num_clients"
    
    if [ "$data" == "$num_clients" ] ;then
        echo "`date`: All process log files  log.*\.${qos}\..*${now_time} show the completion string: $completion_string"
        return 0;
    else
        echo "`date`: $data != $num_clients"

    fi

    return 1;

}

svt_wait_60_for_processes_to_become_active(){
    local now=${1}
    local qos=${2}
    local process_name=${3-mqttsample_array}
    local num_clients=${4-0}
    local rc
    local x
    x=60
    while(($x>0)); do
        echo "`date`: start: svt_check_processes_still_active ${now} ${qos} ${process_name} ${num_clients}"
        svt_check_processes_still_active ${now} ${qos} ${process_name} ${num_clients}
        rc=$?
        if [ "$rc" != "0" ] ;then
            echo "`date`: $rc non zero"
            if svt_check_log_files_for_complete ${now} ${qos} ${num_clients} ; then
                echo "`date`: Detected log files show processes have already exitted."
                break;
            fi
            echo "`date`: The number of processes currently active for qos $qos active does not yet match the number expected $num_clients."
        else
            echo "`date`: Detected $num_clients processes for qos $qos active."
            break;
        fi
        sleep 1;
        let x=$x-1;
    done
}


# WHY: ssh -nx  
#  rsh and/or ssh can break a while read loop due to it using stdin. 
#  Put a -n into the ssh line which stops it from trying to use stdin and it fixed the problem.

# echo "ifconfig" | ssh  admin@10.10.10.10 "busybox"
# let iteration=0; while((1)); do ./ha_fail.sh; ((iteration++)); echo current iteration is $iteration; date; done


clear_server_core()  {
    local server=$1
    local line
    local data
    local var
    local rc
    if [ -n "$server" ] ; then
        data=`ssh -nx  admin@$server advanced-pd-options list | awk '{print $1}'`
        for var in $data ;do
            #line=`echo "$var" | grep -oE 'bt.*|.*_core.*' |awk '{print $1}' `
            #if echo "$line" |grep -E 'bt.*|.*_core.*' >/dev/null ; then
            #line=`echo "$var" | grep -oE 'bt.*' |awk '{print $1}' `
            line=`echo "$var" | awk '{print $1}' `
            #if echo "$line" |grep -E 'bt.*' >/dev/null ; then
                echo ssh  -nx admin@$server advanced-pd-options delete $line
                ssh  -nx admin@$server advanced-pd-options delete $line
                rc=$?
                echo rc is $rc
            #fi
        done
    else
        echo "ERROR: did not specifify server argument"
    fi
}
exist_server_core()  {
    local server=$1;
    local data
    local regex="bt.*"
    data=` ssh -nx  admin@$server advanced-pd-options list`
    #if echo "$data" | grep -E 'bt.*|.*_core.*'  > /dev/null ;then
    if echo "$data" | grep -E 'bt.*'  > /dev/null ;then
        echo "==========================================================="
        echo "==========================================================="
        echo "==========================================================="
        echo ""
        echo ""
        echo "WARNING: core data found is : $data" >> /dev/stderr
        echo ""
        echo ""
        echo ""
        echo "==========================================================="
        echo "==========================================================="
        echo "==========================================================="
    fi
    if [[ $data =~ $regex ]] ; then
        return 0; 
    fi
    return 1;
}
get_server_time()  {
    echo ssh -nx  admin@$ima1 datetime get
    ssh  -nx admin@$ima1 datetime get
    echo "";
    echo ssh  -nx admin@$ima2 datetime get 
    ssh -nx  admin@$ima2 datetime get
}

synchronize_server_time()  {
 local setdate=`date "+%Y-%m-%d %H:%M:%S"`
    echo ssh -nx  admin@$ima1 datetime set $setdate
    ssh  -nx admin@$ima1 datetime set $setdate
    echo ssh  -nx admin@$ima2 datetime set $setdate
    ssh -nx  admin@$ima2 datetime set $setdate
}

get_harole(){
    local server=$1;
    local rc;
    local data;

    data=`run_command_with_timeout "ssh -nx  admin@$server imaserver harole " 5`
    if [ "$rc" == "0" ] ;then
        echo "$data"
        return 0;
    else
        echo "WARNING: harole command failed: ssh -nx  admin@$server imaserver harole after 5 second timeout"
        return 1;
    fi
    return 0;
}

run_command_with_timeout() {
    local command="$1"
    local timeout=${2-60}
    local running_test;
    local test_exit;
    local starttime=`date +%s`
    $command &
    running_test=$!
    echo -e "==========================================================================="
    echo -e "Running $command with timeout $timeout, as pid: running_test=$running_test\n"
    while(($timeout>0)); do
        if ! [ -e /proc/$running_test/cmdline  ] ;then
            echo "INFO: /proc/$running_test/cmdline does not exist anymore" 
            break;
        fi
        let timeout=$timeout-1;
        sleep 1;
    done
    local endtime=`date +%s`
    local totaltime
    let totaltime=($endtime-$starttime)
    
    echo -e "\nINFO: response time: $totaltime $command"
    echo "INFO: After breaking from loop, timeout value is now $timeout"
    if (($timeout<=0)); then
        echo "ERROR: TIMEOUT running command $command, kill -9 $running_test" 
        kill -9 $running_test;
        return 1;
    fi
    wait $running_test
    test_exit=$?
    echo "INFO: test exit is $test_exit for $running_test"
    return $test_exit;
}


do_admin_loop (){
    local server=$1
    local usercmd=${2-""}
    local outlog=${3-"log.admin.${server}"}
    rm -rf $outlog
    run_command_with_timeout "ssh -nx  admin@$server imaserver set AcceptLicense "
    run_command_with_timeout "ssh -nx  admin@$server imaserver update Endpoint Name=DemoEndpoint Enabled=true"
    while ((1)) ; do
        if [ -n "$usercmd" ] ; then
            run_command_with_timeout "ssh -nx  admin@$server $usercmd"
        else
            run_command_with_timeout "ssh -nx  admin@$server imaserver harole "
            run_command_with_timeout "ssh -nx  admin@$server imaserver stat "
            run_command_with_timeout "ssh -nx  admin@$server datetime get "
            run_command_with_timeout "ssh -nx  admin@$server advanced-pd-options list "
            run_command_with_timeout "ssh -nx  admin@$server advanced-pd-options memorydetail "
    
            for var in Memory Store Server Connection; do 
                run_command_with_timeout "ssh -nx  admin@$server imaserver stat $var "
            done
    
            for var in battery ethernet-interface flash imaserver memory mqconnectivity nvdimm temperature uptime vlan-interface voltage volume webui ; do
                run_command_with_timeout "ssh -nx  admin@$server status $var"
            done
        fi
        date
    done  | add_log_time "${server}" | tee -a $outlog


}

add_log_time () {
    local additional_msg=${1-""}
    if [ -n "$additional_msg" ] ;then
        msg=" ${additional_msg} "
    else
        msg=" ";
    fi
    while read line; do
        echo "`date +"%Y-%m-%d %H:%M:%S.%U" `${msg}${line}"
    done
}


svt_automation_master () {
	svt_automation_systems_list 2> /dev/null | head -1 
}

svt_automation_cleanup () {
    local flag=$1
	local v
    local waitpid

    waitpid=""
	for v in `svt_automation_systems_list` ; do ssh $v "/niagara/test/svt_cmqtt/SVT.cleanup.sh $flag " & waitpid+="$! "; done
    echo "-----------------------------------"
    echo "Start -Waiting for cleanup processes"
    echo "-----------------------------------"
    wait $waitpid
    echo "-----------------------------------"
    echo "Done. Waiting for cleanup processes"
    echo "-----------------------------------"
    
}
svt_automation_cleanup_force () {
    local waitpid

    svt_automation_cleanup force

    
}

svt_automation_run_command_and_master() {
    for v in `svt_automation_systems_list ` ; do
        echo "======================== Run command on $v =========================== ";
        ssh $v $@
    done;
}
svt_automation_run_command() {
    local master=`svt_automation_master`;
    echo "args are $@"
    for v in `svt_automation_systems_list | grep -v $master`; do
        echo "======================== Run command on $v =========================== ";
        ssh $v $@
    done;
}

svt_automation_transfer () 
{ 
    local specific_file=${1-""}
    local v;
    local master=`svt_automation_master`;
    local cwd=`pwd`;
    if [ "$cwd" != "/niagara/test/svt_cmqtt" ]; then
        echo "---------------------------------";
        echo "Skipping cleanup for $cwd";
        echo "---------------------------------";
    else
        echo "---------------------------------";
        echo "do clean up for $cwd";
        echo "---------------------------------";
        for v in `svt_automation_systems_list | grep -v $master`;
        do
            echo $v;
            ssh $v "rm -rf /niagara/test/svt_cmqtt/*";
        done;
        rm -i -rf /niagara/test/svt_cmqtt/log.*;
        rm -i -rf /niagara/test/svt_cmqtt/*.log;
        rm -i -rf /niagara/test/svt_cmqtt/10;
        rm -i -rf /niagara/test/svt_cmqtt/11;
    fi;

    if [ -n "$specific_file" ] ;then
        echo "---------------------------------";
        echo "Transfer specific file: $specific_file for $cwd";
        echo "---------------------------------";
        for v in `svt_automation_systems_list | grep -v $master`;
        do
            echo $v;
            echo scp $specific_file $v:$cwd/$specific_file
            scp $specific_file $v:$cwd/$specific_file
        done
    else
        echo "---------------------------------";
        echo "Transfer all files for $cwd";
        echo "---------------------------------";
        for v in `svt_automation_systems_list | grep -v $master`;
        do
            echo $v;
            echo scp -r * $v:$cwd
            scp -r * $v:$cwd
        done
    fi
}


svt_automation_env(){
    test_template_env
    #if [[ -f "/niagara/test/testEnv.sh" ]]
    #then
        ## Official ISM Automation machine assumed
        #source /niagara/test/testEnv.sh
    #else
        ## Manual Runs need to build Customized ISMSetup for THIS param, SSH environment file and Tag Replacement
        #source ../scripts/ISMsetup.sh
        ##../scripts/prepareTestEnvParameters.sh
        ##source ../scripts/ISMsetup.sh
    #fi
} 
svt_automation_systems_list () {
    test_template_systems_list
    #local v=1
    #svt_automation_env
    #{ while (($v<=${M_COUNT})); do
        #echo "M${v}_HOST"
        #let v=$v+1;
    #done ; }  |  xargs -i echo "echo \${}" |sh
}

svt_automation_sync_all_machines () {
    local condition=${1-""}
    local transfer_to_other_M=${2-1}
    local condition_msg=${3-"$condition"}
    local condition_msg_additional=${4-""}
    local suggested_sync_interval=${5-0} ; # suggested sync interval... if it is found it is way to large it can be renegotiated.
    local timeout=${6-0}
    local sync_interval=30; # new: 7/25/13 default is 30 plus dynamic delta
    local waitpid
    local continue_time_suggestion=0;
    local v
    local data
    local delta
    local deltab
    local tmpa
    local tmpb
    local LOGP
    local LOGPM

    svt_automation_env

    LOGP=$(do_get_log_prefix)
    LOGPM=$(do_get_log_prefix MININUM)
   
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION: Start sync on condition : $condition with all machines in M_COUNT: $M_COUNT . THIS is $THIS ."
    echo "-----------------------------------------"
    while ((1)); do
        if [ "$transfer_to_other_M" != "0" ] ; then
            if { for v in `svt_automation_systems_list` ; do ssh $v "if [ -e ${M1_TESTROOT}/svt_cmqtt/${LOGP}.sync.${THIS}.${condition}.complete ] ; then echo $v yes; else echo $v missing_sync;  fi" ; done ; } | grep missing_sync ; then
                echo "-----------------------------------------"
                echo " `date`  Do the sync operation. some of the expected sync files are still missing"
                echo "-----------------------------------------"
            
                echo "-----------------------------------------"
                echo " `date` Tell all Ms that this system has met $condition "
                echo "-----------------------------------------"
                waitpid=""
                for v in `svt_automation_systems_list` ; do
                    echo "-----------------------------------------"
                    echo " ssh $v echo \"$condition\" > ${M1_TESTROOT}/svt_cmqtt/${LOGP}.sync.${THIS}.${condition}.complete  "
                    echo "-----------------------------------------"
                    if [ "${condition_msg_additional}" == "SUGGEST_CONTINUE_TIME_BASED_ON_SYNC_INTERVAL" ] ; then
                        if [ "$continue_time_suggestion" == "0" ] ;then
                            continue_time_suggestion=`date +%s`
                            let continue_time_suggestion=$continue_time_suggestion+$suggested_sync_interval
                            echo "-----------------------------------------"
                            echo " `date` Determined continue_time_suggestion = $continue_time_suggestion for THIS: ${THIS}, Current time is `date +%s` "
                            echo "-----------------------------------------"
                        fi
                    fi
                    if [ "${condition_msg_additional}" == "SUGGEST_CONTINUE_TIME_BASED_ON_SYNC_INTERVAL" ] ; then
                        ssh $v "echo \"${condition_msg} ${continue_time_suggestion}\" > ${M1_TESTROOT}/svt_cmqtt/${LOGP}.sync.${THIS}.${condition}.complete" &
                    else
                        ssh $v "echo \"${condition_msg} ${condition_msg_additional}\" > ${M1_TESTROOT}/svt_cmqtt/${LOGP}.sync.${THIS}.${condition}.complete" &
                    fi
                    waitpid+="$! "
                done
                wait $waitpid           
            else
                echo "-----------------------------------------"
                echo " `date` All sync files present"
                echo "-----------------------------------------"
                break;
            fi
        else
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION: Sync file transfer not required for transfer_to_other_M setting: $transfer_to_other_M"
            echo "-----------------------------------------"
            break; # no transfer needed for 
        fi
        

    done


    #------------------------------
    # Sync point here. wait till everyone else has also met the condition by telling this system that they have met the condition.
    #------------------------------
    svt_verify_condition "$condition_msg" ${M_COUNT} "${LOGPM}*.sync.*.$condition.complete" 0 "mqttsample_array" $timeout

    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION: Completed sync on condition : $condition."
    echo "-----------------------------------------"

    if [ "$transfer_to_other_M" == "1" ] ; then
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION: Show date on all systems. If they are not synchronized within a few seconds..."
        echo " `date` SVT AUTOMATION: This next part won't work.  If clients are not synchronized within a few seconds."
        echo "-----------------------------------------"
        tmpa=`date +%s` 
        delta=0;
        for v in `svt_automation_systems_list` ; do
            echo -n "$v: " ; 
            data=`ssh $v " date +%s ; date ; "`;
            echo "$data" 
            tmpb=`echo $data | awk '{print $1}'`
            let deltab=($tmpb-$tmpa);
            if [ "$delta" == "0" ] ;then
                delta=$deltab
            fi
            if (($deltab>$delta)); then
                delta=$deltab
            fi
        done
        
        echo "-----------------------------------------"
        echo "delta is $delta"
        let delta=($delta*4);
        #if (($delta<5)); then
            #let delta=($delta+10);
        #elif (($delta<10)); then
            #let delta=($delta+5);
        #fi
        
        echo "NEW :recommended delta= 4 * <discovered delta> + constant = $delta"
        if [ "0" == "1" ] ;then # disabled for now
        #if ((($sync_interval/$delta)>2)) ; then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION: It appears the client systems meet all criteria for renegotiation w/ delta: $delta . Will renegotiate sync_interval to a smaller value (hopefully) "
            echo "-----------------------------------------"
            svt_automation_sync_all_machines "$condition.delta" 2 "$condition.delta" "$delta" na 30
            rc=$?
            if [ "$rc" == "0" ] ;then
                echo "-----------------------------------------"
                echo " `date` SVT AUTOMATION: Print all renegotiated delta suggestions, there should be $M_COUNT suggestions... FIXME"
                echo "-----------------------------------------"
                cat ${LOGPM}*.sync.*.${condition}.delta.* | sed "s/$condition.delta//g" | sort -n
                delta=`cat ${LOGPM}*.sync.*.${condition}.delta.** | sed "s/$condition.delta//g" | sort -n | tail -1 | awk '{print $1}'`
                if (($delta<$sync_interval)); then
                    sync_interval=$delta;
                    echo "-----------------------------------------"
                    echo " `date` SVT AUTOMATION: Selected largest delta new delta renegotiated is sync_interval : $sync_interval "
                    echo "-----------------------------------------"
                else
                    echo "-----------------------------------------"
                    echo " `date` SVT AUTOMATION: WARNING: Unable to effectively renegotiate sync_interval. This was not expected. delta: $delta"
                    echo "-----------------------------------------"
                fi
            else
                echo "-----------------------------------------"
                echo " `date` SVT AUTOMATION: WARNING: sync_interval renegotiation failed or did not complete in 30 seconds."
                echo " `date` Will use default sync_interval :$sync_interval "
                echo " `date` SVT AUTOMATION: Just fyi: Print all renegotiated delta suggestions, there should be $M_COUNT suggestions... FIXME"
                echo "-----------------------------------------"
                cat ${LOGPM}*.sync.*.${condition}.delta.* | sed "s/$condition.delta//g" | sort -n
            fi
        else
            #echo "  `date` SVT AUTOMATION: $sync_interval / $delta was < 2 , so renegotiation criteria is not met " 
            echo "  `date` SVT AUTOMATION: renegotiaion of sync_interval is not yet implemented"
        fi
        
        echo "-----------------------------------------"
        continue_time_suggestion=`date +%s`
        v=`date +%s`         
        let continue_time_suggestion=($continue_time_suggestion+$suggested_sync_interval)
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION: Current time is $v, continue_time_suggestion is $continue_time_suggestion"
        echo "-----------------------------------------"
        #svt_automation_sync_all_machines "$condition.time" 2 "$condition.time" "$continue_time_suggestion"
        echo "-----------------------------------------"
        let suggested_sync_interval=(25+$delta); # 2013 07 25 - overriding sync interval based on new ideas
        echo " `date` SVT AUTOMATION: Overiding continue_time_suggestion, will use dynamic suggestion"
        echo " `date` SVT AUTOMATION: new custom suggested_sync_interval : $suggested_sync_interval"
        echo "-----------------------------------------"
        svt_automation_sync_all_machines "$condition.time" 2 "$condition.time" "SUGGEST_CONTINUE_TIME_BASED_ON_SYNC_INTERVAL" $suggested_sync_interval 
    elif [ "$transfer_to_other_M" == "0" ] ; then
        echo "-----------------------------------------"
        echo " `date` SVT AUTOMATION: Syncing on continue time suggestion. This ${THIS} clients suggestion is $continue_time_suggestion."
        echo "-----------------------------------------"
        svt_verify_condition "$condition.time" ${M_COUNT} "${LOGPM}*.sync.*.$condition.time.complete"
    elif [ "$transfer_to_other_M" == "2" ] ; then
        echo "-----------------------------------------"
        echo "For transfer_to_other_M setting of 2, we return now back to the caller."
        echo "-----------------------------------------"
        return;
    else
        echo "-----------------------------------------"
        echo "ERROR: Internal bug in script"
        echo "-----------------------------------------"
        exit 1;
    fi


    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION: Print all continue time suggestions, there should be $M_COUNT suggestions... FIXME"
    echo "-----------------------------------------"
    cat ${LOGPM}*.sync.*.${condition}.time.* | sed "s/$condition.time//g" | sort -n
    continue_time_suggestion=`cat ${LOGPM}*.sync.*.${condition}.time.** | sed "s/$condition.time//g" | sort -n | tail -1 | awk '{print $1}'`
    v=`date +%s`         
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION: Selected latest suggestion :$continue_time_suggestion, current time is $v "
    echo "-----------------------------------------"
    while (($v>$continue_time_suggestion)) ; do
        v=`date +%s`         
        let delta=($v-$continue_time_suggestion);
        if (($delta>300)); then
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION: WARNING: Skew of delta is too great : delta= $delta. Max allowed is 300 " 
            echo " `date` SVT AUTOMATION: WARNING: Will not be able to sync on continue time for this test."
            echo "-----------------------------------------"
            let continue_time_suggestion=0;
            break;
        else
            let continue_time_suggestion=($continue_time_suggestion+$sync_interval);
            echo "-----------------------------------------"
            echo " `date` SVT AUTOMATION: WARNING: Increase continue_time_suggestion :\"$continue_time_suggestion\" . current time is $v"
            echo "-----------------------------------------"
        fi
    done
    
    v=`date +%s`         
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION: Waiting to continue test at $continue_time_suggestion. current time is $v"
    echo "-----------------------------------------"
    while (($v<continue_time_suggestion)) ; do
        usleep 10000;
        v=`date +%s`         
    done
    echo "-----------------------------------------"
    echo " `date` SVT AUTOMATION: Time synced successfully with ALL Ms. Current time is $v. Continuing test..."
    echo "-----------------------------------------"

}


svt_automation_backup() {
    local backup_server=${1-10.10.10.10}
    tar -czvf /tmp/outputs.`date +%m%d%Y.%H%M`.svt.tgz *.sh *.exp ../scripts/SVT.* ../scripts/commonFunctions.sh ../common_src/SVT_IMA_environment.cli ; scp /tmp/outputs*.svt.tgz root@10.10.10.10:/home/developer/BACKUPS
    rm -rf /tmp/outputs.*svt.tgz
}

svt_do_automatic_backup(){
    svt_automation_env
    if [ "$AF_AUTOMATION_ENV" == "SANDBOX" ] ;then
        echo "In sandbox environment. Automatic backup required."
        svt_automation_backup
    else
        echo "Not in sandbox environment. No automatic backup required."
    fi
}



bulk_grep_init(){ 
    rm -rf .tmp.bulk_grep.*
}

bulk_grep(){ 
    local full_date="$1"
    local file_path="$2"
    stem=`echo "$full_date" | cut -b-17`
    stem_path=`echo "$stem" |  tr ':-[[:space:]]' '_'`

    echo "Processing $full_date  stem:$stem  stem_path:$stem_path " >> /dev/stderr
    search_file=".tmp.bulk_grep.$stem_path"
    if [ -e $search_file ] ; then
        search_file=".tmp.bulk_grep.$stem_path"
    else
        echo "Generating new cache file for $stem_path " >> /dev/stderr
        rm -rf .tmp.bulk_grep.*
        echo "grep \"$stem\" $file_path > $search_file " >> /dev/stderr
        grep "$stem" $file_path > $search_file
    fi
    grep "$full_date" $search_file
}


svt_automation_ping(){
    let x=0;
    svt_automation_env
    for v in `svt_automation_systems_list` ; do            
        echo  "================================================ pinging $v " ; 
        ping -c 1 -w 1  $v  ;   
        let x=$x+$?
    done
    echo ""
    echo "Done. RC of $x (number of ping failures) for ${M_COUNT} systems"
    echo ""
}
    
svt_exit_with_result(){
    local fail_count=${1-0};
    local test_name=${2-""};
    if (($fail_count>0)); then
        echo "-----------------------------------------"
        echo "`date` Done - SVT_TEST_RESULT: Failure. Completed all expected processing w/ $fail_count failures. Exit with 1. "
        echo "-----------------------------------------"
        svt_continue_transaction $test_name 1
        exit 1;
    else
        echo "-----------------------------------------"
        echo "`date` Done - SVT_TEST_RESULT: SUCCESS . Completed all expected processing w/ $fail_count failures . Exit with 0."
        echo "-----------------------------------------"
        svt_continue_transaction $test_name 0
        exit 0;
    fi
}

RANDOM=`date +%s`;
#-------------------------------------------------------
# This can be used to return a pseudo random true or
# false with the a 1 in <chance> value of being true
#-------------------------------------------------------
svt_check_random_truth(){
    local chance=${1-2} # chance value == 2 means 50% or 1 in 2 chance of returning true;
    local upper=32767 ; # upper limit of what is returned by RANDOM
    local var;
    local randvar=$RANDOM
    let var=(${randvar}%${chance})

    #echo "rv:$randvar var:$var chance:$chance"

    if [ "$var" == "0" ] ;then
        return 0;  # randomly returned true based on 1 in <chance>
    else
        return 1;  # did not return true
    fi

}

svt_suspend_publishers(){
svt_automation_run_command_and_master "ps -ef |grep mqttsample_array |grep -v kill | grep publish | awk '{print \$2}' | xargs -i kill -19 {}"
}
svt_continue_publishers(){
svt_automation_run_command_and_master "ps -ef |grep mqttsample_array | grep -v kill| grep publish | awk '{print \$2}' | xargs -i kill -18 {}"
}
svt_automation_run_command_and_master2() {
 local cmd
local x=0
    local funcname=${1-""}
    local prompt=${2-""}
        local line
    for v in `svt_automation_systems_list ` ; do
        let x=$x+1;
                cmd=`func $x `
        if ! [ -n "$cmd" ] ;then
                echo "error empty cmd"
                return 1;
        fi
        echo "======================== Run command $cmd on $v =========================== ";
        echo prompt is $prompt
        if [ -n "$prompt" ] ;then
                echo " y or n "
                read line
                if [ "$line" == "y" ] ;then
                        ssh $v $cmd
                else
                        echo skipped.
                fi
        else
                        ssh $v $cmd
        fi
    done;
}


get_client_ip(){
    local client_ip=`hostname -i`

    if [ -z "$client_ip" ] ;then
        svt_automation_env
        client_ip=$(echo "\${${THIS}_HOST} " | xargs -i echo "echo {}" |sh)
    fi

    if [ -z "$client_ip" ] ;then
        echo "ERROR: Cannot determinte client ip with hostname -i or automation env vars" >> /dev/stderr
    else
        echo "$client_ip"
    fi

}



#----------------------------------
# svt_query_subs : Returns 0 if no subscriptions found matching topicstring pattern
# svt_query_subs : Returns 1 if 1 or more subscriptions found matching topicstring pattern
#----------------------------------
svt_query_subs () {
    local server=${1-""}
    local topicstring=${2-""}
    local data
    local rc=1
    local count=0;

    data=`ssh -nx admin@$server imaserver stat Subscription "TopicString=*${topicstring}*" StatType=PublishedMsgsHighest "ResultCount=100" | sed '1d'  | sed "s/\"//g"`
    count=`echo "$data" |wc -l`
    if ! [ -n "$data" ] ;then
        echo "------------------------"
        echo "$count - \"$data\" : empty string - no more subs waiting for deletion"
        echo "------------------------"
        rc=0
    elif (($count>0));  then
        echo "------------------------"
        echo "$count subs still waiting for deletion"
        echo "------------------------"
    else
        echo "------------------------"
        echo "$count - no more subs waiting for deletion"
        echo "------------------------"
        rc=0;
    fi

    return $rc        

}
svt_del_subs () {
    local server=${1-""}
    local topicstring=${2-""}
    local data
    local lastdata="";
    local rc=1
    local count=0;
    
    while((1)); do
        # pull of subscription list deleting the first line which should be the header row.
        echo "topicstring is $topicstring" 
        echo "ssh -nx admin@$server imaserver stat Subscription \"TopicString=*${topicstring}*\" StatType=PublishedMsgsHighest \"ResultCount=100\" | sed '1d'  | sed \"s/\"//g\""
        data=`ssh -nx admin@$server imaserver stat Subscription "TopicString=*${topicstring}*" StatType=PublishedMsgsHighest "ResultCount=100" | sed '1d'  | sed "s/\"//g"`
    
        echo "data is \"$data\"" 
        
        count=`echo "$data" |wc -l`
        if ! [ -n "$data" ] ;then
            echo "------------------------"
            echo "$count - \"$data\" : empty string - no more subs waiting for deletion"
            echo "------------------------"
            rc=0
            break;
        elif (($count>0));  then
            echo "------------------------"
            echo "$count subs still waiting for deletion"
            echo "------------------------"
        else 
            echo "------------------------"
            echo "$count - no more subs waiting for deletion"
            echo "------------------------"
            rc=0;
            break;
        fi
        
        
        if [ "$lastdata" != "" ] ;then
    	    if [ "$lastdata" == "$data" ] ; then
    		    echo "Breaking because no further progress can be made with cleanup up old durable subscriptions."
    		    break;
    	    fi
        fi
        
        lastdata=$data
        
    
        { echo -n "ssh -nx admin@$server ' " ; echo "$data" | awk -F ',' '{printf(" imaserver delete Subscription \"SubscriptionName=%s\" \"ClientID=%s\" ; ", $1,$3);}' ; echo "' "; }  | sh -x
    
    done
    return $rc;

}

#--------------------------------------------------------------------
# The test transaction functions below do handle the run-scenario TIMEOUT, and direct failures. 
# But the other "soft" errors like core file detected or imaserver stopped running during test are not handled.
#--------------------------------------------------------------------
svt_start_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    local last_runscenario_rc_file="./.svt.af.runscenario.last.rc"
    export AUTOMATION_FRAMEWORK_SAVE_RC=$last_runscenario_rc_file
    rm -rf ${transaction_file}.failed
    echo "0" > ./.svt.af.runscenario.last.rc
    echo "`date` " > $transaction_file
    echo "0" > ${transaction_file}.last.exit
}
svt_continue_transaction(){
    local testname=${1-""}
    local exitcode=${2-""}
    local transaction_file="./trans.log.svt.test.transaction"

    if [ -e $transaction_file ] ; then
        echo "`date` $testname $exitcode" >> $transaction_file
        echo "$exitcode" >  ${transaction_file}.last.exit
    fi
}
svt_running_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    if [ -e $transaction_file ] ;then
        return 0; # yes we are running a transactiton
    else
        return 1; # no we are not running a transaction
    fi  
    return 1; # no we are not running a transaction
}  
svt_check_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    local last_runscenario_rc_file="./.svt.af.runscenario.last.rc" # especially used to detect TIMEOUT s
    local var=""
    local var2=""
    if [ -e $transaction_file -a -e $last_runscenario_rc_file ] ;then
        var=`cat ${transaction_file}.last.exit`
        var2=`cat ${last_runscenario_rc_file}`
        if [ -e ${transaction_file}.failed ] ; then
            echo "`date` Error in transaction: Transaction has failed due to existence of ${transaction_file}.failed  "
            return 2; # transaction has failed already
        elif [ "$var" != "0" -o "$var2" != "0" ]; then 
            echo "`date` Error in transaction: v:$var v2:$var2 "
            echo "`date` Error in transaction: v:$var v2:$var2 " > ${transaction_file}.failed
            return 1; # transaction is failing.
        else
            return 0; # transaction is passing
        fi
        return 1; # transaction is failing.
    fi
    return 1 ; # Failure because we should be inside a transaction
}
svt_clear_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    find ${transaction_file}* | xargs -i rm -rf {} 
}
svt_end_transaction(){
    local transaction_file="./trans.log.svt.test.transaction"
    local rc
    cat $transaction_file
    if svt_check_transaction ; then 
        echo "SVT Test Transaction passed"
        rc=0;
    else
        echo "SVT Test Transaction failed "
        rc=1;
    fi
    find ${transaction_file}* | xargs -i mv {} {}.complete
    #rm -rf ${transaction_file}
    #rm -rf ${transaction_file}.*
    sync
    return $rc
}


svt_get_85_percent_mem_utilization_reached_file() {
    local server=$1;
    local mem_utilization_reached_85_file="/niagara/test/svt_cmqtt/trans.log.mem.utilization.reached.85.percent.$server"

    echo "$mem_utilization_reached_85_file"
}
svt_monitor_for_85_percent_mem_utilization() {
    local server=$1;
    local termprocess=${2-"mqttsample_array"} ; # - process to send sigterm to when 85 percent utilization  has been achieved  for sustained seconds
    local sustained=${3-120} ; # - how many seconds do we want to make sure the 85 percent utilization was sustained before indicating success
    local mem_utilization_reached_85_file="/niagara/test/svt_cmqtt/trans.log.mem.utilization.reached.85.percent.$server"
    local smallest_mem_utilization=100
    local data
    local previous
    local starttime=0
    local curtime
    local deltatime
    local enableprocess_checking=0
    
    rm -rf $mem_utilization_reached_85_file

    
    while((1)); do
        data=`ssh admin@$server "advanced-pd-options memorydetail" | grep MemoryFreePercent | awk '{print $3}'`

        if echo "$data" | grep [0-9] > /dev/null ; then
            if [ -n "$data" ] ;then
                if (($data<=15)); then
                    curtime=`date +%s`
                    if [ "$starttime" != "0" ] ;then
                        let deltatime=($curtime-$starttime)
                        if(($deltatime>$sustained)); then
                            echo "`date` Reached 85% message utilization, and stayed >= 85% for $sustained seconds. " >> $mem_utilization_reached_85_file
                            echo "`date` Sending sigterm to all $termprocess"
                            killall -15 $termprocess
                            echo "`date` Breaking..."
                            break;
                        fi
                    else
                        starttime=`date +%s`
                    fi
                elif (($data<$smallest_mem_utilization)); then
                    smallest_mem_utilization=$data
                fi

                if ((data>15)); then
                    starttime=0
                fi

                if [ "$data" != "$previous" ] ;then
                    previous=$data
                    echo "`date` MemoryFreePercent has reached $previous"
                    if (($data<=15)); then
                        echo "`date` MemoryFreePercent has reached $previous, < 15  must be sustained for $sustained seconds before success is reported"
                    fi
                fi
            fi
        fi
    
        data=`ps -ef | grep mqttsample_array | grep -v grep |wc -l`
        if echo "$data" | grep [0-9] > /dev/null ; then
            if [ -n "$data" ] ;then
                if (($data<=0)); then
                    if(($enableprocess_checking>0)); then
                        echo "No mqttsample_array processes are running. Break." >>  $mem_utilization_reached_85_file
                        break;
                    fi
                elif (($data>0)); then
                    enableprocess_checking=1; # Dont fail for 0 processes until at least 1 was detected.
                fi
            fi
        
        fi
    
        #data=`grep "WARNING" /niagara/test/svt_cmqtt/log.p.* |wc -l`
        #if echo "$data" | grep [0-9] > /dev/null ; then
            #if [ -n "$data" ] ;then
                #if (($data>=1000)); then
                    #if (($smallest_mem_utilization<25)); then 
                        #echo "Warning: Greater than 1000 warning messages and Mem Utililization > 75. Break." 
                        #break;
                    #else
                        #echo "Greater than 1000 warning messages, but smallest_mem_utilization:$smallest_mem_utilization > 25"
                    #fi
                #fi
            #fi
        #fi
    
        echo -n "."
        sleep 1;
    done
    
    echo "----------------------------------------------"
    echo "`date` Record status in  $mem_utilization_reached_85_file file."
    echo "----------------------------------------------"
    echo "`date` Reached 85 percent mem utilization" >>  $mem_utilization_reached_85_file
    
    #echo "----------------------------------------------"
    #echo "`date` Record published messages"
    #echo "----------------------------------------------"
    #data=`grep -h "actual_msgs:" log.p.*  | awk 'BEGIN { x=0; } x=x+$2; END {print sum}' |  grep -h "actual_msgs:" log.p.*  | awk 'BEGIN { x=0; } {x=x+$2;} END {print x}' `
    #echo "Record published messages: $data" >>  $mem_utilization_reached_85_file
    
    echo "----------------------------------------------"
    echo "`date` Done."
    echo "----------------------------------------------"
    
    return 0;
}

svt_check_subscription_buffered_msgs(){
    local server=${1}
    local topicpattern=${2}
    local required_buffer_sz=${3-0}
    local data
    local lineno

    data=`ssh admin@$server imaserver stat Subscription "TopicString=*${topicpattern}*" "StatType=BufferedMsgsHighest" |  sed "s/\"//g" `

    #"SubName","TopicString","ClientID","IsDurable","BufferedMsgs","BufferedMsgsHWM","BufferedPercent","MaxMessages","PublishedMsgs","RejectedMsgs","BufferedHWMPercent","IsShared","Consumers"
    #"/51/mar473rtc43580.2/82","/51/mar473rtc43580.2/82","82_51.2.mar473","True",6,6,0.0,20000000,5,0,0.0,"False",0
    #"/51/mar473rtc43580.2/80","/51/mar473rtc43580.2/80","80_51.2.mar473","True",6,6,0.0,20000000,6,0,0.0,"False",0
    #"/51/mar473rtc43580.2/81","/51/mar473rtc43580.2/81","81_51.2.mar473","True",6,6,0.0,20000000,6,0,0.0,"False",0
    #"/51/mar473rtc43580.2/77","/51/mar473rtc43580.
    
    let lineno=1
    for line in $data ; do
    #    echo "line:$line"
        if [ "$lineno" == "1" ] ; then
            var=`echo "$line" | awk -F ',' '{print $5}'`
            if [ "$var" != "BufferedMsgs" ] ; then
                echo "ERROR: 5th column specification changed and is no longer BufferedMsgs. Please fix the script."
                echo "ERROR: 5th column specification is now \"$var\""
                return 1;
            fi
        else
            var=`echo "$line" | awk -F ',' '{print $5}'`
            if (($var!=$required_buffer_sz)); then
                echo "ERROR: Detected unexpected $var buffered messages, expected $required_buffer_sz "
                return 1;
            else
                echo "PASSED: Detected expected $var buffered messages, expected $required_buffer_sz "
                return 0;
            fi
        fi
        
        let lineno=$lineno+1
    
    done
    
    echo "PASSED: no subscriptions detected for that topic pattern"
    return 0; # no Subscriptions detected for that topic pattern
       
} 

declare -a svt_common_test_array=(
    #---------------------------------------------------------
    # Test bucket SVT_01: Scale MQTTv3 connection (3 qos 0 messages to each connection - 3 Milllion total messages)
    #---------------------------------------------------------
    SVT_01a 1800 
        "SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_MSGCNT=6|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60" 
        "Scale MQTTv3 connection workload"

    #---------------------------------------------------------
    # Test bucket SVT_01a: variation - same as Test bucket 1 , but now with qos 1 and publishing every 20 seconds
    #---------------------------------------------------------
    SVT_01b 1800
        "SVT_AT_VARIATION_QOS=1|SVT_AT_VARIATION_MSGCNT=6|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_BURST=2|SVT_AT_VARIATION_PUBQOS=1|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60" 
        "Scale MQTTv3 connection workload"

    #---------------------------------------------------------
    # Test bucket SVT_01b: variation - same as Test bucket 1 , but now with qos 2 and publishing every 20 seconds
    #---------------------------------------------------------
    SVT_01c 1800 
        "SVT_AT_VARIATION_QOS=2|SVT_AT_VARIATION_MSGCNT=6|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_CLEANSESSION=false|SVT_AT_VARIATION_BURST=2|SVT_AT_VARIATION_PUBQOS=2|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60"
        "Scale MQTTv3 connection workload"
    #---------------------------------------------------------
    # Test bucket SVT_01c: variation - same as Test bucket 1 , but now with qos 0, 100 msgs/client, in bursts of 10 every 20 sec
    #---------------------------------------------------------
    SVT_01d 3600 
        "SVT_AT_VARIATION_QOS=0|SVT_AT_VARIATION_MSGCNT=30|SVT_AT_VARIATION_PUBRATE=0.05|SVT_AT_VARIATION_SYNC_CLIENTS=true|SVT_AT_VARIATION_BURST=10|SVT_AT_VARIATION_VERIFYSTILLACTIVE=60" 
        "Scale MQTTv3 connection workload"

    #---------------------------------------------------------
    # End of test array, do not place anything after this comment
    #---------------------------------------------------------
    NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL
    NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL
)
SVT_COMMON_TEST_ARRAY_COLS=4

SVT_LAUNCH_SCALE_TEST_DESCRIPTION=

svt_common_launch_scale_test() {
    local workload=${1-"connections"}
    local num_clients=${2-998000}
    local additional_variation=${3-""}
    local additional_timeout=${4-0}
    local appliance_monitor="m1"; # appliance monitoring will run on this system by default.
    local per_client
    local a_variation=""
    local a_name=""
    local a_timeout=""
    local a_test_bucket=""
    local a_ha_test=""
    local repeat_count=0;
    local regexp

    local chance_of_test_running=$SVT_RANDOM_EXECUTION ;

#    echo "------------1--------------"
#    echo "workload is $workload"
#    echo "------------3--------------"
#    echo $3 
#    echo $additional_variation
#    echo "------------3--------------"
    
    let per_client=($num_clients/${M_COUNT});
    a_command="-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|${workload}|${per_client}|1|${A1_IPv4_1}"
    let j=0
    while [ "${svt_common_test_array[${j}]}" != "NULL" ] ; do
        #echo "j is $j ${svt_common_test_array[$j]} ${svt_common_test_array[$j+1]} ${svt_common_test_array[$j+2]} ${svt_common_test_array[$j+3]}"
        let a_timeout=(${svt_common_test_array[$j+1]}+${additional_timeout});
        #let a_timeout=600
        a_test_bucket="${svt_common_test_array[$j]}_${n}"  ; # n is a global variable incremented in the test_template_ calls 
        a_variation="${svt_common_test_array[$j+2]}"
        if [ -n "$additional_variation" ] ; then
            a_variation+="|$additional_variation"
        fi
        a_name="${svt_common_test_array[$j+3]}  - run ${per_client} ${workload} per client "       

        echo "a_test_bucket is $a_test_bucket"
        echo "a_timeout is $a_timeout"
        echo "a_variation is $a_variation"
        echo "a_name is $a_name"

        if [ -n "$SVT_RANDOM_EXECUTION" ] ; then
            if ! svt_check_random_truth $chance_of_test_running ; then
                regexp="SVT_AT_VARIATION_SINGLE_LOOP=true"
                if [[ $a_variation =~ $regexp ]] ; then
                    echo "Continuing test because SVT_AT_VARIATION_SINGLE_LOOP=true. "
                else
                    echo "Skipping this test, it was not selected for Random Execution."
                    let j=$j+$SVT_COMMON_TEST_ARRAY_COLS
                    continue;
                fi
            else 
                echo "Running test, it was selected for Random Execution, or has SVT_AT_VARIATION_SINGLE_LOOP=true."
            fi
        fi

        #if (($M_COUNT>=2)); then
            #appliance_monitor="m2"; # appliance monitoring will run on this system to distribute work more evenly
        #fi

        #regexp="SVT_AT_VARIATION_TEST_ONLY=true"
        #if ! [[ $a_variation =~ $regexp ]] ; then
            #test_template_add_test_single "${a_name} - start primary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_primary_appliance|${workload}|1|${A1_IPv4_1}" 
	        #if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
                #test_template_add_test_single "${a_name} - start secondary appliance monitoring" cAppDriver ${appliance_monitor} "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_secondary_appliance|${workload}|1|${A1_IPv4_1}|${A2_IPv4_1}" 
            #fi 
        #fi 

        #---------------------------------------
        # New way on 6.26.2013 - should be faster  - not working yet due to not cd'ing to cwd when ssh ing command
        #---------------------------------------
        #regexp="SVT_AT_VARIATION_TEST_ONLY=true"
        #if ! [[ $a_variation =~ $regexp ]] ; then
            #test_template_add_test_single "${a_name} - start client monitoring " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|monitor_client|${workload}|1|" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
        #fi
        #---------------------------------------


        #------------------------------------------------
        # Added next 5 lines for RTC 34564 - in order to turn this off you must set it to false , default - on 
        #------------------------------------------------
        regexp="SVT_AT_VARIATION_RESOURCE_MONITOR=false"
        if ! [[ $a_variation =~ $regexp ]] ; then
            regexp="SVT_AT_VARIATION_TEST_ONLY=true"
            if ! [[ $a_variation =~ $regexp ]] ; then
                test_template_add_test_single "${a_name} - start \$SYS/ResourceMonitor/# topic monitoring  " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|resource_topic_monitor|10|1|${A1_IPv4_1}|${A2_IPv4_1}" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
            fi
        fi


        regexp="SVT_AT_VARIATION_PUB_THROTTLE=true"
        if [[ $a_variation =~ $regexp ]] ; then
            test_template_add_test_single "${a_name} - start pub throttle  " cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|pub_throttle|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 300 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true"
            
        fi


        if [ -n "$SVT_LAUNCH_SCALE_TEST_DESCRIPTION" ] ;then
            description="${SVT_LAUNCH_SCALE_TEST_DESCRIPTION} - run workload variation: ${a_variation}"
        else
            description="${a_name} - run workload variation: ${a_variation} "
        fi

        test_template_add_test_single "$description" cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"


	    resume_test_prefix="${test_template_prefix}`awk 'BEGIN {printf("%02d",'${n}'); }'`"


        #FIXME test_template_add_test_all_M_concurrent  "${a_name} - run workload variation: ${a_variation} " "${a_command}" "${a_timeout}" "${a_variation}" "TEST_RESULT: SUCCESS"


        regexp="(.*)(SVT_AT_VARIATION_REPEAT=)([0-9]+)(.*)"
        if [[ $a_variation =~ $regexp  ]] ; then
            repeat_count=${BASH_REMATCH[3]}
            let repeat_count=$repeat_count-1;
	        regexp="SVT_AT_VARIATION_RESUME=true"
            if [[ $a_variation =~ $regexp ]] ; then
				echo "ERROR: do not try to use SVT_AT_VARIATION_RESUME=true and SVT_AT_VARIATION_REPEAT= flags at the same time"
				exit 1;
	    	fi
	   
            while(($repeat_count>0)); do
                test_template_add_test_single  "${a_name} - run workload variation: ${a_variation} repetition: ${repeat_count}"  cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"
                let repeat_count=$repeat_count-1;
            done
        fi

	    if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
            regexp1="SVT_AT_VARIATION_FAILOVER=STOP"
            if [[ $a_variation =~ $regexp1 ]] ; then
                #-------------------------------------------------
                # STOP - Basic ha test suite - stop primary, and restart it as standby
                #-------------------------------------------------

                test_template_add_test_single "SVT - HA test: stop Primary" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|stopPrimary|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
                test_template_add_test_single "SVT - HA test: start Standby" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|startStandby|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"


            else
                echo "No ha testing scheduled at this time or unrecognized command for SVT_AT_VARIATION_FAILOVER (A)"
            fi
	
       	 	regexp="(.*)(SVT_AT_VARIATION_FAILOVER_SLEEP=)([0-9]+)(.*)"
        	if [[ $a_variation =~ $regexp  ]] ; then
            	echo "User requested a sleep after ha failover specified by SVT_AT_VARIATION_FAILOVER_SLEEP="
            	sleep_count=${BASH_REMATCH[3]}
                test_template_add_test_sleep $sleep_count ; 
			fi

            else 
            echo "No ha testing scheduled at this time (B)"
        fi

		regexp="SVT_AT_VARIATION_RESUME=true"
        if [[ $a_variation =~ $regexp ]] ; then
            echo "-----------------------------------------"
            echo "-----------------------------------------"
            echo "Do RESUME running test: resume_test_prefix=${resume_test_prefix} "
            echo "-----------------------------------------"
            echo "-----------------------------------------"
	    	regexp="SVT_AT_VARIATION_NOW="
            if ! [[ $a_variation =~ $regexp ]] ; then
				echo "ERROR: do not try to use SVT_AT_VARIATION_RESUME=true without specifiying the SVT_AT_VARIATION_NOW flag"
				exit 1;
            fi 
	    	new_variation="$a_variation|"
	    	new_variation+="SVT_AT_VARIATION_SYNC_CLIENTS=false|"
	    	new_variation+="SVT_AT_VARIATION_SKIP_SUBSCRIBE_LAUNCH=true|"
	    	new_variation+="SVT_AT_VARIATION_SKIP_PUBLISH_LAUNCH=true|"
	    	new_variation+="SVT_AT_VARIATION_RESUME_PREFIX=$resume_test_prefix|"
            if [ -n "$SVT_LAUNCH_SCALE_TEST_DESCRIPTION" ] ;then
                description="${SVT_LAUNCH_SCALE_TEST_DESCRIPTION} - resume workload variation: ${new_variation}"
        else
                description="${a_name} - resume workload variation: ${new_variation} "
        fi

            test_template_add_test_single "$description" cAppDriver m1 "${a_command}" "${a_timeout}" "${new_variation}|SVT_AT_VARIATION_ALL_M=true" "TEST_RESULT: SUCCESS"
        fi

        regexp="SVT_AT_VARIATION_TEST_ONLY=true"
        if ! [[ $a_variation =~ $regexp ]] ; then
            echo "-----------------------------------------"
            echo "-----------------------------------------"
            echo "Do cleanup"
            echo "-----------------------------------------"
            echo "-----------------------------------------"
            test_template_add_test_all_M_concurrent  "${a_name} - cleanup" "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|cleanup|${workload}" 600 SVT_AUTOMATION_TEST_LOG_TAR=$a_test_bucket ""
        fi

        regexp="SVT_AT_VARIATION_CLEANSESSION=false"
        if [[ $a_variation =~ $regexp ]] ; then
            regexp="SVT_AT_VARIATION_TEST_ONLY=true"
            if ! [[ $a_variation =~ $regexp ]] ; then
                echo "-----------------------------------------"
                echo "-----------------------------------------"
                echo "Cleanup - Automatically inserting another test to remove the durable subscriptions that were created by last test."
                echo "TODO: if you need to override this behavior add another setting that can be checked"
                echo "-----------------------------------------"
                echo "-----------------------------------------"
                test_template_add_test_single  "${a_name} - performing automatic cleanup for durable subscriptions from previous variation. "  cAppDriver m1 "${a_command}" "${a_timeout}" "${a_variation}|SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_MSGCNT=0|SVT_AT_VARIATION_CLEANSESSION=true|SVT_AT_VARIATION_SYNC_CLIENTS=false" "TEST_RESULT: SUCCESS"
            fi
        fi
    
        regexp="SVT_AT_VARIATION_SINGLE_LOOP=true"
        if [[ $a_variation =~ $regexp ]] ; then
            echo "-----------------------------------------"
            echo "-----------------------------------------"
            echo "Breaking after single loop since SVT_AT_VARIATION_SINGLE_LOOP=true"
            echo "-----------------------------------------"
            echo "-----------------------------------------"
            break;
        fi

        let j=$j+$SVT_COMMON_TEST_ARRAY_COLS
    done
}


svt_common_softlayer_init(){
    if [ "${A1_HOST_OS:0:4}" == "CCI_" ] ;then
        test_template_add_test_single "${scenario_set_name} - Setup for softlayer" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|softlayer_setup|na|1|${A1_IPv4_1}|${A2_IPv4_1}" 600 "" "TEST_RESULT: SUCCESS"
    fi
}

svt_common_email_notify() {
    local appliance=${1-"${A1_HOST}"}
    #---------------------------------------------------------
    # One time setup for all subsequent tests. (Notification that the test has started )
    #---------------------------------------------------------
    test_template_add_test_single "${scenario_set_name} - notification" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|email_notifcation|na|1|${appliance}" 600 "" "TEST_RESULT: SUCCESS"
}

svt_common_setup(){
    local appliance=${1-"${A1_IPv4_1}"}

    #---------------------------------------------------------
    # One time setup for all subsequent tests. (Virtual Nic setup on server)
    #---------------------------------------------------------
    test_template_add_test_single "${scenario_set_name} - setup server for ${appliance}" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|SERVER|1|${appliance}" 600 "SVT_ADMIN_IP=$A1_HOST" "TEST_RESULT: SUCCESS"

    #---------------------------------------------------------
    # One time setup for all subsequent tests. (sysctl, Virtual Nic setup on client)
    #---------------------------------------------------------

    test_template_add_test_single "${scenario_set_name} - setup client for ${appliance}" cAppDriver m1  "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|connections_setup|CLIENT|1|${appliance}" 600 "SVT_AT_VARIATION_ALL_M=true|SVT_AT_VARIATION_QUICK_EXIT=true|SVT_ADMIN_IP=$A1_HOST" "TEST_RESULT: SUCCESS"

}
svt_common_init_test () {
	local my_scenario_number=${1-99}
	export SVT_AT_SCENARIO=50
	export SVT_AT_SCENARIO_8=`echo $SVT_AT_SCENARIO | awk '{printf( "%08d", $1);}'`
	typeset -i n=0
	test_template_set_prefix "cmqtt_${SVT_AT_SCENARIO}_"
	svt_common_softlayer_init
	svt_common_email_notify
	svt_common_setup ${A1_IPv4_1}

}

svt_common_init_ha_test () {
	local my_scenario_number=${1-99}

	svt_common_init_test $my_scenario_number
	
   	if (($A_COUNT>1)) ; then # don't try to do HA testing unless A_COUNT reports at least 2 appliances.
      		svt_common_setup ${A2_IPv4_1} # make sure backup is setup too.
      		test_template_add_test_single "${scenario_set_name} - SVT - setup HA" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|setupHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "TEST_RESULT: SUCCESS"
	
   	else
		echo "ERROR: specified to setup for HA but A_COUNT: $A_COUNT is not > 1"
		exit 1;
   	fi

}
