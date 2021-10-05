# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
GET_SED_FILES () {
   `rm -fr cmos/* `
   `unzip cmos.zip > unzip.cmos.zip.log `
   FILES_192=`grep -l "192\.168" cmos/ims_* `
   FILES_1010=`grep -l "10.10" cmos/ims_* `
   SED_FILES="${FILES_192}  ${FILES_1010} "
   echo "FILES TO SED: ${SED_FILES}"

## SHOULD BE TEMPORARY until CSC starts using ipmi-watch.sh
   if [ ! -f "cmos/ipmi-watch.sh"  ];then
      `cp "ipmi-watch.sh" "cmos/"`
   fi

   `sed -i.tmp -e 's|ssh admin\@192.168.40.\$ip9  \"device RESET noprompt\"|\.\/cmos\/ipmi-watch.sh  \${A1_IMM}; RC=\$?; if [ \"\${RC}\" != \"0\" ]; then echo \"ERROR:  \$0 FAILED with RC=\${RC} running ipmi-watch.sh\"; exit \${RC}; fi; |g'  cmos/ims_clean `  

## Workaround the meesage loss when last msg received on 'current' adapter was really sent from 'next' adapter 
## because the subscriber application terminates before the received the msg from the first publish of 100 msgs.
   if [ "${ISM_AUTOMATION}" == "TRUE" ] ; then
      echo "SED cmos/ims_java for FRANK'S automation environment..."
      `sed -i.tmp -e 's|#####Publish message to the Appliance######|wait; #####Publish message to the Appliance######|g '  -e 's|:16102  -i subClient -b -v -n 1 \&|:16102  -i subClient -b -v -n 1 \& \n sleep 5; |g'  -e 's|\${M1_IMA_SDK}/lib/|\${M1_IMA_SDK}/ImaClient/jms/lib/|g'  cmos/ims_java `
   else
      echo "SED cmos/ims_java for 'private' automation environment..., not Frank's "
      `sed -i.tmp -e 's|#####Publish message to the Appliance######|wait; #####Publish message to the Appliance######|g '  -e 's|:16102  -i subClient -b -v -n 1 \&|:16102  -i subClient -b -v -n 1 \& \n sleep 5; |g'  cmos/ims_java `
   fi

 
# Required remapping of CSC IPs to Austin IPs
   `sed -i.tmp -e 's|10.10.10.\$ip3|\${A1_IMM}|g' -e 's|10.10.10.\$ip9|\${A1_IMM}|g' -e 's|10.10.10.\$ipx|\${A1_IMM}|g' -e 's|10.10.10.\$1|\${A1_IMM}|g' -e 's|10.10.10.\$direccion|\${A1_IMM}|g'  -e 's|192.168.40.\$ip3|\${A1_HOST}|g'  -e 's|192.168.40.\$ip9|\${A1_HOST}|g'  -e 's|192.168.40.\$@|\${A1_HOST}|g'  -e 's|192.168.40.\$direccion|\${A1_HOST}|g'  -e 's|10.10.1.\$ip3|\${A1_mgt1}|g'  -e 's|10.0.1.\$ip3|\${A1_eth0}|g'  -e 's|10.1.1.\$ip3|\${A1_eth1}|g'  -e 's|10.2.1.\$ip3|\${A1_eth2}|g'  -e 's|10.3.1.\$ip3|\${A1_eth3}|g'  -e 's|10.4.1.\$ip3|\${A1_eth4}|g'  -e 's|10.5.1.\$ip3|\${A1_eth5}|g'  -e 's|10.6.1.\$ip3|\${A1_eth6}|g'  -e 's|10.7.1.\$ip3|\${A1_eth7}|g'  -e 's|10.8.1.\$ip3|\${A1_ha0}|g'  -e 's|10.9.1.\$ip3|\${A1_ha1}|g' -e 's|/niagara/ImaClient/jms|\${M1_IMA_SDK}|g' -e 's|ping -c 1 -w 1|ping -c 1 |g' -e 's|/cmos/nvDIMMtest.sh|cmos/nvDIMMtest.sh|g'  -e 's/immdefault/#immdefault/g'  -e 's/#immdefault()/immdefault()/g'  ${SED_FILES} `

   # ONLY DO This sed in cmos/ima_clean
   `sed -i.tmp -e 's|read ip9|ip9=\${1}|g' cmos/ims_clean `

}


###   MAIN   ###

