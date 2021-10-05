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

/*
 * Add CUNIT tests for AdminEndpoint configuration data validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int sptestno = 0;

/*
 * Fake callback for transport
 */
int secProfileTransportCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Transport call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING AdminEndpoint Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateSecurityProfile() API */
static int32_t validate_SecurityProfile(
    char *mbuf,
    int32_t exprc)
{
    json_error_t error = {0};
    json_t *post = json_loads(mbuf, 0, &error);
    if ( !post ) {
        printf("post: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( post != NULL );

    /* Loop thru post json object, validate object, invoke callback and persist data */
    json_t *mobj = json_loads(mbuf, 0, &error);
    if ( !mobj ) {
        printf("mobj: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( mobj != NULL );

    const char *name = NULL;
    json_t *mergedObj = NULL;

    void *objiter = json_object_iter(mobj);
    while (objiter) {
        name = json_object_iter_key(objiter);
        mergedObj = json_object_iter_value(objiter);
        break;
    }

    CU_ASSERT( name != NULL );
    CU_ASSERT( mergedObj != NULL );

    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    /* no difference in CREATE and UPDATE */
    int rc = ism_config_validate_SecurityProfile(post, mergedObj, "SecurityProfile", (char *)name, CREATE, props);
    sptestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", sptestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}

/* Cunit test for AdminEndpoint configuration object validation */
void test_validate_configObject_SecurityProfile(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    setenv("ISMSSN", "SSN4BVT", 1);
    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("SecurityProfile INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, NULL,
        (ism_config_callback_t) secProfileTransportCallback, &handle);

    printf("SecurityProfile ism_config_register(): rc: %d\n", rc);

    /* Add config items required for AdminEndpoint unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;

    /* Add ConfigurationPolicy - name: colpol */
    ibuf = "{ \"conpol\": { \"ClientID\":\"*\", \"ActionList\":\"Browse\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("ConfigurationPolicy", "conpol", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Add SecurityProfile - name: secprof */
    ibuf = "{ \"secpol\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("SecurityProfile", "secprof", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Add AdminEndpoint - name: AdminEndpoint */
    ibuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolicies\":\"conpol\", \"SecurityProfile\":\"secprof\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("AdminEndpoint", "AdminEndpoint", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */
    /*
    "SecurityProfile":{
          "Component":"Transport",
          "About":"Transport level security profile.",
          "ObjectType":"Composite",
          "ListObjects":"Name,TLSEnabled,MinimumProtocolMethod,UseClientCertificate,UsePasswordAuthentication,Ciphers,CertificateProfile,UseClientCipher,LTPAProfile,OAuthProfile",
          "Name":{
              "Type":"Name",
              "MaxLength":"32",
              "Restrictions":"No leading or trailing space. No control characters or comma.",
              "About":"Name of the security profile.",
              "RequiredField":"yes"
          },

          "Ciphers":{
              "Type":"Enum",
              "Options":"Best,Medium,Fast",
              "Default":"Medium",
              "About":"Ciphers.",
              "RequiredField":"yes"
          },
          "CertificateProfile":{
              "Type":"String",
              "Default":"",
              "MaxLength":"256",
              "About":"Name of a certificate profile.",
              "RequiredField":"no"
          },
          "UseClientCipher":{
              "Type":"Boolean",
              "Default":"False",
              "Options":"True,False",
              "About":"Use Client's Cipher.",
              "RequiredField":"yes"
          },
          "LTPAProfile":{
              "Type":"String",
              "Default":"",
              "MaxLength":"1024",
              "About":"Name of an LTPA profile.",
              "RequiredField":"no"
          },
          "OAuthProfile":{
              "Type":"String",
              "Default":"",
              "MaxLength":"1024",
              "About":"Name of an OAuth profile.",
              "RequiredField":"no"
          },
          "CRLProfile":{
          	  "Type":"String",
              "Default":"",
              "MaxLength":"1024",
              "About":"Name of an OAuth profile.",
              "RequiredField":"no"
          }

      } */
    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    // Sample buffer:
    // mbuf = "{ \"SecurityProfile\": { \"secpol\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false }} }";

    /* 1. Create: SecurityProfile with UsePasswordAuthentication: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Update an existing SecurityProfile with MinimumProtocolMethod: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update an existing SecurityProfile with UseClientCertificate: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Update an existing SecurityProfile with Ciphers: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"Ciphers\":\"Fast\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 6. Update an existing SecurityProfile with UseClientCipher: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"Ciphers\":\"Fast\", \"UseClientCipher\":true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 7. Create: SecurityProfile with quotes on true */
    mbuf = "{ \"secpo3\": { \"TLSEnabled\": \"true\", \"UsePasswordAuthentication\": false }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    /* 8. Update an existing SecurityProfile with MinimumProtocolMethod value not in the option : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv7.2\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 9. Update an existing SecurityProfile with non-exist certificate profile: rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"CertificateProfile\":\"notExistCertP\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 10. Update an existing SecurityProfile with non-exist LTPAProfile: rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"LTPAProfile\":\"notExistLTPAP\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 11. Update an existing SecurityProfile with non-exist OAuthProfile: rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"OAuthProfile\":\"notExistOAuthP\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 12. Update an existing SecurityProfile with non-exist CRLProfile: rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"CRLProfile\":\"notExistCRLP\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 13. Create Security Profile with non-alphanumeric characters: rc=6206 */
    mbuf = "{ \"BAD+TOO\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"Ciphers\":\"Fast\", \"UseClientCipher\":true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, 6206) == 6206 );

    /* 14. Create Security Profile with non-alphanumeric characters: rc=ISMRC_UnicodeNotValid */
    mbuf = "{ \"-ReallyBad\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"Ciphers\":\"Fast\", \"UseClientCipher\":true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_UnicodeNotValid) == ISMRC_UnicodeNotValid );

    /* 15. Create Security Profile with non-alphanumeric characters: rc=6206 */
    mbuf = "{ \"BAD_NAME\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"Ciphers\":\"Fast\", \"UseClientCipher\":true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, 6206) == 6206 );

    /* 16. Create Security Profile with non-alphanumeric characters: rc=6206 */
    mbuf = "{ \"è¡¨å™‚ã‚½å��è±¹ç«¹æ•·ï½žå…Žæ¤„\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"MinimumProtocolMethod\":\"TLSv1.2\", \"UseClientCertificate\":true, \"Ciphers\":\"Fast\", \"UseClientCipher\":true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, 6206) == 6206 );

    /* 17. Create: SecurityProfile with AllowNullPassword: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": true, \"AllowNullPassword\": true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 18. Create: SecurityProfile with AllowNullPassword: rc=ISMRC_OK */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": true, \"AllowNullPassword\": false }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 19. Create: SecurityProfile with AllowNullPassword: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": true, \"AllowNullPassword\": \"badData\" }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    /* 20. Create: SecurityProfile with AllowNullPassword: rc=6207 */
    mbuf = "{ \"secpo2\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false, \"AllowNullPassword\": true }}";
    CU_ASSERT( validate_SecurityProfile(mbuf, 6207) == 6207 );

    return;
}
