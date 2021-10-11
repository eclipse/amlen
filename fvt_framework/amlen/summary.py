#!/usr/bin/python3
import sys
import os
import socket

[bldType, serverRelease, serverLabel, clientRelease, clientLabel, testRelease, testLabel, testGroup, totalTime, userID] = sys.argv[1:] 

pubServerIP = '10.73.131.210'
staxLocation  = 'softlayer'

migrateRelease = ''
migrateLabel   = ''
  
passed = '0'
failed = '0'
totalPassed = 0
totalFailed = 0
totalCount  = 0
percentPassed = '0.00'

jms_totalPassed = 0
jms_totalFailed = 0
jms_totalCount  = 0
jms_percentPassed = '0.00'

mqtt_totalPassed = 0
mqtt_totalFailed = 0
mqtt_totalCount  = 0
mqtt_percentPassed = '0.00'

jms_mqtt_totalPassed = 0
jms_mqtt_totalFailed = 0 
jms_mqtt_totalCount  = 0 
jms_mqtt_percentPassed = '0.00'

jms_tck_totalPassed = 0
jms_tck_totalFailed = 0 
jms_tck_totalCount  = 0 
jms_tck_percentPassed = '0.00'

ws_totalPassed = 0
ws_totalFailed = 0
ws_totalCount  = 0
ws_percentPassed = '0.00'

cli_totalPassed = 0
cli_totalFailed = 0
cli_totalCount  = 0
cli_percentPassed = '0.00'

rest_totalPassed = 0
rest_totalFailed = 0
rest_totalCount  = 0
rest_percentPassed = '0.00'

proxy_totalPassed = 0
proxy_totalFailed = 0
proxy_totalCount  = 0
proxy_percentPassed = '0.00'

plugin_totalPassed = 0
plugin_totalFailed = 0
plugin_totalCount  = 0
plugin_percentPassed = '0.00'

ha_totalPassed = 0
ha_totalFailed = 0
ha_totalCount  = 0
ha_percentPassed = '0.00'

cluster_totalPassed = 0
cluster_totalFailed = 0
cluster_totalCount  = 0
cluster_percentPassed = '0.00'

ui_totalPassed = 0
ui_totalFailed = 0
ui_totalCount  = 0
ui_percentPassed = '0.00'

snmp_totalPassed = 0
snmp_totalFailed = 0
snmp_totalCount  = 0
snmp_percentPassed = '0.00'

mq_conn_totalPassed = 0
mq_conn_totalFailed = 0
mq_conn_totalCount  = 0
mq_conn_percentPassed = '0.00'

a9006_totalPassed = 0
a9006_totalFailed = 0
a9006_totalCount  = 0
a9006_percentPassed = '0.00'

bm_totalPassed = 0
bm_totalFailed = 0
bm_totalCount  = 0
bm_percentPassed = '0.00'

esx_totalPassed = 0
esx_totalFailed = 0
esx_totalCount  = 0
esx_percentPassed = '0.00'

vmware_totalPassed = 0
vmware_totalFailed = 0
vmware_totalCount  = 0
vmware_percentPassed = '0.00'

kvm_totalPassed = 0
kvm_totalFailed = 0
kvm_totalCount  = 0
kvm_percentPassed = '0.00'

cci_totalPassed = 0
cci_totalFailed = 0
cci_totalCount  = 0
cci_percentPassed = '0.00'

cci_rh_docker_totalPassed = 0
cci_rh_docker_totalFailed = 0
cci_rh_docker_totalCount  = 0
cci_rh_docker_percentPassed = '0.00'

ec2_rh_docker_totalPassed = 0
ec2_rh_docker_totalFailed = 0
ec2_rh_docker_totalCount  = 0
ec2_rh_docker_percentPassed = '0.00'

msa_rh_docker_totalPassed = 0
msa_rh_docker_totalFailed = 0
msa_rh_docker_totalCount  = 0
msa_rh_docker_percentPassed = '0.00'

esx_rh_docker_totalPassed = 0
esx_rh_docker_totalFailed = 0
esx_rh_docker_totalCount  = 0
esx_rh_docker_percentPassed = '0.00'

cci_co_docker_totalPassed = 0
cci_co_docker_totalFailed = 0
cci_co_docker_totalCount  = 0
cci_co_docker_percentPassed = '0.00'

ec2_co_docker_totalPassed = 0
ec2_co_docker_totalFailed = 0
ec2_co_docker_totalCount  = 0
ec2_co_docker_percentPassed = '0.00'

msa_co_docker_totalPassed = 0
msa_co_docker_totalFailed = 0
msa_co_docker_totalCount  = 0
msa_co_docker_percentPassed = '0.00'

esx_co_docker_totalPassed = 0
esx_co_docker_totalFailed = 0
esx_co_docker_totalCount  = 0
esx_co_docker_percentPassed = '0.00'

cci_rh_rpm_totalPassed = 0
cci_rh_rpm_totalFailed = 0
cci_rh_rpm_totalCount  = 0
cci_rh_rpm_percentPassed = '0.00'

ec2_rh_rpm_totalPassed = 0
ec2_rh_rpm_totalFailed = 0
ec2_rh_rpm_totalCount  = 0
ec2_rh_rpm_percentPassed = '0.00'

msa_rh_rpm_totalPassed = 0
msa_rh_rpm_totalFailed = 0
msa_rh_rpm_totalCount  = 0
msa_rh_rpm_percentPassed = '0.00'

esx_rh_rpm_totalPassed = 0
esx_rh_rpm_totalFailed = 0
esx_rh_rpm_totalCount  = 0
esx_rh_rpm_percentPassed = '0.00'

cci_co_rpm_totalPassed = 0
cci_co_rpm_totalFailed = 0
cci_co_rpm_totalCount  = 0
cci_co_rpm_percentPassed = '0.00'

