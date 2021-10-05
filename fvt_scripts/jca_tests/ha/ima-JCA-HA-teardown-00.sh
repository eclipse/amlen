#!/bin/bash

########################################################
# THIS IS A SPECIAL FILE! --->>> DO NOT MODIFY! <<<--- #
########################################################

scenario_set_name="JCA HA Tests - 00"
typeset -i n=0

#----------------------------------------------------
# Scenario 0 - JCA HA Teardown
#----------------------------------------------------
xml[${n}]="HA_teardown"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Configure HA"
component[1]=cAppDriverLog,m1,"-e!../scripts/haFunctions.py,-o!-a!disableHA"
components[${n}]="${component[1]}"
((n+=1))
