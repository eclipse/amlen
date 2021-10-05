#!/bin/bash

#----------------------------------------------------
#  Description:
#  Make sure HA is disabled
#
#
#   8/22/14 - created
#   2/18/16 - moved SVT.setup.data.sh here from svt-shared
#
#----------------------------------------------------


#----------------------------------------------------
# TODO!:  MODIFY scenario_set_name to a short description appropriate for your testcase
# Set the name of this set of scenarios
#----------------------------------------------------

scenario_set_name="Setup Disable HA"
source ./svt_test.sh
test_template_set_prefix "cmqtt_00_"
typeset -i n=0

# test_template_add_test_single "SVT - ensure that we have disabled HA" cAppDriver m1 "-e|${M1_TESTROOT}/svt_cmqtt/SVT.start.workload.sh,-o|haFunctions.sh|disableHA|1|${A1_HOST}|${A2_HOST}" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" ""
test_template_add_test_single "SVT - ensure that we have disabled HA" cAppDriver m1 "-e|${M1_TESTROOT}/scripts/haFunctions.py,-o|-a|disableHA|-t|600" 600 "SVT_AT_VARIATION_QUICK_EXIT=true" "Test result is Success!"


#----------------------------------------------------
# Setup LTPA, client certificate cache -
# Note : this test_template macro automatically increments n
# Note: this test may take a long time the first time, but all subsequent
# times it should be very fast after the cache has been initialized.
#
# The cache is not reintialized after each automation run, but persists until
# the master data cache changes or an expected file is missing.
#
# TODO: try to make it smarter about where to create the directory / or /home or silently fail
#----------------------------------------------------
test_template_add_test_all_Ms  "SVT Environment data cache setup" "-e|${M1_TESTROOT}/scripts/SVT.setup.data.sh,-o|" 3600 "" "" "cmqtt_00_"
