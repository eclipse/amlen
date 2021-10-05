#!/bin/bash

########################################################
# THIS IS A SPECIAL FILE! --->>> DO NOT MODIFY! <<<--- #
########################################################

scenario_set_name="JCA HA Tests - 00"
typeset -i n=0

#----------------------------------------------------
# Scenario 0 - JCA HA PRE-SETUP
#----------------------------------------------------
# A1 = Primary
# A2 = Standby
xml[${n}]="HA_setup"
timeouts[${n}]=400
scenario[${n}]="${xml[${n}]} - Configure HA"
component[1]=cAppDriverLog,m1,"-e|../scripts/haFunctions.py","-o|-a|setupHA"
components[${n}]="${component[1]}"
((n+=1))
