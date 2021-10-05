

del_subs () {
local server=$1
local data


rm -rf .tmp.delsubs
while((1)); do

    # pull of subscription list deleting the first line which should be the header row.
    data=`ssh -nx admin@$server imaserver stat Subscription StatType=PublishedMsgsHighest "ResultCount=100" | sed '1d'  `

    echo "data is \"$data\"" 
    
    count=`echo "$data" |wc -l`
    if ! [ -n "$data" ] ;then
        echo "------------------------"
        echo "$count - empty string - no more subs waiting for deletion"
        echo "------------------------"
        break;
    elif (($count>0));  then
        echo "------------------------"
        echo "$count subs still waiting for deletion"
        echo "------------------------"
    else 
        echo "------------------------"
        echo "$count - no more subs waiting for deletion"
        echo "------------------------"
        break;
    fi
    { echo -n "ssh -nx admin@$server \"" ; echo "$data" | awk -F ',' '{printf(" imaserver delete Subscription SubscriptionName=%s ClientID=%s; ", $1,$3);}' ; echo "\" "; }  >> .tmp.delsubs
    
    { echo -n "ssh -nx admin@$server \"" ; echo "$data" | awk -F ',' '{printf(" imaserver delete Subscription SubscriptionName=%s ClientID=%s; ", $1,$3);}' ; echo "\" "; }  | sh -x
    #{ echo -n "ssh -nx admin@$server \"" ; echo "$data" | awk -F ',' '{printf(" imaserver delete Subscription SubscriptionName=%s ClientID=%s; ", $1,$3);}' ; echo "\" "; }  |sh
done

}
