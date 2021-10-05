# TPUT.FANIN.MQTT

 This benchmark consists of a series of throughput tests which measure peak message rate
 from many MQTT publishing clients to a backend MQTT consuming application. The MQTT
 consuming application also consists of multiple MQTT subscribers. In addition to peak 
 message rate, the end-to-end (publisher to subscriber) message latency is also measured.
 
 Publishers are MQTT v5 clients which connect over the public network to port 8883 over TLS
 (i.e. the public internet facing endpoint). Backend MQTT subscribers are MQTT v5 clients which 
 connect over the private network to port 16901 over non-TLS (i.e. the private intranet facing 
 endpoint).
 
 Number of Publishers:
 * 1K, 10K, 50K, 100K, 250K, 500K, 1M
 
 Number of Backend MQTT Subscribers:
 * 1, 10, 100, 1K

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
 This variant of the TPUT.FANIN.MQTT benchmark has the following characteristics:
 * QoS 1 (non-durable client sessions for publishing and subscribing clients, i.e. no session expiry is specified)
 * The topic tree is partitioned into non-overlapping segments.  
 * For each partition an administrative subscription is created and for each administrative subscription 
 there are multiple shared subscribers.

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system:   
  * 384GB Memory
  * 18 IPv4 addresses
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
For each variant of the benchmark follow the steps below.
* 1M publishers to 1 subscribers
* 1M publishers to 10 subscribers
* 1M publishers to 100 subscribers
* 1M publishers to 1K subscribers
* 500K publishers to 10 subscribers
* 500K publishers to 100 subscribers
* ...

```
1. Generate the client list for mqttbench, passing the number of publishers, the number of subscribers, and 
the comma separated list of public and private IP addresses/DNS names of the MessageSight server(s) to connect 
to, for example
   
   python ./genClientList-XPubber-YSubber.py --numPubber 1M
                                             --numSubber 10
                                             --publicNetDestList W.X.Y.Z 
                                             --privateNetDestList A.B.C.D

2. For each message size listed above, execute the run script for the combination of publishers and subscribers  
	./run-XPubber-YSubber.sh <numPubber> <numSubber> <msg size in bytes>

   For example, run the following tests for 1M publishers and 10 subscribers:
   ./run-XPubber-YSubber.sh 1M 10 64
   ./run-XPubber-YSubber.sh 1M 10 256
   ./run-XPubber-YSubber.sh 1M 10 1024
   ./run-XPubber-YSubber.sh 1M 10 4096
   ./run-XPubber-YSubber.sh 1M 10 16834
   ./run-XPubber-YSubber.sh 1M 10 65536
   ./run-XPubber-YSubber.sh 1M 10 262144

3. For each message size gradually increase the message rate until you have reached the peak sustainable message rate without message loss (i.e. 0 discarded messages at the peak message rate for 10 minutes)
   
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
