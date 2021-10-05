# Ordering MessageSight baremetal systems in SoftLayer
SoftLayer monthly baremetal ordering specifications:

- CPU: 			Dual Intel Xeon E5-2690 v4 (28 cores, 2.6 GHz)
- Memory:		512 GB (16 x 32GB DDR4 2Rx4)
- Disk:     	4 x 1.7TB SanDisk CloudSpeed 1000 SSD (RAID 10 configuration)
- Bandwidth:	Take the default
- Network:  	10GbE Redundant Public and Private Network Uplinks (SuperMicro AOC-UR-I4XTF-P)
- OS:       	CentOS 7.5
- PostInstall:	SVTPVT_PROVISIONING
- SSH Key:      perfkey

# Ordering LoadTest baremetal systems in SoftLayer
SoftLayer monthly baremetal ordering specifications:

- CPU: 			Dual Intel Xeon SkyLake Gold 5120 (28 cores, 2.2 GHz)
- Memory:		384 GB (12 x 32GB DDR4 2Rx4)
- Disk:     	1TB SATA (Seagate Constellation ES)
- Bandwidth:	Take the default
- Network:  	10GbE Redundant Public and Private Network Uplinks (SuperMicro AOC-UR-I4XTF-P)
- IPv4:     	32 additional public secondary IPv4 addresses
- OS:       	CentOS 7.5
- PostInstall:	SVTPVT_PROVISIONING
- SSH Key:      perfkey

# Ordering LDAP baremetal systems in SoftLayer
SoftLayer monthly baremetal ordering specifications:

- CPU: 			Dual Intel Xeon SkyLake Gold 5120 (28 cores, 2.2 GHz)
- Memory:		64 GB
- Disk:     	1 x 960GB SSD 
- Bandwidth:	Take the default
- Network:  	10GbE Redundant Public and Private Network Uplinks (SuperMicro AOC-UR-I4XTF-P)
- OS:       	CentOS 7.5
- PostInstall:	SVTPVT_PROVISIONING
- SSH Key:      perfkey

# Ordering OAuth baremetal systems in SoftLayer
SoftLayer monthly baremetal ordering specifications:

- CPU: 			Dual Intel Xeon SkyLake Gold 5120 (28 cores, 2.2 GHz)
- Memory:		64 GB
- Disk:     	1 x 960GB SSD 
- Bandwidth:	Take the default
- Network:  	10GbE Redundant Public and Private Network Uplinks (SuperMicro AOC-UR-I4XTF-P)
- OS:       	CentOS 7.5
- PostInstall:	SVTPVT_PROVISIONING
- SSH Key:      perfkey

# SoftLayer PostInstall script for Performance systems
The output from the post install script is written to `/root/bootstrap-centos.log`. At the end of the log you should see an entry indicating the system is
about to be rebooted. This is how you know that the postinstall script completed.

```
 Going to reboot now in order for GRUB2 boot parameters to take effect...
```

NOTE: Occasionally, SoftLayer may interrupt the postinstall script while it is setting up the system and it may be necessary to restart the post install script.
To do this simply login as root on the newly provisioned system and restart the script.  For example, `/root/post_install.<random chars>`.  

# Ordering Public Secondary IPv4 addresses for LoadTest systems in SoftLayer
NOTE: ADDITIONAL IPV4 ADDRESSES ARE ONLY REQUIRED FOR LOAD SYSTEMS THAT ARE RUNNING MQTTBENCH

The preferred method of ordering additional public IPv4 addresses for load systems is to order them with the load system. If this option is not available
you can order a public portable subnet from the IBM SoftLayer portable by following the steps below. 
- From the IBM SoftLayer portal, identify the VLAN number of the public network subnet in which the LoadTest system was provisioned, and record it.
- From https://control.softlayer.com/network/subnets/order, order a block of `32 Portable Public IP Addresses` in the VLAN identified above.
