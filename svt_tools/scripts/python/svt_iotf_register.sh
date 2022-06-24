#
#BAT file to run svt-register-iotf to register all of the necessary organizations and devices

#first we set up the environment variables we need
#Expected environment variables & example values: 

export ORG_OWNER=someone@example.com


export ORG_NAME=org123
export API_KEY=a-org123-example123
export API_TOKEN=exampleExample123

#export ORG_NAME=org123
#export API_KEY=a-org123-example123
#export API_TOKEN=exampleExample123

export REST_URL=test.iot.example.com/api/v0001
export NUM_THREADS=255

/usr/local/bin/python3 svt_iotf_register_sample.py
