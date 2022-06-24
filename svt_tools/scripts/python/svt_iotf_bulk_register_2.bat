REM 
REM BAT file to run svt-register-iotf to register all of the necessary organizations and devices

REM first we set up the environment variables we need
REM Expected environment variables & example values: 

SET ORG_OWNER=someone@example.com

REM STAGING ORGANIZATIONS AND KEYS
REM SET ORG_NAME=org123
REM SET API_KEY=a-org123-example123
REM SET API_TOKEN=exampleExample123

REM SET ORG_NAME=org123
REM SET API_KEY=a-org123-example123
REM SET API_TOKEN=exampleExample123

REM TEST ORGANIZATION AND KEYS

SET ORG_NAME=org123
SET API_KEY=a-org123-example123
SET API_TOKEN=exampleExample123

SET FIRST_DEVICE_ID=0
SET LAST_DEVICE_ID=0

REM SET REST_URL=test.iot.example.com/api/v0001
SET REST_URL=test.iot.example.com/api/v0001
SET NUM_THREADS=10
REM SET CSV_DEVICE_FILE=next20kdevices.csv
SET CSV_DEVICE_FILE=50kcar.csv

REM c:\opensource\python34\python.exe -m trace --trace svt_iotf_bulk_register_sample.py
svt_iotf_bulk_register_sample.py > bulk2.txt 2>&1

