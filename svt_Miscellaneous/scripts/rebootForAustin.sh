#!/usr/bin/expect
### To be used for automation Reconfigure after a DEVICE RESET of IMA appliance
###
### derived from PXEinstall.sh from Frank's Automation Setup
###
### The following prerequisites are required prior to running the script:
###
### - ipmitool and expect must be installed on the linux machine where this script is run.
###
### - The linux machine used to execute this script should be behind the BSO
#######################################################################################################################################################

#### Verify arguement count 
if {[llength $argv] < 4} {
  send_user "\nUsage:   ./rebootForAustin.sh  immIP  mgt0IP  mgt0NM  httpDIR  \[testPW\]\n\n"
  send_user "Where:    immIP   = The IMM IP of the 9006 that is to be PXE installed (ie. 9.3.174.XX)\n"
  send_user "          mgt0IP  = The IP address to configure the mgt0 interface (ie. 9.3.177.XX)\n"
  send_user "          mgt0NM  = The Netmask to use for the above mgt0 interface (ie. /22)\n"
  send_user "Optional: testPW  = The current lab password for test machines (If provided, will configure passwordless ssh on appliance with Automation Framework SSH keys)\n\n\n"
  send_user "Example: ./rebootForAustin.sh  9.3.174.XX  9.3.177.XX  /22  \[password\]\n\n\n"
  exit 1
}

set immIP   [lindex $argv 0]
set mgt0IP  [lindex $argv 1]
set mgt0NM  [lindex $argv 2]

if {[llength $argv] == 4} {
  set testPW  [lindex $argv 3]
}

set timeout 20
set staxIP   9.3.179.67

#### Determine default gateway and HTTP IP to use
if { [string match "9.3.177.*" $mgt0IP] == 1 } {
  set mgt0GW 9.3.177
}
if { [string match "9.3.179.*" $mgt0IP] == 1 } {
  set mgt0GW 9.3.179
}

### Deactivate the ipmitool serial console session (in case it is already active)
spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate

send_user "\n\n########################################\n"
send_user "Verifying chassis power is on for $immIP."
send_user "\n########################################\n"
spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD power on

sleep 2
spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD power status
expect {
   timeout { send_user "\nFailed to get power status of $immIP\n"; exit 1 }
   "Chassis Power is on"
}


# REBOOT TIMEOUT
set timeout 7200

spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol activate
send_user "\n\n########################################\n"
send_user "Waiting on Login prompt\n"
send_user "########################################\n"
sleep 2
send "\r"
sleep 2
expect {
   timeout { send_user "\n\n ERROR, appliance did NOT reboot to login prompt.\n $expect_out(buffer)\n\n";  exit 1 }
   "*(none) login:" { send_user "\n Rebooted to LOGIN Prompt successfully.\n"; }
}


set timeout 360

