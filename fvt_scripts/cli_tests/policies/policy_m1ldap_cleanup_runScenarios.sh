#! /bin/bash

#-------------------------------------------------------------------
#  This script defines the M1 Cleanup of LDAP for the Policies tests
#-------------------------------------------------------------------

# Set the name of this set of scenarios

scenario_set_name="Policy M1 Cleanup LDAP"

typeset -i n=0

#----------------------------------------------------
# Cleanup and disable LDAP on M1 
#----------------------------------------------------
xml[${n}]="Policy_M1_LDAP_cleanup"
scenario[${n}]="${xml[${n}]} - disable and clean"
timeouts[${n}]=10
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanupm1ldap|-c|../common/m1ldap_config.cli"
component[2]=cAppDriverLogWait,m1,"-e|../scripts/ldap-config.sh","-o|-a|stop"
components[${n}]="${component[1]} ${component[2]}"
((n+=1))


