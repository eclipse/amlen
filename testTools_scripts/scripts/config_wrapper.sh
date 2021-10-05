#!/bin/bash


# The goal of this wrapper is to create a common script that is called by
# test drivers which only need to provide a generic configure statement.
# Initially the script calls run-cli.sh to do this configuration however
# in the future this script may use other resources to do the configuration.

#----------------------------------------------------------------------------
# Source the ISMsetup file to get access to information for the remote machine
#----------------------------------------------------------------------------
if [[ -f "../testEnv.sh"  ]]
then
  source  "../testEnv.sh"
else
  source ../scripts/ISMsetup.sh
fi

# build argument string for the -i option
# we expect it to pass and will return rc=1 if this command fails
i_arg=
for argument in $@ ; do
  i_arg="$i_arg$argument "
done

a_arg=${i_arg%% *}
i_arg="config_wrapper 0 ${i_arg:${#a_arg}+1}"


$M1_TESTROOT/scripts/run-cli.sh -a $a_arg -i "$i_arg" -f /dev/stdout
