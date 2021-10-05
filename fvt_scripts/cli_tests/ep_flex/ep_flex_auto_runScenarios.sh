#!/bin/bash
scenario_set_name="cli_ENDPOINT_FLEX_AUTO_Tests"
typeset -i n=0

#----------------------------------------------------
# cli_epflex_security_001
#----------------------------------------------------
xml[${n}]="cli_epflex_security_001"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Creation of Connection Factory Administered Objects and Policy Setup"
component[1]=cAppDriverLogWait,m1,"-e|../scripts/run-cli.sh","-o|-s|setup|-c|ep_flex/cli_epflex_security.cli"
components[${n}]="${component[1]} "
((n+=1))

# Test number: EndPointFlex-01
xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-01"
timeouts[${n}]=50
scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-01"
component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_01/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-01.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-01"
component[2]=sync,m1
component[3]=jmsDriver,m1,ep_flex/ep_flex_01/jms_rmdr_policy_validation_EndPointFlex-01.xml,rmdr
component[4]=jmsDriver,m1,ep_flex/ep_flex_01/jms_rmdt_policy_validation_EndPointFlex-01.xml,rmdt
components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
((n+=1))

if [ "${FULLRUN}" == "TRUE" ] ; then

    # Test number: EndPointFlex-02
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-02"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-02"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_02/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-02.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-02"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_02/jms_rmdr_policy_validation_EndPointFlex-02.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_02/jms_rmdt_policy_validation_EndPointFlex-02.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-03
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-03"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-03"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_03/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-03.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-03"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_03/jms_rmdr_policy_validation_EndPointFlex-03.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_03/jms_rmdt_policy_validation_EndPointFlex-03.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-04
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-04"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-04"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_04/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-04.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-04"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_04/jms_rmdr_policy_validation_EndPointFlex-04.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_04/jms_rmdt_policy_validation_EndPointFlex-04.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-05
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-05"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-05"
    component[1]=sync,m1
    component[2]=jmsDriver,m1,ep_flex/ep_flex_05/jms_rmdr_policy_validation_EndPointFlex-05.xml,rmdr
    component[3]=jmsDriver,m1,ep_flex/ep_flex_05/jms_rmdt_policy_validation_EndPointFlex-05.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]}"
    ((n+=1))

    # Test number: EndPointFlex-06
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-06"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-06"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_06/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-06.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-06"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_06/jms_rmdr_policy_validation_EndPointFlex-06.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_06/jms_rmdt_policy_validation_EndPointFlex-06.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-07
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-07"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-07"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_07/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-07.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-07"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_07/jms_rmdr_policy_validation_EndPointFlex-07.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_07/jms_rmdt_policy_validation_EndPointFlex-07.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-08
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-08"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-08"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_08/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-08.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-08"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_08/jms_rmdr_policy_validation_EndPointFlex-08.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_08/jms_rmdt_policy_validation_EndPointFlex-08.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-09
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-09"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-09"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_09/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-09.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-09"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_09/jms_rmdr_policy_validation_EndPointFlex-09.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_09/jms_rmdt_policy_validation_EndPointFlex-09.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))
    
    # Test number: EndPointFlex-10
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-10"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-10"
    component[1]=sync,m1
    component[2]=jmsDriver,m1,ep_flex/ep_flex_10/jms_rmdr_policy_validation_EndPointFlex-10.xml,rmdr
    component[3]=jmsDriver,m1,ep_flex/ep_flex_10/jms_rmdt_policy_validation_EndPointFlex-10.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]}"
    ((n+=1))

fi

# Clean-up test number: EndPointFlex-10
xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-10_CLEAN"
timeouts[${n}]=20
scenario[${n}]="${xml[${n}]} - clean policy test EndPointFlex-10"
component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_10/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-10.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-10_CLEAN"
components[${n}]="${component[1]}"
((n+=1))

