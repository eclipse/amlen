#!/bin/sh

function envvars {
  echo "Required env vars for client: IMAServer, IMAPort, IMAServerName, MACH_NIC, CLNT_CPU_LIST"
  echo "Required env vars for server: MACH_NIC, SRVR_CPU_LIST, SRVR_THREADS"
  echo "General env vars: CLEAN (CLEAN=1; means cleanup past result files)"
}

if [ "$IMAServer" == "" ] ; then
  echo "The env variable IMAServer=$IMAServer , this env variable is required.  It is a space separated list of destination IP addresses. Same length as IMAPort"
  envvars
  exit 1
fi

if [ "$IMAPort" == "" ] ; then
  echo "The env variable IMAPort=$IMAPort , this env variable is required.  It is a space separated list of destination ports. Same length as IMAServer."
  envvars
  exit 1
fi

if [ "$IMAServerName" == "" ] ; then
  echo "The env variable IMAServerName=$IMAServerName , this env variable is required.  It is the hostname of the system that will run the server program."
  envvars
  exit 1
fi

if [ "$MACH_NIC" == "" ] ; then
  echo "The env variable MACH_NIC=$MACH_NIC , this env variable is required.  It is a space separated list of destination IP addresses"
  envvars
  exit 1
fi

if [ "$CLNT_CPU_LIST" == "" ] ; then
  echo "The env variable CLNT_CPU_LIST=$CLNT_CPU_LIST , this env variable is required.  It is a list of CPU(s) for client affinity"
  envvars
  exit 1
fi

if [ "$SRVR_CPU_LIST" == "" ] ; then
  echo "The env variable SRVR_CPU_LIST=$SRVR_CPU_LIST , this env variable is required.  It is a list of CPU(s) for server affinity"
  envvars
  exit 1
fi

if [ "$1" == "" ] ; then
  echo "usage: $0 <netperf | sockperf | sfnt-pingpong>"
  exit 1
else 
  case $1 in 
	netperf|sockperf|sfnt-pingpong)
		TEST=$1 ;;
	*)
		echo "usage: $0 <netperf | sockperf | sfnt-pingpong>"
		exit 1	;;
  esac
fi

echo "Running TEST=$TEST..."

# Run ethtool on local client system and remote server system to tune NICs
# before running the test
REMOTE_NIC=`ssh $IMAServerName ". ./perfenv.sh ; if [ '\$MACH_NIC' == '' ] ; then echo 0 ; else echo 1 ; fi"`
if [ "$REMOTE_NIC" == "0" ] ; then
  echo "The env variable $MACH_NIC is not set in ~/perfenv.sh on $IMAServerName. Please set this env var to the remote NIC(s) to be used in this test"
  exit 1
fi

#Tune local NIC(s)
$PERFTOOLSHOME/ethtool.sh
#Tune remote NIC(s)
ssh $IMAServerName ". ~/perfenv.sh ; $PERFTOOLSHOME/ethtool.sh"

if [ "$TEST" == "netperf" ] ; then
  if [ "$CLEAN" == "1" ] ; then
    rm -f np_*.csv
  fi
  # cleanup any old remote netserver instances
  ssh $IMAServerName "pkill -9 netserver"

  # netserver only takes one port number and one dest IP
  firstport=`echo $IMAPort | cut -d' ' -f1`
  firstdst=`echo $IMAServer | cut -d' ' -f1`

  # start the netperf server
  ssh $IMAServerName "taskset -c $SRVR_CPU_LIST netserver -p $firstport"
  
  # run a series of netperf request/response tests
  datafile=np_$(date +%Y%m%d-%T).csv

  taskset -c $CLNT_CPU_LIST netperf -H $firstdst -t TCP_RR -p $firstport -j -- -r 32 -s 32768,8388608 -S 32768,8388608 -D L,R -o stats_basic.csv_template | tail -n 1 >> $datafile
  taskset -c $CLNT_CPU_LIST netperf -H $firstdst -t TCP_RR -p $firstport -j -- -r 128 -s 32768,8388608 -S 32768,8388608 -D L,R -o stats_basic.csv_template | tail -n 1 >> $datafile
  taskset -c $CLNT_CPU_LIST netperf -H $firstdst -t TCP_RR -p $firstport -j -- -r 512 -s 32768,8388608 -S 32768,8388608 -D L,R -o stats_basic.csv_template | tail -n 1 >> $datafile
  taskset -c $CLNT_CPU_LIST netperf -H $firstdst -t TCP_RR -p $firstport -j -- -r 2048 -s 32768,8388608 -S 32768,8388608 -D L,R -o stats_basic.csv_template | tail -n 1 >> $datafile
  taskset -c $CLNT_CPU_LIST netperf -H $firstdst -t TCP_RR -p $firstport -j -- -r 8192 -s 32768,8388608 -S 32768,8388608 -D L,R -o stats_basic.csv_template | tail -n 1 >> $datafile
  taskset -c $CLNT_CPU_LIST netperf -H $firstdst -t TCP_RR -p $firstport -j -- -r 32768 -s 32768,8388608 -S 32768,8388608 -D L,R -o stats_basic.csv_template | tail -n 1 >> $datafile

## Onload kernel bypass
# Tune onload for low latency  (use profile instead)
#export EF_POLL_USEC=100000
#export EF_TX_PUSH=1
#echo "starting offloaded netperf test..."
#onload taskset -c 1 netserver -L 10.10.13.1 -p 12346

  # cleanup any old remote netserver instances
  ssh $IMAServerName "pkill -9 netserver"
