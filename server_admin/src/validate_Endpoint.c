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

#define TRACE_COMP Admin

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

#include <janssonConfig.h>
#include "validateInternal.h"

extern json_t * ism_config_listTruststoreCertificate(char *object, char *profileName, char *certName, int *count);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern int standbySyncInProgress;
extern int revalidateEndpointForCRL;

/* Returns 1 if Standby */
static int ism_config_isNodeHAStandby(void) {
    int mode = 0;
    int hrc = 0;

    ismHA_Role_t harole = ism_admin_getHArole(NULL, &hrc);
    if ( standbySyncInProgress == 1 || (hrc == ISMRC_OK && harole == ISM_HA_ROLE_STANDBY)) {
        mode = 1;
    }
    return mode;
}

/*
 * mode = 1 - returns Interface name
 * mode = 2 - returns Interface address
 */
static char * ism_config_getIfaNameOrAddress(char *IPAddress, char *name, int mode) {
    char *ifcName = NULL;
    char *ifcIP = NULL;
    struct ifaddrs *ifafirst;
    struct ifaddrs *ifap;
    int family = AF_INET;
    char *addrStr = NULL;
    char *lastname = NULL;
    int count = 0;
    char *tmpstr = IPAddress;

    /* Check first interface */
    if ( getifaddrs(&ifafirst) != 0 )
        return NULL;

    /* Check Address type */
    if ( IPAddress && strstr(IPAddress, ":") ) {
        family = AF_INET6;

        if (*tmpstr == '[') {
            tmpstr++;
            char *p = tmpstr + strlen(tmpstr) - 1;
            if(*p != ']')
                return NULL;
            *p = 0;
        }
    }

    /* loop thru all interfaces */
    for (ifap = ifafirst; ifap; ifap = ifap->ifa_next)
    {
        /* discard loopback, unconfigured and down interfaces */
        if ( ifap->ifa_addr == NULL || (ifap->ifa_flags & IFF_UP) == 0 ||
            (ifap->ifa_flags & IFF_LOOPBACK) )
        {
            continue;
        }

        char buf[64];

        if ( family == AF_INET && ifap->ifa_addr->sa_family == AF_INET ) {
            /* Check for IPv4 address */
            struct sockaddr_in *sa4;

            /* get IP address */
            sa4 = (struct sockaddr_in *)(ifap->ifa_addr);
            addrStr = (char *)inet_ntop( ifap->ifa_addr->sa_family,
                 &(sa4->sin_addr), buf, sizeof(buf));
        } else if ( family == AF_INET6 && ifap->ifa_addr->sa_family == AF_INET6 ) {
            /* Check for IPv6 address */
            struct sockaddr_in6 *sa6;

            /* get IP address */
            sa6 = (struct sockaddr_in6 *)(ifap->ifa_addr);
            addrStr = (char *)inet_ntop( ifap->ifa_addr->sa_family,
                 &(sa6->sin6_addr), buf, sizeof(buf));
        }

        if ( addrStr )
        {
            /* To handle virtual IP addresses assigned to an interface,
             * use InterfaceName_IPAddress to find the correct IP address
             */

            /* if IPAddress is specified - return matching interface name */
            if ( mode == 1 && IPAddress && !strcmp(buf, tmpstr))
            {
                if ( lastname == NULL ) {
                    lastname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ifap->ifa_name);
                } else {
                    if ( strcmp(ifap->ifa_name, lastname) == 0) {
                        count++;
                    } else {
                        count = 0;
                        ism_common_free(ism_memory_admin_misc,lastname);
                        lastname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ifap->ifa_name);
                    }
                }

                char tmpbuf[256];
                snprintf(tmpbuf, 256, "%s_%d", ifap->ifa_name, count);
                ifcName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpbuf);
                break;
            } else if ( mode == 2 && name ) {

                /* if name is specified - return matching IP address */

                if ( lastname == NULL ) {
                    lastname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ifap->ifa_name);
                } else {
                    if ( strcmp(ifap->ifa_name, lastname) == 0) {
                        count++;
                    } else {
                        count = 0;
                        ism_common_free(ism_memory_admin_misc,lastname);
                        lastname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ifap->ifa_name);
                    }
                }

                char tmpbuf[256];
                snprintf(tmpbuf, 256, "%s_%d", ifap->ifa_name, count);
                if ( !strcmp(tmpbuf, name)) {
                    ifcIP = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buf);
                    break;
                }
            }
        }
    }

    if ( mode == 2 )
        return ifcIP;

    return ifcName;
}

