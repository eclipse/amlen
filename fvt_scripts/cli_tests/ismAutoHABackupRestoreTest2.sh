#!/bin/bash

#
# This script will execute the scenario scripts required for HA backup restore test
# 

echo "Running test using false for restore appliance arg"
./ismAutoComplexSetup.sh 

./ismAutoBackupPrimary.sh 
./ismAutoComplexPostBackupSetup.sh

./ismAutoRestorePrimary.sh  2

./ismAutoComplexRestoreValidate.sh 

