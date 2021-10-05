#! /bin/bash +x
declare RC;
if [ $# -ne 4 ]; then
   echo "Inputs are SSH_USER, SERVER_IP, IP to get Interface name of, envVar to set with Intfname value"
   return 99;
fi
USER=$1
HOST=$2
GETIFNAME=$3
SETIFNAME=$4

ifnames=`ssh $USER@$HOST "ifconfig -s| cut -d ' ' -f '1' " `; 
#echo ${ifnames};

for  name  in  ${ifnames} ;
do
    RC=`ssh $USER@$HOST "ifconfig ${name}"  2> x | grep $3 `;
    if [ ${?} -eq 0 ]; then 
       echo ${name};
	   export ${SETIFNAME}=${name};
       break;
    fi;

done;


