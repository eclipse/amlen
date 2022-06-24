REM 
REM BAT file to run svt-register-iotf to register all of the necessary organizations and devices

REM first we set up the environment variables we need
REM Expected environment variables & example values: 

SET ORG_OWNER=someone@example.com

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


REM SET REST_URL=test.iot.example.com/api/v0001
SET REST_URL=test.iot.example.com/api/v0001

SET NUM_THREADS=255

SET CSV_DEVICE_FILE=100kcar.csv

c:\opensource\python34\python.exe svt_iotf_register_sample.py