ec2_co_rpm_totalPassed = 0
ec2_co_rpm_totalFailed = 0
ec2_co_rpm_totalCount  = 0
ec2_co_rpm_percentPassed = '0.00'

msa_co_rpm_totalPassed = 0
msa_co_rpm_totalFailed = 0
msa_co_rpm_totalCount  = 0
msa_co_rpm_percentPassed = '0.00'

esx_co_rpm_totalPassed = 0
esx_co_rpm_totalFailed = 0
esx_co_rpm_totalCount  = 0
esx_co_rpm_percentPassed = '0.00'

nonZeroMsg = ' '

dataFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '.dat'
reportFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '.rpt'
csvFILE = '/staf/tools/amlen/reports/' + serverRelease + '/' + testGroup + '.csv'

try:
  dataFile = open( dataFILE, 'r')
except IOError:
  print("Error: file does not exist: " + dataFILE)
  sys.exit(0)
 
for line in dataFile:
  if (line.find('Passed:')) != -1 and line.find('9006') == -1 and line.find('Bare_Metal') == -1 and line.find('ESX') == -1 and line.find('VM') == -1 and line.find('_Docker') == -1 and line.find('_RPM') == -1 and line.find('CCI:') == -1:
    passed = line.split('Passed:')[1].split('Failed:')[0]
    failed = line.split('Passed:')[1].split('Failed:')[1]
    totalPassed = totalPassed + int(passed)
    totalFailed = totalFailed + int(failed)
  if (line.find('JMS:')) != -1 and (line.find('MQTT:')) == -1 and (line.find('COUNT')) != -1:
    jms_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    jms_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    jms_totalPassed = jms_totalPassed + int(jms_passed)
    jms_totalFailed = jms_totalFailed + int(jms_failed)
  if (line.find('MQTT:')) != -1 and (line.find('JMS')) == -1 and (line.find('COUNT')) != -1:
    mqtt_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    mqtt_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    mqtt_totalPassed = mqtt_totalPassed + int(mqtt_passed)
    mqtt_totalFailed = mqtt_totalFailed + int(mqtt_failed)
  if (line.find('JMS_MQTT:')) != -1 and (line.find('COUNT')) != -1:
    jms_mqtt_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    jms_mqtt_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    jms_mqtt_totalPassed = jms_mqtt_totalPassed + int(jms_mqtt_passed)
    jms_mqtt_totalFailed = jms_mqtt_totalFailed + int(jms_mqtt_failed)
  if (line.find('JMS_TCK:')) != -1 and (line.find('COUNT')) != -1:
    jms_tck_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    jms_tck_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    jms_tck_totalPassed = jms_tck_totalPassed + int(jms_tck_passed)
    jms_tck_totalFailed = jms_tck_totalFailed + int(jms_tck_failed)
  if (line.find('WS:')) != -1 and (line.find('COUNT')) != -1:
    ws_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ws_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ws_totalPassed = ws_totalPassed + int(ws_passed)
    ws_totalFailed = ws_totalFailed + int(ws_failed)
  if (line.find('CLI:')) != -1 and (line.find('COUNT')) != -1:
    cli_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cli_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cli_totalPassed = cli_totalPassed + int(cli_passed)
    cli_totalFailed = cli_totalFailed + int(cli_failed)
  if (line.find('REST_API:')) != -1 and (line.find('COUNT')) != -1:
    rest_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    rest_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    rest_totalPassed = rest_totalPassed + int(rest_passed)
    rest_totalFailed = rest_totalFailed + int(rest_failed)
  if (line.find('PLUGIN:')) != -1 and (line.find('COUNT')) != -1:
    plugin_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    plugin_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    plugin_totalPassed = plugin_totalPassed + int(plugin_passed)
    plugin_totalFailed = plugin_totalFailed + int(plugin_failed)
  if (line.find('PROXY:')) != -1 and (line.find('COUNT')) != -1:
    proxy_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    proxy_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    proxy_totalPassed = proxy_totalPassed + int(proxy_passed)
    proxy_totalFailed = proxy_totalFailed + int(proxy_failed)
  if (line.find('HA:')) != -1 and (line.find('COUNT')) != -1:
    ha_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ha_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ha_totalPassed = ha_totalPassed + int(ha_passed)
    ha_totalFailed = ha_totalFailed + int(ha_failed)
  if (line.find('CLUSTER:')) != -1 and (line.find('COUNT')) != -1:
    cluster_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cluster_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cluster_totalPassed = cluster_totalPassed + int(cluster_passed)
    cluster_totalFailed = cluster_totalFailed + int(cluster_failed)
  if (line.find('SNMP:')) != -1 and (line.find('COUNT')) != -1:
    snmp_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    snmp_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    snmp_totalPassed = snmp_totalPassed + int(snmp_passed)
    snmp_totalFailed = snmp_totalFailed + int(snmp_failed)
  if (line.find('UI:')) != -1 and (line.find('COUNT')) != -1:
    ui_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ui_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ui_totalPassed = ui_totalPassed + int(ui_passed)
    ui_totalFailed = ui_totalFailed + int(ui_failed)
  if (line.find('MQ_CONN:')) != -1 and (line.find('COUNT')) != -1:
    mq_conn_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    mq_conn_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    mq_conn_totalPassed = mq_conn_totalPassed + int(mq_conn_passed)
    mq_conn_totalFailed = mq_conn_totalFailed + int(mq_conn_failed)
  if (line.find('9006:')) != -1 and (line.find('COUNT')) != -1:
    a9006_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    a9006_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    a9006_totalPassed = a9006_totalPassed + int(a9006_passed)
    a9006_totalFailed = a9006_totalFailed + int(a9006_failed)
  if (line.find('Bare_Metal')) != -1 and (line.find('COUNT')) != -1:
    bm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    bm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    bm_totalPassed = bm_totalPassed + int(bm_passed)
    bm_totalFailed = bm_totalFailed + int(bm_failed)
  if (line.find('ESX:')) != -1 and (line.find('COUNT')) != -1:
    esx_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    esx_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    esx_totalPassed = esx_totalPassed + int(esx_passed)
    esx_totalFailed = esx_totalFailed + int(esx_failed)
  if (line.find('VMWARE')) != -1 and (line.find('COUNT')) != -1:
    vmware_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    vmware_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    vmware_totalPassed = vmware_totalPassed + int(vmware_passed)
    vmware_totalFailed = vmware_totalFailed + int(vmware_failed)
  if (line.find('KVM')) != -1 and (line.find('COUNT')) != -1:
    kvm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    kvm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    kvm_totalPassed = kvm_totalPassed + int(kvm_passed)
    kvm_totalFailed = kvm_totalFailed + int(kvm_failed)
  if (line.find('CCI')) != -1 and (line.find('_')) == -1 and (line.find('COUNT')) != -1:
    cci_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cci_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cci_totalPassed = cci_totalPassed + int(cci_passed)
    cci_totalFailed = cci_totalFailed + int(cci_failed)
  if (line.find('CCI_RHEL7_Docker')) != -1 and (line.find('COUNT')) != -1:
    cci_rh_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cci_rh_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cci_rh_docker_totalPassed = cci_rh_docker_totalPassed + int(cci_rh_docker_passed)
    cci_rh_docker_totalFailed = cci_rh_docker_totalFailed + int(cci_rh_docker_failed)
  if (line.find('EC2_RHEL7_Docker')) != -1 and (line.find('COUNT')) != -1:
    ec2_rh_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ec2_rh_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ec2_rh_docker_totalPassed = ec2_rh_docker_totalPassed + int(ec2_rh_docker_passed)
    ec2_rh_docker_totalFailed = ec2_rh_docker_totalFailed + int(ec2_rh_docker_failed)
  if (line.find('MSA_RHEL7_Docker')) != -1 and (line.find('COUNT')) != -1:
    msa_rh_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    msa_rh_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    msa_rh_docker_totalPassed = msa_rh_docker_totalPassed + int(msa_rh_docker_passed)
    msa_rh_docker_totalFailed = msa_rh_docker_totalFailed + int(msa_rh_docker_failed)
  if (line.find('ESX_RHEL7_Docker')) != -1 and (line.find('COUNT')) != -1:
    esx_rh_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    esx_rh_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    esx_rh_docker_totalPassed = esx_rh_docker_totalPassed + int(esx_rh_docker_passed)
    esx_rh_docker_totalFailed = esx_rh_docker_totalFailed + int(esx_rh_docker_failed)
  if (line.find('CCI_CentOS7_Docker')) != -1 and (line.find('COUNT')) != -1:
    cci_co_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cci_co_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cci_co_docker_totalPassed = cci_co_docker_totalPassed + int(cci_co_docker_passed)
    cci_co_docker_totalFailed = cci_co_docker_totalFailed + int(cci_co_docker_failed)
  if (line.find('EC2_CentOS7_Docker')) != -1 and (line.find('COUNT')) != -1:
    ec2_co_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ec2_co_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ec2_co_docker_totalPassed = ec2_co_docker_totalPassed + int(ec2_co_docker_passed)
    ec2_co_docker_totalFailed = ec2_co_docker_totalFailed + int(ec2_co_docker_failed)
  if (line.find('MSA_CentOS7_Docker')) != -1 and (line.find('COUNT')) != -1:
    msa_co_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    msa_co_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    msa_co_docker_totalPassed = msa_co_docker_totalPassed + int(msa_co_docker_passed)
    msa_co_docker_totalFailed = msa_co_docker_totalFailed + int(msa_co_docker_failed)
  if (line.find('ESX_CentOS7_Docker')) != -1 and (line.find('COUNT')) != -1:
    esx_co_docker_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    esx_co_docker_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    esx_co_docker_totalPassed = esx_co_docker_totalPassed + int(esx_co_docker_passed)
    esx_co_docker_totalFailed = esx_co_docker_totalFailed + int(esx_co_docker_failed)
  if (line.find('CCI_RHEL7_RPM')) != -1 and (line.find('COUNT')) != -1:
    cci_rh_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cci_rh_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cci_rh_rpm_totalPassed = cci_rh_rpm_totalPassed + int(cci_rh_rpm_passed)
    cci_rh_rpm_totalFailed = cci_rh_rpm_totalFailed + int(cci_rh_rpm_failed)
  if (line.find('EC2_RHEL7_RPM')) != -1 and (line.find('COUNT')) != -1:
    ec2_rh_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ec2_rh_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ec2_rh_rpm_totalPassed = ec2_rh_rpm_totalPassed + int(ec2_rh_rpm_passed)
    ec2_rh_rpm_totalFailed = ec2_rh_rpm_totalFailed + int(ec2_rh_rpm_failed)
  if (line.find('MSA_RHEL7_RPM')) != -1 and (line.find('COUNT')) != -1:
    msa_rh_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    msa_rh_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    msa_rh_rpm_totalPassed = msa_rh_rpm_totalPassed + int(msa_rh_rpm_passed)
    msa_rh_rpm_totalFailed = msa_rh_rpm_totalFailed + int(msa_rh_rpm_failed)
  if (line.find('ESX_RHEL7_RPM')) != -1 and (line.find('COUNT')) != -1:
    esx_rh_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    esx_rh_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    esx_rh_rpm_totalPassed = esx_rh_rpm_totalPassed + int(esx_rh_rpm_passed)
    esx_rh_rpm_totalFailed = esx_rh_rpm_totalFailed + int(esx_rh_rpm_failed)
  if (line.find('CCI_CentOS7_RPM')) != -1 and (line.find('COUNT')) != -1:
    cci_co_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    cci_co_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    cci_co_rpm_totalPassed = cci_co_rpm_totalPassed + int(cci_co_rpm_passed)
    cci_co_rpm_totalFailed = cci_co_rpm_totalFailed + int(cci_co_rpm_failed)
  if (line.find('EC2_CentOS7_RPM')) != -1 and (line.find('COUNT')) != -1:
    ec2_co_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    ec2_co_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    ec2_co_rpm_totalPassed = ec2_co_rpm_totalPassed + int(ec2_co_rpm_passed)
    ec2_co_rpm_totalFailed = ec2_co_rpm_totalFailed + int(ec2_co_rpm_failed)
  if (line.find('MSA_CentOS7_RPM')) != -1 and (line.find('COUNT')) != -1:
    msa_co_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    msa_co_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    msa_co_rpm_totalPassed = msa_co_rpm_totalPassed + int(msa_co_rpm_passed)
    msa_co_rpm_totalFailed = msa_co_rpm_totalFailed + int(msa_co_rpm_failed)
  if (line.find('ESX_CentOS7_RPM')) != -1 and (line.find('COUNT')) != -1:
    esx_co_rpm_passed = line.split('PCOUNT:')[1].split('FCOUNT:')[0]
    esx_co_rpm_failed = line.split('PCOUNT:')[1].split('FCOUNT:')[1]
    esx_co_rpm_totalPassed = esx_co_rpm_totalPassed + int(esx_co_rpm_passed)
    esx_co_rpm_totalFailed = esx_co_rpm_totalFailed + int(esx_co_rpm_failed)

  if (line.find('RC!=0')) != -1:
    nonZeroMsg = '(RC!=0 for some tests, check individual results for details)'

