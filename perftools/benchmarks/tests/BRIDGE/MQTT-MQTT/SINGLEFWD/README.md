# BRIDGE.SINGLEFWD

 This benchmark consists of a series of throughput tests using the MessageSight bridge to forward
 messages from a source MessageSight server to a destination MessageSight server (same server in
 this benchmark) and perform a topic mapping from source topics to a destination topic.
 deployed on systems with varying CPU counts. This is achieved by provisioning a system
 with a large CPU count and disabling CPUs in multiples of 2, ensuring that CPUs being disabled
 belong to the same core (i.e. hyperthreads) and thus are disabled together.
 
 The throughput tests measure peak message rate from many MQTT publishing clients to a 
 backend MQTT consuming application. The MQTT consuming application also consists of 
 multiple MQTT subscribers. In addition to peak message rate, the end-to-end (publisher to subscriber) 
 message latency is also measured.
 
 Publishers are MQTT v5 clients which connect over the public network to port 8883 over TLS
 (i.e. the public internet facing endpoint). Backend MQTT subscribers are MQTT v5 clients which 
 connect over the private network to port 16901 over non-TLS (i.e. the private intranet facing 
 endpoint).
 
 MessageSight Bridge features under test:
 * MQTT server to MQTT server forwarding
 * Topic mapping
 * Message selection
 
 CPU counts:
 * 4, 8, 16, 24, 32, 40, 48, 56
 
 Number of Publishers:
 * 10K, 1M
 
 Number of Backend MQTT Subscribers:
 * 100

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
 This variant of the TPUT.FANIN.MQTT benchmark has the following characteristics:
 * QoS 0 (non-durable client sessions for publishing and subscribing clients, i.e. no session expiry is specified)
 * The topic tree is partitioned into non-overlapping segments.  Each partition is subscribed to by a single shared
 subscription and there are multiple shared subscribers per shared subscription.

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system:   
  * 384GB Memory
  * 18 IPv4 addresses
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
For each variant of the benchmark follow the steps below.
* 1M publishers to 100 subscribers, 56 CPU MessageSight server
* 10K publishers to 100 subscribers, 56 CPU MessageSight server
* 1M publishers to 100 subscribers, 48 CPU MessageSight server
* 10K publishers to 100 subscribers, 48 CPU MessageSight server
* ...

```
1. Generate the client list for mqttbench, passing the number of publishers, the number of subscribers, and
the comma separated list of public and private IP addresses/DNS names of the MessageSight server(s) to connect 
to, for example
   
   python ./genClientList-XPubber-YSubber.py --numPubber 1M
                                             --numSubber 100
                                             --publicNetDestList W.X.Y.Z 
                                             --privateNetDestList A.B.C.D

2. Disable CPUs on the PRIMARY AND STANDBY systems to achieve the desired number of CPUs for each test.  For example, the following 
command disables CPUs 4 thru 27 and 32 thru 55. In the example below, 4 and 32 are sibling hyperthreads on the
same core. It is best to disable CPUs in pairs, which are hyperthreads.

for i in `seq 4 27` `seq 32 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done


NOTE: Each time the CPU count is changed the MessageSight server must be restarted so that it can re-calibrate to
the new CPU count.  To restart the MessageSight server issue the command `systemctl restart
IBMIoTMessageSightServer`.  Be sure to change the CPU count and restart the STANDBY server first, followed by the
PRIMARY. A final failover can be issued so that the original PRIMARY becomes PRIMARY again.

3. For each message size listed above, execute the run script for the combination of publishers and subscribers  
   ./run-XPubber-YSubber.sh <numPubber> <numSubber> <msg size in bytes> <# of CPU on MessageSight server>

   For example, run the following tests for 1M publishers and 100 subscribers and 56 CPU:
   ./run-XPubber-YSubber.sh 1M 100 64 56
   ./run-XPubber-YSubber.sh 1M 100 4096 56
   ./run-XPubber-YSubber.sh 1M 100 262144 56
   
4. For each message size gradually increase the message rate until you have reached the peak sustainable message
rate without message loss (i.e. 0 discarded messages at the peak message rate for 10 minutes)
```

# CPU Counts
For the Intel Xeon Broadwell E5-2690 v4

* 48 CPUs

```
 for i in `seq 24 27` `seq 52 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
```

* 40 CPUs

```
 for i in `seq 20 27` `seq 48 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
```

* 32 CPUs

```
 for i in `seq 16 27` `seq 44 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
```

* 24 CPUs

```
 for i in `seq 12 27` `seq 40 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
```

* 16 CPUs

```
 for i in `seq 8 27` `seq 36 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
```

* 8 CPUs

```
 for i in `seq 4 27` `seq 32 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
```

* 4 CPUs

```
 for i in `seq 2 27` `seq 30 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart IBMIoTMessageSightServer
 sudo /root/tuning/setirqs.sh
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
