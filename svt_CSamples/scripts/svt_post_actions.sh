#!/bin/bash 

. ./svt_test.sh

#declare -a plot_actions=(
#producers.dat producers.rate.png "cmqtt*p.*log" "Producer Msg Rate" "Time (seconds)" "msgs/sec"
#consumers.dat consumers.rate.png "cmqtt*s.*log" "Consumer Msg Rate" "Time (seconds)" "msgs/sec"
#producers.dat producers.msgs.png "cmqtt*p.*log" "Producer Msg Total" "Time (seconds)" "msgs/sec"
#consumers.dat consumers.msgs.png "cmqtt*s.*log" "Consumer Msg Total" "Time (seconds)" "msgs/sec"

procs=`cat /proc/cpuinfo  |grep processor |wc -l`

for mydir in 03 ; do
    outdata=${mydir}/producers.${SVTPP}.dat
    outimg=${mydir}/producers.${SVTPP}.png
    out_title="Producer Msg Rate"
    file_pattern="$mydir/cmqtt*p.*log"
    times=`grep -h SVT_MQTT_C_STATUS, $file_pattern | grep -v SVT_MQTT_C_STATUS,0.000000,0,0 | cut -b-19|  sort -u`
    #times_count=`echo "$times" | wc -l`
    #outfile=""
    #let lineno=0; 
    #for v in {1..$procs} ;do 
        #while 
            #if (($times_count/4));     then
                #if (($lineno!=0)); then
                    #outfile=$outdata.$v
                    #break;
                #fi
            #else
            #fi
            #let lineno=$lineno+1;
        #done
    #done
    
    
    #echo "$times" | xargs -i echo "echo -n \"{} \" ; grep \"{}\" $file_pattern | grep -oE 'SVT_MQTT_C_STATUS,.*' | awk -F ',' '{print \$2}' | ./pstat.sh " |sh  | awk '{print $2" "$8}' > $outdata

    bulk_grep_init
    echo "$times" | xargs -i echo "echo -n \"{} \" ; . ./svt_test.sh; bulk_grep \"{}\" \"$file_pattern\" | grep -oE 'SVT_MQTT_C_STATUS,.*' | awk -F ',' '{print \$2}' | ./pstat.sh " |sh  | awk '{print $2" "$8}' > $outdata

    do_xyHMS_plot "$outdata" "$outimg" "$out_title" "Time (seconds)" "msgs/sec" 


    outdata=${mydir}/consumers.${SVTPP}.dat
    outimg=${mydir}/consumers.${SVTPP}.png
    out_title="Consumer Msg Rate"
    file_pattern="$mydir/cmqtt*s.*log"
    times=`grep -h SVT_MQTT_C_STATUS, $file_pattern | grep -v SVT_MQTT_C_STATUS,0.000000,0,0 | cut -b-19|  sort -u`
    #echo "$times" | xargs -i echo "echo -n \"{} \" ; grep \"{}\" $file_pattern | grep -oE 'SVT_MQTT_C_STATUS,.*' | awk -F ',' '{print \$2}' | ./pstat.sh " |sh  | awk '{print $2" "$8}' > $outdata

    bulk_grep_init
    echo "$times" | xargs -i echo "echo -n \"{} \" ; . ./svt_test.sh; bulk_grep \"{}\" \"$file_pattern\" | grep -oE 'SVT_MQTT_C_STATUS,.*' | awk -F ',' '{print \$2}' | ./pstat.sh " |sh  | awk '{print $2" "$8}' > $outdata
    do_xyHMS_plot "$outdata" "$outimg" "$out_title" "Time (seconds)" "msgs/sec" 


    #2013-04-16 05:22:14.499051 SVT_MQTT_C_STATUS,0.000000,0,1664,2013-04-16 05:22:12.335247,555,555,0,0.100442,0.738274,74.699066,(actual msg rate,msgs since last interval,total msgs,lastmsgtime, connection attempts,success,fail,connection time min,max,total)

    ./monitor.c.client.sh   "${mydir}/cmqtt_03_*.p.0.connections.*" > ${mydir}/summary.producers.${SVTPP}.dat
    ./monitor.c.client.sh   "${mydir}/cmqtt_03_*.s.0.connections.*" > ${mydir}/summary.consumers.${SVTPP}.dat

done
