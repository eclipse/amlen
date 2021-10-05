#!/bin/bash

LOGFILE=/root/bootstrap-fvt-centos.log
CENTOSRHELVER=""
ERIC_SVTPVT_SEED="ms-seed.loadtest.internetofthings.ibmcloud.com"
# List of new packages.  Edit this to install different packages
packages=(sendmail nfs-utils bash-completion bc bind-utils binnutils boost bzip2 cpio curl curl-devel device-mapper device-mapper-multipath dsniff dos2unix gdb icu iotop jansson java-1.8.0-openjdk.x86_64 java-1.8.0-openjdk-devel.x86_64 kbd-legacy libasan libicu libicu-devel libcgroup-tools libstdc++ libstdc++-devel logrotate lsof lvm2 net-tools net-snmp ntp ntpdate openldap openldap-devel openldap-servers openldap-clients openssl-devel openssh-clients pciutils perf procps procps-ng rsync rsyslog screen socat sockperf sshpass sslscan strace sudo sysstat sysvinit-tools tar tcpdump telnet traceroute unzip vim vim-enhanced wget yum-utils zip zlib)

function config_drive {
    if [ -e /dev/xvdc ]; then
        (  echo o; echo n; echo p; echo 1; echo; echo; echo w; ) | fdisk /dev/xvdc
        sleep 5
        mkfs.xfs -f /dev/xvdc1
        mkdir -p /data
        mount -t xfs /dev/xvdc1 /data
        systemctl stop docker
        if [ ! -d /var/lib/docker/image ]; then
            mkdir /data/docker_image
            if [ ! -d /var/lib/docker ]; then
                mkdir -p /var/lib/docker
            fi
            ln -s /data/docker_image /var/lib/docker/image
        else
            mv /var/lib/docker/image /data/docker_image
            ln -s /data/docker_image /var/lib/docker/image
        fi

        mkdir -p /data/A1 /data/A2 /data/A3 /data/A4 /data/A5 /data/A6 /data/A7 /data/A8
        ln -s /data/A1 /mnt/A1
        ln -s /data/A2 /mnt/A2
        ln -s /data/A3 /mnt/A3
        ln -s /data/A4 /mnt/A4
        ln -s /data/A5 /mnt/A5
        ln -s /data/A6 /mnt/A6
        ln -s /data/A7 /mnt/A7
        ln -s /data/A8 /mnt/A8
        mkdir /data/messagesight
        ln -s /data/messagesight /var
        systemctl start docker
    fi
}

function LOG {
    msg=$1
    echo "*******************************************************************************************"
    echo "$(date) ***** $msg"
    echo "*******************************************************************************************"
}

