#! /bin/bash

echo -e "\nmoving file $1 to $2"

mv $1 $2

# This sleep is needed because the script runs too fast for
# run_scenarios. This allows run_scenarios to get the PID

sleep 1


