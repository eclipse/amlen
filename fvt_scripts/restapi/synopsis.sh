#!/bin/bash +x

find . -maxdepth 1 -name "*.screenout.log"  -exec grep -l --color " passing ("  {} \;  -exec grep -A2 " passing ("  {} \; >passing.synopsis.log 


files=` ls *.screenout.log`

for file in $files
do
#   echo "File: " $file

   line=`grep -nr  " passing (" $file`
#   echo "line: " $line
   firstLine=$(echo $line | cut -d':' -f'1')
#   echo "firstLine: " $firstLine

   lastLine=`cat $file | wc -l `
#   echo "lastLine: " $lastLine


   `sed -n "$firstLine,$lastLine p" $file > $file.synopsis.log`

done
