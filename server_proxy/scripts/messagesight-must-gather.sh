#!/bin/bash
# Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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

function print_help {
    echo "Usage: messagesight-must-gather.sh [ -d containerId  [ -e {true|false} ] ] "
    echo "                                   [ -a additional_directories ] [ -w working_dir ] [ -o output_file ]"
    echo ""
    echo "Create a must-gather output for a MessageSight installation."
    echo "This script can be used for Docker and native RPM installations."
    echo "For Docker installations, it has to be executed in the host environment."
    echo "The container type (server or WebUI) is determined automatically."
    echo ""
    echo "   -d, --docker-container         Docker container name or ID (only for Docker installations)"
    echo "   -e, --export-container=true    Export container into must-gather (only for Docker installations)"
    echo "   -a, --additional-directories   A colon-separated list of directories to be included in must-gather"
    echo "                                  This list can include host directories containing core files"
    echo "                                  Note that contents of /var/messagesight/data and /var/messagesight/diag/logs"
    echo "                                  from the container are always included"
    echo "   -w, --working-directory=\".\"  Working directory that can be used for temporary files"
    echo "                                  If not specified, the current directory is used"
    echo "   -o, --output-file              Output must-gather file name"
    echo "                                  If not specified, ./messagesight-must-gather-\"timestamp\".tar.gz is created"
}

