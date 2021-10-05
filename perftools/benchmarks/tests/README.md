# General Information about performing MessageSight benchmarks
The information contained within this README is general background information useful to those who are new to performing MessageSight performance benchmarks.

# Benchmark test tools
There are several test tools used in MessageSight performance benchmark testing.
* mqttbench - generates MQTT load, see mqttbench "Getting Started Guide" for details on using the tool
* jmsbench - generates JMS load
* mqbench - generates MQ client load (for MQConnectivity load tests)

# Changing the message rate in mqttbench
From the interactive command shell, enter the command `rate=<new rate>`, where <new rate> is in units of messages per second. The message
rate can be less than 1, for example 0.1 will result in sending 1 msg every 10 seconds.

# QoS 2 testing
mqttbench does not persist state to disk so if during a QoS 2 benchmark you abruptly quit without waiting for all the ACKS to be sent and received and messages delivered it will be necessary to delete any persistent subscriptions created by the test before restarting it since the state is lost.  This can be done by reconnecting with clean start/session True and a session expiry of 0 (for MQTT v5) OR by cleaning the MessageSight store.

# Rate steps
Determining peak message rate during a benchmark test can be tricky and time consuming. If you have a rough idea of what the peak message rate should be for a particular test then in order to save time you should increase the message rate in steps as follows.
* First rate step 50% of expected peak message rate
* Second rate step 75% of expected peak message rate
* Third rate step 85% of expected peak message rate
* Fourth rate step 95% of expected peak message rate
* Fifth rate step 100% of expected peak message rate

The above is a general rule of thumb and may not be the optimal rate step algorithm for all workloads.  Once you have reached the peak message rate the test should be allowed to run for at least 10 minutes to ensure it is a sustainable rate.

# MessageSight File Flips
For QoS 1 and 2 persistent messaging tests the MessageSight file flip can present a bit of a challenge for determining the peak message rate.  MessageSight implements batching of multiple message store transactions into a single disk write and the batching algorithm requires the file backed buffers to rotate or "flip" when they become full. When this file flip occurs the message store is locked for a brief period of time (on the order of a couple seconds), while the message store is locked new messages are buffered in the receive socket buffers and when the store is unlocked a large burst of messages is processed resulting in the message rate graphs showing a large spike in processed messages after the file flip. It is important to monitor the subscription queues to ensure no messages are discarded, especially immediately after file flips.

# Statistics to monitor while running performance benchmarks
Pay attention to the following metrics during performance testing.

* CPU utilization (both individual CPUs and in aggregate) on the MessageSight system and load generation systems
* Network I/O utilization 
    - Small message testing - pay attention to packet rate limits (>500K pps per NIC is a high packet rate) 
    - Large message testing - pay attention to bandwidth limits  
* Good network interrupt distribution (look at output from the `top` command and verify the %si column is NOT high on only CPU 0)
* Disk I/O utilization becomes important to monitor during QoS 1 and 2 persistent messaging tests, this can become the bottleneck.
* MessageSight monitoring REST API statistics 
    - Buffered Messages Percent HWM (high water mark)
    - Discarded Messages
* During FANIN throughput tests pay attention to receive message rate in mqttbench statistics. If it becomes too "bursty" this can be a symptom that message rate is too high.
	     
