#!/bin/bash
#
# ismAutoJMS.sh was split into 2 script to allow the configuration setup to be run separate from the rest of the tests
# This was to allow us to run configuration setup, do an upgrade to version 1.1 and then run the rest of the bucket to 
# verify that the configuration got migrated correctly. 

./ismAutoJMS-setup.sh
./ismAutoJMS-run.sh