function gather_run_parameters {
    # Get all environment variables
    ADDITIONAL_PARMS=""
    ENV_VARS=$(docker inspect -f '{{.Config.Env}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${ENV_VARS:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} -e $i"
    done

	# Get extra hosts
    HOSTS_PARMS=$(docker inspect -f '{{.HostConfig.ExtraHosts}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${HOSTS_PARMS:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --add-host $i"
    done

	# Get block IO weight
	BLKIO_WEIGHT=$(docker inspect -f '{{.HostConfig.BlkioWeight}}' "$CONTAINER_ID" 2> /dev/null)
	if [ "$BLKIO_WEIGHT" -ne "0" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --blkio-weight=$BLKIO_WEIGHT"
	fi

	# Get CPU shares
	CPU_SHARES=$(docker inspect -f '{{.HostConfig.CpuShares}}' "$CONTAINER_ID" 2> /dev/null)
	if [ "$CPU_SHARES" -ne "0" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cpu-shares=$CPU_SHARES"
	fi

    # Get cap add parameters
    CAP_PARMS=$(docker inspect -f '{{.HostConfig.CapAdd}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${CAP_PARMS:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cap-add $i"
    done

    # Get cap drop parameters
    CAP_PARMS=$(docker inspect -f '{{.HostConfig.CapDrop}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${CAP_PARMS:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cap-drop $i"
    done

	# Get CGroup parent
	CGROUP_PARENT=$(docker inspect -f '{{.HostConfig.CgroupParent}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$CGROUP_PARENT" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cgroup-parent=\"$CGROUP_PARENT\""
	fi

	# Get CPU period
	CPU_PERIOD=$(docker inspect -f '{{.HostConfig.CpuPeriod}}' "$CONTAINER_ID" 2> /dev/null)
	if [ "$CPU_PERIOD" -ne "0" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cpu-period=$CPU_PERIOD"
	fi

	# Get CPU quota
	CPU_QUOTA=$(docker inspect -f '{{.HostConfig.CpuQuota}}' "$CONTAINER_ID" 2> /dev/null)
	if [ "$CPU_QUOTA" -ne "0" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cpu-quota=$CPU_QUOTA"
	fi

	# Get CPU set CPUs
	CPUSET_CPUS=$(docker inspect -f '{{.HostConfig.CpusetCpus}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$CPUSET_CPUS" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cpuset-cpus=\"$CPUSET_CPUS\""
	fi

	# Get CPU set mems
	CPUSET_MEMS=$(docker inspect -f '{{.HostConfig.CpusetMems}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$CPUSET_MEMS" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --cpuset-mems=\"$CPUSET_MEMS\""
	fi

    # Get added host devices
    DEVICES=$(docker inspect -f '{{.HostConfig.Devices}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${DEVICES:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --device $i"
    done

    # Get custom DNS servers
    DNS_SERVERS=$(docker inspect -f '{{.HostConfig.Dns}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${DNS_SERVERS:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --dns $i"
    done

    # Get IPC mode
    IPC_MODE=$(docker inspect -f '{{.HostConfig.IpcMode}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$IPC_MODE" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --ipc=\"$IPC_MODE\""
	fi

    # Get privileged mode
    PRIV_MODE=$(docker inspect -f '{{.HostConfig.Privileged}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$PRIV_MODE" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --privileged=$PRIV_MODE"
	fi

    # Get "publish all ports" setting
    PUBLISH_ALL=$(docker inspect -f '{{.HostConfig.PublishAllPorts}}' "$CONTAINER_ID" 2> /dev/null)
	if [ "$PUBLISH_ALL" == "true" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} -P"
	fi

    # Get network mode
    NET_MODE=$(docker inspect -f '{{.HostConfig.NetworkMode}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$NET_MODE" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --net=\"$NET_MODE\""
	fi

    # Get security options
    SEC_OPTS=$(docker inspect -f '{{.HostConfig.SecurityOpt}}' "$CONTAINER_ID" 2> /dev/null)
    for i in ${SEC_OPTS:1:-1}
    do
        ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --security-opt $i"
    done

    # Get user name
    USER_NAME=$(docker inspect -f '{{.Config.User}}' "$CONTAINER_ID" 2> /dev/null)
	if [ ! -z "$USER_NAME" ]
	then
		ADDITIONAL_PARMS="${ADDITIONAL_PARMS} --user=\"$USER_NAME\""
	fi

	ADDITIONAL_PARMS="${ADDITIONAL_PARMS} -it"

	echo "$ADDITIONAL_PARMS"
}

IS_DOCKER=0
ADDITIONAL_DIRECTORIES="/etc /var/log /var/imabridge"
NOW=$(date +"%m%d%Y-%H%M%S")
OUTPUT_FILE=messagesight-must-gather-${NOW}.tar.gz
WORK_DIR="$(pwd)"
DEBUG_IMAGE_NAME="imamustgather:debug"

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    print_help
    exit 0
    ;;
    -d|--docker-container)
    IS_DOCKER=1
    CONTAINER_ID="$2"
    FULL_CONTAINER_ID=$(docker inspect --format '{{ .Id }}' "$CONTAINER_ID" 2> /dev/null)
    shift
    ;;
    -e|--export-container)
    SAVE_CONTAINER="$2"
    shift
    ;;
    -a|--additional-directories)
    DIRS=$2
    while IFS=':' read -ra DIR; do
        for i in "${DIR[@]}"; do
            ADDITIONAL_DIRECTORIES=$ADDITIONAL_DIRECTORIES" $i"
        done
    done <<< "$DIRS"
    shift
    ;;
    -o|--output-file)
    OUTPUT_FILE="$2"
    shift
    ;;
    -w|--working-directory)
    WORK_DIR=$(cd "$2" || exit 2; pwd)
    shift
    ;;
    *)
    echo "Unknown option $key"
    exit 1
    ;;
esac
shift # past argument or value
done

function process_cmd_args {
# Check valid combinations of options
# 1. -f and -r can only be specified along with -d
    if [ "$IS_DOCKER" -eq "0" ] && [ ! -z "$SAVE_CONTAINER" ]
    then
        echo "-f option cannot be used without container name/ID"
        exit 1
    fi

# 2. Check if working directory exists (if specified)
    if [ ! -z "$WORK_DIR" ];
    then
        if [ -d "$WORK_DIR" ]
        then
            echo "Work dir exists: $WORK_DIR"
        else
            echo "Work dir does not exist: ${WORK_DIR}. Exiting."
            exit 1
        fi

        #AVAIL_SPACE=$(df . | tail -n 1 | awk '{print $4}')
    fi

# 3. Locate imaserver directory
    IMASERVER_SOURCE=$(dirname "$(rpm -q --filesbypkg IBMWIoTPMessageGatewayBridge 2> /dev/null | grep lib64 | head -n 1  | awk '{print $2}')" 2> /dev/null)
    if [ -z "$IMASERVER_SOURCE" ]
    then
        IMASERVER_SOURCE=/opt/ibm/imabridge
    fi

# 4. Check if container exists
    if [ "$IS_DOCKER" -eq "1" ] && [ -z "$FULL_CONTAINER_ID" ]
    then
        echo "Container $CONTAINER_ID does not exist or could not be accessed"
        exit 1
    fi
}

function collect_host_data {
    echo "====================================="
    echo " General info section"
    echo "====================================="
    echo "1. uname -a"
    uname -a 2> /dev/null
    echo "-------------------------------------"
    echo "2. uptime"
    uptime 2> /dev/null
    echo "-------------------------------------"
    echo "3. df -lh"
    df -lh 2> /dev/null
    echo "-------------------------------------"
    echo "4. du -sxm --exclude=/proc"
    if [ "$IS_DOCKER" -eq "0" ]
    then
        du -sxm ${IMASERVER_SOURCE}/* 2> /dev/null
    else
        du -sxm --exclude=/proc /* 2> /dev/null
    fi
    echo "-------------------------------------"
    echo "5. locale"
    locale 2>&1
    echo "-------------------------------------"
    echo "6. date"
    date 2>&1
    echo "-------------------------------------"
    echo "7. free"
    free 2>&1
    echo "-------------------------------------"
    echo "8. lsmod"
    lsmod 2>&1
    echo "-------------------------------------"
    echo "9. systemctl -a"
    systemctl -a 2>&1
    echo "-------------------------------------"
    echo "10. systemctl -l status"
    systemctl -l status 2>&1
    echo "-------------------------------------"
    echo "11. udevadm info --export-db"
    udevadm info --export-db 2>&1
    echo "-------------------------------------"
    echo "12. lspci -vvv -n"
    lspci -vvv -n 2>&1
    echo "-------------------------------------"
    echo "13. rpm -qa --last"
    rpm -qa --last 2>&1
    echo "-------------------------------------"

    echo "====================================="
    echo " Disk info section"
    echo "====================================="
    echo "14. pvdisplay"
    pvdisplay 2>&1
    echo "-------------------------------------"
    echo "15. vgdisplay"
    vgdisplay 2>&1
    echo "-------------------------------------"
    echo "16. lvdisplay"
    lvdisplay 2>&1
    echo "-------------------------------------"
    echo "17. parted -l"
    parted -l 2>&1
    echo "-------------------------------------"
    echo "18. sfdisk -l"
    sfdisk -l 2>&1
    echo "-------------------------------------"
    echo "19. fdisk -l"
    fdisk -l 2>&1
    echo "-------------------------------------"
    echo "20. dumpfssize"
    run_dumpfssize 2>&1
    echo "-------------------------------------"
    echo "21. dumpe2fs"
    run_dumpe2fs 2>&1
    echo "-------------------------------------"
    echo "22. dumpxfs"
    run_dumpxfs 2>&1
    echo "-------------------------------------"

    echo "====================================="
    echo " Network info section"
    echo "====================================="
    echo "23. hostname"
    hostname 2>&1
    echo "-------------------------------------"
    echo "24. ip -s xfrm state"
    ip -s xfrm state 2>&1
    echo "-------------------------------------"
    echo "25. ip -s xfrm policy"
    ip -s xfrm policy 2>&1
    echo "-------------------------------------"
    echo "26. ip neigh"
    ip neigh 2>&1
    echo "-------------------------------------"

    for i in {1..5}
    do
        echo "27. netstat -ansv [$i]"
        netstat -ansv 2>&1
        echo "-------------------------------------"
    done
    echo "28. netstat -anlp"
    netstat -anlp 2>&1
    echo "-------------------------------------"
    echo "29. ip maddr"
    ip maddr 2>&1
    echo "-------------------------------------"
    echo "30. ifconfig -a"
    ifconfig -a 2>&1
    echo "-------------------------------------"
    echo "31. route"
    route 2>&1
    echo "-------------------------------------"
    echo "32. cat /etc/resolv.conf"
    cat /etc/resolv.conf 2>&1
    echo "-------------------------------------"
    echo "33. collect_internal_nic_state"
    collect_internal_nic_state 2>&1
    echo "-------------------------------------"
    echo "34. ethtool"
    for nic in /sys/class/net/*
    do
        echo "ethtool for $nic:"
        ethtool "$nic" 2>&1
        ethtool -i "$nic" 2>&1
        ethtool -c "$nic" 2>&1
        ethtool -k "$nic" 2>&1
        ethtool -g "$nic" 2>&1
    done
    echo "-------------------------------------"

    echo "====================================="
    echo " Process and file descriptor section"
    echo "====================================="
    echo "35. ulimit -a"
    echo "ulimit -a" | /bin/bash 2>&1
    echo "36. ps -ef"
    ps -ef 2>&1
    echo "-------------------------------------"

    echo "====================================="
    echo " /proc section"
    echo "====================================="
    echo "37. sysctl -a"
    sysctl -a 2>&1
    echo "-------------------------------------"
    echo "38. cat /proc/cpuinfo"
    cat /proc/cpuinfo 2>&1
    echo "-------------------------------------"
    echo "39. cat /proc/meminfo"
    cat /proc/meminfo 2>&1
    echo "-------------------------------------"
    echo "40. cat /proc/mounts"
    cat /proc/mounts 2>&1
    echo "-------------------------------------"
    echo "41. cat /proc/cmdline"
    cat /proc/cmdline 2>&1
    echo "-------------------------------------"
    echo "42. cat /proc/misc"
    cat /proc/misc 2>&1
    echo "-------------------------------------"
    echo "43. cat /proc/locks"
    cat /proc/locks 2>&1
    echo "-------------------------------------"
    echo "44. cat /proc/mtrr"
    cat /proc/mtrr 2>&1
    echo "-------------------------------------"
    echo "45. cat /proc/partitions"
    cat /proc/partitions 2>&1
    echo "-------------------------------------"
    echo "46. cat /proc/iomem"
    cat /proc/iomem 2>&1
    echo "-------------------------------------"
    echo "47. cat /proc/ioports"
    cat /proc/ioports 2>&1
    echo "-------------------------------------"
    echo "48. cat /proc/modules"
    cat /proc/modules 2>&1
    echo "-------------------------------------"
    echo "49. cat /proc/net/sockstat"
    cat /proc/net/sockstat
    echo "-------------------------------------"

    for i in {1..5}
    do
        echo "50. cat /proc/interrupts [$i]"
        cat /proc/interrupts 2>&1
        echo "-------------------------------------"
        echo "51. cat /proc/slabinfo [$i]"
        cat /proc/slabinfo 2>&1
        echo "-------------------------------------"
        echo "52. cat /proc/vmstat [$i]"
        cat /proc/vmstat 2>&1
        echo "-------------------------------------"
        sleep 2
    done

    if [ "$IS_DOCKER" -eq "1" ]
    then
        echo "====================================="
        echo " Docker section"
        echo "====================================="
        echo "53. docker ps -a"
        docker ps -a 2>&1
        echo "-------------------------------------"
        echo "54. docker images"
        docker images 2>&1
        echo "-------------------------------------"
        echo "55. memory cgroup"
        if [ -e /sys/fs/cgroup/memory/system.slice/docker-"${FULL_CONTAINER_ID}".scope ]
        then
            for item in /sys/fs/cgroup/memory/system.slice/docker-"${FULL_CONTAINER_ID}".scope/*
            do
                cat /sys/fs/cgroup/memory/system.slice/docker-"${FULL_CONTAINER_ID}".scope/"$item" 2>&1
            done
        fi
        echo "-------------------------------------"
        echo "56. cpuset cgroup"
        if [ -e /sys/fs/cgroup/cpuset/system.slice/docker-"${FULL_CONTAINER_ID}".scope ]
        then
            for item in /sys/fs/cgroup/cpuset/system.slice/docker-"${FULL_CONTAINER_ID}".scope/*
            do
                cat /sys/fs/cgroup/cpuset/system.slice/docker-"${FULL_CONTAINER_ID}".scope/"$item" 2>&1
            done
        fi
        echo "-------------------------------------"
    fi
}

function collect_internal_nic_state {
  local _nic
  _IP=ip

  RECOGNIZED_INTF_EGREP="^docker"
  IPTABLES_LOADED=$(lsmod 2> /dev/null|grep -c ip_tables)

  for _nic in $( cd /sys/class/net && ls -d 2>/dev/null | grep -v "\\." | grep -v "$RECOGNIZED_INTF_EGREP" )
  do
    echo "# $_nic layer2 settings:"
    ethtool "$_nic"
    ethtool -d "$_nic"
    echo "# $_nic show ethernet MII-MAU:"
    mii-tool -v "$_nic"
    mii-diag -v "$_nic" 2>/dev/null
    echo "# $_nic layer2 statistics:"
    ethtool -S "$_nic"
    echo
  done

  if [[ -d /proc/net/bonding ]]
  then
    for _nic in /proc/net/bonding*
    do
      echo "# $_nic layer2 settings:"
      ethtool "$_nic"
      echo "# $_nic layer2 statistics:"
      cat /proc/net/bonding/"$_nic"
      echo
    done
  fi

  echo "# link:"
  $_IP link
  echo

  echo "# addr:"
  $_IP addr
  echo

  local _ipv
  for _ipv in 4 6
  do
    echo "########## ipv$_ipv ##########"

    echo "# v$_ipv addr:"
    $_IP -$_ipv addr
    echo

    echo "# v$_ipv rule:"
    $_IP -$_ipv rule
    echo

    local _TABLE
    for _TABLE in main $( $_IP -$_ipv route show table 0 | grep -v -e "table local"  -e "table main" -e "table default" -e "table unspec" | grep " table " | sed -e 's/^.* table //g' -e 's/ .*$//g' | sort | uniq )
    do
      echo "# v$_ipv route table $_TABLE:"
      $_IP -$_ipv route show table "$_TABLE"
      echo
    done
    echo

    if [ "$IPTABLES_LOADED" = "0" ]
    then
      echo "# v$_ipv isolation OFF"
    else
      echo "# v$_ipv isolation:"
      if [[ "$_ipv" = "4" ]]
      then
        iptables -vnxL
      else
        ip6tables -vnxL
      fi
    fi
    echo
  done

  echo "# arp:"
  arp -a
  echo

  echo "# ss(summary):"
  ss -s
  echo

  # Collect firmware version for each network interface card
    echo "# Firmware version for each Ethernet interface card follows:"
    local _nic
    #for _nic in $( cd /sys/class/net && ls -d 2>/dev/null | grep -v "\." | grep -v $RECOGNIZED_INTF_EGREP )
    for _nic in $( cd /sys/class/net && ls -d 2>/dev/null | grep -v "\\." | grep -v $RECOGNIZED_INTF_EGREP )
    do
      echo "# $_nic interface"
      ethtool -i "$_nic" | grep firmware
    done
}

function run_dumpfssize {
    for DISK in /sys/block/sd*
    do
        echo "${DISK}"/size: "$(cat "${DISK}"/size)"

        for PART in "${DISK}"/sd*/size/*
        do
            echo "${PART}": "$(cat "$PART")"
        done
    done
}

function run_dumpe2fs {
    for P in $(fdisk -l | grep -v Disk | grep /dev/ | awk '{print $1}')
    do
        echo "# dumpe2fs -h $P"
        dumpe2fs -h "$P" 2> /dev/null
    done
}

function run_dumpxfs {
    for P in $(mount | grep xfs | grep /dev/ | awk '{print $1}' | uniq)
    do
        echo "# xfs_info $P"
        xfs_info "$P" 2> /dev/null
    done
}

# Main processing starts here
process_cmd_args

if [ "$IS_DOCKER" -eq "1" ]
then
    echo "Processing MessageSight Docker container ${CONTAINER_ID}"
else
    if [ ! -z "$IMASERVER_SOURCE" ]
    then
        echo "Processing MessageSight installation in ${IMASERVER_SOURCE}"
    fi

fi

echo "Collecting general host data"
collect_host_data > "host_data-${NOW}.txt"
dmesg > dmesg.txt
dmidecode > dmidecode.txt 2> /dev/null || { echo "Could not collect dmidecode data: $?" > dmidecode.txt; }

if [ "$IS_DOCKER" -eq "1" ];
then
    IS_RUNNING=$(docker inspect --format='{{.State.Running}}' "$CONTAINER_ID")
    if [ "$IS_RUNNING" == "true" ]
    then
        # Run commands in the container using docker exec
        echo "Collecting data in container $CONTAINER_ID"

        docker exec -it "$CONTAINER_ID" /opt/ibm/imabridge/bin/internal_mustgather.sh > "${WORK_DIR}"/docker_mustgather.txt 2>&1

        docker cp "${CONTAINER_ID}":/var/imabridge/diag/cores/container_mustgather.tar.gz . 2> /dev/null
    else
        # Container is not running, create a new temporary image and run it with another entrypoint
        echo "Creating temporary image $DEBUG_IMAGE_NAME from container $CONTAINER_ID"
        docker commit "$CONTAINER_ID" "$DEBUG_IMAGE_NAME" > /dev/null 2>&1
        if [ $? -eq 0 ]
        then
            echo "Collecting data in stopped container"

			ADDITIONAL_PARMS=$(gather_run_parameters)

            NEW_NAME=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 24 | head -n 1)
            docker run "$ADDITIONAL_PARMS" --volumes-from "$CONTAINER_ID" --entrypoint=/opt/ibm/imabridge/bin/internal_mustgather.sh --name "$NEW_NAME" "$DEBUG_IMAGE_NAME" > "${WORK_DIR}"/docker_mustgather.txt 2>&1
            if [ $? -eq 0 ]
            then
                NEW_ID=$(docker inspect -f '{{.Id}}' "$NEW_NAME" 2> /dev/null)
                docker cp "${NEW_ID}":/var/imabridge/diag/cores/container_mustgather.tar.gz .
                docker stop "$NEW_ID" > /dev/null 2>&1
                docker rm --force=true "$NEW_ID" > /dev/null 2>&1
            fi
            echo "Removing temporary image"
            docker rmi --force=true "$DEBUG_IMAGE_NAME" > /dev/null 2>&1
        fi
    fi

    # Finally, get container data
    if [ -z "$SAVE_CONTAINER" ] || [ "$SAVE_CONTAINER" == "true" ];
    then
        NAME="messagesight-container-${NOW}"

        echo "Exporting container to a file"
        docker export "$CONTAINER_ID" | gzip > "${WORK_DIR}"/"${NAME}".tar.gz 2> /dev/null || { echo "docker export failed with RC=$?"; exit 3; }
    fi

    FILES_TO_COMPRESS="./${NAME}.tar.gz ./host_data-${NOW}.txt ./container_mustgather.tar.gz ./docker_mustgather.txt ./dmesg.txt ./dmidecode.txt"
elif [ "$IS_DOCKER" -eq "0" ];
then
    echo "Non-docker MessageSight"
    "${IMASERVER_SOURCE}"/bin/internal_mustgather.sh "${WORK_DIR}" software > "${WORK_DIR}"/software_mustgather.txt 2>&1
    FILES_TO_COMPRESS="./host_data-${NOW}.txt ./dmesg.txt ./dmidecode.txt ./software_mustgather.txt ./software_mustgather.tar.gz"
else
    echo "Either source directory or container ID has to be specified"
    exit 1
fi

echo "Redirecting journalctl output to /var/log ..."
journalctl | gzip > /var/log/journalctl.out.gz
echo "Compressing must-gather"
cd "${WORK_DIR}" || { echo "Could not find work dir."; exit 2; }
tar --exclude="*shadow*" --exclude="pki" --exclude="/var/imabridge/keystore" -czf "$OUTPUT_FILE" $FILES_TO_COMPRESS $ADDITIONAL_DIRECTORIES --ignore-failed-read 2> /dev/null
rm -rf "$FILES_TO_COMPRESS" 2> /dev/null
echo "Must-gather processing is complete: $OUTPUT_FILE"
