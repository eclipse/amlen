#/bin/bash

# Create cars.csv file with 100K devices
for i in {0..99999} ; do printf "c%07d,car,svtfortest\n" $i ; done > cars.csv

# Creates users.csv file with 100K devices
for i in {0..99999} ; do printf "u%07d,user,svtfortest\n" $i ; done > users.csv

