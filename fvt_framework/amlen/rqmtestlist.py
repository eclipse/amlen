#!/usr/bin/python3
import sys

[serverRelease, testGroup] = sys.argv[1:] 

dataFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '_rqm.dat'
reportFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '.rqm'

### READ THE RAW DATA FROM THE RQM DATA FILE ###
rawList = []
lineCount = 0
try:
  dataFile = open( dataFILE, 'r')
except IOError:
  print("Error: file does not exist: " + dataFILE)
  sys.exit(0)
  lineCount =+ lineCount

dataFile.flush()
dataFile.close()

if lineCount > 1:
  ### STRIP DULICATES AND EMPTY LINES FROM THE LIST ###
  rawList.sort()
  last = rawList[-1]
  for i in range(len(rawList)-2, -1, -1):
    if last == rawList[i] or not rawList[i].strip():
      del rawList[i]
    else:
      last = rawList[i]

### REMOVE "PASSED" TESTCASE FROM LIST IF THE SAME TESTCASE ALSO "FAILED" ###
### SINCE WE ALREADY SORTED THE LIST, ALL "FAILED" TESTS WILL BE 1ST IN THE LIST ###
newList = []
for i in range(len(rawList)):
  alreadyThere = 'FALSE'
  for j in range(len(newList)):
    if newList[j].split(',')[5].strip() == rawList[i].split(',')[5].strip():
      alreadyThere = 'TRUE'
  if alreadyThere == 'FALSE':
    newList.append( rawList[i] )    
  
### WRITE THE RESULTS TO THE RQM INPUT FILE ###
reportFile = open( reportFILE, 'w')
for i in newList:
  reportFile.write(i)
reportFile.flush()
reportFile.close()
