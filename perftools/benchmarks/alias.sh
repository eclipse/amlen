#!/bin/bash

MBPIPE=/tmp/mqttbench-np
JMSPIPE=/tmp/jmsbench-np
SUDOCMD=`if [ $(whoami) != "root" ] ; then echo sudo ; fi`

# --------------------------------------------------------------------------------------------------
# Docker related functions and aliases
# --------------------------------------------------------------------------------------------------
function fdre           { $SUDOCMD docker exec -it $1 bash ; }
function fdra           { $SUDOCMD docker exec -it $1 bash -ic ${@:2} ; }
function fdrc           { $SUDOCMD docker exec -it $1 ${@:2} ; }
function fdrstop        { $SUDOCMD docker stop $1 ; }
function fdrstart       { $SUDOCMD docker start $1 ; }
function fdrrestart     { $SUDOCMD docker restart $1 ; }
function fdrrm          { $SUDOCMD docker rm $1 ; }
function fdrbld         { $SUDOCMD docker build --force-rm=true -t $1 ; }
function fdrsave        { $SUDOCMD docker save -o $1 $2 ; gzip $1 ; }
function fdrpull        { wget http://eric-svtpvt-seed-1.priv:/dockerhub/latest-msserver.tar.gz ; gunzip -f latest-msserver.tar.gz ; $SUDOCMD docker load --input=latest-msserver.tar ; }
function fwsubs         { fdrc $1 watch -d -n1 /opt/ibm/imaserver/bin/msshell imaserver stat Subscription StatType=BufferedHWMPercentHighest ; }
function frmiptmods     { $SUDOCMD rmmod ipt_MASQUERADE nf_nat_masquerade_ipv4 iptable_filter iptable_nat nf_nat_ipv4 nf_nat nf_conntrack_ipv4 nf_defrag_ipv4 nf_conntrack xt_addrtype ip_tables ipt_REJECT ; }
function fiperfc		{ iperf -c $1 -p 8000 -t 10 -P 4 -w 64K ; }

alias drenter='fdre'
alias drcmd='fdrc'
alias dralias='fdra'
alias drstop='fdrstop'
alias drstart='fdrstart'
alias drrestart='fdrrestart'
alias drrm='fdrstop ; fdrrm'
alias drbld='fdrbld'
alias drsave='fdrsave'
alias drpull='fdrpull'
alias drstopall='${SUDOCMD} docker stop `${SUDOCMD} docker ps -a -q`'
alias drrmall='drstopall ; ${SUDOCMD} docker rm `${SUDOCMD} docker ps -a -q`'
alias drrmiall='drrmall ; ${SUDOCMD} docker rmi `${SUDOCMD} docker images -qa`'
alias drgetdr='wget https://test.docker.com/builds/Linux/x86_64/docker-latest -O docker'

# --------------------------------------------------------------------------------------------------
# IMA related functions and aliases with a Docker Messagesight
# --------------------------------------------------------------------------------------------------
function fimastat       { curl -X GET http://$1/ima/v1/service/status ; }
function fimarestart    { curl -X POST -d "{\"Service\":\"Server\"}" http://$1/ima/v1/service/restart ; }

alias imastatus='fimastat'
alias imarestart='fimarestart'

# --------------------------------------------------------------------------------------------------
# General functions and aliases
# --------------------------------------------------------------------------------------------------
# Watches interrupts that match argument 1
function fwirqs {
  if [ "$1" != "" ] ; then
    echo "watch -d -n2 'cat /proc/interrupts | grep $1'" | sudo sh
  else
    echo "watch -d -n2 'cat /proc/interrupts'" | sudo sh
  fi
}

alias pyhttpsrv='python -m SimpleHTTPServer 80'
alias shorel='echo "~/workspace -> $(readlink ~/workspace)"'
# Future add script to run ipmitool given hostname and ipmi command
alias wirqs='fwirqs'
alias rmiptmods='frmiptmods'
alias iperfs='iperf -s -w 128k -i 2 -p 8000'
alias iperfc='fiperfc'
alias pu='pstree -up -G'

# --------------------------------------------------------------------------------------------------
# MessageSight functions and aliases
# --------------------------------------------------------------------------------------------------
alias wsubs='fwsubs'
alias topima='top -p `ps -C imaserver -o pid=`'
alias gsima='gstack `ps -C imaserver -o pid=`'
alias gdbima='gdb -p `ps -C imaserver -o pid=`'
alias pmima='pmap -p `ps -C imaserver -o pid=`'
alias lsima='lsof -p `ps -C imaserver -o pid=`'

# --------------------------------------------------------------------------------------------------
# Java functions and aliases
# --------------------------------------------------------------------------------------------------
alias topjava='top -p `ps -C java -o pid= | tail -1`'
alias nsjava='while(true) ; do sudo netstat -tnp | grep java ;sleep 1;echo "--------------------" ; done'
alias gsjava='gstack `ps -C java -o pid= | tail -1`'
alias k3java='kill -3 `ps -C java -o pid= | tail -1`'

# --------------------------------------------------------------------------------------------------
# mqttbench functions and aliases
# --------------------------------------------------------------------------------------------------
alias topmb='top -p `ps -C mqttbench -o pid= | tail -1`'
alias gdbmb='gdb -p `ps -C mqttbench -o pid=`'
alias gsmb='gstack `ps -C mqttbench -o pid= | tail -1`'
alias pmmb='pmap `ps -C mqttbench -o pid=`'
alias lsmb='lsof -p `ps -C mqttbench -o pid=`'


