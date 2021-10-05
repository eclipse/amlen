#
#BAT file to run svt-register-iotf to register all of the necessary organizations and devices

#first we set up the environment variables we need
#Expected environment variables & example values: 

export ORG_OWNER=imaperf@us.ibm.com


export ORG_NAME=dxdwqs
export API_KEY=a-dxdwqs-6xzmkugb93
export API_TOKEN=wSON-Grf16*kNSasC5

#export ORG_NAME=nwxtmk
#export API_KEY=a-nwxtmk-s1bmnuh5ho
#export API_TOKEN=Ot89XoKfqrrnlN3bR4

export REST_URL=staging.internetofthings.ibmcloud.com/api/v0001
export NUM_THREADS=1

/usr/local/bin/python3 svt_iotf_list_sample.py
