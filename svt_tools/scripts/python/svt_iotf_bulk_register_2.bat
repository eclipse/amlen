REM 
REM BAT file to run svt-register-iotf to register all of the necessary organizations and devices

REM first we set up the environment variables we need
REM Expected environment variables & example values: 

SET ORG_OWNER=imaperf@us.ibm.com

REM STAGING ORGANIZATIONS AND KEYS
REM SET ORG_NAME=dxdwqs
REM SET API_KEY=a-dxdwqs-6xzmkugb93
REM SET API_TOKEN=wSON-Grf16*kNSasC5

REM SET ORG_NAME=nwxtmk
REM SET API_KEY=a-nwxtmk-s1bmnuh5ho
REM SET API_TOKEN=Ot89XoKfqrrnlN3bR4

REM TEST ORGANIZATION AND KEYS

SET ORG_NAME=5dhmel
SET API_KEY="a-5dhmel-hbrn8kfwr1"
SET API_TOKEN="3XsIR2lqdaM1Z@s0Om"

SET FIRST_DEVICE_ID=0
SET LAST_DEVICE_ID=0

REM SET REST_URL=staging.internetofthings.ibmcloud.com/api/v0001
SET REST_URL="lon02-1.test.internetofthings.ibmcloud.com/api/v0001"
SET NUM_THREADS=10
REM SET CSV_DEVICE_FILE=next20kdevices.csv
SET CSV_DEVICE_FILE=50kcar.csv

REM c:\opensource\python34\python.exe -m trace --trace svt_iotf_bulk_register_sample.py
svt_iotf_bulk_register_sample.py > bulk2.txt 2>&1

