# CONNBURST.TLS.CLIENTCERT

 This benchmark initiates 1M MQTT v5 client connections to port 8884 over TLS.
 The clients send a client certificate for authentication by the MessageSight
 server.  The MessageSight server must verify the client cert against 10K
 revoked certificates in a certificate revocation list associated with the certificate
 authority that issued the client certificate.  All clients send the same client certificate. 
 The clients connect with non-durable sessions (i.e. cleanStart = True and no session expiry 
 is specified). No subscriptions are made and no messages are published during this benchmark, 
 i.e. it is purely a connection rate and connection latency benchmark.

 TLS Protocol:
 * 1.2

 Ciphers:
 * TLS_AES_256_GCM_SHA384
 
 Server and Client Key Size:
 * 2048 bits

# HW requirements:
MessageSight system:  
  * 512 GB Memory

Load test system:   
  * 96 GB Memory
  * 18 IPv4 addresses
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation
                                                        
# Steps
- Generate the client list for mqttbench, passing the comma separated list of public IP addresses/DNS names of the MessageSight server(s)
  to connect to `python ./genClientList.py --destlist <list of MS server IP/DNS name>`
- Invoke the `./run.sh` script to start the test.  The test will periodically disconnect and reconnect all clients.
- Let the test run for at least an hour

Monitor test from https://<hostname of Graphite relay>:8443
  - SystemHealth stats
  - MessageSight stats
  - mqttbench stats

# Metrics to collect for the performance report
- Connection Latency (min, avg, 50P, 75P, 90P, 95P, 99P)
- Peak connection rate (conn/sec) while maintaining average connection latency less than 500 milliseconds and 
individual cpu utilization no less than 15% idle (or more than 85% busy) https://<hostname of Graphite relay>:8443/dashboard/db/systemhealth-stats?refresh=10s&panelId=2&fullscreen&orgId=1

# Analysis to perform
- Determine which side is the bottleneck: mqttbench or Messagesight. **netstat -tnp | grep <port number>** is often a helpful to
  determine which side of a connection is not reading fast enough.
- What is the bottleneck on MessageSight: CPU, disk I/O, network I/O, Memory? Use the SystemHealth stats Grafana dashboard to assist in
  making this determination.  
- If CPU is the bottleneck: Use **top -p `ps -C imaserver -o pid=`** to determine which threads are busiest in the imaserver process.
  Then use **perf top -t <tid>** to analyze in which function most of the CPU time is being spent in the busiest thread.
- Review MessageSight source code and propose an optimization
