#!/bin/bash
set -x
cp  /root/.ssh/authorized_keys  /root/.ssh/authorized_keys.ORIGINAL
cp  /etc/ssh/ssh_config         /etc/ssh/ssh_config.ORIGINAL
cp  /etc/ssh/sshd_config        /etc/ssh/ssh_config.ORIGINAL
cp  /root/.bashrc               /root/.bashrc.ORIGINAL
cp  /etc/sysctl.conf            /etc/sysctl.conf.ORIGINAL
cp  /etc/security/limits.conf   /etc/security/limits.conf.ORIGINAL
#
scp  10.10.10.10:/root/.ssh/A*               /root/.ssh/
scp  10.10.10.10:/root/.ssh/authorized_keys  /root/.ssh/
scp  10.10.10.10:/etc/ssh/ssh*_config        /etc/ssh/
scp  10.10.10.10:/root/.ssh/environment      /root/.ssh/
scp  10.10.10.10:/root/.bashrc               /root/
scp  10.10.10.10:/root/ibm-yum.sh            /root/
scp  10.10.10.10:/etc/bash.bashrc.local      /etc/
scp  10.10.10.10:/etc/sysctl.conf            /etc/
scp  10.10.10.10:/etc/security/limits.conf   /etc/security/

mkdir -p /mnt/mar200
mkdir -p /mnt/mar201
mkdir -p /mnt/mar145
mkdir -p /mnt/mar145_HOME
mkdir -p /niagara

scp     10.10.10.10:/niagara/*   /niagara/
scp -r  10.10.10.10:/niagara/test/*   /niagara/test/
scp -r  10.10.10.10:/niagara/application/*   /niagara/application/
set +x
echo " "
echo " %===>   Check success above, and then do these steps: " 
echo "Make Virtual Nic's for 10.2.* 10.6.* and 10.7* off 10.10.* adapter"
echo "RUN:  rpm -i --nodeps /mnt/mar201/product/..../java.rpm"
echo "RUN:  ~/ibm-yum.sh update"
echo "RUN:  . ~/.bashrc "

