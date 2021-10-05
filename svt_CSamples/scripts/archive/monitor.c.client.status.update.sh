#!/bin/bash

ARG1=${1-"CONTINUOUS"}

print_stats(){
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



#INPUT_FILE_PATTERN=${1-"log.*"};

while ((1)); do  
     #echo -n "P:"
     #tail -n 1 log.p.* |grep SVT_MQTT | awk -F ',' '{print $2}'  | print_stats ;
     echo -n "S Rate:"
     data=`find log.s.* -type f |grep -v .mqtt$ | xargs -i  echo "tac {} | grep SVT_MQTT_C_STATUS | head -1" |sh `
	echo "$data" | awk -F ',' '{print $2}'  | print_stats ;
     echo -n "S Total:"
	echo "$data" | awk -F ',' '{print $4}'  | print_stats ;
    date;
	if [ "$ARG1" == "SINGLE" ] ;then break; fi
     sleep 1; 
	
done
