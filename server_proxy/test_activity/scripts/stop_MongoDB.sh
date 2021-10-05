#! /bin/bash

#Command to stop the mongodb instance the test is running against
ssh tock@lnx-wiotp3 'sudo service mongod stop' | tr -d '[:cntrl:]'
echo " "