if [ $# -lt 1 ]; then
   echo "What is the IP of the Appliance to be tested?"
   read  A1_HOST
else
   A1_HOST=$1
fi 

if [ "$2" == "TRUE" ]; then
   . ./austin_network_shortcut.sh  ${A1_HOST}
   ISM_AUTOMATION=$2
else
   . ./austin_network.sh  ${A1_HOST}
fi

#Verify expected variables for future execution
if [ "${M1_IMA_SDK}" == "" ]; then
   export M1_IMA_SDK="/niagara/application/client_ship"
   echo "  ###=== M1_IMA_SDK was not 'sourced' - set to automation default: ${M1_IMA_SDK} "
fi
echo " M1_IMA_SDK=${M1_IMA_SDK}"
echo " A1_ HOST=${A1_HOST}, MGT1=${A1_mgt1}, HA0=${A1_ha0}, HA1=${A1_ha1} , IMM=${A1_IMM} "
echo " ETH0=${A1_eth0}, ETH1=${A1_eth1}, ETH2=${A1_eth2}, ETH3=${A1_eth3}, ETH4=${A1_eth4}, ETH5=${A1_eth5}, ETH6=${A1_eth6}, ETH7=${A1_eth7} " 
echo " IP_F4=${IP_F4}"
`rm   -fr /var/www/html/evidence`
`mkdir -p /var/www/html/evidence`

SERIAL=`/opt/ibm/toolscenter/asu/asu64 show  all --host ${A1_IMM} | grep KQ | head -1 | cut -c 35-41`

declare -a SED_FILES
GET_SED_FILES;

#set -x

declare -i  RC_ALL=0
declare -i  RC=0
declare -a EXEC_FILES=( "cmos/ims_bootordercheck" "cmos/ims_java"  "cmos/ims_pre_nvdimm" "cmos/ims_raid" "cmos/ims_clean"  )

#declare -a EXEC_FILES=( "cmos/ims_bootordercheck" "cmos/ims_java"  "cmos/ims_pre_nvdimm" "cmos/ims_raid"                  )

ssh admin@${A1_HOST} "imaserver update Endpoint Name=DemoEndpoint Enabled=true"

for FILE in ${EXEC_FILES[@]}
do
   #Start Execution of the CSC Mfg Process ${FILES}
#set -x
   if [[ "${FILE}" == *"ims_raid"*  || "${FILE}" == *"ims_clean"*  ]]; then
      echo "Y" | ${FILE} ${IP_F4} 

   elif [[ "${FILE}" == *"ims_bootordercheck"* ]]; then
      `echo ${IP_F4} | ${FILE} > csc-2-austin.log`

   else
      echo ${IP_F4} | ${FILE} 

   fi

   # Validation routines since RtnCode check is unreliable 
   if [[ "${FILE}" == *"ims_bootordercheck"* ]]; then
      FINDIT=`grep "UEFI Verification PASSED" csc-2-austin.log |wc -l`
      
   elif  [[ "${FILE}" == *"ims_java"* ]]; then
      FINDIT=`grep "Client subClient received TextMessage 100 on topic" /var/www/html/evidence/java_${SERIAL}.txt | wc -l` 

   elif  [[ "${FILE}" == *"ims_pre_nvdimm"* ]]; then
      FINDIT=`grep "Total PASSED: 4 FAILED: 0" /var/www/html/evidence/nvdimm_${SERIAL}.txt | wc -l`

   elif  [[ "${FILE}" == *"ims_raid"* ]]; then
      FINDIT=`grep "RAID Status:  All HDDs are formatted." /var/www/html/evidence/raid_${SERIAL}.txt | wc -l`

   elif  [[ "${FILE}" == *"ims_clean"* ]]; then
      FINDIT=`grep "device RESET noprompt" /var/www/html/evidence/clean_${SERIAL}.txt |wc -l`

   else
      echo " ERROR:  Added a file, ${FILE}, in EXEC_FILES but did not add a validation check"
      RC_ALL=69
      break
   fi


   if [ "${FINDIT}" -ge "1" ]; then
      echo "  %==>  "
      echo "  %==>  SUCCESS running  ${FILE}"
      echo "  %==>  "
   else
      echo "  %==>  "
      echo "  %==>  FAILURE running  ${FILE}"
      echo "  %==>  "
      ((RC_ALL+=1))
   fi
      
#set +x
done


# Final Test to verify 'device RESET' and Serial are set, have to clear known_host to avoid ManInMiddle msg
rm -f /root/.ssh/known_hosts
ssh $A1_USER@$A1_HOST exit
rebootForAustin.sh $A1_IMM $A1_HOST /22 $TEST_PW
RC=$?
if [ "${RC}" != "0" ]; then
   echo "ERROR:  $0 FAILED with RC=${RC} running the final rebootForAustin.sh";
   ((RC_ALL+=RC))
fi;

#set -x
if [ "${RC_ALL}" != "0" ]; then
   echo "  %%====>  "
   echo "  %%====>  FAIL:  A CSC test case failed. Also Check logs at /var/www/html/evidence. "
   echo "  %%====>  "
else
   echo "  %%====>  "
   echo "  %%====>  PASS:  ALL CSC test cases were successful"
   echo "  %%====>  "
fi

exit ${RC_ALL}