totalCount          = totalPassed + totalFailed
jms_totalCount      = jms_totalPassed + jms_totalFailed
mqtt_totalCount     = mqtt_totalPassed + mqtt_totalFailed
jms_mqtt_totalCount = jms_mqtt_totalPassed + jms_mqtt_totalFailed
jms_tck_totalCount  = jms_tck_totalPassed + jms_tck_totalFailed
ws_totalCount       = ws_totalPassed + ws_totalFailed
cli_totalCount      = cli_totalPassed + cli_totalFailed
rest_totalCount     = rest_totalPassed + rest_totalFailed
plugin_totalCount   = plugin_totalPassed + plugin_totalFailed
proxy_totalCount    = proxy_totalPassed + proxy_totalFailed
ha_totalCount       = ha_totalPassed + ha_totalFailed
cluster_totalCount  = cluster_totalPassed + cluster_totalFailed
snmp_totalCount     = snmp_totalPassed + snmp_totalFailed
ui_totalCount       = ui_totalPassed + ui_totalFailed
mq_conn_totalCount  = mq_conn_totalPassed + mq_conn_totalFailed
a9006_totalCount    = a9006_totalPassed + a9006_totalFailed
bm_totalCount       = bm_totalPassed + bm_totalFailed
esx_totalCount      = esx_totalPassed + esx_totalFailed
vmware_totalCount   = vmware_totalPassed + vmware_totalFailed
kvm_totalCount      = kvm_totalPassed + kvm_totalFailed
cci_totalCount      = cci_totalPassed + cci_totalFailed
cci_rh_docker_totalCount = cci_rh_docker_totalPassed + cci_rh_docker_totalFailed
ec2_rh_docker_totalCount = ec2_rh_docker_totalPassed + ec2_rh_docker_totalFailed
msa_rh_docker_totalCount = msa_rh_docker_totalPassed + msa_rh_docker_totalFailed
esx_rh_docker_totalCount = esx_rh_docker_totalPassed + esx_rh_docker_totalFailed
cci_co_docker_totalCount = cci_co_docker_totalPassed + cci_co_docker_totalFailed
ec2_co_docker_totalCount = ec2_co_docker_totalPassed + ec2_co_docker_totalFailed
msa_co_docker_totalCount = msa_co_docker_totalPassed + msa_co_docker_totalFailed
esx_co_docker_totalCount = esx_co_docker_totalPassed + esx_co_docker_totalFailed
cci_rh_rpm_totalCount    = cci_rh_rpm_totalPassed + cci_rh_rpm_totalFailed
ec2_rh_rpm_totalCount    = ec2_rh_rpm_totalPassed + ec2_rh_rpm_totalFailed
msa_rh_rpm_totalCount    = msa_rh_rpm_totalPassed + msa_rh_rpm_totalFailed
esx_rh_rpm_totalCount    = esx_rh_rpm_totalPassed + esx_rh_rpm_totalFailed
cci_co_rpm_totalCount    = cci_co_rpm_totalPassed + cci_co_rpm_totalFailed
ec2_co_rpm_totalCount    = ec2_co_rpm_totalPassed + ec2_co_rpm_totalFailed
msa_co_rpm_totalCount    = msa_co_rpm_totalPassed + msa_co_rpm_totalFailed
esx_co_rpm_totalCount    = esx_co_rpm_totalPassed + esx_co_rpm_totalFailed

