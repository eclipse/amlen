# Installing and Configuring MessageSight HA pairs for testing
Run the following steps on both the PRIMARY and STANDBY MessageSight system
- Login and pull the performance benchmark tools from the MessageSight release server: `~/pullperftools.sh`
- Logout and Login again to run the .profile script which prepares the performance benchmarking environment
- Install the MessageSight RPM and start the MessageSight System-D service: `sudo ~/workspace/perftools/benchmarks/msinstall.sh`
- Record the IPv4 addresses of the **bond0** and **bond1** interfaces on both the PRIMARY and STANDBY MessageSight servers, this is needed for configuring the MessageSight servers

On the PRIMARY MessageSight system

- Configure the PRIMARY and STANDBY MessageSight servers for performance benchmarking: 
`python initialMSSetup.py --adminEndpoint <primary bond0 IP> --adminEndpointStandby <standby bond0 IP> --primaryReplicaIntf <primary bond0 IP> --standbyReplicaIntf <standby bond0 IP> --primaryDiscoveryIntf <primary bond1 IP> --standbyDiscoveryIntf <standby bond1 IP>`

# Installing and Setting up LoadTest systems for testing
Run the following steps on the each load system
- Login and pull the performance benchmark tools from the MessageSight release server: `~/pullperftools.sh`
- Logout and Login again to run the .profile script which prepares the performance benchmarking environment

# Manually configuring additional IPv4 addresses on LoadTest systems
If the additional IPv4 addresses were ordered separately from the LoadTest system follow the steps below to manually configure
the additional IPv4 addresses on the public network interface, bond1
- For each IPv4 address in the portable public subnet, add the following configuration into the `/etc/sysconfig/network-scripts/ifcfg-bond1` config file.  Where X below is a number from 1 to N, N being the number of IP addresses required to run the test.  18 IP addresses should be sufficient to run any of the benchmark tests.

```
IPADDRX=W.X.Y.Z
NETMASKX=A.B.C.D
```
- restart the network subsystem to pick up the new IP addresses `sudo systemctl restart network`
- confirm the IP addresses have been added using the command `ip addr show dev bond1`

