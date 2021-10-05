. ./ha_test.sh

ima1=10.10.10.10
ima2=10.10.10.10
 let iteration=0; 
while((1)); do 
echo "`date` start iteration $iteration"; 

#echo "`date` inject UNSYNC by stopping both"
#ssh admin@$ima1 imaserver stop
#ssh admin@$ima2 imaserver stop
#
#sleep 60
#echo "`date` inject UNSYNC by starting both"
#ssh admin@$ima1 imaserver start
#ssh admin@$ima2 imaserver start
#
#sleep 60
#
#echo "`date` show status"
#ssh admin@$ima1 status imaserver 
#ssh admin@$ima2 status imaserver 
#
echo "`date` do cleanup"
ha_pair_cleanup_b

echo "`date` Sleep 120"
sleep 120

echo "`date` start test "
./ismAutoMQTT_C_Sample.sh 

mypid=$!

echo "`date` sleep 1800"

sleep 1800

echo "`date` killprocess $mypid"

kill -9 $mypid

./SVT.cleanup.sh


echo `date` end iteration $iteration; 



let iteration=$iteration+1; 
done