if totalPassed > 0:
  percentPassed = '%.2f' % ((float(totalPassed) / float(totalCount)) * 100)
if jms_totalPassed > 0:
  jms_percentPassed = '%.2f' % ((float(jms_totalPassed) / float(jms_totalCount)) * 100)
if mqtt_totalPassed > 0:
  mqtt_percentPassed = '%.2f' % ((float(mqtt_totalPassed) / float(mqtt_totalCount)) * 100)
if jms_mqtt_totalPassed > 0:
  jms_mqtt_percentPassed = '%.2f' % ((float(jms_mqtt_totalPassed) / float(jms_mqtt_totalCount)) * 100)
if jms_tck_totalPassed > 0:
  jms_tck_percentPassed = '%.2f' % ((float(jms_tck_totalPassed) / float(jms_tck_totalCount)) * 100)
if ws_totalPassed > 0:
  ws_percentPassed = '%.2f' % ((float(ws_totalPassed) / float(ws_totalCount)) * 100)
if cli_totalPassed > 0:
  cli_percentPassed = '%.2f' % ((float(cli_totalPassed) / float(cli_totalCount)) * 100)
if rest_totalPassed > 0:
  rest_percentPassed = '%.2f' % ((float(rest_totalPassed) / float(rest_totalCount)) * 100)