fi

if [ "$TEST" == "sfnt-pingpong" ] ; then
  echo "Needs to be reworked..."
fi

if [ "$TEST" == "sockperf" ] ; then
  if [ "$SRVR_THREADS" == "" ] ; then
    echo "SRVR_THREADS is a required env variable for the sockperf test. For example, export SRVR_THREADS=5"
    exit 1
  fi

  if [[ "$SRVR_CPU_LIST" == *-* ]] ; then
    echo "SRVR_CPU_LIST=$SRVR_CPU_LIST ; sockperf does not allow the '-' character, use comma separated list instead"
    exit 1
  fi

  if [[ "$CLNT_CPU_LIST" == *-* ]] ; then
    echo "CLNT_CPU_LIST=$CLNT_CPU_LIST ; sockperf does not allow the '-' character, use comma separated list instead"
    exit 1
  fi

  if [ "$CLEAN" == "1" ] ; then
    rm -f sp_*.txt
  fi

  # cleanup any old remote sockperf instances
  ssh $IMAServerName "pkill -9 sockperf >/dev/null"

  # configure the port file
  echo > sockperf.txt
  list_clnt_cpus=`echo $CLNT_CPU_LIST | tr , ' '`
  array_clnt_cpus=($list_clnt_cpus)
  ports=($IMAPort)
  ip=($IMAServer)
  len1=`echo $IMAPort | wc -w`
  len2=`echo $IMAServer | wc -w`
  len3=`echo $list_clnt_cpus | wc -w`
  if [ "$len1" != "$len2" ] ; then
    echo "length of IMAPort($IMAPort) list must be equal to length of IMAServer($IMAServer) list"
    exit 1
  fi
  if [ $len1 -lt  $SRVR_THREADS ] ; then
    echo "length of IMAPort($IMAPort) and length of IMAServer($IMAServer) list must be greater than or equal to SRVR_THREADS($SRVR_THREADS)"
    exit 1
  fi

  for i in $(seq 0 $((len1 - 1)))
  do
    echo "T:${ip[$i]}:${ports[$i]}" >> sockperf.txt
  done
  
  # start the sockperf server
  scp sockperf.txt $IMAServerName:~ >/dev/null
  ssh $IMAServerName "sockperf server --threads-num=$SRVR_THREADS -F e -f sockperf.txt --cpu-affinity=$SRVR_CPU_LIST --nonblocked --tcp-skip-blocking-send >/dev/null" &

  # give sockperf server a chance to startup
  sleep 3

  # run throughput tests
  msgs="32 128 512 2048 8192 32768"
  for m in $msgs
  do
    echo "running throughput test ($m byte messages)..."
    datafile=sp_tput_$(date +%Y%m%d-%T)_${m}byte.txt
    stdout=sp_stdout.txt
    aggrfile=sp_aggr.txt
    # start clients
    for i in $(seq 0 $((len3 - 1)))
    do
      #echo "i=$i, len3=$len3, array_clnt_cpus[$i]=${array_clnt_cpus[$i]}, i%len3=$((i % len3))"
      #taskset -c ${array_clnt_cpus[$((i % len3))]} sockperf throughput --tcp -i ${ip[$i]} -p ${ports[$i]} -m $m -t 10 >stdout.txt 2>stderr.txt &
      taskset -c ${array_clnt_cpus[$((i % len3))]} sockperf throughput -f sockperf.txt -m $m -t 10 >>$stdout &
      lastpid=$!
      sleep 0.5
    done
    wait $lastpid
    grep "Message Rate" $stdout > $aggrfile
    sum=0
    while read line 
    do
      count=`echo $line | cut -d' ' -f6`
      sum=$((sum + count))
    done < $aggrfile
    echo "Total message rate=$sum [msgs/sec]" >> $stdout
    mv $stdout $datafile
    rm -f $aggrfile
  done

  # run ping-pong tests
  msgs="32 128 512 2048 8192 32768"
  for m in $msgs
  do
    echo "running ping-pong test ($m byte messages)..."
    datafile=sp_pp_$(date +%Y%m%d-%T)_${m}byte.txt
    stdout=sp_stdout.txt
    aggrfile=sp_aggr.txt
    # start clients
    for i in $(seq 0 $((len3 - 1)))
    do
      #echo "i=$i, len3=$len3, array_clnt_cpus[$i]=${array_clnt_cpus[$i]}, i%len3=$((i % len3))"
      #taskset -c ${array_clnt_cpus[$((i % len3))]} sockperf throughput --tcp -i ${ip[$i]} -p ${ports[$i]} -m $m -t 10 >stdout.txt 2>stderr.txt &
      taskset -c ${array_clnt_cpus[$((i % len3))]} sockperf ping-pong -f sockperf.txt -m $m -t 10 >>$stdout &
      lastpid=$!
      sleep 0.5
    done
    wait $lastpid
#    grep "Message Rate" $stdout > $aggrfile
#    sum=0
#    while read line
#    do
#      count=`echo $line | cut -d' ' -f6`
#      sum=$((sum + count))
#    done < $aggrfile
#    echo "Total message rate=$sum [msgs/sec]" >> $aggrfile
#    mv $aggrfile $datafile
#    rm -f $stdout
     mv $stdout $datafile
  done


  ## cleanup
  ssh $IMAServerName "rm -f sockperf.txt"
  rm -f sockperf.txt
 
  # cleanup any old remote sockperf instances
  ssh $IMAServerName "pkill -9 sockperf" 
fi
















