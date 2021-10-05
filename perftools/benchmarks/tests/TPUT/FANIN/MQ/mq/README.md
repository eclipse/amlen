# Installing and Configuring MQ Server for MessageSight Performance benchmark testing
As user `root`, perform the following steps to install and configure MQ server on each MQ QMgr system used in 
the MessageSight performance benchmarking.

## Download the MQ Server code
Download the code from https://public.dhe.ibm.com/ibmdl/export/pub/software/websphere/messaging/mqadv/mqadv_dev910_linux_x86-64.tar.gz

* mkdir ~/mq
* cd ~/mq
* wget https://public.dhe.ibm.com/ibmdl/export/pub/software/websphere/messaging/mqadv/mqadv_dev910_linux_x86-64.tar.gz

## Install the MQ Server code
Accept the license and install the MQ code

* tar zxvf mqadv_dev910_linux_x86-64.tar.gz
* cd MQServer
* ./mqlicense.sh -accept
* yum install MQSeriesRuntime-9.1.0-0.x86_64.rpm MQSeriesServer-9.1.0-0.x86_64.rpm MQSeriesClient-9.1.0-0.x86_64.rpm

## Create and Configure MQ QMgr(s)
Run the following script to setup and configure the MQ QMgr server.  This script will check for an existing MQ QMgr and delete
it, if it exists. It will then create the MQ QMgr and configure it for benchmark testing.  This script can be run multiple
times on the same system.

```
./crtqms.sh ~/perfqm-data
```

NOTE: if the user running this script is not already a member of the mqm user group you will have to run the script twice.
Once to add the current user to the mqm group, logout, login, and the second time to create the QMgrs.

## Add a DNS 'A' record for each MQ system
From the SoftLayer portal navigate to DNS Forward zones and add a new `A` record for each MQ QMgr system in the load test DNS forward zone
.

For example, PerfQMgr on machine X with private IP address A.B.C.D

DNS `A` record: 
* HOST: ms-qmgr0.<load test DNS forward zone>
* POINTS TO: A.B.C.D

For PerfQMgr on machine Y with private IP address L.M.N.O

DNS `A` record: 
* HOST: ms-qmgr1.<load test DNS forward zone>
* POINTS TO: L.M.N.O

... and so on (one DNS `A` record for each MQ QMgr system)

## Ensure that MessageSight MQConnectivity service is configured
When setting up MessageSight using the `initialMSSetup.py` script, ensure the `--configMQConn`, `--numQMgr`, and
`--numConnPerQMgr` command line parameters are passed to the script and match the number of MQ QMgrs configured for the 
benchmark.  For example,

python initialMSSetup.py --adminEndpoint W.X.Y.Z --configMQConn --numQMgr 5 --numConnPerQMgr 10

## Copy /root/pullperftools.sh to /home/mqm and run it
The MQ benchmark client runs in the same memory space as the MQ QMgr (which runs as user 'mqm') and as such the MQ benchmark client
must also be run as user 'mqm'.

As `root`
* cp /root/pullperftools.sh ~mqm
* cp ~/.ssh/id_rsa ~mqm/.ssh/id_rsa
* chown mqm:mqm ~mqm/.ssh/id_rsa
* su - mqm
* ./pullperftools.sh

## Next Steps
Proceed to the README.md for each QoS to test (e.g. QOS0/README.md) for the next steps in performing the benchmark testing.

