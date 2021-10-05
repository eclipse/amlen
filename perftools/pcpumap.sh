#!/bin/sh

# This script will print a map of CPUs to core and HW threads
# Currently only 3 levels of cache (0 and 1 are both Level 1)
caches="0 1 2 3"
for cache in $caches ; do
  echo "Level $cache cache"
  procentry="/sys/devices/system/cpu/cpu*/cache/index${cache}/shared_cpu_map"
  for x in `ls $procentry` ; do 
    cpux=`echo $x | cut -d'/' -f6 | cut -d'u' -f2`
    extmap=`cat $x` 
    msg="CPU $cpux shares Level $cache cache with CPU: "
    list=""
    for y in `ls $procentry` ; do
      cpuy=`echo $y | cut -d'/' -f6 | cut -d'u' -f2`
      intmap=`cat $y`
      if [ "$extmap" = "$intmap" ] ; then
        list="${list} $cpuy" 
      fi
    done
    echo "${msg}${list}"
  done
done



