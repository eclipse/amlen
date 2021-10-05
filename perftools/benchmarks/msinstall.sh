#!/bin/bash

# -----------------------------------------------------------------------------------------------------
# This script installs MessageSight and starts the MessageSight System-D service
#
# -----------------------------------------------------------------------------------------------------

BASEDIR=`dirname "$0"`

echo "Installing MessageSight Server"
sleep 1
sudo yum install -y IBMWIoTPMessageGatewayServer.x86_64

echo "Enabling and starting MessageSight service in System-D"
sudo systemctl enable imaserver
sudo systemctl start imaserver
sleep 2

echo "Starting MessageSight monagent"
# Kill any existing monagent processes
for pid in `ps -C "python ms-monagent.py" -o pid=` ; do sudo kill -9 $pid; done

# Copy monagent and install cron job
sudo cp -f $BASEDIR/ms-monagent.py /usr/bin
sudo cp -f $BASEDIR/chkMsMonagent.sh /usr/bin
crontab -l > cronjobs
sed -i "/chkMsMonagent/d" cronjobs
echo "*/5 * * * * sudo /usr/bin/chkMsMonagent.sh" >> cronjobs
crontab cronjobs
rm cronjobs
    
    
