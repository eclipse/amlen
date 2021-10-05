# TPUT.FANIN.MQ

 This benchmark consists of a series of throughput tests which measure peak message rate
 from many MQTT publishing clients to a backend MQ application. Messages are forwarded to a
 set of MQ QMgrs and subsequently consumed by the backend MQ application. The MQ application
 uses the MQ FASTPATH bindings for high performance communication between the MQ QMgrs and the
 MQ application consuming the messages. In addition to peak message rate, the end-to-end 
 (publisher to subscriber) message latency is also measured.
 
 Publishers are MQTT v5 clients which connect over the public network to port 8883 over TLS
 (i.e. the public internet facing endpoint). The MQ Connectivity service in MessageSight connects
 to MQ QMgrs over a non-TLS MQ server connection channel.
 
 Number of MQ QMgrs:
 * 1, 2, 3, 4, 5
 
 Number of Connections per MQ QMgr:
 * 1, 10
 
 Number of Publishers:
 * 1K, 10K, 100K, 1M
 
 Number of Backend MQ consumers:
 * 1, 10

 Message Sizes (bytes):
 * 64, 4096, 262144

 TLS Protocol (publishers only):
 * 1.3

 Ciphers (publishers only):
 * v1.2 - ECDHE-RSA-AES256-GCM-SHA384
 * v1.3 - TLS_AES_256_GCM_SHA384
 
 Key Size (publishers only):
 * 2048 bits
 
# Variant Info
 This variant of the TPUT.FANIN.MQ benchmark has the following characteristics:
 * QoS 0 (non-durable client sessions for publishing clients, i.e. no session expiry is specified)
 * The backend MQ consuming application uses MQ FASTPATH bindings to connect to MQ QMgrs, receiving from a non-persistent queue

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system #1: MQTT Publisher load test system
  * 384GB Memory
  * 18 IPv4 addresses
  
Load test system #2 - N: MQ QMgr and MQ application load test system
  * 32GB Memory
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
* Setup/configure the MQ QMgrs used in the test by running the mq/crtqms.sh script on all the MQ QMgr systems 
involved in the test (see mq/README.md for details).

```   
For QoS 0 benchmarks run the following command on each MQ QMgr system: ./crtqms.sh ~/perfqm-data
```

For each variant of the benchmark follow the steps below.
* 1M publishers to 1 MQ subscriber, using 1 QMgr and 1 connection per QMgr (from MessageSight)
* 1M publishers to 1 MQ subscriber, using 1 QMgr and 10 connections per QMgr (from MessageSight)
* 1M publishers to 10 MQ subscribers, using 1 QMgr and 1 connection per QMgr (from MessageSight)
* 1M publishers to 10 MQ subscribers, using 1 QMgr and 10 connection per QMgr (from MessageSight)
* 1M publishers to 1 MQ subscribers, using 2 QMgr and 1 connections per QMgr (from MessageSight)
* 1M publishers to 1 MQ subscribers, using 2 QMgr and 10 connections per QMgr (from MessageSight)
* 1M publishers to 10 MQ subscribers, using 2 QMgr and 1 connection per QMgr (from MessageSight)
* 1M publishers to 10 MQ subscribers, using 2 QMgr and 10 connections per QMgr (from MessageSight)
* 1M publishers to 1 MQ subscribers, using 3 QMgr and 1 connections per QMgr (from MessageSight)
* 1M publishers to 1 MQ subscribers, using 3 QMgr and 10 connection per QMgr (from MessageSight)
* 1M publishers to 10 MQ subscribers, using 3 QMgr and 1 connections per QMgr (from MessageSight)
* ...

```
1. Setup the MessageSight MQ connectivity service with the MQ QMgr connection configuration and destination mapping rules required
to run the test. NOTE: this will reset all MessageSight configuration back to "factory defaults" before reconfiguring MessageSight

python initialMSSetup.py --adminEndpoint W.X.Y.Z --configMQConn --numQMgr X --numConnPerQMgr Y

where: 
	X is the number MQ QMgr systems used in the test
	Y is the number of connections per MQ QMgr in the test   
 
2. Generate the client list for mqttbench, passing the number of publishers and the comma separated list of 
public IP addresses/DNS names of the MessageSight server(s) to connect to, for example
   
   python ./genClientList.py --numPubber 1M
                             --publicNetDestList W.X.Y.Z 

3. For each message size listed above, execute the MQ consumer run script followed by the MQTT run script, for the 
combination of publishers and subscribers  

	MQ QMgr System #1 (as user 'mqm')
	./run-MQ-Consumers.sh <numQMgrs> <numConnPerQMgr> <numPubber> <numSubber> <msg size in bytes>

	Load test system #2
	./run-MQTT-Publishers.sh <numQMgrs> <numConnPerQMgr> <numPubber> <numSubber> <msg size in bytes>

   For example, run the following tests for:
     - 1 MQ QMgr
     - 1 Connection per MQ QMgr
     - 1M MQTT publishers
     - 1 MQ backend subscriber

   MQ QMgr System #1:   ./run-MQ-Consumers.sh    1 1 1M 1 64
   Load test system #2: ./run-MQTT-Publishers.sh 1 1 1M 1 64  
   
   MQ QMgr System #1:   ./run-MQ-Consumers.sh    1 1 1M 1 4096
   Load test system #2: ./run-MQTT-Publishers.sh 1 1 1M 1 4096
   
   MQ QMgr System #1:   ./run-MQ-Consumers.sh    1 1 1M 1 262144
   Load test system #2: ./run-MQTT-Publishers.sh 1 1 1M 1 262144
   
4. For each message size gradually increase the message rate (on load test system #2) until you have 
reached the peak sustainable message rate without message loss (i.e. 0 discarded messages at the peak 
message rate for 10 minutes)
   
```

Monitor test from https://<hostname of Graphite relay>:8443
  - SystemHealth stats
  - MessageSight stats
  - mqttbench stats

# Metrics to collect for the performance report
- End to end message latency (min, avg, 50P, 75P, 90P, 95P, 99P)
- Max sustained (minimum of 10 minutes) message rate with 0 messages discarded from the subscription queue(s) and an 
average (over a ten minute period) message latency of less than 500 milliseconds and 
individual cpu utilization no less than 15% idle (or more than 85% busy) https://<hostname of Graphite relay>:8443/dashboard/db/systemhealth-stats?refresh=10s&panelId=2&fullscreen&orgId=1

# Analysis to perform
- Determine which side is the bottleneck: mqttbench or Messagesight. **netstat -tnp | grep <port number>** is often a helpful to
  determine which side of a connection is not reading fast enough.
- What is the bottleneck on MessageSight: CPU, disk I/O, network I/O, Memory? Use the SystemHealth stats Grafana dashboard to assist in
  making this determination.  
- If CPU is the bottleneck: Use **top -p `ps -C imaserver -o pid=`** to determine which threads are busiest in the imaserver process.
  Then use **perf top -t <tid>** to analyze in which function most of the CPU time is being spent in the busiest thread.
- Review MessageSight source code and propose an optimization
