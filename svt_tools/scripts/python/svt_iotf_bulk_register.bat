:REM 
REM BAT file to run svt-register-iotf to register all of the necessary organizations and devices

REM first we set up the environment variables we need
REM Expected environment variables & example values: 

SET ORG_OWNER=imaperf@us.ibm.com

SET ORG_NAME=w7vwxv
set API_KEY="a-w7vwxv-dixplhfggn"
SET API_TOKEN="bUjVV5D9hQG9dzoH-d"

SET FIRST_DEVICE_ID=0
SET LAST_DEVICE_ID=0

REM SET REST_URL=staging.internetofthings.ibmcloud.com/api/v0001
SET REST_URL="lon02-1.test.internetofthings.ibmcloud.com/api/v0001"
SET NUM_THREADS=1
REM SET CSV_DEVICE_FILE=next20kdevices.csv
SET CSV_DEVICE_FILE=missingCars.csv
SET CSV_DEVICE_FILE=100kcar.csv

REM c:\opensource\python34\python.exe -m trace --trace svt_iotf_bulk_register_sample.py
svt_iotf_bulk_register_sample.py > bulk1.txt 2>&1


