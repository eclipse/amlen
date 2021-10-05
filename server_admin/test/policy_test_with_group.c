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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

extern int ism_security_getGroupAndValidateDestination(char *uid, char *igrp, char * grpSuffix, int grpSuffixHexLen, char * hexGrpSuffix, const char *objectName,
		const char *destination, ima_transport_info_t *tport, int noCaseCheck);

ism_trclevel_t * ismDefaultTrace;
int nogrptestnum = 0;

static void test_destWithGroup(char *uid, char *cid, char *igrp, char * grpSuffix, int grpSuffixHexLen,
		char * hexGrpSuffix, char *objectName, char *destination, int noCaseCheck, int exprc) {
	int rc = 0;
    char trans[1024] = {0};
    ism_transport_t *tport = (ism_transport_t *)trans;

    tport->clientID = cid;
    tport->userid = uid;
    tport->cert_name = "CertA";
    tport->client_addr = "127.0.0.1";
    tport->protocol = "mqtt";
    tport->protocol_family = "mqtt";
    tport->listener = NULL;
    tport->trclevel = ism_defaultTrace;

    nogrptestnum += 1;

    rc = ism_security_getGroupAndValidateDestination(uid, igrp, grpSuffix, grpSuffixHexLen, hexGrpSuffix, objectName,
		destination, (ima_transport_info_t *)tport, noCaseCheck);

    printf("TestDestWithGroup-%d: RC:%d  ExpectedRC:%d\n", nogrptestnum, rc, exprc);
    CU_ASSERT(rc == exprc);

}

static char *getEscString(char *str, int *strHexLen) {
    int len = strlen(str);
    int  hexLen = ism_admin_ldapHexExtraLen(str, len);
    char *hexStr = NULL;
    char *hexStrPtr = NULL;

    if (hexLen > 0) {
        hexStr = calloc(1, (len + hexLen + 1));
        hexStrPtr = hexStr;
        ism_admin_ldapHexEscape(&hexStrPtr, str, len);
    } else {
        hexStr = strdup(str);
    }

    *strHexLen = hexLen;

    return hexStr;
}




/* Cunit test for TopicPolicy authorization */
void test_authorize_destWithGroup(void)
{
    char *hexGrpSuffix = NULL;
    char *igrp = NULL;
    char *uid = NULL;
    char *cid = NULL;
    char *objectName = NULL;
    char *destination = NULL;
    int noCaseCheck = 0;

    char *grpSuffix = "ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    int grpHexLen = 0;
    int grpSuffixHexLen = 0;

    hexGrpSuffix = getEscString(grpSuffix, &grpSuffixHexLen);

    ism_common_initUtil();
    ism_common_setTraceLevel(0);
    ismDefaultTrace = &ismDefaultDomain.trace;

    printf("\nTest Destination string with GroupID\n");

    /**** case sensitive, dn not escaped ****/
    uid = "MQTT_David";
    igrp = "cn=MQTT_DHL,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    cid = "testClient";
    objectName = "MQTT_DHL/chat";
    destination = "${GroupID}/chat";
    grpHexLen = 0;
    noCaseCheck = 0;

    /* group match */
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_Matched);

    /* Group mismatch in topic */
    objectName = "MQTT_UPS/chat";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_GroupIDMismatch);

    /* Group not found */
    igrp = "cn=MQTT_UPS,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    objectName = "MQTT_DHL/chat";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_GroupIDMismatch);

    /* User id match */
    igrp = "cn=MQTT_DHL,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    objectName = "iot/MQTT_DHL/MQTT_David/testtopic/subtopic";
    destination = "iot/${GroupID}/${UserID}/*";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_Matched);

    /* User id mis-match */
    uid = "MQTT_user";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_UserIDMismatch);


    /**** not case sensitive, dn not escaped ****/
    uid = "MQTT_David";
    igrp = "cn=MQTT_DHL,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    cid = "testClient";
    objectName = "MQTT_DHL/chat";
    destination = "${GroupID}/chat";
    grpHexLen = 0;
    noCaseCheck = 1;

    /* group match */
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_Matched);

    /* Group mismatch in topic */
    objectName = "MQTT_UPS/chat";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_GroupIDMismatch);

    /* Group not found */
    igrp = "cn=MQTT_UPS,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    objectName = "MQTT_DHL/chat";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_GroupIDMismatch);

    /* User id match */
    igrp = "cn=MQTT_DHL,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    objectName = "iot/MQTT_DHL/MQTT_David/testtopic/subtopic";
    destination = "iot/${GroupID}/${UserID}/*";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_Matched);

    /* User id mis-match */
    uid = "MQTT_user";
    test_destWithGroup(uid, cid, igrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_UserIDMismatch);


    /**** no case sensitive, dn escaped ****/
    uid = "MQTT_David";
    igrp = "cn=MQTT_DHL,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    cid = "testClient";
    objectName = "MQTT_DHL/chat";
    destination = "${GroupID}/chat";
    grpHexLen = grpSuffixHexLen;
    noCaseCheck = 1;

    int hexIgrpLen = 0;
    char *hexIgrp = NULL;

    hexIgrp = getEscString(igrp, &hexIgrpLen);

    /* group match */
    test_destWithGroup(uid, cid, hexIgrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_Matched);

    /* Group mismatch in topic */
    objectName = "MQTT_UPS/chat";
    test_destWithGroup(uid, cid, hexIgrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_GroupIDMismatch);

    /* Group not found */
    igrp = "cn=MQTT_UPS,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    hexIgrp = getEscString(igrp, &hexIgrpLen);
    objectName = "MQTT_DHL/chat";
    test_destWithGroup(uid, cid, hexIgrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_GroupIDMismatch);

    /* User id match */
    igrp = "cn=MQTT_DHL,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    hexIgrp = getEscString(igrp, &hexIgrpLen);
    objectName = "iot/MQTT_DHL/MQTT_David/testtopic/subtopic";
    destination = "iot/${GroupID}/${UserID}/*";
    test_destWithGroup(uid, cid, hexIgrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_Matched);

    /* User id mis-match */
    uid = "MQTT_user";
    test_destWithGroup(uid, cid, hexIgrp, grpSuffix, grpHexLen, hexGrpSuffix, objectName, destination, noCaseCheck, MWVRC_UserIDMismatch);

    return;
}