if plugin_totalPassed > 0:
  plugin_percentPassed = '%.2f' % ((float(plugin_totalPassed) / float(plugin_totalCount)) * 100)
if proxy_totalPassed > 0:
  proxy_percentPassed = '%.2f' % ((float(proxy_totalPassed) / float(proxy_totalCount)) * 100)
if ha_totalPassed > 0:
  ha_percentPassed = '%.2f' % ((float(ha_totalPassed) / float(ha_totalCount)) * 100)
if cluster_totalPassed > 0:
  cluster_percentPassed = '%.2f' % ((float(cluster_totalPassed) / float(cluster_totalCount)) * 100)
if snmp_totalPassed > 0:
  snmp_percentPassed = '%.2f' % ((float(snmp_totalPassed) / float(snmp_totalCount)) * 100)
if ui_totalPassed > 0:
  ui_percentPassed = '%.2f' % ((float(ui_totalPassed) / float(ui_totalCount)) * 100)
if mq_conn_totalPassed > 0:
  mq_conn_percentPassed = '%.2f' % ((float(mq_conn_totalPassed) / float(mq_conn_totalCount)) * 100)
if a9006_totalPassed > 0:
  a9006_percentPassed = '%.2f' % ((float(a9006_totalPassed) / float(a9006_totalCount)) * 100)
if bm_totalPassed > 0:
  bm_percentPassed = '%.2f' % ((float(bm_totalPassed) / float(bm_totalCount)) * 100)
if esx_totalPassed > 0:
  esx_percentPassed = '%.2f' % ((float(esx_totalPassed) / float(esx_totalCount)) * 100)
if vmware_totalPassed > 0:
  vmware_percentPassed = '%.2f' % ((float(vmware_totalPassed) / float(vmware_totalCount)) * 100)
if kvm_totalPassed > 0:
  kvm_percentPassed = '%.2f' % ((float(kvm_totalPassed) / float(kvm_totalCount)) * 100)
if cci_totalPassed > 0:
  cci_percentPassed = '%.2f' % ((float(cci_totalPassed) / float(cci_totalCount)) * 100)
if cci_rh_docker_totalPassed > 0:
  cci_rh_docker_percentPassed = '%.2f' % ((float(cci_rh_docker_totalPassed) / float(cci_rh_docker_totalCount)) * 100)
if ec2_rh_docker_totalPassed > 0:
  ec2_rh_docker_percentPassed = '%.2f' % ((float(ec2_rh_docker_totalPassed) / float(ec2_rh_docker_totalCount)) * 100)
if msa_rh_docker_totalPassed > 0:
  msa_rh_docker_percentPassed = '%.2f' % ((float(msa_rh_docker_totalPassed) / float(msa_rh_docker_totalCount)) * 100)
if esx_rh_docker_totalPassed > 0:
  esx_rh_docker_percentPassed = '%.2f' % ((float(esx_rh_docker_totalPassed) / float(esx_rh_docker_totalCount)) * 100)
if cci_co_docker_totalPassed > 0:
  cci_co_docker_percentPassed = '%.2f' % ((float(cci_co_docker_totalPassed) / float(cci_co_docker_totalCount)) * 100)
if ec2_co_docker_totalPassed > 0:
  ec2_co_docker_percentPassed = '%.2f' % ((float(ec2_co_docker_totalPassed) / float(ec2_co_docker_totalCount)) * 100)
if msa_co_docker_totalPassed > 0:
  msa_co_docker_percentPassed = '%.2f' % ((float(msa_co_docker_totalPassed) / float(msa_co_docker_totalCount)) * 100)
if esx_co_docker_totalPassed > 0:
  esx_co_docker_percentPassed = '%.2f' % ((float(esx_co_docker_totalPassed) / float(esx_co_docker_totalCount)) * 100)
if cci_rh_rpm_totalPassed > 0:
  cci_rh_rpm_percentPassed = '%.2f' % ((float(cci_rh_rpm_totalPassed) / float(cci_rh_rpm_totalCount)) * 100)
if ec2_rh_rpm_totalPassed > 0:
  ec2_rh_rpm_percentPassed = '%.2f' % ((float(ec2_rh_rpm_totalPassed) / float(ec2_rh_rpm_totalCount)) * 100)
if msa_rh_rpm_totalPassed > 0:
  msa_rh_rpm_percentPassed = '%.2f' % ((float(msa_rh_rpm_totalPassed) / float(msa_rh_rpm_totalCount)) * 100)
if esx_rh_rpm_totalPassed > 0:
  esx_rh_rpm_percentPassed = '%.2f' % ((float(esx_rh_rpm_totalPassed) / float(esx_rh_rpm_totalCount)) * 100)
if cci_co_rpm_totalPassed > 0:
  cci_co_rpm_percentPassed = '%.2f' % ((float(cci_co_rpm_totalPassed) / float(cci_co_rpm_totalCount)) * 100)
if ec2_co_rpm_totalPassed > 0:
  ec2_co_rpm_percentPassed = '%.2f' % ((float(ec2_co_rpm_totalPassed) / float(ec2_co_rpm_totalCount)) * 100)
if msa_co_rpm_totalPassed > 0:
  msa_co_rpm_percentPassed = '%.2f' % ((float(msa_co_rpm_totalPassed) / float(msa_co_rpm_totalCount)) * 100)
if esx_co_rpm_totalPassed > 0:
  esx_co_rpm_percentPassed = '%.2f' % ((float(esx_co_rpm_totalPassed) / float(esx_co_rpm_totalCount)) * 100)