send_user "\n\n########################################\n"
send_user "Log in as admin/admin on $immIP."
send_user "\n########################################\n"
send "admin\r"
sleep 2
send "admin\r"
sleep 2
expect {
  timeout { send_user "\nFailed to login as admin/admin on $immIP\n $expect_out(buffer)\n\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*Which interface would you like to configure (default is mgt0):"
}

set timeout 120

send_user "\n\n########################################\n"
send_user "Configuring mgt0 network interface with IP=$mgt0IP, Netmask=$mgt0NM, and Gateway=$mgt0GW.1\n"
send_user "########################################\n"
sleep 2
send "\r"
sleep 2
send "\r"
sleep 2
send "$mgt0IP$mgt0NM\r"
sleep 2
send "$mgt0GW.1\r"
sleep 2
send "\r"
sleep 2
expect {
  timeout { send_user "\nFailed to configure mgt0 network interface for $mgt0IP\n $expect_out(buffer)\n\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*$mgt0IP*"
}

set timeout 300

send_user "\n\n########################################\n"
send_user "Waiting for MessageSight Initialization to complete for $mgt0IP\n"
send_user "########################################\n"
expect {
  timeout { send_user "\nMessageSight Initialization Failed to complete for $mgt0IP before the timeout expired.\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*IBM MessageSight is not fully functional*" { }
  "*IBM MessageSight will not be fully functional*" { }
}

sleep 15
set timeout 120
send_user "\n\n########################################\n"
send_user "Send Enter key 3 times to bypass changing admin password if prompted\n"
send_user "########################################\n"
send "\r"
send "\r"
send "\r"

send_user "\n\n########################################\n"
send_user "Pinging mgt0 network interface with IP=$mgt0IP\n"
send_user "########################################\n"
send "net-test ping $mgt0IP\r"
expect {
  timeout { send_user "\nFailed to ping $mgt0IP\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*Ok*"
}

##### Configure passwordless SSH if the test password was provided #####
if [info exists testPW] {
  sleep 10
  #### Verify mgt0 network interface can ping STAX server
  send_user "\n\n########################################\n"
  send_user "Pinging STAX server at $staxIP\n"
  send_user "########################################\n"
  send "net-test ping $staxIP\r"
  expect {
    timeout { send_user "\nFailed to ping STAX server at $staxIP\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
    "*Ok*"
  }

  #### Get sshkey (authorized_keys) from STAX server
  send_user "\n\n########################################\n"
  send_user "Getting sshkey (authorized_keys) from $staxIP\n"
  send_user "########################################\n"
  send "user sshkey add scp://root@$staxIP:/staf/tools/niagara/authorized_keys\r"
  expect {
    "*(yes/no)?*" { send "yes\r" }
  }
  expect {
    "*assword:*" { send "$testPW\r" }
  }
  expect {
    timeout { send_user "\nFailed to get sshkey (authrized_keys) from $staxIP\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
    "*100%*"
  }

  #### Getting ssh identity (id_sample_harness_sshkey) from STAX server
  send_user "\n\n########################################\n"
  send_user "Getting ssh identity (id_sample_harness_sshkey) from $staxIP\n"
  send_user "########################################\n"
  send "file get scp://root@$staxIP:/staf/tools/niagara/id_sample_harness_sshkey identity\r"
  expect {
    "*assword:*" { send "$testPW\r" }
  }
  expect {
    timeout { send_user "\nFailed to get ssh identity (id_sample_harness_sshkey) from $staxIP\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
    "*100%*"
  }

  set timeout 120
  #### Accept the IMA license
  send_user "\n\n########################################\n"
  send_user "Accepting the IMA license on $mgt0IP\n"
  send_user "########################################\n"
  send "imaserver set AcceptLicense\r"
  expect {
    timeout { send_user "\nFailed to accept IMA license on $mgt0IP\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
    "*>"
  }

  #### Apply the identity file for passwordless ssh
  send_user "\n\n########################################\n"
  send_user "Applying the identity file for passwordless ssh on $mgt0IP\n"
  send_user "########################################\n"
  send "imaserver apply identity identity\r"

  #### Temp fix for new code that by default management can only be done from first port configured 
  send_user "\n\n########################################\n"
  send_user "TEMP FIX: Setting SSHHost=ALL to bypass that IMA management can only be done on mtg0\n"
  send_user "########################################\n"
  send "imaserver set SSHHost=ALL\r"

}

set timeout 15

send_user "\n\n########################################\n"
send_user "Show mgt1 and eth0 network interfaces were RESET"
send_user "\n########################################\n"
send " show ethernet-interface mgt1\r"
expect {
  timeout { send_user "\nMGT1 ethernet-interface was NOT Reset\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*ethernet-interface mgt1: *Down*"
}

send " show ethernet-interface eth0\r"
expect {
  timeout { send_user "\nETH0 ethernet-interface was NOT Reset\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*ethernet-interface eth0: *Down*"
}

send_user "\n\n########################################\n"
send_user "Verify Serial and MTM 6188SM17800000"
send_user "\n########################################\n"
send " show version\r"
expect {
  timeout { send_user "\nSerial and MTM 6188SM17800000 WAS NOT RESET!!\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*Machine type/model: 6188SM1*Serial number: 7800000*Entitlement: 7800000"
}




send_user "\n\n########################################\n"
send_user "Exiting from bedrock console on $mgt0IP"
send_user "\n########################################\n"
send "exit\r"
expect {
  timeout { send_user "\nFailed to exit the bedrock console on $mgt0IP\n"; spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate; exit 1 }
  "*login:"
}

#### Deactivate the ipmitool serial console session
spawn ipmitool -I lanplus -H $immIP -U USERID -P PASSW0RD sol deactivate

send_user "\n\n########################################\n"
send_user "Reconfigure for Automation has completed."
send_user "\n########################################\n"
exit