if [ "${FULLRUN}" == "TRUE" ] ; then

    # Test number: EndPointFlex-11
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-11"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-11"
    component[1]=sync,m1
    component[2]=jmsDriver,m1,ep_flex/ep_flex_11/jms_rmdr_policy_validation_EndPointFlex-11.xml,rmdr
    component[3]=jmsDriver,m1,ep_flex/ep_flex_11/jms_rmdt_policy_validation_EndPointFlex-11.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]}"
    ((n+=1))

    # Test number: EndPointFlex-12
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-12"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-12"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_12/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-12.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-12"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_12/jms_rmdr_policy_validation_EndPointFlex-12.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_12/jms_rmdt_policy_validation_EndPointFlex-12.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-13
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-13"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-13"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_13/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-13.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-13"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_13/jms_rmdr_policy_validation_EndPointFlex-13.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_13/jms_rmdt_policy_validation_EndPointFlex-13.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-14
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-14"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-14"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_14/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-14.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-14"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_14/jms_rmdr_policy_validation_EndPointFlex-14.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_14/jms_rmdt_policy_validation_EndPointFlex-14.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))


    # Test number: EndPointFlex-15
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-15"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-15"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_15/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-15.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-15"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_15/jms_rmdr_policy_validation_EndPointFlex-15.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_15/jms_rmdt_policy_validation_EndPointFlex-15.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-16
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-16"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-16"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_16/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-16.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-16"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_16/jms_rmdr_policy_validation_EndPointFlex-16.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_16/jms_rmdt_policy_validation_EndPointFlex-16.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-17
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-17"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-17"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_17/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-17.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-17"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_17/jms_rmdr_policy_validation_EndPointFlex-17.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_17/jms_rmdt_policy_validation_EndPointFlex-17.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-18
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-18"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-18"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_18/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-18.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-18"
    component[2]=sync,m1
    component[3]=jmsDriver,m1,ep_flex/ep_flex_18/jms_rmdr_policy_validation_EndPointFlex-18.xml,rmdr
    component[4]=jmsDriver,m1,ep_flex/ep_flex_18/jms_rmdt_policy_validation_EndPointFlex-18.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]} ${component[4]}"
    ((n+=1))

    # Test number: EndPointFlex-19
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-19"
    timeouts[${n}]=50
    scenario[${n}]="${xml[${n}]} - policy test EndPointFlex-19"
    component[1]=sync,m1
    component[2]=jmsDriver,m1,ep_flex/ep_flex_19/jms_rmdr_policy_validation_EndPointFlex-19.xml,rmdr
    component[3]=jmsDriver,m1,ep_flex/ep_flex_19/jms_rmdt_policy_validation_EndPointFlex-19.xml,rmdt
    components[${n}]="${component[1]} ${component[2]} ${component[3]}"
    ((n+=1))

    # Clean-up test number: EndPointFlex-19
    xml[${n}]="cli_ENDPOINT_FLEX_AUTO_EndPointFlex-19_CLEAN"
    timeouts[${n}]=20
    scenario[${n}]="${xml[${n}]} - clean policy test EndPointFlex-19"
    component[1]=cAppDriverLogWait,m1,"-e|${M1_TESTROOT}/scripts/run-cli.sh","-o|-c|ep_flex/ep_flex_19/cli_ENDPOINT_FLEX_AUTO_EndPointFlex-19.cli|-s|cli_ENDPOINT_FLEX_AUTO_EndPointFlex-19_CLEAN"
    components[${n}]="${component[1]}"
    ((n+=1))

fi

#----------------------------------------------------
#  cli_epflex_security_002
#----------------------------------------------------
xml[${n}]="cli_epflex_security_002"
timeouts[${n}]=75
scenario[${n}]="${xml[${n}]} - Clean-up of Connection Factory Administered Objects and Policies"
component[1]=cAppDriverLog,m1,"-e|../scripts/run-cli.sh","-o|-s|cleanup|-c|ep_flex/cli_epflex_security.cli"
components[${n}]="${component[1]} "
((n+=1))
