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
whichNic=0
ethStem="10.10."
eth3Start=1
eth4Start=2
eth4Max=255
	while getopts "e:n:s:3:4:m:h" option ${OPTIONS}
	do
		case ${option} in
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
FILE="/etc/sysconfig/network-scripts/ifcfg-eth${whichNic}"
OUT_FILE="/etc/sysconfig/network-scripts/ifcfg-eth"
STUB_FILE="/etc/sysconfig/network-scripts/ifcfg-eth_VLAN"
rm -f ${STUB_FILE} 
touch ${STUB_FILE}

# Read the /etc/sysconfig/network-scripts/ifcfg-eth${whichNic} to be used to create the ifcfg-eth${iNic} files
while read line 
do
  trimline=`echo ${line}`
  trimline1char=`echo ${trimline} | cut -c 1`

  if [ "${trimline1char}" == "#" ]
  then
    echo ${trimline} >> ${STUB_FILE}
  else
    theParam=`echo ${trimline} | cut -d "=" -f "1"`
    if [ "${theParam}" != "DEVICE"  -a  "${theParam}" != "IPADDR"  -a  "${theParam}" != "GATEWAY" ]
    then
      echo ${trimline} >> ${STUB_FILE}
    fi
  fi

done < ${FILE} 

iNic=1
while [ "${iNic}" -le "${numNic}" ]
do

  if [ ${eth4} -gt ${eth4Max} ]
  then
    eth3=`expr ${eth3} + 1 `
    eth4=${eth4Start}
  fi
set -x
  ifconfig eth${whichNic}:${iNic} ${ethStem}${eth3}.${eth4} netmask 255.255.255.0
set +x
# Complete the ../networking-scripts/ifcfg-eth# file
  rm -f ${FILE}:${iNic}
  cp ${STUB_FILE} ${FILE}:${iNic}
  echo "DEVICE=eth${whichNic}:${iNic}" >> ${FILE}:${iNic}
  echo "IPADDR=${ethStem}${eth3}.${eth4}" >> ${FILE}:${iNic}
  echo "GATEWAY=${ethStem}${eth3}.1" >> ${FILE}:${iNic}
  echo "VLAN=yes" >> ${FILE}:${iNic}

  iNic=`expr ${iNic} + 1 `
  eth4=`expr ${eth4} + 1 `

done

rm -f ${STUB_FILE}
/etc/init.d/network restart
