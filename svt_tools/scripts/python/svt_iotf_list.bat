REM 
REM BAT file to run svt-register-iotf to register all of the necessary organizations and devices

REM first we set up the environment variables we need
REM Expected environment variables & example values: 

SET ORG_OWNER=imaperf@us.ibm.com


SET ORG_NAME=shvknh
SET API_KEY="a-shvknh-crkijrrazg"

SET API_TOKEN="8utxEmuh14UtV*3dkB"

REM SET REST_URL=staging.internetofthings.ibmcloud.com/api/v0001
SET REST_URL=lon02-1.test.internetofthings.ibmcloud.com/api/v0001
SET DEVICE_TYPE=
SET NUM_THREADS=1

svt_iotf_list_sample.py
