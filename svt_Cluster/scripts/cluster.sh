source ../scripts/ISMsetup.sh
source ../testEnv.sh


curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA12","LocalDiscoveryNIC":"'${A1_IPv4_1}'","LocalReplicationNIC":"'${A1_IPv4_1}'","RemoteDiscoveryNIC":"'${A2_IPv4_1}'","PreferredPrimary":true}}' http://${A1_HOST}:${A1_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A1_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A1_HOST}:${A1_PORT}/ima/v1/configuration
curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA12","LocalDiscoveryNIC":"'${A2_IPv4_1}'","LocalReplicationNIC":"'${A2_IPv4_1}'","RemoteDiscoveryNIC":"'${A1_IPv4_1}'","PreferredPrimary":false}}' http://${A2_HOST}:${A2_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A2_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A2_HOST}:${A2_PORT}/ima/v1/configuration

curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA34","LocalDiscoveryNIC":"'${A3_IPv4_1}'","LocalReplicationNIC":"'${A3_IPv4_1}'","RemoteDiscoveryNIC":"'${A4_IPv4_1}'","PreferredPrimary":true}}' http://${A3_HOST}:${A3_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A3_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A3_HOST}:${A3_PORT}/ima/v1/configuration
curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA34","LocalDiscoveryNIC":"'${A4_IPv4_1}'","LocalReplicationNIC":"'${A4_IPv4_1}'","RemoteDiscoveryNIC":"'${A3_IPv4_1}'","PreferredPrimary":false}}' http://${A4_HOST}:${A4_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A4_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A4_HOST}:${A4_PORT}/ima/v1/configuration

curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA56","LocalDiscoveryNIC":"'${A5_IPv4_1}'","LocalReplicationNIC":"'${A5_IPv4_1}'","RemoteDiscoveryNIC":"'${A6_IPv4_1}'","PreferredPrimary":true}}' http://${A5_HOST}:${A5_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A5_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A5_HOST}:${A5_PORT}/ima/v1/configuration
curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA56","LocalDiscoveryNIC":"'${A6_IPv4_1}'","LocalReplicationNIC":"'${A6_IPv4_1}'","RemoteDiscoveryNIC":"'${A5_IPv4_1}'","PreferredPrimary":false}}' http://${A6_HOST}:${A6_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A6_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A6_HOST}:${A6_PORT}/ima/v1/configuration

curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA78","LocalDiscoveryNIC":"'${A7_IPv4_1}'","LocalReplicationNIC":"'${A7_IPv4_1}'","RemoteDiscoveryNIC":"'${A8_IPv4_1}'","PreferredPrimary":true}}' http://${A7_HOST}:${A7_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A7_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A7_HOST}:${A7_PORT}/ima/v1/configuration
curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA78","LocalDiscoveryNIC":"'${A8_IPv4_1}'","LocalReplicationNIC":"'${A8_IPv4_1}'","RemoteDiscoveryNIC":"'${A7_IPv4_1}'","PreferredPrimary":false}}' http://${A8_HOST}:${A8_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A8_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A8_HOST}:${A8_PORT}/ima/v1/configuration

curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA910","LocalDiscoveryNIC":"'${A9_IPv4_1}'","LocalReplicationNIC":"'${A9_IPv4_1}'","RemoteDiscoveryNIC":"'${A10_IPv4_1}'","PreferredPrimary":true}}' http://${A9_HOST}:${A9_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A9_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A9_HOST}:${A9_PORT}/ima/v1/configuration
curl -X POST -d '{"HighAvailability":{"EnableHA":true,"Group":"'${MQKEY}'_HA910","LocalDiscoveryNIC":"'${A10_IPv4_1}'","LocalReplicationNIC":"'${A10_IPv4_1}'","RemoteDiscoveryNIC":"'${A9_IPv4_1}'","PreferredPrimary":false}}' http://${A10_HOST}:${A10_PORT}/ima/v1/configuration
curl -X POST -d '{"ClusterMembership":{"EnableClusterMembership":true,"UseMulticastDiscovery":true,"ControlPort":9201,"ControlAddress":"'${A10_IPv4_1}'","MessagingPort":9202,"ClusterName":"'${MQKEY}'_CLUSTER","DiscoveryPort":9106,"DiscoveryServerList":"'${A1_IPv4_1}':9201"}}' http://${A10_HOST}:${A10_PORT}/ima/v1/configuration









