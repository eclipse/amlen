#!/bin/bash

scenario_set_name="JMS Non - TestDriver Tests - 00"

# Contrary to the name of this bucket, we use the test
# driver, however, only to launch a single action.

typeset -i n=0

# 
#-----------------------------------------------------------------------------
# Scenario 1 (real last) - JMS Message Pseudo-Translation Test
#-----------------------------------------------------------------------------
xml[${n}]="jms_pseudo_translation"
timeouts[${n}]=210
scenario[${n}]="${xml[${n}]} - JMS Message Pseudo-Translation Test"
# Enable JMSTestDriver Log Level 7 to see the message output
#component[1]=jmsDriver,m1,nontd/jms_pseudo_translation.xml,ALL,-o_-l_7
component[1]=jmsDriver,m1,nontd/jms_pseudo_translation.xml,ALL
components[${n}]="${component[1]}"
((n+=1))
