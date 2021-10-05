# MQTTV5.TOPICALIASES

 This benchmark consists of a series of throughput tests which measure peak message rate
 from multiple MQTT publishing clients to multiple MQTT subscribing clients. In each test
 a different topic string length and message payload size is used. This test is
 intended to measure the effect of using MQTTV5 topic aliases on message rate and message latency. 
  
 Publishers are MQTT v5 clients which connect over the public network to port 1885
 (i.e. a non-secure public internet facing endpoint). MQTT subscribers are MQTT v5 clients which 
 connect over the private network to port 16901 over non-TLS (i.e. the private intranet facing 
 endpoint).
 
 Length of topic string (in bytes)
 * 10, 20, 30, 40, 50, 100, 200
 
 Number of Publishers:
 * 1K
 
 Number of Backend MQTT Subscribers:
 * 1K

 Message Sizes (bytes):
 * 64, 1024, 2048

 QoS:
 * 0

 Topic Aliases:
 * enabled, disabled

# HW requirements:
MessageSight system:  
  * 16GB Memory

Load test system:   
  * 16GB Memory
  * 1 IPv4 address
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
* For each topic string length

```
1. Generate the client list for mqttbench, passing a boolean to indicate whether topic aliases
are enabled/disabled, the length of the topic string (in bytes), and the comma separated list 
of public and private IP addresses/DNS names of the MessageSight server(s) to connect to, for example

# Topic Aliases (disabled)   
python ./genClientList.py --enableAliases False
								 --topicLength 10
                          --publicNetDestList W.X.Y.Z 
                          --privateNetDestList A.B.C.D

# Topic Aliases (enabled)
python ./genClientList.py --enableAliases True
								 --topicLength 10
                          --publicNetDestList W.X.Y.Z 
                          --privateNetDestList A.B.C.D
	
2. Execute the `run.sh` script to start the test, passing the length of the topic string
(in bytes), the message size (in bytes) to test, and <True|False> to indicate whether or not
to use topic aliases as parameters to the run.sh script, for example

# Topic Aliases (disabled)
   ./run.sh 10 64    False
   ./run.sh 10 1024  False
   ./run.sh 10 2048  False
# Topic Aliases (enabled)   
   ./run.sh 10 64    True
   ./run.sh 10 1024  True
   ./run.sh 10 2048  True

3. Gradually increase the message rate until you have reached the peak 
sustainable message rate without message loss (i.e. 0 discarded messages 
at the peak message rate for 10 minutes)

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
