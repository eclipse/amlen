Install MQ
1. Check is previous install already there
rpm -qa | grep MQ
(Mar400 has v7.1 in /opt/mqm_v7.1/)

2. Check - Preparing the System in MQ Infocenter - Additional Settings page for Kernel changes
pic.dhe.ibm.com/infocenter/wmqv7/v7r5/index.jsp

3. InfoCenter:  Installing Components - Linux
Accept the MQ License
	./mqlicense.sh -accept
if previous MQ install, run
	./crtmqpkg 750   (750 suffix and be any A-Z, 0-9)
then cd to the directory specified to install the renamed packages i.e. /var/tmp/mq_rpms/750/x86_64
Install MQ Packages:   
root@mar400 /var/tmp/mq_rpms/750/x86_64> rpm --prefix /opt/mqm_v7.5 -ivh MQSeriesS*.rpm MQSeriesR*.rpm MQSeriesXR*.rpm MQSer
iesClient*.rpm MQSeriesJ*.rpm MQSeriesE*.rpm MQSeriesG*.rpm MQSeriesMan*.rpm

Installs:
Preparing...                ########################################### [100%]
   1:MQSeriesRuntime_750    ########################################### [  8%]
   2:MQSeriesJRE_750        ########################################### [ 17%]
   3:MQSeriesServer_750     ########################################### [ 25%]
   4:MQSeriesJava_750       ########################################### [ 33%]
   5:MQSeriesSDK_750        ########################################### [ 42%]
   6:MQSeriesSamples_750    ########################################### [ 50%]
   7:MQSeriesXRClients_750  ########################################### [ 58%]
   8:MQSeriesXRService_750  ########################################### [ 67%]
   9:MQSeriesClient_750     ########################################### [ 75%]
  10:MQSeriesExplorer_750   ########################################### [ 83%]
  11:MQSeriesGSKit_750      ########################################### [ 92%]
  12:MQSeriesMan_750        ########################################### [100%]
  
4. Set this install as the primary install:  (/opt/mqm_v7.5 is the value passed in --prefix of rpm cmd)
/opt/mqm_v7.5/bin/setmqinst -i -p /opt/mqm_v7.5

root@mar400 /opt/mqm_v7.5/bin> ./setmqinst -i -p /opt/mqm_v7.5
118 of 118 tasks have been completed successfuly.
'Installation2' (/opt/mqm_v7.5) set as the Primary Installation.


. /opt/mqm_v7.5/bin/setmqenv -s -k
	NOTE:  crtmqenv -s -k gave ELF error

5. Verify the install
As mqm group user
	. /opt/mqm_v7.5/bin/setmqenv -s
	dspmqver

mqm@mar400:~> . /opt/mqm_v7.5/bin/setmqenv  -s
mqm@mar400:~> dspmqver
Name:        WebSphere MQ
Version:     7.5.0.0
Level:       p000-L120604
BuildType:   IKAP - (Production)
Platform:    WebSphere MQ for Linux (x86-64 platform)
Mode:        64-bit
O/S:         Linux 2.6.32.12-0.7-default
InstName:    Installation2
InstDesc:
InstPath:    /opt/mqm_v7.5
DataPath:    /var/mqm
Primary:     Yes
MaxCmdLevel: 750	

	
------------------------
Step 1 : Create Default Queue manager
------------------------
mquser@mar231:~> crtmqm -q SVTBRIDGE.QUEUE.MANAGER
WebSphere MQ queue manager created.
Directory '/var/mqm/qmgrs/SVTBRIDGE!QUEUE!MANAGER' created.
The queue manager is associated with installation 'Installation1'.
Creating or replacing default objects for queue manager 'SVTBRIDGE.QUEUE.MANAGER'.
Default objects statistics : 71 created. 0 replaced. 0 failed.
Completing setup.
Setup completed.

------------------------
Step x : Start QUEUE manager
------------------------
mquser@mar231:~> strmqm
WebSphere MQ queue manager 'SVTBRIDGE.QUEUE.MANAGER' starting.
The queue manager is associated with installation 'Installation1'.
5 log records accessed on queue manager 'SVTBRIDGE.QUEUE.MANAGER' during the log replay phase.
Log replay for queue manager 'SVTBRIDGE.QUEUE.MANAGER' complete.
Transaction manager state recovered for queue manager 'SVTBRIDGE.QUEUE.MANAGER'.
WebSphere MQ queue manager 'SVTBRIDGE.QUEUE.MANAGER' started using V7.1.0.0.

------------------------
Step x : 
------------------------
mquser@mar231:~> runmqsc
Starting MQSC for queue manager SVTBRIDGE.QUEUE.MANAGER.

------------------------
Step x : 
------------------------

define qlocal (BRIDGE2MQ)
     5 : define qlocal (BRIDGE2MQ)

define qlocal (MQ2BRIDGE)
     9 : define qlocal (MQ2BRIDGE)
AMQ8006: WebSphere MQ queue created.

------------------------
Step x :  define listeners
OPEN: Do we need two different listeners on two separate ports?
------------------------

     3 : define listener (listener1) trptype (tcp) control (qmgr) port (16102)
AMQ8626: WebSphere MQ listener created.

     define listener (listener2) trptype (tcp) control (qmgr) port (16103)

------------------------
Step x :  start listeners
OPEN: Do we need two different listeners on two separate ports?
------------------------

       : 
start listener (listener2)
     4 : start listener (listener1)
AMQ8021: Request to start WebSphere MQ listener accepted.


------------------------
Step x :  define channels
OPEN: Do we need two different channels ? I did that.
------------------------

10 : define channel (BRIDGE2MQ.CHANNEL) chltype (SVRCONN) trptype (tcp)
AMQ8014: WebSphere MQ channel created.


11 : define channel (MQ2BRIDGE.CHANNEL)  chltype (SVRCONN) trptype (tcp)
AMQ8014: WebSphere MQ channel created.


------------------------
Step x :  disable channel authentication. (workaround)
TODO: for better security figure out how to authorize access for
mquser to the channel.
------------------------

    12 : ALTER QMGR CHLAUTH (DISABLED)
AMQ8005: WebSphere MQ queue manager changed.




