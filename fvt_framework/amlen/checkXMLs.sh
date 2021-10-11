#!/bin/sh
source /staf/STAFEnv.sh STAF
for i in *.xml; do echo "TESTING $i..."; staf local stax execute file `pwd`/$i test; done
