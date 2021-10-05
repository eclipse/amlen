# SCALE.VERTICAL.CONNBURST.TLS-LDAPAUTH

 This benchmark consists of a series of connection rate tests against MessageSight servers
 deployed on systems with varying CPU counts. This is achieved by provisioning a system
 with a large CPU count and disabling CPUs in multiples of 2, ensuring that CPUs being disabled
 belong to the same core (i.e. hyperthreads) and thus are disabled together. 
 
 MQTT v5 clients connect to port 8886 over TLS and clients are authenticated using LDAP user
 authentication.  Clients connect with non-durable sessions (i.e. cleanStart = True and no session 
 expiry is specified). No subscriptions are made and no messages are published during this 
 benchmark, i.e. it is purely a connection rate and connection latency benchmark.

 CPU counts:
 * 4, 8, 16, 24, 32, 40, 48, 56

 TLS Protocol:
 * 1.2

 Ciphers:
 * TLS_AES_128_GCM_SHA256
 
 Key Size:
 * 2048 bits

# HW requirements:
MessageSight system:  
  * 512GB Memory

Load test system:   
  * 384GB Memory
  * 18 IPv4 addresses
  
LDAP server system:
  * 64GB Memory
                             
For details on ordering and setting up systems to run this test, refer to the `stacks` documentation

# LDAP Server Setup
- Perform setup steps in tests/CONNBURST/TLS-LDAPAUTH/openldap/README.md to setup LDAP server
- Add the `ms-ldap` DNS A record to the load test DNS forward zone in the SoftLayer Load test account.
- Enable LDAP user authentication on MessageSight

```
python ../../../CONNBURST/TLS-LDAPAUTH/toggleLDAP.py --adminEndpoint A.B.C.D --ldapAuth True
```
                                                        
# Steps
Generate the client list for mqttbench, passing the comma separated list of public IP addresses/DNS names of the MessageSight server(s)
to connect to `python ./genClientList.py --destlist <list of MS server IP/DNS name>`

Disable CPUs on the system to achieve the desired number of CPUs for each test.  For example, the following command
disables CPUs 4 thru 27 and 32 thru 55. In the example below, 4 and 32 are sibling hyperthreads on the same core. It is
best to disable CPUs in pairs, which are hyperthreads.

```
for i in `seq 4 27` `seq 32 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
```

NOTE: Each time the CPU count is changed the MessageSight server must be restarted so that it can re-calibrate to the new CPU count.  To
restart the MessageSight server issue the command `systemctl restart imaserver`.  Be sure to change the CPU count
and restart the STANDBY server first, followed by the PRIMARY. A final failover can be issued so that the original PRIMARY becomes PRIMARY
again.

Each test has its own run script and will produce a different set of metrics. These tests will periodically
disconnect and reconnect all clients.  

# CPU Counts

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-56CPU

* invoke the `./run-56CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-48CPU

```
 for i in `seq 24 27` `seq 52 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-48CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-40CPU

```
 for i in `seq 20 27` `seq 48 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-40CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-32CPU

```
 for i in `seq 16 27` `seq 44 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-32CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-24CPU

```
 for i in `seq 12 27` `seq 40 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-24CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-16CPU

```
 for i in `seq 8 27` `seq 36 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-16CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-8CPU

```
 for i in `seq 4 27` `seq 32 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-8CPU.sh` script to start the test.

SCALE-VERTICAL-CONNBURST-TLS-LDAPAUTH-4CPU

```
 for i in `seq 2 27` `seq 30 55` ; do echo 0 > /sys/devices/system/cpu/cpu$i/online ; done
 systemctl restart imaserver
 sudo /root/tuning/setirqs.sh
```
* invoke the `./run-4CPU.sh` script to start the test.

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