XAPI int ism_config_updateEndpointInterfaceName(json_t *mobj, char *name) {
    char *interfaceStr = NULL;
    char *interfaceNameStr = NULL;

    /* Get HA mode */
    int isStandby = ism_config_isNodeHAStandby();

    json_t *interface = json_object_get(mobj, "Interface");
    json_t *interfaceName = json_object_get(mobj, "InterfaceName");

    if ( interface ) {
        interfaceStr = (char *)json_string_value(interface);
    }

    if ( interfaceName ) {
        interfaceNameStr = (char *)json_string_value(interfaceName);
    }

    if ( isStandby == 0 ) {
        /* This is a Primary node, HA is not enabled or in the process of initial sync:
         * Update InterfaceName
         */
        if ( !interface || !interfaceStr || *interfaceStr == '\0' ) {
            /* Something is wrong - on Primary node Interface can not be NULL */
            TRACE(3, "Endpoint %s - Interface is NULL or empty\n", name);
            ism_common_setError(ISMRC_IPNotValid);
            return ISMRC_IPNotValid;
        }

        /* Check for all interface cases */
        if ( !strcmpi(interfaceStr, "all") || !strcmpi(interfaceStr, "*") || !strcmpi(interfaceStr, "0.0.0.0") || !strcmpi(interfaceStr, "[::]")) {
            /* Set InterfaceName to the same value as Interface */
            json_object_set(mobj, "InterfaceName", json_string(interfaceStr));
            return ISMRC_OK;
        }

        /* check for local host */
        if ( !strcmpi(interfaceStr, "127.0.0.1") ) {
            /* Set InterfaceName to localhost */
            json_object_set(mobj, "InterfaceName", json_string("localhost"));
            return ISMRC_OK;
        }

        /* Find interface name for this interface */
        interfaceNameStr = ism_config_getIfaNameOrAddress(interfaceStr, NULL, 1);
        if ( !interfaceNameStr || *interfaceNameStr == '\0' ) {
            TRACE(3, "Unable to resolve Endpoint Interface address. EndpointName:%s Interface:%s\n", name, interfaceStr);
            ism_common_setError(ISMRC_IPNotValid);
            return ISMRC_IPNotValid;
        }

        /* Set InterfaceName */
        json_object_set(mobj, "InterfaceName", json_string(interfaceNameStr));

    } else {

        /* This is a Standby node, update InterfaceName based on InterfaceName
         */
        if ( !interfaceName || !interfaceNameStr || *interfaceNameStr == '\0' ) {
            /* Something is wrong - on Primary node Interface can not be NULL */
            TRACE(3, "Endpoint %s - InterfaceName on Standby node is NULL or empty\n", name);
            ism_common_setError(ISMRC_IPNotValid);
            return ISMRC_IPNotValid;
        }

        /* Check for all interface cases */
        if ( !strcmpi(interfaceNameStr, "all") || !strcmpi(interfaceNameStr, "*") || !strcmpi(interfaceNameStr, "0.0.0.0") || !strcmpi(interfaceNameStr, "[::]")) {
            /* Set Interface to the same value as InterfaceName */
            json_object_set(mobj, "Interface", json_string(interfaceNameStr));
            return ISMRC_OK;
        }

        /* check for local host */
        if ( !strcmpi(interfaceNameStr, "localhost") ) {
            /* Set Interface to 127.0.0.1 */
            json_object_set(mobj, "Interface", json_string("127.0.0.1"));
            return ISMRC_OK;
        }

        /* Find interface name for this interface */
        interfaceStr = ism_config_getIfaNameOrAddress(NULL, interfaceNameStr, 2);
        if ( !interfaceStr || *interfaceStr == '\0' ) {
            TRACE(3, "Unable to resolve Endpoint InterfaceName on Standby node. EndpointName:%s InterfaceName:%s\n", name, interfaceNameStr);
            ism_common_setError(ISMRC_IPNotValid);
            return ISMRC_IPNotValid;
        }

        /* Set InterfaceName */
        json_object_set(mobj, "Interface", json_string(interfaceStr));

    }

    return ISMRC_OK;
}

