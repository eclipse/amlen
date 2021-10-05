#!/bin/bash

#
# This script will execute the scenario scripts required for HA backup restore test
#

echo "Running test using true for restore appliance arg"
./ismAutoComplexSetup.sh 

./ismAutoBackupPrimary.sh 
./ismAutoComplexPostBackupSetup.sh

./ismAutoRestorePrimary.sh  1

./ismAutoComplexRestoreValidate.sh 

