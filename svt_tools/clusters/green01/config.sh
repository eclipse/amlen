#!/bin/bash -x

#TODO:  update to call getclusterserverportlist.py -c green01 -e admin
SERVER[4]=10.113.50.31:5003
SERVER[5]=10.113.50.35:5003
SERVER[6]=10.113.50.44:5003
SERVER[7]=10.113.50.23:5003
SERVER[8]=10.113.50.33:5003
SERVER[9]=10.113.50.39:5003


for i in {4..9}; do
curl -X PUT -o output -T /niagara/test/svt_jmqtt/ssl/ismserver-crt.pem  http://${SERVER[$i]}/ima/v1/file/ismserver-crt.pem
curl -X PUT -o output -T /niagara/test/svt_jmqtt/ssl/ismserver-key.pem  http://${SERVER[$i]}/ima/v1/file/ismserver-key.pem
curl -X PUT -o output -T /niagara/test/common/imaserver-crt.pem  http://${SERVER[$i]}/ima/v1/file/imaserver-crt.pem
curl -X PUT -o output -T /niagara/test/common/imaserver-key.pem  http://${SERVER[$i]}/ima/v1/file/imaserver-key.pem
curl -X PUT -o output -T /niagara/test/common/rootCA-crt.pem  http://${SERVER[$i]}/ima/v1/file/rootCA-crt.pem
curl -X PUT -o output -T /niagara/test/common/mar400.wasltpa.keyfile  http://${SERVER[$i]}/ima/v1/file/mar400.wasltpa.keyfile

curl -H "Content-Type: application/json" --data @SVT_IMA_rest_env1.json http://${SERVER[$i]}/ima/v1/configuration/

cp -f SVT_IMA_rest_env2.json SVT_IMA_rest_env2b.json
sed -i "s/A1_IPv4_1/All/g" SVT_IMA_rest_env2b.json
sed -i "s/A1_IPv4_2/All/g" SVT_IMA_rest_env2b.json

curl -H "Content-Type: application/json" --data @SVT_IMA_rest_env2b.json http://${SERVER[$i]}/ima/v1/configuration/
curl -H "Content-Type: application/json" --data @SVT_IMA_rest_env3.json http://${SERVER[$i]}/ima/v1/configuration/
done
