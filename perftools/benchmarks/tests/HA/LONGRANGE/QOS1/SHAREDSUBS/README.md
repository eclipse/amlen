# HA.LONGRANGE

 This benchmark consists of a series of throughput tests which measure peak message rate
 from many MQTT publishing clients to a backend MQTT consuming application. The MQTT
 consuming application also consists of multiple MQTT subscribers. These tests are intended
 to measure the effect of large network latency between the PRIMARY and STANDBY members of
 an MessageSight HA pair on message rate and end-to-end (publisher to subscriber) message latency.
 
 Publishers are MQTT v5 clients which connect over the public network to port 8883 over TLS
 (i.e. the public internet facing endpoint). Backend MQTT subscribers are MQTT v5 clients which 
 connect over the private network to port 16901 over non-TLS (i.e. the private intranet facing 
 endpoint).
 
 SoftLayer Data Centers:
 * PRIMARY in: dal12
 * STANDBY in: dal12, wdc06, lon06
 
 Number of Publishers:
 * 100K, 1M
 
 Number of Backend MQTT Subscribers:
 * 1K

 Message Sizes (bytes):
 * 64, 1024, 4096, 65536

 TLS Protocol (publishers only):
 * 1.3

 Ciphers (publishers only):
 * v1.2 - ECDHE-RSA-AES256-GCM-SHA384
 * v1.3 - TLS_AES_256_GCM_SHA384
 
 Key Size (publishers only):
 * 2048 bits
 
# Variant Info
 This variant of the HA.LONGRANGE benchmark has the following characteristics:
 * QoS 1 (durable client sessions for publishing and subscribing clients, i.e. session expiry is set to a non-zero value)
 * The backend consuming application uses MQTT v5 shared subscriptions

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system:   
  * 384GB Memory
  * 18 IPv4 addresses
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
For each variant of the benchmark follow the steps below.
* 1M publishers to 1K subscribers, STANDBY MessageSight server in dal12
* 1M publishers to 1K subscribers, STANDBY MessageSight server in wdc06
* 1M publishers to 1K subscribers, STANDBY MessageSight server in lon06
* 100K publishers to 1K subscribers, STANDBY MessageSight server in dal12
* 100K publishers to 1K subscribers, STANDBY MessageSight server in wdc06
* 100K publishers to 1K subscribers, STANDBY MessageSight server in lon06

```
1. Generate the client list for mqttbench, passing the number of publishers, the number of subscribers, and 
the comma separated list of public and private IP addresses/DNS names of the MessageSight server(s) to connect 
to, for example
   
   python ./genClientList-XPubber-YSubber.py --numPubber 1M
                                             --numSubber 1K
                                             --publicNetDestList W.X.Y.Z 
                                             --privateNetDestList A.B.C.D

2. For each message size listed above, execute the run script for the combination of publishers and subscribers  
	./run-XPubber-YSubber.sh <numPubber> <numSubber> <msg size in bytes> <standby DC>

   For example, run the following tests for 1M publishers and 1 subscriber:
   ./run-XPubber-YSubber.sh 1M 1K 64 dal12
   ./run-XPubber-YSubber.sh 1M 1K 1024 dal12
   ./run-XPubber-YSubber.sh 1M 1K 4096 dal12
   ./run-XPubber-YSubber.sh 1M 1K 65536 dal12

3. For each message size gradually increase the message rate until you have reached the peak sustainable message rate without message loss (i.e. 0 discarded messages at the peak message rate for 10 minutes)
   
4. At the end of each test clean up the persistent client state by passing the "deletestate" parameter
to the run script.
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
