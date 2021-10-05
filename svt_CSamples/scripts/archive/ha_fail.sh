#echo "ifconfig" | ssh admin@10.10.10.10 "busybox"

# let iteration=0; while((1)); do ./ha_fail.sh; ((iteration++)); echo current iteration is $iteration; date; done

ima1=${1-10.10.10.10}
ima2=${2-10.10.10.10}

kill_servers() {
ssh admin@$ima1 imaserver stop force
ssh admin@$ima2 imaserver stop force
}
start_servers() {
ssh admin@$ima1 imaserver start
ssh admin@$ima2 imaserver start
}

get_status () {
local needed_status_1=${1-""}
local needed_status_count_1=${2-0}
local needed_status_2=${2-""}
local needed_status_count_2=${3-0}
local total_status_1=0
local total_status_2=0
local s1;
local s2;
local h1;
local h2;
local msg="";
local prevmsg="";
   
while((1)) ; do
    let total_status_1=0;
    let total_status_2=0;

    msg=$'\n==================================================='
    msg+=$'\nharole of ima1 is : '
    h1=`ssh admin@$ima1 imaserver harole`
    msg+="$h1"
    msg+=$'\nstatus of ima1 is : '
    s1=`ssh admin@$ima1 status imaserver`
    msg+="$s1"
    if [[ "$s1" =~ "$needed_status_1" ]] ;then
        ((total_status_1++));
    elif [[ "$s1" =~ "$needed_status_2" ]] ;then
        ((total_status_2++));
    fi
    msg+=$'\n'
    msg+=$'\nharole of ima2 is : '
    h2=`ssh admin@$ima2 imaserver harole`
    msg+="$h2"
    msg+=$'\nstatus of ima2 is : '
    s2=`ssh admin@$ima2 status imaserver`
    msg+="$s2"
    if [[ "$s2" =~ "$needed_status_1" ]] ;then
        ((total_status_1++));
    elif [[ "$s2" =~ "$needed_status_2" ]] ;then
        ((total_status_2++));
    fi
   
    msg+=$'\n'
    msg+="total_status_1 = $total_status_1 , total_status_2 = $total_status_2"
    msg+=$'\n'
    if (($needed_status_count_1>0)); then
        if (($total_status_1==$needed_status_count_1)); then
            if (($needed_status_count_2>0)); then
                if (($total_status_2==$needed_status_count_2)); then
                    msg+="Expected status reached on both servers. $ima1 $ima2"
                    echo "$msg"
                    break;
                fi
            elif (($needed_status_count_2==0)); then
                msg+="Expected status reached on both servers. $ima1 $ima2"
                echo "$msg"
                break;
            fi
        fi 
    fi 
    if [ "$msg" != "$prevmsg" ] ;then
        echo "$msg"
        prevmsg="$msg"
    else
        echo -n "."
    fi
    sleep 1;
done

}

kill_servers
get_status "Stopped" 2 
start_servers
get_status "Running" 1 "Standby" 1