/*
 * Item: Endpoint
 *
 * Description:
 * - MessageSight clients connect to endpoint
 *
 * "Endpoint": {
 *     "<EndpointName>": {
 *          "Description": "string",
 *          "MessageHub": "string",
 *          "Interface": "string",
 *          "Port": "integer",
 *          "Protocol": "string",
 *          "Enabled": "boolean",
 *          "SecurityProfile": "string",
 *          "TopicPolicies": "string",
 *          "QueuePolicies": "string",
 *          "SubscriptionPolicies": "string",
 *          "MaxSendSize": "integer",
 *          "MaxMessageSize": "integer",
 *          "BatchMessages": "boolean",
 *          "ConnectionPolicies": "string"
 *     }
 * }
 *
 * Where:
 *
 * Interface: IP address, All and * allowed.
 * Port: Integer, range 1-65535
 * SecurityProfile: String - Security Profile is optional.
 *                           If LTPA and OAuth is set in the SecurityProfile, it will be ignored
 *                           for authenticating a user using LTPA or OAuth.
 * ConnectionPolicies: Comma-separated list of ConnectionPolicy.
 * .....
 *
 *
 * Callback component(s):
 * - Transport
 *
 */

XAPI int32_t ism_config_validate_Endpoint(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }


    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }


    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL ) {
        rc = ISMRC_NotImplemented;
        ism_common_setErrorData(rc, "%s", "Endpoint");
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    int  dataType;
    int topPoliciesDefined = 0;
    int quePoliciesDefined = 0;
    int subPoliciesDefined = 0;
    int subPolicyProtocols = 0;
    int topPolicySubscribe = 0;

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
    	int gotValue = 0;
    	char *propValue = NULL;
        key = json_object_iter_key(itemIter);

        if ( !strcmp(key, "InterfaceName")) {
            /* Ignore Endpoint configuration item InterfaceName - though it is not
             * user settable, we need to allow Endpoint configuration to handle the case
             * of export/import
             */
            itemIter = json_object_iter_next(mergedObj, itemIter);
            continue;
        }

        value = json_object_get(mergedObj, key);
        objType = json_typeof(value);

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
		if (rc != ISMRC_OK) goto VALIDATION_END;

		if (objType == JSON_STRING) {
			propValue = (char *)json_string_value(value);
			gotValue = 1;
		}

        /* Validate ConnectionPolicies */
        if ( !strcmp(key, "ConnectionPolicies")) {
            if (!gotValue || *propValue == '\0') {
            	rc = ISMRC_NoConnectionPolicy;
            	ism_common_setError(ISMRC_NoConnectionPolicy);
            	goto VALIDATION_END;
            }
            /* Check if specified policies in the list are valid */
            /* Could be a comma-separated list */
            char * token;
            char * nexttoken = NULL;
            int len = strlen(propValue);
            char *option = alloca(len + 1);
            memcpy(option, propValue, len);
            option[len] = '\0';
            int count = 0;

            /* Remove leading and trailing spaces around comma */
            ism_config_fixCommaList(mergedObj, key, option);
            for (token = strtok_r(option, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
            {
                /* check if policy exist */
                int found = ism_config_objectExist("ConnectionPolicy", token, currPostObj);
                if ( found == ISMRC_OK ) {
                    rc = ISMRC_ObjectNotFound;
                    ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "ConnectionPolicy", token);
                    goto VALIDATION_END;
                }
                count += 1;
            }

            if ( count >= MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "ConnectionPolicies", count, MAX_POLICIES_PER_SECURITY_CONTEXT);
                goto VALIDATION_END;
            }

        } else if ( !strcmp(key, "TopicPolicies")) {
            /* Validate TopicPolicies */

            if (!gotValue || *propValue == '\0') {
                itemIter = json_object_iter_next(mergedObj, itemIter);
                continue;
            }
            /* Check if specified policies in the list are valid */
            /* Could be a comma-separated list */
            char * token;
            char * nexttoken = NULL;
            int len = strlen(propValue);
            char *option = alloca(len + 1);
            memcpy(option, propValue, len);
            option[len] = '\0';
            int count = 0;

            /* Remove leading and trailing spaces around comma */
            ism_config_fixCommaList(mergedObj, key, option);
            for (token = strtok_r(option, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
            {
                /* check if policy exist */
                int ptype = JSON_NULL;
                json_t *topicPol = NULL;
                topicPol = ism_config_json_getObject("TopicPolicy", token, NULL, 0, &ptype);
                if ( !topicPol || ptype != JSON_OBJECT ) {
					rc = ISMRC_ObjectNotFound;
					ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "TopicPolicy", token);
					goto VALIDATION_END;
                } else {
                	/* Check for Policies with Subscribe is set in the ActionList */
                	json_t *actListObj = json_object_get(topicPol, "ActionList");
                	char *actListStr = (char *)json_string_value(actListObj);
                	if ( actListStr && strstr(actListStr, "Subscribe")) topPolicySubscribe += 1;

                	/* check MQTT or JMS or All protocols */
                	json_t *protcol = json_object_get(topicPol, "Protocol");
                	char *ptclVal = (char *)json_string_value(protcol);

                	if ( ptclVal && *ptclVal != '\0' ) {
						char * ptoken;
						char * pnexttoken = NULL;
						int plen = strlen(ptclVal);
						char *pOption = alloca(plen + 1);
						memcpy(pOption, ptclVal, plen);
						pOption[plen] = '\0';

						/* Remove leading and trailing spaces around comma */
                        ism_config_fixCommaList(NULL, NULL, pOption);
						for (ptoken = strtok_r(pOption, ",", &pnexttoken); ptoken != NULL; ptoken = strtok_r(NULL, ",", &pnexttoken))
						{
							if (!strcmpi(ptoken, "MQTT") || !strcmpi(ptoken, "JMS") || !strcmpi(ptoken, "All")) {
								subPolicyProtocols++;
							}
						}
                	}else {
                		// if Protocol is empty or not defined --means all
                		subPolicyProtocols++;
                	}
                }

                count += 1;

                topPoliciesDefined = 1;
            }

            if ( count >= MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "TopicPolicies", count, MAX_POLICIES_PER_SECURITY_CONTEXT);
                goto VALIDATION_END;
            }

        } else if ( !strcmp(key, "QueuePolicies")) {
            /* Validate QueuePolicies */

            if (!gotValue || *propValue == '\0') {
                itemIter = json_object_iter_next(mergedObj, itemIter);
                continue;
            }
            /* Check if specified policies in the list are valid */
            /* Could be a comma-separated list */
            char * token;
            char * nexttoken = NULL;
            int len = strlen(propValue);
            char *option = alloca(len + 1);
            memcpy(option, propValue, len);
            option[len] = '\0';
            int count = 0;

            /* Remove leading and trailing spaces around comma */
            ism_config_fixCommaList(mergedObj, key, option);
            for (token = strtok_r(option, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
            {
                /* check if policy exist */
                int found = ism_config_objectExist("QueuePolicy", token, currPostObj);
                if ( found == ISMRC_OK ) {
                    rc = ISMRC_ObjectNotFound;
                    ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "QueuePolicy", token);
                    goto VALIDATION_END;
                }
                count += 1;

                quePoliciesDefined = 1;
            }

            if ( count >= MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "QueuePolicies", count, MAX_POLICIES_PER_SECURITY_CONTEXT);
                goto VALIDATION_END;
            }

        } else if ( !strcmp(key, "SubscriptionPolicies")) {
            /* Validate SubscriptionPolicies */

            if (!gotValue || *propValue == '\0') {
                itemIter = json_object_iter_next(mergedObj, itemIter);
                continue;
            }
            /* Check if specified policies in the list are valid */
            /* Could be a comma-separated list */
            char * token;
            char * nexttoken = NULL;
            int len = strlen(propValue);
            char *option = alloca(len + 1);
            memcpy(option, propValue, len);
            option[len] = '\0';
            int count = 0;

            /* Remove leading and trailing spaces around comma */
            ism_config_fixCommaList(mergedObj, key, option);
            for (token = strtok_r(option, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
            {
                /* check if policy exist */
                int found = ism_config_objectExist("SubscriptionPolicy", token, currPostObj);
                if ( found == ISMRC_OK ) {
                    rc = ISMRC_ObjectNotFound;
                    ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "SubscriptionPolicy", token);
                    goto VALIDATION_END;
                }
                count += 1;

                subPoliciesDefined = 1;
            }

            if ( count >= MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                rc = ISMRC_TooManyItems;
                ism_common_setErrorData(ISMRC_TooManyItems, "%s%d%d", "SubscriptionPolicies", count, MAX_POLICIES_PER_SECURITY_CONTEXT);
                goto VALIDATION_END;
            }
        } else if ( !strcmp(key, "SecurityProfile")) {

        	/* Validate SecurityProfile */
            if ( gotValue && *propValue != '\0' ) {
                /* Check if security profile exist and get usePasswordAuthentication item configured in the SecurityProfile */
                value = NULL;

                int found = ism_config_objectExist("SecurityProfile", propValue, currPostObj);
                if ( found == 0 ) {
                    rc = ISMRC_ObjectNotFound;
                    ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "SecurityProfile", propValue);
                    goto VALIDATION_END;
                }

                /*
                 * If usePasswordAuthentication is enabled in the used security profile,
                 * then it is required that either AdminUserID and AdminUserPassword is set, or
                 * an external LDAP is configured.
                 */
                value = ism_config_json_getObject("SecurityProfile", propValue, "UsePasswordAuthentication", 0, &dataType);
                if ( value ) {
                    char *usePWAuth = NULL;
                    if ( dataType == JSON_STRING ) {
                        usePWAuth = (char *)json_string_value(value);
                    }

                    if ( usePWAuth && !strcmpi(usePWAuth, "true")) {
                        /* check if AdminUserID and AdminUserPassword is set or external LDAP is configured */
                        json_t *adminUserObj = ism_config_json_getObject("AdminUserID", NULL, NULL, 0, &dataType);
                        json_t *adminPassObj = ism_config_json_getObject("AdminUserPassword", NULL, NULL, 0, &dataType);

                        if (!adminUserObj || !adminPassObj ) {
                            /* check if external LDAP is enabled */
                            value = ism_config_json_getObject("LDAP", "ldapconfig", "Enabled", 0, &dataType);

                            if ( value == NULL ) {
                                rc = ISMRC_AdminUserRequired;
                                ism_common_setError(ISMRC_AdminUserRequired);
                                goto VALIDATION_END;
                            }
                            int ldapEnabled = json_is_true(value);
                            if ( !ldapEnabled ) {
                                rc = ISMRC_AdminUserRequired;
                                ism_common_setError(ISMRC_AdminUserRequired);
                                goto VALIDATION_END;
                            }
                        }
                    }
                }

                value = NULL;
                value = ism_config_json_getObject("SecurityProfile", propValue, "UseClientCertificate", 0, &dataType);
                if (value) {
                	if (dataType!= JSON_TRUE && dataType != JSON_FALSE ) {
						/* Return error */
						ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "InvalidType");
						rc = ISMRC_BadOptionValue;
						goto VALIDATION_END;
					} else if (dataType == JSON_TRUE) {
		                /* check TrustedCertificate */
					    int count = 0;
					    json_t * retval = ism_config_listTruststoreCertificate("TrustedCertificate", propValue, NULL, &count);
					    json_decref(retval);
		                if ( count == 0 ) {
		                    /* check ClientCertificate */
		                    retval = ism_config_listTruststoreCertificate("ClientCertificate", propValue, NULL, &count);
		                    json_decref(retval);
		                    if ( count == 0 ) {
		                        rc = ISMRC_NoTrustedCertificate;
		                        ism_common_setError(rc);
		                        goto VALIDATION_END;
		                    }
		                }
                	}
                }

                /* Has security profile changed ? We also need to know if we need to revalidate endpoint using this security profile
                 * because of the CRL change. The revalidation is called after the new security context has been updated in the callback to
                 * transport component. We do not check if the source of the CRL is a url or filename here. */

                char * crlProfStr = NULL;
                json_t * oldSecProfObj = NULL;
                char * oldSecProfStr = NULL;

                /* Get the current security profile of object - can't use mergedObj */
                oldSecProfObj = ism_config_json_getObject("Endpoint", name, "SecurityProfile", 0, &dataType);
                if (oldSecProfObj) {
                    oldSecProfStr = (char *) json_string_value(oldSecProfObj);
                }

                if (oldSecProfStr && (*oldSecProfStr != '\0')) {
                    /* same security profile don't bother */
                    if (!strcmp(oldSecProfStr, propValue)) {
                        goto NEXT_KEY;
                    }
                }
                /* Switching security profiles or endpoint being set with a new one */
                value = NULL;
                value = ism_config_json_getObject("SecurityProfile", propValue, "CRLProfile", 0, &dataType);
                if (value) {
                    crlProfStr = (char *) json_string_value(value);
                    if (crlProfStr && (*crlProfStr != '\0')) {
                        (void) ism_config_json_getObject("CRLProfile", crlProfStr, "RevalidateConnection", 0, &dataType);
                        if (dataType == JSON_TRUE)
                            revalidateEndpointForCRL = 1;
                    }
                }
            }
        } else if ( !strcmp(key, "Protocol")) {

            if (gotValue && strcasecmp(propValue, "all")) {
            	char *opttoken, *optnexttoken = NULL;
            	int len = strlen(propValue);			/* BEAM suppression: passing null object */
            	int gotaprotocol = 0;
            	char *iprotocol = (char *)alloca(len+1);
            	memcpy(iprotocol, propValue, len);
            	iprotocol[len]='\0';

                /* Remove leading and trailing spaces around comma */
                ism_config_fixCommaList(NULL, NULL, iprotocol);
            	for (opttoken = strtok_r(iprotocol, ",", &optnexttoken); opttoken != NULL; opttoken
            			= strtok_r(NULL, ",", &optnexttoken)) {

            	    char * protocolName = opttoken;
            	    if (-1 == ism_admin_getProtocolId(protocolName)) {
            	    	rc = ISMRC_BadPropertyValue;
            	    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue);
            	    	TRACE(7, "%s: %s is an invalid protocol\n", __FUNCTION__ ,protocolName);
            	    	goto VALIDATION_END;
            	    } else {
            	    	gotaprotocol = 1;
            	    }

            	}
            	if (!gotaprotocol) {
        	    	rc = ISMRC_BadPropertyValue;
        	    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, *propValue ? propValue : "\"\"");
        	    	goto VALIDATION_END;
            	}
            }

        } else if ( !strcmp(key, "Interface")) {
        	if (gotValue && !*propValue) {
    	    	rc = ISMRC_BadPropertyValue;
    	    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "\"\"");
    	    	goto VALIDATION_END;
            }
        } else if ( !strcmp(key, "MessageHub")) {
        	if (gotValue && !*propValue ) {
    	    	rc = ISMRC_BadPropertyValue;
    	    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "\"\"");
    	    	goto VALIDATION_END;
        	}

            /* find object */
        	int found = 0;
            json_t *objroot = json_object_get(srvConfigRoot, "MessageHub");
            if ( objroot ) {
                /* find instance */
                json_t *instroot = json_object_get(objroot, propValue);
                if ( instroot ) {
                    found = 1;
                }
            }
            if (!found) {
            	rc = ISMRC_InvalidMessageHub;
            	ism_common_setErrorData(rc, "%s", propValue?propValue:"null");
            	goto VALIDATION_END;
            }

        }
NEXT_KEY:
        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* check for policies */
    if ( !topPoliciesDefined && !quePoliciesDefined && !subPoliciesDefined ) {
        rc = ISMRC_NoMessagingPolicy;
        ism_common_setError(ISMRC_NoMessagingPolicy);
        goto VALIDATION_END;
    }

    // check if a TopicPolicy is defined and also has MOTT/JMS/All defined as protocol
    if ( subPoliciesDefined ) {
    	if ( topPoliciesDefined == 0 || subPolicyProtocols == 0 || topPolicySubscribe == 0 ) {
			rc = ISMRC_SubTopicPolError;
			ism_common_setError(ISMRC_SubTopicPolError);
			goto VALIDATION_END;
    	}
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Update interface or interface name for HA configuration */
    rc = ism_config_updateEndpointInterfaceName(mergedObj, name);

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

