# TPUT.BROADCAST

 This benchmark consists of a series of throughput tests which measure peak message rate
 from a single MQTT publishing client to a large number of MQTT subscribers. In addition to 
 peak message rate, the end-to-end (publisher to subscriber) message latency is also measured.
 
 The publisher publishes messages to a single topic and all MQTT subscribers subscribe to that
 topic, as such, the message is delivered to all subscribers.
 
 The publisher is an MQTT v5 client which connects over the private network to port 16901 over 
 non-TLS (i.e. the private intranet facing endpoint). The MQTT subscribers are MQTT v5 clients 
 which connect over the public network to port 8883 over TLS (i.e. the public internet facing 
 endpoint). 
 
 NOTE: This benchmark requires a 10GbE network connection.
 
 Number of MQTT backend Publishers:
 * 1, 10
 
 Number of Topics:
 * 1, 10
 
 Number of MQTT Subscribers:
 * 1K, 10K, 50K, 100K, 500K, 1M

 Message Sizes (bytes):
 * 64, 512, 4096

 TLS Protocol (subscribers only):
 * 1.3

 Ciphers (subscribers only):
 * v1.2 - ECDHE-RSA-AES256-GCM-SHA384
 * v1.3 - TLS_AES_256_GCM_SHA384
 
 Key Size (subscribers only):
 * 2048 bits
 
# Variant Info
 This variant of the TPUT.BROADCAST benchmark has the following characteristics:
 * QoS 2 (durable client sessions for publishing and subscribing clients, i.e. session expiry is set to a non-zero value)

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system:   
  * 384GB Memory
  * 18 IPv4 addresses
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
For each variant of the benchmark follow the steps below.
* 1 publisher, 1 topic, and 1K subscribers
* 1 publisher, 1 topic, and 10K subscribers
* 1 publisher, 1 topic, and 50K subscribers
* 1 publisher, 1 topic, and 100K subscribers
* 1 publisher, 1 topic, and 500K subscribers
* 1 publisher, 1 topic, and 1M subscribers
* 10 publishers, 10 topics, and 1K subscribers
* 10 publishers, 10 topics, and 10K subscribers

```
1. Generate the client list for mqttbench, passing the number of publishers, number of subscribers, number of
topics to publish to, and the comma separated list of public and private IP addresses/DNS names of the
MessageSight server(s) to connect to, for example
   
   python ./genClientList-XPubber-YSubber.py --numPubber 1
                                             --numSubber 1K
                                             --numTopics 1
                                             --publicNetDestList W.X.Y.Z 
                                             --privateNetDestList A.B.C.D

2. For each message size listed above, execute the run script for the combination of publisher, subscribers, 
and topics  
	./run-XPubber-YSubber.sh <numPubber> <numSubber> <numTopics> <msg size in bytes>

   For example, run the following tests for 1 publisher, 1K subscribers, and 1 topic:
   ./run-XPubber-YSubber.sh 1 1K 1 64
   ./run-XPubber-YSubber.sh 1 1K 1 512
   ./run-XPubber-YSubber.sh 1 1K 1 4096

3. For each message size gradually increase the message rate until you have reached the peak sustainable 
message rate without message loss (i.e. 0 discarded messages at the peak message rate for 10 minutes).
   
NOTE: closely monitor network bandwidth utilization during this test, as network bandwidth is likely to be the
the bottleneck in this workload.
```

Monitor test from https://<hostname of Graphite relay>:8443
  - SystemHealth stats
  - MessageSight stats
  - mqttbench stats

# Metrics to collect for the performance report
- End to end message latency (min, avg, 50P, 75P, 90P, 95P, 99P, max)
- Peak message rate (msgs/sec) and data rate (Gbps) from MessageSight to subscribers
- Total time to deliver the broadcast message to all subscribers.  This should be equal to the max message latency.

# Analysis to perform
- Determine which side is the bottleneck: mqttbench or Messagesight. **netstat -tnp | grep <port number>** is often a helpful to
  determine which side of a connection is not reading fast enough.
- What is the bottleneck on MessageSight: CPU, disk I/O, network I/O, Memory? Use the SystemHealth stats Grafana dashboard to assist in
  making this determination.  
- If CPU is the bottleneck: Use **top -p `ps -C imaserver -o pid=`** to determine which threads are busiest in the imaserver process.
  Then use **perf top -t <tid>** to analyze in which function most of the CPU time is being spent in the busiest thread.
- Review MessageSight source code and propose an optimization
