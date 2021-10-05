ima1=10.10.10.10
ima2=10.10.10.10
 let iteration=0; 
while((1)); do 
echo "`date` check iteration $iteration"; 
for serv in $ima1 $ima2; do
data=`ssh admin@$serv advanced-pd-options list `
echo " check data $data"
if echo "$data" | grep "bt\."; then
 echo "`date` it looks like a bt" 
 echo "`date`" |  mail -s "it looks like a bt" someuser@some.company.com
    exit 1;
fi

done
sleep 60
let iteration=$iteration+1; 
done


