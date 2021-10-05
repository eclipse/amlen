#!/bin/bash

# -----------------------------------------------------------------------------------------------------
# This script is invoked on login and sets up the users environment.  The origin of this script
# is $HOME/workspace/perftools/benchmarks.  It is copied to $HOME whenever pullperftools.sh is invoked.
# -----------------------------------------------------------------------------------------------------

# Pickup aliases and functions from system bash defaults
if [ -f ~/.bashrc ]; then
  source ~/.bashrc
fi

# I don't need an on-screen notification everytime a cron job sends mail to my local mailbox
unset MAILCHECK
export HISTSIZE=5000

# In case these commands are interactive, disable this
unalias rm mv cp 2>/dev/null

SLHOME=$HOME/workspace/perftools/softlayer
BMHOME=$HOME/workspace/perftools/benchmarks

# Setup user base environment
if [ -e $BMHOME/baseenv.sh ] ; then
  echo "Setting up user base environment (source $BMHOME/baseenv.sh)..."
  source $BMHOME/baseenv.sh
fi

# Setup user test specific environment.  These environment variables are test specific and are not necessarily checked into RTC
if [ -e $HOME/svtpvtenv.sh ] ; then
  echo "Setting up user SVT/PVT environment (source $HOME/svtpvtenv.sh)..."
  source $HOME/svtpvtenv.sh    
fi

# Setup user aliases 
if [ -e $BMHOME/alias.sh ] ; then
  echo "Setting up aliases (source $BMHOME/alias.sh)..."
  source $BMHOME/alias.sh
fi

# --------------------------------------------------------------------------------------------------------------
# Tuning for test systems only (most, but not all, tuning is done at boot up from the /root/tunehost.sh script)
# --------------------------------------------------------------------------------------------------------------
# Max processes per user (some test applications create many processes)
ulimit -u unlimited

# For some reason we have to chmod /var/run/screen more than once
sudo chmod 777 /var/run/screen

echo
echo -e "Current local workspace (\$WSHOME): \e[5m`readlink workspace`\e[m"

cd ~/workspace/perftools/benchmarks

