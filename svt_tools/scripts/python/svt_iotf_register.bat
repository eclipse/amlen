REM 
REM BAT file to run svt-register-iotf to register all of the necessary organizations and devices

REM first we set up the environment variables we need
REM Expected environment variables & example values: 

SET ORG_OWNER=imaperf@us.ibm.com

REM SET ORG_NAME=dxdwqs
REM SET API_KEY=a-dxdwqs-6xzmkugb93
REM SET API_TOKEN=wSON-Grf16*kNSasC5

REM SET ORG_NAME=nwxtmk
REM SET API_KEY=a-nwxtmk-s1bmnuh5ho
REM SET API_TOKEN=Ot89XoKfqrrnlN3bR4

REM TEST ORGANIZATION AND KEYS
SET ORG_NAME=nnvgkj
SET API_KEY="a-nnvgkj-cnqtzpikn8"
SET API_TOKEN="vDgHTD_jUj3ahpOpCH"


REM SET REST_URL=staging.internetofthings.ibmcloud.com/api/v0001
SET REST_URL="lon02-1.test.internetofthings.ibmcloud.com/api/v0001"

SET NUM_THREADS=255

SET CSV_DEVICE_FILE=100kcar.csv

c:\opensource\python34\python.exe svt_iotf_register_sample.py
