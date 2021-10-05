/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

/******************************************************************************************************
 * ----------   CUNIT TES FUNCTIONS FOR VALIDATING ClusterMembership  Object   ----------------------
 * EnableClusterMembership : Identifies whether clustering is enabled.
 * ClusterName            : The name of the cluster. Required.
 * UseMulticastDiscovery  : Identifies whether cluster members are in a list or
 *                          discovered by multicast. Optional.
 * MulticastDiscoveryTTL  : When UseMulticastDiscovery is true, the number of
 *                          routers (hops) that multicast traffic is allowed to
 *                          pass through. Optional.
 * DiscoveryServerList    : Comma-delimited list of members belonging to the
 *                          cluster this server wants to join. Required if
 *                          UseMulticastDiscovery is False.
 * ControlAddress         : The IP Address of the control interface. Required if
 *                          EnableClusterMembership is True.
 * ControlPort            : The port number to use for the ControlInterface.
 *                          Though required, does not need to be specified
 *                          because there is a default.
 * ControlExternalAddress : The external IP Address or Hostname of the control interface
 *                          if different from ControlAddress.
 * ControlExternalPort    : The external port number to use for the ControlExternalAddress.
 * MessagingAddress       : The IP Address of the messaging channel, if different from
 *                          the ControlAddress.
 * MessagingPort          : The port number to use for the MessagingInterface, must be
 *                          different from the ControlPort.
 * MessagingExternalAddress : The external IP Address or Hostname of the messaging channel
 *                          if different from MessagingAddress.
 * MessagingExternalPort  : The external port number to use for the MessagingExternalAddress.
 * MessagingUseTLS        : Specify if the messaging channel should use TLS.
 * DiscoveryPort          : Port number used for multicast discovery. This port must
 *                          be identical for all members of the cluster.
 * DiscoveryTime          : The time, in seconds, that Cluster spends during server
 *                          start up to discover other servers in the cluster and
 *                          get updated information from them."
 ******************************************************************************************************/

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int clustestno = 0;

/*
 * Fake callback for Cluster
 */
int clusterCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Cluster call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING ClusterMembership Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_ClusterMembership(
    char *mbuf,
    int32_t exprc)
{
    json_error_t error = {0};
    const char *object = NULL;
    json_t *mergedObj = NULL;
    json_t *mobj = NULL;
    json_t *post = NULL;

    post = json_loads(mbuf, 0, &error);
    if ( post ) {
        mobj = json_loads(mbuf, 0, &error);
        if ( mobj ) {
            void *objiter = json_object_iter(mobj);
            while (objiter) {
                object = json_object_iter_key(objiter);
                mergedObj = json_object_iter_value(objiter);
                break;
            }
        }
    }

    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    /* no difference in CREATE and UPDATE */
    int rc = ism_config_validate_ClusterMembership(post, mergedObj, (char *)object, "cluster", CREATE, props);
    clustestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", clustestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_ClusterMembership(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("Admin INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_CLUSTER, NULL, (ism_config_callback_t) clusterCallback, &handle);

    printf("ClusterMembership ism_config_register(): rc: %d\n", rc);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": true }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": true, \"ClusterName\":\"TestCluster\", \"ControlAddress\":\"127.0.0.1\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": true, \"ClusterName\":\"TestCluster\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": true, \"ClusterName\":\"TestCluster\", \"ControlAddress\":\"127.0.0.1\", \"UseMulticastDiscovery\": true }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": true, \"ClusterName\":\"TestCluster\", \"ControlAddress\":\"127.0.0.1\", \"UseMulticastDiscovery\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": true, \"ClusterName\":\"TestCluster\", \"ControlAddress\":\"127.0.0.1\", \"UseMulticastDiscovery\": false, \"DiscoveryServerList\":\"9.3.179.145:7789\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"UseMulticastDiscovery\": true }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"UseMulticastDiscovery\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"UseMulticastDiscovery\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    /* Test MulticastDiscoveryTTL */
    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": 1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": 256 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": 257 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": 0 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": -1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": 10.2 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MulticastDiscoveryTTL\": null }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    /* Test ControlPort */
    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": 1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": 65535 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": 65536 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": 0 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": -1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": 10.2 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlPort\": null }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    /* Test ControlExternalPort */
    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": 1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": 65535 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": 65536 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": 0 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": -1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": 10.2 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"ControlExternalPort\": null }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    /* Test MessagingPort */
    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": 1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": 65535 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": 65536 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": 0 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": -1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": 10.2 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingPort\": null }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );


    /* Test MessagingExternalPort */
    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": 1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": 65535 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": \"false\" }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": false }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": 65536 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": 0 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": -1 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": 10.2 }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"ClusterMembership\": { \"EnableClusterMembership\": false, \"MessagingExternalPort\": null }}";
    CU_ASSERT( validate_ClusterMembership(mbuf, ISMRC_OK) == ISMRC_OK );


    return;
}