function yum_install {
    local -a packages=("$@")
    local -a install_packages
    if [ ${#packages[@]} -eq 0 ]; then
        echo "No packages received to install."
        return 0
    else
        echo "Received the list of the following packages to be installed:"
        echo "${packages[@]}"
    fi

    for p in "${packages[@]}"; do
        if rpm -q "$p" > /dev/null; then
            echo "RPM $p is already installed."
        else echo "RPM $p was not installed.  Adding to the list to be installed."
            install_packages+=("$p")
        fi
    done
    if [ ${#install_packages[@]} -eq 0 ]; then
        echo "Either no packages were provided to install, or they are all installed already."
        return 0
    else
        echo "Installing the following packages:"
        echo "******************************************************************"
        echo "${install_packages[@]}"
        if ! yum install -y "${install_packages[@]}"; then
            echo "We got errors trying to install packages."
            echo "We need to sleep and try again. Sleeping for 30 seconds."
            sleep 30
            yum_install "${install_packages[@]}"
        else
            echo "******************************************************************"
            echo "Installation of all packages succeeded!"
            echo "******************************************************************"
        fi
    fi
}

rm -f $LOGFILE

LOG "Running $0" 2>&1 | tee -a $LOGFILE

if [ ! -e  /etc/redhat-release ] ; then
  LOG "$0 cannot run on a non-CentOS/RHEL system, exiting..."
  exit 1
else
  if [ "1" == "$(grep -c 'release.*6' /etc/redhat-release)" ] ; then CENTOSRHELVER=6 ; fi
  if [ "1" == "$(grep -c 'release.*7' /etc/redhat-release)" ] ; then CENTOSRHELVER=7 ; fi
fi

if [ "$(whoami)" != "root" ] ; then
   LOG "must be root to run this script, exiting ..."
  exit
fi

function installrepos {
  LOG "Installing additional YUM repositories"
  # Install and enable additional YUM repositories

  # Python33 from SCL (www.softwarecollections.org)
  #yum_install centos-release-scl

  # Python PIP from Fedora EPEL (dl.fedoraproject.org)
  yum_install epel-release

  # Enable the CentOS debuginfo yum repo
  #sed -i '/enabled/d' /etc/yum.repos.d/CentOS-Debuginfo.repo
  #echo "enabled=1" >> /etc/yum.repos.d/CentOS-Debuginfo.repo

  # Install the MessageSight YUM repo file (disabled by default, enable later in linuxcommon function)
#cat << EOF >> /etc/yum.repos.d/imaserver.repo
## MessageSight YUM repository
#[imaserver]
#name=MessageSight YUM repository for Enterprise Linux 7 - $basearch
#baseurl=http://ms-seed.loadtest.internetofthings.ibmcloud.com/msrepo
#timeout=5
#retries=2
#metadata_expire=0
#metadata_expire_filter=never
#http_caching=none
#enabled=0
#gpgcheck=0
#keepcache=0
#EOF

}

function runyum {

  # First update existing packages (we have see cases in which docker and devicemapper packages get of sync)
  LOG "Run yum update..."
  while ! yum -y upgrade --skip-broken; do
      echo "yum upgrade failed.  We need to sleep and try again."
      sleep 30
  done

  # install additional packages (in batches of 10)
  LOG "Installing new packages..."
  yum_install "${packages[@]}"

  # start rsyslog so we have a /var/log/messages file on CentOS 7
  if [ "$CENTOSRHELVER" == "7" ] ; then
    systemctl enable rsyslog.service --now
  fi

  #  Setup Docker CE repo and install
  yum_install device-mapper-persistent-data
  yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
  yum_install docker-ce
  systemctl enable docker --now
  echo '{"dns": ["10.0.80.11"]}' >> /etc/docker/daemon.json

  # DebugInfo packages
  #debuginfo-install -y glibc
  #debuginfo-install -y openssl
  #debuginfo-install -y kernel-$(uname -r)

  # Don't need these packages
  yum   -y  remove avahi              \
                   postfix            \
                   mrmonitor
  # Download and install crowdstrike
  LOG "Downloading and installing Crowdstrike ..."
  wget http://${ERIC_SVTPVT_SEED}/rpms/crowdstrike-centos7-v.6003.rpm -O /root/crowdstrike.rpm
  yum -y install /root/crowdstrike.rpm
  /opt/CrowdStrike/falconctl -s --cid=20709802E00E4B4C9442CE5F8CA3E69D-9C
  systemctl enable falcon-sensor --now
  systemctl restart docker
}

# This could run on CentOS or Ubuntu (if we need to refactor this to a separate script)
function linuxcommon {
  LOG "Starting linuxcommon..."
  LOG "Adding lines to ssh config file pointing at ssh key"
  echo "Host 10.*" >> /root/.ssh/config
  echo "    ForwardX11Trusted yes" >> /root/.ssh/config
  echo "    User root" >> /root/.ssh/config
  echo "    IdentityFile /root/.ssh/id_harness_rsa" >> /root/.ssh/config

  LOG "Configuring core patterns and limits"
  echo "/var/messagesight/diag/cores/bt.%e" > /proc/sys/kernel/core_pattern
  echo "kernel.core_pattern = /tmp/cores/bt.%e" >> /etc/sysctl.d/ima-core-pattern.conf
  echo "kernel.core_pattern = /tmp/cores/bt.%e" >> /etc/sysctl.d/core.conf
  echo 10000000 > /proc/sys/fs/file-max
  echo 10000000 > /proc/sys/fs/nr_open
  sed -i '/^fs.file-max/d' /etc/sysctl.conf
  sed -i '/^fs.nr_open/d' /etc/sysctl.conf
  echo "fs.file-max = 10000000" >> /etc/sysctl.conf
  echo "fs.nr_open = 10000000" >> /etc/sysctl.conf
  echo "* soft core unlimited" >> /etc/security/limits.conf
  echo "* hard core unlimited" >> /etc/security/limits.conf
  echo "* soft rss unlimited" >> /etc/security/limits.conf
  echo "* soft rss unlimited" >> /etc/security/limits.conf
  echo "* hard nofile 30000" >> /etc/security/limits.conf
  echo "* hard nofile 30000" >> /etc/security/limits.conf
  echo "* hard data unlimited" >> /etc/security/limits.conf
  echo "* hard data unlimited" >> /etc/security/limits.conf
  echo "DefaultLimitCORE=infinity" >> /etc/systemd/system.conf
  systemctl daemon-reload


  # Install SoftLayer API client (have to install this after YUM installs
  #LOG "Install SoftLayer API client"
  #cd /root || exit; rm -rf pip ; git clone https://github.com/pypa/pip.git
  #easy_install --upgrade pip
  #rm -rf softlayer-python ; git clone https://github.com/softlayer/softlayer-python.git
  #easy_install --upgrade six
  #easy_install --upgrade importlib
  # new dependencies for slcli client (discovered 20170515)
  #easy_install --upgrade "click==5"
  #easy_install --upgrade "prettytable>=0.7.0"
  #easy_install --upgrade wcwidth
  #easy_install --upgrade "requests>=2.7.0"
  # end new deps for slcli client
  #easy_install --upgrade softlayer

  #LOG "Setting up Softlayer client API key file"
  #rm -f /root/.softlayer
  #echo "[softlayer]" 									>> /root/.softlayer
  #echo "username = afuser"   								>> /root/.softlayer
  #echo "endpoint_url = https://api.softlayer.com/xmlrpc/v3.1/"				>> /root/.softlayer

  # Softlayer API client 3.x -> 4.x broke backward compatibility, now we have to check version
  #sl --version 1>/dev/null 2>/dev/null ; rc=$?
  #if [ "$rc" != "0" ] ; then
  #  LOG "sl Softlayer API client is deprecated, using slcli"
  #  SLAPIC=slcli
  #else
  #  SLAPIC=sl
  #fi

  # Allow all users who are in the root group to be able to write to /var/log
  chmod -R 775 /var/log

  # Add harness pub key to authorized_keys for root
  wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/authorized_keys" -O "/root/authorized_keys"
  cat /root/authorized_keys >> /root/.ssh/authorized_keys

  LOG "Downloading utility scripts"

  # Copy .toprc file to /root
  wget http://${ERIC_SVTPVT_SEED}/postinstall/.toprc -O /root/.toprc

  # Copy pullperftools.sh to /root
  #wget http://${ERIC_SVTPVT_SEED}/postinstall/pullperftools.sh -O /root/pullperftools.sh
  #chmod 777 /root/pullperftools.sh

  # Add ssh keys to root account
  #wget http://${ERIC_SVTPVT_SEED}/postinstall/ssh/ltkey -O /root/.ssh/id_rsa ; chmod 600 /root/.ssh/id_rsa

  # I don't need an on-screen notification everytime a cron job sends mail to my local mailbox
  echo "unset MAILCHECK" >> /root/.bash_profile

  # Put up warning message about direct logins
#  cat << EOF >> /etc/profile.d/itcs104.sh
#  #!/bin/bash
#  if [[ -t 0 && "\$(logname)" == "\$USER" ]] ; then
#    echo "###############################################################################"
#    echo "IBM Cloud Security Policy states that:"
#    echo "Userids are not to be shared unless individual accountability (unique identity) can be maintained."
#    echo "Direct login as $USER contravenes this."
#    echo "See https://w3-connections.ibm.com/wikis/home?lang=en-us#!/wiki/Wbaca767aac15_4918_91ec_7d5014efde02/page/Security%20Links"
#    echo "to obtain a user id."
#    echo "################################################################################"
#    echo
#    echo "Please login using your own user id and SSH key, not directly as \$USER."
#    logger Direct login as \$USER from \$SSH_CONNECTION
#    echo "This event has been logged."
#    echo "Wait for 10 seconds to continue."
#    sleep 10
#
#fi
#EOF
#chmod +x /etc/profile.d/itcs104.sh


# Configure PS1
cat << EOF >> /etc/profile.d/ps1.sh
export PS1="[\u@\h \w]$ "
EOF

  # Setup MOTD to check for netfilter or iptables kernel modules
  #wget http://${ERIC_SVTPVT_SEED}/postinstall/motd.sh -O /etc/profile.d/motd.sh ; chmod +x /etc/profile.d/motd.sh

  # Lock down sshd to private network only
  LOG "Lock down sshd service to private network only"
  privip=$(ip addr | grep "inet 10\.[0-9]*\.[0-9]*\.[0-9]*" | tr -s ' ' | awk '{print $2}' | cut -d'/' -f1)
  # ping self-test
  ping -c1 "$privip" ; rc=$?
  if [ "$rc" == "0" ] ; then
    # change settings in sshd_config to improve performance or preent DOSes
    sed -i 's/#UseDNS yes/UseDNS no/' /etc/ssh/sshd_config
    # This setting prevents brute force ssh attacks from causing ssh disconnections
    sed -i 's/.*MaxStartups.*/MaxStartups 200/' /etc/ssh/sshd_config
    # Restrict password authentication to the private network
    # Remove all instances of PasswordAuthentication from the sshd_config file and append the following config block
    sed -i '/PasswordAuthentication/d' /etc/ssh/sshd_config
    sed -i '/Match LocalAddress/d' /etc/ssh/sshd_config
    sed -i '/PermitRootLogin/d' /etc/ssh/sshd_config

    if [ "$CENTOSRHELVER" == "7" ] ; then
      cat << EOF >> /etc/ssh/sshd_config
PermitRootLogin no
PasswordAuthentication no
Ciphers aes128-ctr,aes192-ctr,aes256-ctr,aes128-cbc,aes192-cbc,aes256-cbc
MACs hmac-sha1,umac-64@openssh.com,hmac-sha2-256,hmac-sha2-512,hmac-ripemd160,hmac-ripemd160@openssh.com
Match LocalAddress 10.47.65.43,184.*,10.*,50.23.*,173.193.*,127.0.0.1,32.97.110.*,70.122.16.*,76.23.218.42,195.212.29.*
	PermitRootLogin yes
EOF
      systemctl reload sshd
    fi
  fi

  # Disable host key checking on ssh clients
  LOG "Disable host key checking on ssh clients"
  # Remove all instances of StrictHostKeyChecking and UserKnownHostsFile from the ssh_config file and append config to the end
  sed -i "/StrictHostKeyChecking/d" /etc/ssh/ssh_config
  sed -i "/UserKnownHostsFile/d" /etc/ssh/ssh_config
  echo "StrictHostKeyChecking no" >> /etc/ssh/ssh_config
  echo "UserKnownHostsFile /dev/null" >> /etc/ssh/ssh_config

  # Open up sudoers, add /usr/local/bin to secure_path, and disable requiretty
  LOG "Open up /etc/sudoers"
  sed -i "/ALL ALL=(ALL) NOPASSWD:ALL/d" /etc/sudoers
  echo "ALL ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
  sed -i "/secure_path/d" /etc/sudoers
  sed -i "s/requiretty/\!requiretty/" /etc/sudoers
  echo "Defaults    secure_path = /sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin" >>  /etc/sudoers

  # Update screen (remove setgid to allow passing env vars e.g. LD_LIBRARY_PATH)
  chmod g-s $(which screen)
  chmod 777 /var/run/screen

  # Install cron job to update /etc/hosts periodically (every 20 mins) with device name mapping to public (hostname.pub) and private (hostname.priv) IP address
  #LOG "Installing cron job to populate /etc/hosts file with SoftLayer devices in the MessageSight account"
  #wget http://${ERIC_SVTPVT_SEED}/postinstall/getslhosts.sh -O /root/getslhosts.sh ; chmod +x /root/getslhosts.sh
  #/root/getslhosts.sh /etc/hosts
  #echo "*/20 * * * * /root/getslhosts.sh /etc/hosts" > cronjob
  #if [ -e /var/spool/cron/root ] ; then
  #  sed -i "/getslhosts/d" /var/spool/cron/root
  #fi
  #crontab -l -u root | cat - cronjob | crontab -u root -

  # Enable the imaserver YUM repository on seed (now that the host entry exists for seed)
  #sed -i 's/enabled=0/enabled=1/' /etc/yum.repos.d/imaserver.repo
  # motd.rb
  cat << EOF >> /etc/motd
************************************************************************
$(hostname)
************************************************************************
IBM's internal systems must only be used for conducting IBM's business
or for purposes authorized by IBM management
************************************************************************
EOF
  chmod 00644 /etc/motd
}

function thirdpartypkgs {
    LOG "Installing third party packages"
  
    # Install non-yum rpm packages from the "seed" machine
    LOG "Install rpm packages from the seed machine"
    wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/ibm-java-x86_64-jre-8.0-5.22.x86_64.rpm" -O "/root/ibm-java-x86_64-jre-8.0-5.22.x86_64.rpm"
    yum -y --nogpgcheck localinstall /root/ibm-java-x86_64-jre-8.0-5.22.x86_64.rpm
}

# OS tuning
function tuneos {
  LOG "Tuning a few things ..."

  if [ "$CENTOSRHELVER" == "7" ] ; then
    TUNEDIR=/root/tuning
    OSDIR=centos7

    # Copy latest tuning scripts from publishing server to $TUNEDIR directory
    #mkdir -p $TUNEDIR
	#wget --accept="*.sh,*.template,*.service,ifup-local" -nd -r --no-parent http://${ERIC_SVTPVT_SEED}/postinstall/tuning/ -P $TUNEDIR ; chmod +x $TUNEDIR/*.sh ; chmod +x $TUNEDIR/ifup-local

    # By default disable NAT networking option on docker daemon, removing this file and restarting tunehost service will restart docker
    # daemon with iptables option enabled
    #f [[ "$(hostname)" != *"loadclient"* ]] ; then
    #    touch /etc/.dockernonat
    #fi

    # Install boot script for tuning CentOS.
    #LOG "Installing CentOS${CENTOSRHELVER} tuning script in systemd ..."

    #cp -f $TUNEDIR/tunehost.service /etc/systemd/system
    #systemctl daemon-reload
    #systemctl enable tunehost
    #systemctl start tunehost

    # Tune kernel modules
    # NOTE: Several modules may be in use and require a reboot to reload (reboot is at the end of this script)
    #$TUNEDIR/tunemods.sh

    # Update kernel boot parameters in GRUB2
    # The noop I/O scheduler seems to be the recommended I/O scheduler for performance, from several tuning articles. We have seen problems with CFQ.
    # For large scale environments increasing the TCP connection hash table size improves connection lookup significantly.  The default is too small.
	# NOTE: Updating GRUB2 requires a reboot (reboot is at the end of this script)
	SYSTEMGB=$(( ( $(getconf PAGESIZE) * $(getconf _PHYS_PAGES) ) / (1024 * 1024 * 1000) ))
	# 32K hash table entries per GB of system memory
	THASH_ENTRIES=$((SYSTEMGB * 32768))
    grubby --update-kernel=DEFAULT --args="elevator=noop thash_entries=$THASH_ENTRIES"

    # Disable SE Linux
    #sed -i 's/SELINUX=permissive/SELINUX=disabled/' /etc/sysconfig/selinux
    #sed -i 's/SELINUX=permissive/SELINUX=disabled/' /etc/selinux/config

  elif [ "$CENTOSRHELVER" == "6" ] ; then
    echo "Currently does not handle tuning for CentOS 6, skip tuning..."
  fi
}

function staf_setup {
  wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/ssh_config" -O "/root/.ssh/config"
  wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/Dockerfile" -O "/root/Dockerfile"
  privip=$(ip addr | grep "inet 10\.[0-9]*\.[0-9]*\.[0-9]*" | tr -s ' ' | awk '{print $2}' | cut -d'/' -f1)
  afhostname=$(hostname)
  afshorthost=$(hostname | cut -d"." -f1)
  echo "$privip     $afhostname $afshorthost" >> /etc/hosts
  echo "10.73.131.202    AFharness7.softlayer.com AFharness7" >> /etc/hosts
  echo "10.77.207.24    MessageSightPublish.softlayer.com MessageSightPublish" >> /etc/hosts
  mkdir -p /staf
  mkdir -p /gsacache
  LOG "Adding line to /etc/fstab for mounting staf ..."
  echo "10.73.131.202:/staf /staf  nfs     rsize=8192,wsize=8192,timeo=14,intr" >> /etc/fstab
  echo "10.77.207.24:/gsacache /gsacache  nfs     rsize=8192,wsize=8192,timeo=14,intr 0 0" >> /etc/fstab
  LOG "Mounting /staf ..."
  mount -a
  LOG "Downloading the /etc/systemd/system/staf.service file ..."
  wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/staf.service" -O "/etc/systemd/system/staf.service"
  LOG "Reloading systemd ..."
  systemctl daemon-reload
  if [ -f "/staf/STAFEnv.sh" ]; then
      echo "/staf is already mounted. We can continue."
      LOG "/staf is already mounted. We can continue."
      LOG "Starting and enabling STAF service ..."
      systemctl enable staf.service --now
      sleep 3
      if systemctl is-active staf.service; then
          echo "STAF service is active."
          LOG "STAF service is active."
      else
          echo "STAF service is not active."
          LOG "STAF service is not active."
      fi
  else
      echo "./staf is not mounted."
      LOG "./staf is not mounted."
  fi
  #wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/bootstrap-fvt-monitoring.sh" -O "/root/bootstrap-fvt-monitoring-test.sh"
  #chmod 755 /root/bootstrap-fvt-monitoring-test.sh
  #wget "http://${ERIC_SVTPVT_SEED}/postinstall/fvt/ima-af-monitoring-test.service" -O "/etc/systemd/system/ima-af-monitoring-test.service"
  #systemctl enable ima-af-monitoring-test.service --now
}

# Set DNS entries (there are bugs in CentOS 7 that wipe them out, so we are doing this as a workaround until CentOS 7.2)
function setdns {
  LOG "Setting SoftLayer DNS entries  "

  BASENET=/etc/sysconfig/network
  sed -i '/DNS/d' /etc/sysconfig/network
  sed -i '/RES_OPTIONS/d' /etc/sysconfig/network
  echo "DNS1=10.0.80.11" >> $BASENET
  echo "DNS2=10.0.80.12" >> $BASENET

# We should not have to do this, there is a bug in CentOS 7 (https://bugzilla.redhat.com/show_bug.cgi?id=1212883) that wipes /etc/resolv.conf
# This should be fixed in RHEL 7.2, but for now this is a workaround.  The correct way to do this is to set DNS entries in /etc/sysconfig/network,
# but that is what is broken
  rm -f /etc/resolv.conf
  echo "# Set DNS @ $(date) by bootstrap"    >> /etc/resolv.conf
  echo "nameserver 10.0.80.11"  			 >> /etc/resolv.conf
  echo "nameserver 10.0.80.12"  			 >> /etc/resolv.conf

  # NOTE - 20170821 - We are now running dnsmasq from a container this section of bootstrap is deprecated
  #echo "***********************************"
  #echo "*** Enable dnsmasq "
  #echo "***********************************"
  #echo "listen-address=127.0.0.1" >> /etc/dnsmasq.conf
  #echo "bind-interfaces"          >> /etc/dnsmasq.conf
  # Add address= lines for dns entries in dnsmasq.conf (examples below)
  #echo "address=/.wdc04.ibm-iot-wdc.openstack.blueboxgrid.com/169.45.234.149"               >> /etc/dnsmasq.conf
  #echo "address=/.messaging.wdc04.ibm-iot-wdc.openstack.blueboxgrid.com/169.45.240.165"     >> /etc/dnsmasq.conf
  #echo "address=/.testdriver.wdc04.ibm-iot-wdc.openstack.blueboxgrid.com/169.45.240.167"    >> /etc/dnsmasq.conf
  #systemctl daemon-reload
  #systemctl enable dnsmasq
  #systemctl start dnsmasq
}

# Secure the system from itcs104 recipe
function secure {
  LOG "Securing system                "
  # faillog.rb (create faillog)
  touch /var/log/faillog ; chmod 00600 /var/log/faillog

  # full-path.rb (insert HOME=/)
  sed -i 's/.*MAILTO.*/&\nHOME=\//' /etc/cron.d/0hourly

  # IBMSinit.rb
  cat << EOF >>  /etc/profile.d/IBMsinit.sh
#!/bin/bash
# This file enforces IBM required environment settings
# Contents may not be globally overridden but you can do so on a per user basis

if [ \$UID -gt 199 ]; then
  umask 077
fi

# The following must be uncommented if processing data subject to HIPPA
#TMOUT=1800
#export TMOUT
EOF
  chmod 00755 /etc/profile.d/IBMsinit.sh

  cat << EOF >>  /etc/profile.d/IBMsinit.csh
if (\$uid > 199) then
umask 077
endif
EOF
  chmod 00755 /etc/profile.d/IBMsinit.csh

  # motd.rb
  cat << EOF >> /etc/motd
************************************************************************
 $(hostname)
************************************************************************
 IBM's internal systems must only be used for conducting IBM's business
 or for purposes authorized by IBM management
************************************************************************
EOF
  chmod 00644 /etc/motd

  # syslog.rb
  touch /var/log/tallylog ; chmod 00600 /var/log/tallylog


  # Restrict NTP to private network
  LOG "Lock down ntpd service to private network only"
  privip=`ip addr | grep "inet 10\.[0-9]*\.[0-9]*\.[0-9]*" | tr -s ' ' | awk '{print $2}' | cut -d'/' -f1`
  # ping self-test
  ping -c1 $privip ; rc=$?
  if [ "$rc" == "0" ] ; then
    sed -i '/interface ignore wildcard/d' /etc/ntp.conf
    echo "interface ignore wildcard" >> /etc/ntp.conf
    sed -i '/interface listen/d' /etc/ntp.conf
    echo "interface listen $privip" >> /etc/ntp.conf
	sed -i '/server time.service.networklayer/d' /etc/ntp.conf
	echo "server time.service.networklayer.com iburst prefer" >> /etc/ntp.conf
  fi

  # Enable firewalld and configure zones
  systemctl enable firewalld --now
  firewall-cmd --zone=public --change-interface=eth1
  firewall-cmd --zone=internal --change-interface=eth0
  firewall-cmd --permanent --zone=internal --add-port "6500/tcp"
  firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="173.193.115.137" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv4" source address="10.47.65.43" accept'
  firewall-cmd --permanent --zone=internal --add-rich-rule='rule family="ipv4" source address="10.47.65.43" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv4" source address="10.73.131.202" accept'
  firewall-cmd --permanent --zone=internal --add-rich-rule='rule family="ipv4" source address="10.73.131.202" accept'
  firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="184.173.54.133" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv4" source address="10.77.207.24" accept'
  firewall-cmd --permanent --zone=internal --add-rich-rule='rule family="ipv4" source address="10.77.207.24" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv4" source address="10.0.0.0/8" accept'
  firewall-cmd --permanent --zone=internal --add-rich-rule='rule family="ipv4" source address="10.0.0.0/8" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv6" source address="2607:f0d0:1202::/48" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv6" source address="2607:f0d0:2702:e3::0/64" accept'
  firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv6" source address="2607:f0d0:2701:18b::0/64" accept'
  for port in 1883 4323 6636 8080 8883 9088 9089 9090 9443 16102 17001 18468 18490 18500 19500 20029 21450 21453 21454 21460 21461 21470 21471 21472 21473 25000 33000 33001 55986 55988 55990 56090 56092 56094 56134 56188 56196 56198 56336 56340 56342; do
    firewall-cmd --permanent --zone=internal --add-port "${port}/tcp"
  done
  # Remove public ssh access
  firewall-cmd --zone=public --remove-service=ssh
  firewall-cmd --reload
}

# Updates for enabling docker registry comms
function dockerupdates {
  LOG "Downloading docker scripts"
}

# Setup and configure diamond system stats collector
function setupdiamond {
  LOG "Setting up diamond system stats collectors "

  pip2 install diamond
  mkdir -p /var/log/diamond
  mkdir -p /etc/diamond
  wget http://${ERIC_SVTPVT_SEED}/postinstall/diamond/diamond.conf -O /etc/diamond/diamond.conf

  # Randomize metric interval between 30-50 seconds for diamond stats
  INTERVAL=$(( (RANDOM % 20) + 30 ))
  sed -i "s/interval = 30/interval = $INTERVAL/g" /etc/diamond/diamond.conf

  wget http://${ERIC_SVTPVT_SEED}/postinstall/diamond/chkdiamond.sh -O /root/chkdiamond.sh
  chmod +x /root/chkdiamond.sh
  echo "*/5 * * * * /root/chkdiamond.sh" >> /var/spool/cron/root
}

# If the system contains a secondary disk set it up as a store disk, otherwise /store will simply be a directory on the boot disk
function storedisk {
  LOG "Checking system for a secondary disk"

  # For some reason SoftLayer Virtual Servers use the second disk for a swap file (/dev/xvdb), so the users "2nd disk" is actually the 3rd disk
  VS_DISK2=/dev/xvdc
  BM_DISK2=/dev/sdb
  SL_DISK1=/disk1
  SL_PART1=/dev/sdb1

  if [ "$CENTOSRHELVER" == "6" ] ; then
    STOREFS=ext4
  else
    STOREFS=xfs
  fi

  # Check if SoftLayer has already formatted the second disk
  if [ -e "${SL_DISK1}" ] ; then
    LOG "Removing existing mount point for SoftLayer provisioned secondary disk."
    sed -i '/disk1/d' /etc/fstab
    umount ${SL_DISK1}
    rm -rf ${SL_DISK1}
  fi

  # Check if a partition exists on second disk
  if [ -e "${SL_PART1}" ] ; then
    LOG "Removing existing partition on the secondary disk."
    parted ${BM_DISK2} rm 1
  fi

  # Create MessageSight and Docker host directories
  LOG "Creating MessageSight directories"
  mkdir -p /var/messagesight/config
  mkdir -p /var/messagesight/diag
  mkdir -p /var/messagesight/store
  mkdir -p /var/crash ; chmod 777 /var/crash


  if [ -b "${VS_DISK2}" ] ; then STOREDISK=${VS_DISK2} ; fi
  if [ -b "${BM_DISK2}" ] ; then STOREDISK=${BM_DISK2} ; fi
  if [ -b "${STOREDISK}" ] ; then
    LOG "Creating store partition on the secondary disk."
    parted -s -a optimal ${STOREDISK} mklabel gpt
    parted -s -a optimal ${STOREDISK} -- mkpart primary $STOREFS 1 -1

    mkfs.$STOREFS -f "${STOREDISK}1"
    sed -i '/messagesight/d' /etc/fstab
    echo "${STOREDISK}1 /var/messagesight/store $STOREFS defaults 0 0" >> /etc/fstab
    mount /var/messagesight/store
  fi
}

###########
## Main ###
###########
CURRDIR=$(pwd)

# In case these commands are interactive, disable this
unalias rm mv cp 2>/dev/null

chattr -i /root/.ssh/authorized_keys
chattr -i /root/authorized_keys

config_drive        2>&1 | tee -a $LOGFILE
installrepos        2>&1 | tee -a $LOGFILE
runyum              2>&1 | tee -a $LOGFILE
thirdpartypkgs      2>&1 | tee -a $LOGFILE
staf_setup          2>&1 | tee -a $LOGFILE
linuxcommon         2>&1 | tee -a $LOGFILE
#storedisk           2>&1 | tee -a $LOGFILE
tuneos	            2>&1 | tee -a $LOGFILE
setdns              2>&1 | tee -a $LOGFILE
secure	  	        2>&1 | tee -a $LOGFILE
#dockerupdates       2>&1 | tee -a $LOGFILE
#setupdiamond        2>&1 | tee -a $LOGFILE

#LOG "Going to reboot now in order for GRUB2 boot parameters to take effect..." 2>&1 | tee -a $LOGFILE

LOG "Making sure /etc/resolv.conf does not contain 127.0.0.1."
sed -i '/nameserver 127.0.0.1/d' /etc/resolv.conf
sed -i '/options single-request/d' /etc/resolv.conf


chmod 600 /root/.ssh/authorized_keys
#chattr +i /root/.ssh/authorized_keys

LOG "Touching provisioned file ..."
touch /.provisioned

systemctl restart docker
docker pull centos:latest


exit 0
