#!/usr/bin/python3
import sys

[serverRelease, bldType, serverLabel, clientLabel, testLabel, testGroup] = sys.argv[1:]


dataFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '_mach.dat'
reportFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '_mach.rpt'

try:
  dataFile = open( dataFILE, 'r')
except IOError:
  print("Error: file does not exist: " + dataFILE)
  sys.exit(0)
 
totalsList = []
hostsList  = []
for line in dataFile:
  if (line.find('TOTAL')) != -1:
    totalsList.append(line)
  else:
    hostsList.append(line)

reportFile = open( reportFILE, 'w')
reportFile.write("\nIMA Automated Framework Machine Times Report\n")
reportFile.write("============================================\n")
reportFile.write("Release:          " + serverRelease + "\n")
reportFile.write("Build Type:       " + bldType + "\n")
reportFile.write("Server Label:     " + serverLabel + "\n")
reportFile.write("Client Label:     " + clientLabel + "\n")
reportFile.write("Test Label:       " + testLabel + "\n")
reportFile.write("Test Group:       " + testGroup + "\n")
reportFile.write("============================================\n\n")

# totalsList.sort()
hostsList.sort()
for i in totalsList:
  reportFile.write(i)
reportFile.write("\n===========================================================================\n")
for i in hostsList:
  reportFile.write(i)

reportFile.flush()
reportFile.close()

dataFile.flush()
dataFile.close()
