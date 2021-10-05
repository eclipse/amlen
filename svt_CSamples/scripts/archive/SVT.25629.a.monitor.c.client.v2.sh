#!/bin/bash 

ARG1=${1-"CONTINUOUS"}
INPUT_FILE_PATTERN=${2-"\"log.p.*\" \"log.s.*\""};

print_latest_stats(){
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



print_results(){
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
    print "Min: "min," Max: "max " Sum: "sum, " Number of lines: "NR, " Average: "sum/NR " Standard Deviation: "sqrt(sumsq/NR)
    
    }' ;
}

get_output () {
for file_pattern in "log.*.p.*[0-9]" "log.*.s.*[0-9]" ;  do
#for file_pattern in $INPUT_FILE_PATTERN ;  do
    result_available=0;
    files=`find $file_pattern -type f 2>/dev/null |grep -v .mqtt$`
    echo -n "1" >> /dev/stderr

    data=`echo "$files" | xargs -i  echo "grep -oE 'actual_.*' {} " | sh `
    rc=$?;
    echo -n "." >> /dev/stderr
    #echo "$data" > .tmp.data.2	
    echo -n "." >> /dev/stderr
	
    if [ -n "$data" ] ; then
        result_available=1;
        echo -n "$file_pattern actual_rate     :$result_available:"
        echo "$data" | grep actual_rate | awk '{print $2}' | print_results
        echo -n "$file_pattern actual_msgs     :$result_available:"
        echo "$data" | grep actual_msg  | awk '{print $2}' | print_results
    else
        echo    "$file_pattern                 :$result_available: no result data : rc: $rc"; 
    fi

    	data=`echo "$files" | xargs -i  echo "tac {} | grep -oE 'SVT_MQTT_C_STATUS.*' | head -1 " | sh ` 
    	rc=$?;
    	echo -n "2" >> /dev/stderr
    	if [ "$rc" == "0" -a -n "$data" ] ; then
        	echo -n "$file_pattern Connect Attempts:$result_available:"
	    	echo "$data" | awk -F ',' '{print $6}'  | print_latest_stats ;
        	echo -n "$file_pattern Connect Success :$result_available:"
	    	echo "$data" | awk -F ',' '{print $7}'  | print_latest_stats ;
        	echo -n "$file_pattern Connect Failures:$result_available:"
	    	echo "$data" | awk -F ',' '{print $8}'  | print_latest_stats ;
        	echo -n "$file_pattern Connect Min Time:$result_available:"
	    	echo "$data" | awk -F ',' '{print $9}'  | print_latest_stats ;
        	echo -n "$file_pattern Connect Max Time:$result_available:"
	    	echo "$data" | awk -F ',' '{print $10}'  | print_latest_stats ;
        	echo -n "$file_pattern Connect Sum Time:$result_available:"
	    	echo "$data" | awk -F ',' '{print $11}'  | print_latest_stats ;
        	echo -n "$file_pattern Message Rate    :$result_available:"
	    	echo "$data" | awk -F ',' '{print $2}'  | print_latest_stats ;
        	echo -n "$file_pattern Message Total   :$result_available:"
	    	echo "$data" | awk -F ',' '{print $4}'  | print_latest_stats ;
    	else
        	echo    "$file_pattern                 :$result_available:: no data - SVT_MQTT_C_STATUS "; 
    	fi 
	
    echo -n "3" >> /dev/stderr
    warnings=`echo "$files" | xargs -i  echo "grep WARNING {}" | sh | wc -l `
    echo -n "4" >> /dev/stderr
    errors=`echo "$files" | xargs -i  echo "grep ERROR {}" | sh | wc -l `
    echo -n "5" >> /dev/stderr
    echo    "$file_pattern warnings/errors :$result_available: warnings: $warnings errors: $errors"

done
} 

#clear;
#get_output

output=`get_output`

{
clear;
date;
echo "$output"
}
