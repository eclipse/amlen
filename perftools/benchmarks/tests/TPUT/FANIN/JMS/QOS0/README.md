# TPUT.FANIN.JMS

 This benchmark consists of a series of throughput tests which measure peak message rate
 from many MQTT publishing clients to a backend JMS consuming application. The JMS
 consuming application also consists of multiple JMS subscribers. In addition to peak 
 message rate, the end-to-end (publisher to subscriber) message latency is also measured.
 
 Publishers are MQTT v5 clients which connect over the public network to port 8883 over TLS
 (i.e. the public internet facing endpoint). Backend JMS subscribers connect over the private 
 network to port 16901 over non-TLS (i.e. the private intranet facing endpoint).
 
 Number of Publishers:
 * 1K, 10K, 100K, 1M
 
 Number of Backend JMS Subscribers:
 * 1, 10, 100

 Message Sizes (bytes):
 * 64, 256, 1024, 4096, 16384, 65536, 262144

 TLS Protocol (publishers only):
 * 1.3

 Ciphers (publishers only):
 * v1.2 - ECDHE-RSA-AES256-GCM-SHA384
 * v1.3 - TLS_AES_256_GCM_SHA384
 
 Key Size (publishers only):
 * 2048 bits
 
# Variant Info
 This variant of the TPUT.FANIN.JMS benchmark has the following characteristics:
 * QoS 0 (non-durable client sessions for publishing and subscribing clients, i.e. no session expiry is specified)
 * The backend consuming application uses JMS shared subscriptions

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system #1: MQTT Publisher load test system
  * 384GB Memory
  * 18 IPv4 addresses
  
Load test system #2: JMS application load test system
  * 32GB Memory
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
For each variant of the benchmark follow the steps below.
* 1M publishers to 1 subscriber
* 1M publishers to 10 subscribers
* 1M publishers to 100 subscribers
* 100K publishers to 1 subscriber
* 100K publishers to 10 subscribers
* 100K publishers to 100 subscribers
* ...

```
1. Generate the client list for mqttbench, passing the number of publishers and the comma separated list of 
public IP addresses/DNS names of the MessageSight server(s) to connect to, for example
   
   python ./genClientList.py --numPubber 1M
                             --publicNetDestList W.X.Y.Z 

2. For each message size listed above, execute the JMS consumer run script followed by the MQTT run script, for the 
combination of publishers and subscribers  

	Load test system #1
	./run-JMS-Consumers.sh <private IP/DNS name of MessageSight server> <numPubber> <numSubber> <msg size in bytes>

	Load test system #2
	./run-MQTT-Publishers.sh <numPubber> <numSubber> <msg size in bytes>

   For example, run the following tests for 1M MQTT publishers and 1 JMS subscriber:
   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 64
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 64
	
   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 256
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 256

   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 1024
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 1024
	
   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 4096
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 4096
	
   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 16384
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 16384

   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 65536
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 65536
   
   Load test system #1: ./run-JMS-Consumers.sh A.B.C.D 1M 1 262144
   Load test system #2: ./run-MQTT-Publishers.sh 1M 1 262144
   
3. For each message size gradually increase the message rate (on load test system #2) until you have 
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
