#! /bin/bash

#Command to start the mongodb instance the test is running against
ssh tock@lnx-wiotp3 'sudo service mongod start' | tr -d '[:cntrl:]' 
echo " "