reportFile = open( reportFILE, 'a+')
reportFile.write("\nIMA Automated Framework Summary Report\n")
reportFile.write("==========================================================================================================================================\n")
reportFile.write("Build Type:       " + bldType + "\n")
if testGroup.find('migrate') != -1:
  reportFile.write("Server Release:   " + migrateRelease + " @ " + migrateLabel + " migrated to " + serverRelease + " @ " + serverLabel + "\n")
else:
  reportFile.write("Server Release:   " + serverRelease + " @ " + serverLabel + "\n")
  reportFile.write("Proxy Release:    " + serverRelease + " @ " + serverLabel + "\n")
reportFile.write("Client Release:   " + clientRelease + " @ " + clientLabel + "\n")
reportFile.write("Test Release:     " + testRelease + " @ " + testLabel + "\n")
reportFile.write("Test Group:       " + testGroup + "\n")
reportFile.write("Total Time:       " + totalTime +"\n\n")
reportFile.write("Total Results:    Passed: " + str(totalPassed).rjust(5) + "   Failed: " + str(totalFailed).rjust(5) + "   Success: " + str(percentPassed).rjust(6) + "%\n")
reportFile.write("                  " + str(nonZeroMsg) + "\n\n")
reportFile.write("Platform Results:\n")
reportFile.write("-------------------\n")
if int(a9006_totalCount) != 0:
  reportFile.write("9006:                 Passed: " + str(a9006_totalPassed).rjust(5) + "   Failed: " + str(a9006_totalFailed).rjust(5) + "   Success: " + str(a9006_percentPassed).rjust(6) + "%\n")
if int(bm_totalCount) != 0:
  reportFile.write("Bare Metal:           Passed: " + str(bm_totalPassed).rjust(5) + "   Failed: " + str(bm_totalFailed).rjust(5) + "   Success: " + str(bm_percentPassed).rjust(6) + "%\n")
if int(esx_totalCount) != 0:
  reportFile.write("ESX:                  Passed: " + str(esx_totalPassed).rjust(5) + "   Failed: " + str(esx_totalFailed).rjust(5) + "   Success: " + str(esx_percentPassed).rjust(6) + "%\n")
if int(vmware_totalCount) != 0: 
  reportFile.write("VMWARE:               Passed: " + str(vmware_totalPassed).rjust(5) + "   Failed: " + str(vmware_totalFailed).rjust(5) + "   Success: " + str(vmware_percentPassed).rjust(6) + "%\n")
if int(kvm_totalCount) != 0:
  reportFile.write("KVM:                  Passed: " + str(kvm_totalPassed).rjust(5) + "   Failed: " + str(kvm_totalFailed).rjust(5) + "   Success: " + str(kvm_percentPassed).rjust(6) + "%\n")
if int(cci_totalCount) != 0:
  reportFile.write("CCI:                  Passed: " + str(cci_totalPassed).rjust(5) + "   Failed: " + str(cci_totalFailed).rjust(5) + "   Success: " + str(cci_percentPassed).rjust(6) + "%\n")
if int(cci_rh_docker_totalCount) != 0:
  reportFile.write("CCI_RHEL_DOCKER:      Passed: " + str(cci_rh_docker_totalPassed).rjust(5) + "   Failed: " + str(cci_rh_docker_totalFailed).rjust(5) + "   Success: " + str(cci_rh_docker_percentPassed).rjust(6) + "%\n")
if int(ec2_rh_docker_totalCount) != 0:
  reportFile.write("EC2_RHEL_DOCKER:      Passed: " + str(ec2_rh_docker_totalPassed).rjust(5) + "   Failed: " + str(ec2_rh_docker_totalFailed).rjust(5) + "   Success: " + str(ec2_rh_docker_percentPassed).rjust(6) + "%\n")
if int(msa_rh_docker_totalCount) != 0:
  reportFile.write("MSA_RHEL_DOCKER:      Passed: " + str(msa_rh_docker_totalPassed).rjust(5) + "   Failed: " + str(msa_rh_docker_totalFailed).rjust(5) + "   Success: " + str(msa_rh_docker_percentPassed).rjust(6) + "%\n")
if int(esx_rh_docker_totalCount) != 0:
  reportFile.write("ESX_RHEL_DOCKER:      Passed: " + str(esx_rh_docker_totalPassed).rjust(5) + "   Failed: " + str(esx_rh_docker_totalFailed).rjust(5) + "   Success: " + str(esx_rh_docker_percentPassed).rjust(6) + "%\n")
if int(cci_co_docker_totalCount) != 0:
  reportFile.write("CCI_CENTOS_DOCKER:    Passed: " + str(cci_co_docker_totalPassed).rjust(5) + "   Failed: " + str(cci_co_docker_totalFailed).rjust(5) + "   Success: " + str(cci_co_docker_percentPassed).rjust(6) + "%\n")
if int(ec2_co_docker_totalCount) != 0:
  reportFile.write("EC2_CENTOS_DOCKER:    Passed: " + str(ec2_co_docker_totalPassed).rjust(5) + "   Failed: " + str(ec2_co_docker_totalFailed).rjust(5) + "   Success: " + str(ec2_co_docker_percentPassed).rjust(6) + "%\n")
if int(msa_co_docker_totalCount) != 0:
  reportFile.write("MSA_CENTOS_DOCKER:    Passed: " + str(msa_co_docker_totalPassed).rjust(5) + "   Failed: " + str(msa_co_docker_totalFailed).rjust(5) + "   Success: " + str(msa_co_docker_percentPassed).rjust(6) + "%\n")
if int(esx_co_docker_totalCount) != 0:
  reportFile.write("ESX_CENTOS_DOCKER:    Passed: " + str(esx_co_docker_totalPassed).rjust(5) + "   Failed: " + str(esx_co_docker_totalFailed).rjust(5) + "   Success: " + str(esx_co_docker_percentPassed).rjust(6) + "%\n")
