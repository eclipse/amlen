#!/usr/bin/expect
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

if { [llength $argv] != 1  } {
   send_user "\nEnter the IMM IP Address as the only parameter and resubmit\n\n";
   exit 99;
} else {
   set A1_IMM [lindex $argv 0];
}

set SHORT_SLEEP 5
set MEDIUM_SLEEP 30
set LONG_SLEEP 45
set REBOOT_TIME 1800

# Terminate any existing Serial Connections using the IMM Port
spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD sol deactivate
sleep $SHORT_SLEEP
#######################################################
send_user "\n\n Prepare for Device RESET on ${A1_IMM} and WAIT for reboots to complete.\n\n"

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD sol activate
sleep $SHORT_SLEEP
send "\r"
expect {
   timeout { send_user "\n\n ERROR: Appliance did NOT give login prompt.\n $expect_out(buffer)\n\n";  exit 1 }
   "*(none) login:" { send_user "\n @ Login Prompt: LOGIN as admin.\n";  }
}

sleep $SHORT_SLEEP
send "admin\r"
sleep $SHORT_SLEEP
send "admin\r"
expect {
   timeout { send_user "\n\n ERROR: Unexpected screen after the login.\n $expect_out(buffer)\n\n";  exit 1 }
   "*Welcome to IBM MessageSight*" { send_user "\n LOGIN was successful.\n";  }
}

set timeout $REBOOT_TIME

send "device RESET noprompt\r"
sleep $SHORT_SLEEP
expect {
   timeout { send_user "\n\n ERROR: Unexpected screen after the device RESET.\n $expect_out(buffer)\n\n";  exit 1 }
   "r6  Restarting system" { send_user "\n device RESET reboot has started. \n";  }
}

expect {
   timeout { send_user "\n\n ERROR: appliance did NOT reboot to login prompt after device RESET.\n $expect_out(buffer)\n\n";  exit 1 }
   "*(none) login:" { send_user "\n Rebooted to LOGIN Prompt as expected. \n";  }
}
send "~.\r"

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD sol deactivate
sleep $SHORT_SLEEP


#######################################################
send_user "\n\nStop and Restart the appliance\n\n"

sleep $SHORT_SLEEP
spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD chassis power off
expect {
   timeout { send_user "\n\n ERROR:  Failed to set Chassis Power OFF, expected OFF.\n $expect_out(buffer) .\n\n"; exit 1} 
   "*Chassis Power Control: Down/Off" { send_user "\n Chassis Powered Down requested, checking power status in $MEDIUM_SLEEP seconds.\n"; }
}
sleep $MEDIUM_SLEEP

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD chassis power status
expect {
   timeout { send_user "\n\n ERROR:  Failed to get Power Status, expected OFF.\n $expect_out(buffer) \n\n"; exit 1}
   "Chassis Power is on" { send_user "\n\n ERROR:  Chassis is NOT Powered Off. \n  $expect_out(buffer) \n\n"; exit 1}
   "Chassis Power is off" { send_user "\n Chassis is OFF, restart Chassis in $SHORT_SLEEP seconds. \n"; }
}
sleep $SHORT_SLEEP 


spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD chassis power on
expect {
   timeout { send_user "\n\n ERROR:  Failed to set Chassis Power ON, expected ON.\n $expect_out(buffer) .\n\n"; exit 1}
   "*Chassis Power Control: Up/On" { send_user "\n Chassis Power On requested, checking power status in $MEDIUM_SLEEP seconds.\n"; }
}
sleep $MEDIUM_SLEEP

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD chassis power status
expect {
   timeout { send_user "\n\n ERROR: Failed to set Power Status ON, expected ON.\n $expect_out(buffer) .\n\n"; exit 1}
   "Chassis Power is on" { send_user "\n Chassis Powered Up, repeat the power sequence to Login Prompt in $MEDIUM_SLEEP seconds.\n"; }
}
sleep $MEDIUM_SLEEP

# Terminate the CHASSIS POWER ON session if it is still active. it tends to linger
spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD sol deactivate
sleep $SHORT_SLEEP

#######################################################
send_user "\n\n Wait for ${A1_IMM} to complete Second Reboot (like at Customer Location)\n\n"

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD sol activate
send "\r"

#sleep $LONG_SLEEP
expect {
   timeout { send "\x04\r"; send_user "\n\n -- ERROR:  CTRL+D WORKAROUND WAS NEEDED FOR INTERRUPTED BOOT SEQUENCE!!! --\n $expect_out(buffer)\n\n"; exit 1 }
   "* cfu " { send_user "\n\n -- WORKAROUND NOT needed to complete BOOT SEQUENCE --\n\n"}
}

expect {
   timeout { send_user "\n\n ERROR, appliance did NOT reboot to login prompt.\n $expect_out(buffer)\n\n";  exit 1 }
   "*(none) login:" { send_user "\n Rebooted to LOGIN Prompt successfully, Test Completed Successfully and PASSED.\n"; send "~.\r" }
}

# Terminate SOL for the CHASSIS POWER OFF
spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD sol deactivate
sleep $SHORT_SLEEP


send_user "\n\n Final clean up:  Power off the Chassis - Reset IMM.\n"

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD chassis power off
expect {
   timeout { send_user "\n\n ERROR:  Failed Final set Chassis Power OFF, expected OFF.\n $expect_out(buffer) .\n\n"; exit 1} 
   "*Chassis Power Control: Down/Off" { send_user "\n Chassis Power Off requested, checking power status in $MEDIUM_SLEEP seconds.\n"}
}
sleep $MEDIUM_SLEEP

spawn ipmitool -I lanplus -H ${A1_IMM} -U USERID -P PASSW0RD chassis power status
expect {
   timeout { send_user "\n\n ERROR:  Failed to get Power Status, expected OFF. \n $expect_out(buffer) .\n\n"; exit 1}
   "Chassis Power is on" { send_user "\n\n ERROR:  Chassis is NOT Powered Off. \n  $expect_out(buffer) \n\n"; exit 1 }
   "Chassis Power is off" { send_user "\n Chassis is OFF, Ready to ship to customer after IMM is reset!\n"; }
}


exit 0
