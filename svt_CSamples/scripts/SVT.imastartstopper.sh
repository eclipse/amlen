#!/bin/bash

ima=10.10.10.10

while((1)); do
	data=`ssh admin@$ima "advanced-pd-options list"`
	regex="bt"
	if [[ $data =~ $regex ]] ;then
		echo "`date` Detected bt in advanced-pd-options list"
		exit 0;

	fi

	data=`ssh admin@$ima "status imaserver"`
	
	regex="Running"
	if [[ $data =~ $regex ]] ;then
		echo "`date` Detected Running - sleep 81800, then issue stop"	
		curtime=`date +%s`
		let endtime=($curtime+81800);
		echo "`date` currtime is $curtime , end time is $endtime"
		lastdata=""
		while(($curtime<$endtime)); do
			data=`ssh admin@$ima "imaserver stat" | grep MsgRead | awk '{ print $3}'`
			echo "`date` current data is $data"		
			if [ "$lastdata" == "" ] ; then
				lastdata=$data;
				echo "`date` $lastdata"
			else
				echo "`date` data is $data, lastdata is $lastdata"
				let delta=($data-$lastdata);
				echo "`date` delta of MsgRead is $delta"
				if (($delta>10000)); then
					echo "`date` this looks like a period of high activity, issue stop now"
					break;
				fi
			fi
			
		    sleep 1;	
			curtime=`date +%s`
		done
		echo "`date` issue imaserver stop"
		ssh admin@$ima "imaserver stop"	

	    while [[ $data =~ $regex ]] ; do
		    echo "`date` now sleep 5 -wait for the state to no longer be Running."
            sleep 5;
	        data=`ssh admin@$ima "status imaserver"`
        done

		continue;	
	fi

	regex="Stopped"
	if [[ $data =~ $regex ]] ;then
		echo "`date` Detected Stopped - issue imaserver start "
		ssh admin@$ima "imaserver start"	
	    while [[ $data =~ $regex ]] ; do
		    echo "`date` now sleep 5 -wait for the state to no longer be Stopped."
            sleep 5;
	        data=`ssh admin@$ima "status imaserver"`
        done
	    regex="StoreStarting"
	    while [[ $data =~ $regex ]] ; do
		    echo "`date` now sleep 5 -wait for the state to no longer be StoreStarting"
            sleep 10;
	        data=`ssh admin@$ima "status imaserver"`
        done
	    regex="StoreStarted"
	    while [[ $data =~ $regex ]] ; do
		    echo "`date` now sleep 5 -wait for the state to no longer be StoreStarted"
            sleep 10;
	        data=`ssh admin@$ima "status imaserver"`
        done
		echo "`date` sleep 120 before continuing the loop"
		echo "`date` sleep 120 before continuing the loop"
        sleep 120
		continue;	
	fi

	echo "`date` non-essential status; $data "
	echo "`date` now sleep 15 before continuing"
	sleep 15

done

