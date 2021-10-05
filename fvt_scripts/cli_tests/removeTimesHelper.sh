#! /bin/bash
# sed Helper
set -x

ISOTime="2[0-9]*-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]\.[0-9]*Z"
ResetTime="s/,\\\"ResetTime\\\":\\\"${ISOTime}\\\"//g"
LastConnectedTime="s/,\\\"LastConnectedTime\\\":\\\"${ISOTime}\\\"//g"

echo -e "\nexecuting sed with the following parameters: \"$1\" \"$2\""

cat $1 | sed -e "$ResetTime" -e "$LastConnectedTime" > $2

# We have to sleep so that run-scenarios.sh can verify that it is running by getting the PID
sleep 0.1