if int(cci_rh_rpm_totalCount) != 0:
  reportFile.write("CCI_RHEL_RPM:         Passed: " + str(cci_rh_rpm_totalPassed).rjust(5) + "   Failed: " + str(cci_rh_rpm_totalFailed).rjust(5) + "   Success: " + str(cci_rh_rpm_percentPassed).rjust(6) + "%\n")
if int(ec2_rh_rpm_totalCount) != 0:
  reportFile.write("EC2_RHEL_RPM:         Passed: " + str(ec2_rh_rpm_totalPassed).rjust(5) + "   Failed: " + str(ec2_rh_rpm_totalFailed).rjust(5) + "   Success: " + str(ec2_rh_rpm_percentPassed).rjust(6) + "%\n")
if int(msa_rh_rpm_totalCount) != 0:
  reportFile.write("MSA_RHEL_RPM:         Passed: " + str(msa_rh_rpm_totalPassed).rjust(5) + "   Failed: " + str(msa_rh_rpm_totalFailed).rjust(5) + "   Success: " + str(msa_rh_rpm_percentPassed).rjust(6) + "%\n")
if int(esx_rh_rpm_totalCount) != 0:
  reportFile.write("ESX_RHEL_RPM:         Passed: " + str(esx_rh_rpm_totalPassed).rjust(5) + "   Failed: " + str(esx_rh_rpm_totalFailed).rjust(5) + "   Success: " + str(esx_rh_rpm_percentPassed).rjust(6) + "%\n")
if int(cci_co_rpm_totalCount) != 0:
  reportFile.write("CCI_CENTOS_RPM:       Passed: " + str(cci_co_rpm_totalPassed).rjust(5) + "   Failed: " + str(cci_co_rpm_totalFailed).rjust(5) + "   Success: " + str(cci_co_rpm_percentPassed).rjust(6) + "%\n")
if int(ec2_co_rpm_totalCount) != 0:
  reportFile.write("EC2_CENTOS_RPM:       Passed: " + str(ec2_co_rpm_totalPassed).rjust(5) + "   Failed: " + str(ec2_co_rpm_totalFailed).rjust(5) + "   Success: " + str(ec2_co_rpm_percentPassed).rjust(6) + "%\n")
if int(msa_co_rpm_totalCount) != 0:
  reportFile.write("MSA_CENTOS_RPM:       Passed: " + str(msa_co_rpm_totalPassed).rjust(5) + "   Failed: " + str(msa_co_rpm_totalFailed).rjust(5) + "   Success: " + str(msa_co_rpm_percentPassed).rjust(6) + "%\n")
if int(esx_co_rpm_totalCount) != 0:
  reportFile.write("ESX_CENTOS_RPM:       Passed: " + str(esx_co_rpm_totalPassed).rjust(5) + "   Failed: " + str(esx_co_rpm_totalFailed).rjust(5) + "   Success: " + str(esx_co_rpm_percentPassed).rjust(6) + "%\n")
reportFile.write("\nIndividual Results:\n")
reportFile.write("-------------------\n")
if int(jms_totalCount) != 0:
  reportFile.write("JMS:                  Passed: " + str(jms_totalPassed).rjust(5) + "   Failed: " + str(jms_totalFailed).rjust(5) + "   Success: " + str(jms_percentPassed).rjust(6) + "%\n")
if int(jms_mqtt_totalCount) != 0:
  reportFile.write("JMS_MQTT:             Passed: " + str(jms_mqtt_totalPassed).rjust(5) + "   Failed: " + str(jms_mqtt_totalFailed).rjust(5) + "   Success: " + str(jms_mqtt_percentPassed).rjust(6) + "%\n")
if int(jms_tck_totalCount) != 0:
  reportFile.write("JMS_TCK:              Passed: " + str(jms_tck_totalPassed).rjust(5) + "   Failed: " + str(jms_tck_totalFailed).rjust(5) + "   Success: " + str(jms_tck_percentPassed).rjust(6) + "%\n")
if int(mqtt_totalCount) != 0:
  reportFile.write("MQTT:                 Passed: " + str(mqtt_totalPassed).rjust(5) + "   Failed: " + str(mqtt_totalFailed).rjust(5) + "   Success: " + str(mqtt_percentPassed).rjust(6) + "%\n")
if int(ws_totalCount) != 0:
  reportFile.write("WS:                   Passed: " + str(ws_totalPassed).rjust(5) + "   Failed: " + str(ws_totalFailed).rjust(5) + "   Success: " + str(ws_percentPassed).rjust(6) + "%\n")
if int(cli_totalCount) != 0: 
  reportFile.write("CLI:                  Passed: " + str(cli_totalPassed).rjust(5) + "   Failed: " + str(cli_totalFailed).rjust(5) + "   Success: " + str(cli_percentPassed).rjust(6) + "%\n")
if int(rest_totalCount) != 0:
  reportFile.write("REST_API:             Passed: " + str(rest_totalPassed).rjust(5) + "   Failed: " + str(rest_totalFailed).rjust(5) + "   Success: " + str(rest_percentPassed).rjust(6) + "%\n")
if int(plugin_totalCount) != 0: 
  reportFile.write("PLUGIN:               Passed: " + str(plugin_totalPassed).rjust(5) + "   Failed: " + str(plugin_totalFailed).rjust(5) + "   Success: " + str(plugin_percentPassed).rjust(6) + "%\n")
if int(proxy_totalCount) != 0:
  reportFile.write("PROXY:                Passed: " + str(proxy_totalPassed).rjust(5) + "   Failed: " + str(proxy_totalFailed).rjust(5) + "   Success: " + str(proxy_percentPassed).rjust(6) + "%\n")
if int(mq_conn_totalCount) != 0:
  reportFile.write("MQ_CONN:              Passed: " + str(mq_conn_totalPassed).rjust(5) + "   Failed: " + str(mq_conn_totalFailed).rjust(5) + "   Success: " + str(mq_conn_percentPassed).rjust(6) + "%\n")
