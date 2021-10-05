touch ./status_${THIS}_${n}.log
ssh admin@${A1_IPv4_1} imaserver stop > ./status_${THIS}_${n}.log
sleep 1
ssh admin@${A1_IPv4_1} imaserver start >> ./status_${THIS}_${n}.log

