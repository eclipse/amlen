ima=${1-"10.10.10.10"}
client=${2-`hostname -i`}
release=${3-"IMA14a"}
action=${4-"upgrade"} # pristine-install  upgrade or rollback
drivertype=${5-"development"} # development or production
interactive=${6-1} # 0 or 1 for "interactive" driver selection
firmware="dev_bedrock.scrypt2" # dev_bedrock.scrypt2 or rel_bedrock.scrypt2

#-----------------------------------------------------------
# example doing pristine install with driver selection
#-----------------------------------------------------------
# ./codeupgrade.sh 10.10.10.10 10.10.10.10 IMA100 pristine-install production  1



if [ "$drivertype" == "production" ] ;then
    firmware="rel_bedrock.scrypt2"
fi

if [ "$release" == "IMA14a" ] ;then
    echo -n ""   
    myreleasedir=/mnt/mar145/release/IMA14a/$drivertype
elif [ "$release" == "IMA13b" ] ;then
    echo -n ""   
    myreleasedir=/mnt/mar145/release/IMA13b/$drivertype
elif [ "$release" == "IMA100" ] ;then
    echo -n ""   
    myreleasedir=/mnt/mar145/release/IMA100/$drivertype
else
    echo "ERROR: unsupported release"
    exit 1;
fi

echo "--------------------------------------------------"
echo " Code upgrade to -> $ima"
echo " From client $client"
echo " Release $release"
echo "--------------------------------------------------"
echo ""


do_it(){
    local driver=$1
    local myinput
    echo "Using $driver "
    echo ""
    echo "Continue (y/n) ?"
    read myinput
    if [ "$myinput" == "y" ] ;then
        echo "Continuing"
        echo "Uploading firmware"
        /niagara/test/svt_cmqtt/appliance.exp -a putfile -u root -p d0lph1ns -s $ima -c $client -f myfirmware -d $driver -f myfirmware
        echo "Upgrading firmware"
        /niagara/test/svt_cmqtt/appliance.exp -a runcmd -x " firmware $action myfirmware" -s $ima
        echo "Done. Upgraded $ima to $driver"
        
    else
        echo "Exitting , user typed $myinput "
        exit 1;
    fi

}

if [ "$interactive" == "1" ] ;then
 find $myreleasedir -maxdepth 1 | grep -E '[0-9]{8,8}\-[0-9]{4,4}'  |sort 
 echo "Which driver do you want? Enter full path now"
 read data
 echo "$data"
 echo "Correct? y or n"
 read myinput
 if [ "$myinput" != "y" ] ;then
        echo "Exitting , user typed $myinput "
        exit 1;
 fi
else
    data=`find $myreleasedir -maxdepth 1 -mtime 0 | grep -E '[0-9]{8,8}\-[0-9]{4,4}'  |sort |tac `
fi
driver=`echo "$data" | head -1`

echo "--------------------------------------------------"
echo " Recent driver list"
echo "--------------------------------------------------"
echo "$data" | tac | awk '{printf("      %s\n",$0);}'
echo "--------------------------------------------------"

if [ -e $driver/appliance/$firmware ] ;then
    driver="$driver/appliance/$firmware"
    
    do_it $driver
else
    echo "Warning: $driver is not a suitable driver."
fi