if int(ha_totalCount) != 0:
  reportFile.write("HA:                   Passed: " + str(ha_totalPassed).rjust(5) + "   Failed: " + str(ha_totalFailed).rjust(5) + "   Success: " + str(ha_percentPassed).rjust(6) + "%\n")
if int(cluster_totalCount) != 0:
  reportFile.write("CLUSTER:              Passed: " + str(cluster_totalPassed).rjust(5) + "   Failed: " + str(cluster_totalFailed).rjust(5) + "   Success: " + str(cluster_percentPassed).rjust(6) + "%\n")
if int(ui_totalCount) != 0:
  reportFile.write("WEBUI:                Passed: " + str(ui_totalPassed).rjust(5) + "   Failed: " + str(ui_totalFailed).rjust(5) + "   Success: " + str(ui_percentPassed).rjust(6) + "%\n")
if int(snmp_totalCount) != 0:
  reportFile.write("SNMP:                 Passed: " + str(snmp_totalPassed).rjust(5) + "   Failed: " + str(snmp_totalFailed).rjust(5) + "   Success: " + str(snmp_percentPassed).rjust(6) + "%\n")

if totalFailed > 0:
  reportFile.write("\n==========================================================================================================================================\n")
  reportFile.write("=========================================== Summary of Failures by Number of Occurrences =================================================\n")
  reportFile.write("==========================================================================================================================================\n")
  failedList = []
  dataFile.seek(0)
  for line in dataFile:
#    print line
    if (line.find('FAILED')) != -1:
      line = line.split('FAILED',1)[1]
      if (line.find(':')) != -1:
        line = line.split(':',1)[1]
      if (line.lower()).find('scenario') != -1:
        if (line.find('-')) != -1:
          line = line.split('-',1)[1]
      if len(line) > 5:
        line = line.strip()
        failedList.append(line)

#  print str(len(failedList))

#  frc = 1
  count = 1
  newfailedList = []
  failedList.sort()
  last = failedList[-1]
  for i in range(len(failedList)-2, -1, -1):
    if last == failedList[i]:
      count = count + 1
      del failedList[i]
    else:
#      frc = frc + count
      newfailedList.append( [ count , last ] )
      count = 1
      last = failedList[i]  
  newfailedList.append( [ count , last ] )

#  print frc

  newfailedList.sort()
  newfailedList.reverse()
  for line in newfailedList:
    if len(str(line[0])) == 1:
      reportFile.write(" " + str(line[0]) + "  " + line[1] + "\n")
    else:
      reportFile.write(str(line[0]) + "  " + line[1] + "\n")
reportFile.write("\n==========================================================================================================================================\n")
reportFile.write("================================================== Summary of Individual Test Runs =======================================================\n")
reportFile.write("==========================================================================================================================================\n")

dataFile.seek(0)
for line in dataFile:
  if (line.find('FAILED:')) != -1: ### write FAILED lines to the report
    if (line.rstrip().endswith('FAILED:')):
      reportFile.write("  " + line)
    else:
      reportFile.write("      " + line)
  else:
    if (line.find('PCOUNT:')) == -1: #### write the HEADER lines to the report (do not write PCOUNT lines)
      reportFile.write(line.replace(testGroup + ": ", "").replace("(32bit)", "32bit").replace("(64bit)", "64bit"))

reportFile.flush()
reportFile.close()
dataFile.flush()
dataFile.close()

#### ADD RESULTS TO CSV FILE ####
csvFile = open( csvFILE, 'a')
csvFile.write( serverLabel + "," + str(percentPassed) + "%\n")
csvFile.flush()
csvFile.close()

#### POST RESULTS ####
if testGroup == 'bvt' or testGroup == 'fvt_default' or testGroup == 'fvt_prod' or testGroup == 'svtlong_prod' or testGroup == 'svt_prod':
  os.system("ssh " + pubServerIP + " mkdir -p /gsacache/data/af/" + serverRelease + "/" + staxLocation + "/" + testGroup )
  os.system("scp " + csvFILE + " " + pubServerIP + ":/gsacache/data/af/" + serverRelease + "/" + staxLocation + "/" + testGroup )
  os.system("ssh " + pubServerIP + " mkdir -p /gsacache/data/af/" + serverRelease + "/" + staxLocation + "/" + testGroup + "/reports/" )
  os.system("scp " + reportFILE + " " + pubServerIP + ":/gsacache/data/af/" + serverRelease + "/" + staxLocation + "/" + testGroup + "/reports/" + testGroup + '.' + serverLabel)
if bldType == 'personal':
  os.system("ssh " + pubServerIP + " mkdir -p /gsacache/data/af/" + serverRelease + "/" + staxLocation + "/" + "/bvt_personal/" + userID + "/" + serverLabel )
  os.system("scp " + reportFILE + " " + pubServerIP + ":/gsacache/data/af/" + serverRelease + "/" + staxLocation + "/bvt_personal" + "/" + userID + "/" + serverLabel)
#### FOR RELEASES 15a and LATER, POST SUMMARY REPORT IN results directory for the build
if serverRelease.find('IMA100') == -1 and serverRelease.find('IMA110') == -1 and serverRelease.find('IMA120') == -1:
  if testGroup == 'bvt':
    os.system("scp " + reportFILE + " " + pubServerIP + ":/gsacache/release/" + serverRelease + "/" + bldType + "/" + serverLabel + "/results/bvt_completed")
  if testGroup == 'fvt_default' or testGroup == 'fvt_prod':
    os.system("scp " + reportFILE + " " + pubServerIP + ":/gsacache/release/" + serverRelease + "/" + bldType + "/" + serverLabel + "/results/fvt_completed")
  if testGroup == 'svt_prod':
    os.system("scp " + reportFILE + " " + pubServerIP + ":/gsacache/release/" + serverRelease + "/" + bldType + "/" + serverLabel + "/results/svt_completed")
