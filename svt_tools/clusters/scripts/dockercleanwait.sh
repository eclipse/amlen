#!/bin/bash

./dockerstop.sh
./rmtrace.sh
./rmstorefiles.sh
./dockerstart.sh
./waitproduction.sh

