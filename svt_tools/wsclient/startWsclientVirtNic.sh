#!/bin/bash
#----------------------------------------------------------------------------------------
#
#----------------------------------------------------------------------------------------
function printUsage() {
set +x
  echo "---------------------- "
  echo " $0 : create ifconfig definitions and network-scripts files for Virtual Network Interfaces "
  echo ""
  echo "Syntax:"
  echo "  $0  -e[eth_adapter_number]  -n[number_of_virtual_adapters]  -s[eth_stem]  -3[third_segemnt_of_IP] -4[fourth_segment_of_IP]  -m[fourth_segment_MAX]"
  echo "    where,"
  echo " eth_adapter_number  -  'ifconfig' adapter number, ex. eth0 would be -e0."
  echo "    Default is ${whichNic}"
  echo " number_of_virtual_adapters  -  number of virtual adapters to make on 'eth_adapter_number'"
  echo "    Default is ${numNic}"
  echo " eth_stem  --  First two numbers of the IP address,"
  echo "    Default=${ethStem}"
  echo " third_segemnt_of_IP  -  10.10.x.y : where you want 'x' to start,"
  echo "    Default=${eth3Start}"
  echo " fourth_segment_of_IP  -  10.10.x.y : where you want 'y' to start,"
  echo "    Default=${eth4Start} "
  echo " fourth_segment_MAX  -  10.10.x.y : where you want 'y' to stop and repeat,"
  echo "    Default=${eth4Max}"
  echo " "
  exit 1
}

#----------------------------------------------------------------------------------------
function parseInputs() {
numNic=5
numClients=10
whichNic=1
ethStem="10.4."
eth3Start=1
eth4Start=2
eth4Max=255
	while getopts "c:e:n:s:3:4:m:h" option ${OPTIONS}
	do
		case ${option} in
		c )
			numClients=${OPTARG};;

		e )
			whichNic=${OPTARG};;

		n )
			numNic=${OPTARG};;

		s )
                     theArgs=${OPTARG}
			lastChar=`echo ${OPTARG} | cut -c ${#theArgs}`
                     if [ "${lastChar}" == "." ]
                     then
                       ethStem=${OPTARG}
                      else
                       ethStem=${OPTARG}.
                      fi;;

		3 )
			eth3start=${OPTARG};;

		4 )
			eth4start=${OPTARG};;

		m )
			eth4Max=${OPTARG};;

		h )
			printUsage;;
		* )
			echo "Unknown option: ${option}"
			printUsage;;
		esac	
	done

}

#----------------------------------------------------------------------------------------
# MAIN
#----------------------------------------------------------------------------------------
#set -x
OPTIONS="$@"
parseInputs

eth3=${eth3Start}
eth4=${eth4Start}

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../lib
wsclient="/demo/application/server_ship/bin/wsclient"
APPLIANCE_IP="10.10.1.61"
APPLIANCE_IP="10.10.10.10"
#APPLIANCE_IP="10.4.1.47"
APPLIANCE_PORT="16103"


iNic=1
iClient=1
#new parameter #clients - which may wrap when it exceeds numNic's - change while for numClients?

while [ "${iClient}" -le "${numClients}" ]
do

  if [ ${eth4} -gt ${eth4Max} ]
  then
    eth3=`expr ${eth3} + 1 `
    eth4=${eth4Start}
  fi

# for round robin assignment of wsclient to the available Virtual Nics 
if [ ${iNic} -gt ${numNic} ]
then
  eth3=${eth3Start}
  eth4=${eth4Start}
  iNic=1
fi
  

#new parameter #clients - which may wrap when it exceeds numNic's


set -x
  `${wsclient} -a${APPLIANCE_IP} -o${APPLIANCE_PORT} -l ${ethStem}${eth3}.${eth4} -n300 -p1 -s1 -T2 -M32 2> ${wsclient}.${iNic}.log  1> ${1} `
set +x

  iClient=`expr ${iClient} + 1 `
  iNic=`expr ${iNic} + 1 `
  eth4=`expr ${eth4} + 1 `

done
