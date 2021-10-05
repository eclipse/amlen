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

#include <validateConfigData.h>
#include "validateInternal.h"
#include <sys/socket.h>
#include <netdb.h>
#include "jansson.h"


int ismqmc_validateIPAddress(const char *value, int mode) {

    struct addrinfo hints, *res;
    const char *ipAddr=value;

    if ( !value )
        return 1;

    if ( ( mode == 0 ) && ( *value == '*' || strcasecmp(value, "all") == 0) )
        return 1;

    if (strstr(value, " ") != NULL || strstr(value, ",") != NULL)
        return 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = 0;

     /*
     * Allow the IP address to have a surrounding brackets.
     * This is normally used for IPv6 but we actually allow it for anything.
     */
    if (ipAddr && *ipAddr == '[') {
    	hints.ai_family = AF_INET6;
        int  iplen = strlen(ipAddr);
        if (iplen > 1) {
            char * newip = alloca(iplen);
            strcpy(newip, ipAddr+1);
            if (newip[iplen-2] == ']')
                newip[iplen-2] = 0;
            ipAddr = newip;
        }
    }

    if ( getaddrinfo(ipAddr, NULL, &hints, &res) != 0 ) {
        /*Failed to resolve the address.*/
        return 0;
    }

    freeaddrinfo(res);

    return 1;
}

int ismqmc_validateConnectionName(const char *value) {
    int rc = 1;
    char *token, *nexttoken=NULL;
    int len = strlen(value);
    char *tmpvalue = (char *)alloca(len+1);
    memcpy(tmpvalue, value, len);
    tmpvalue[len] = 0;
    for ( token = strtok_r(tmpvalue, ",", &nexttoken); token != NULL;
          token = strtok_r(NULL, ",", &nexttoken)) {
        int len1 = strlen(token);
        char *tmpstr = (char *)alloca(len1+2);
        memcpy(tmpstr, token, len1);
        tmpstr[len1] = 0;
        tmpstr[len1+1] = 0;

        /*Check for multiple ports in one connection*/
        int i = 0;
        int found = 0;
        for (i=0; i<len1; i++) {
            if ( tmpstr[i] == '(') {
                found += 1;
            }
        }
        if ( found > 1 ) {
            rc=0;
            break;
        }

        char *addr = strtok(tmpstr, "(");
        tmpstr += strlen(addr) + 1;
        char *portStr = strtok(tmpstr, ")");
        rc = ismqmc_validateIPAddress((char *)addr, 1);
        if ( rc == 1 ) {
            if ( portStr && *portStr != '\0' ) {
                int pno = atoi(portStr);
                if ( pno > 0 && pno < 65536 ) {
                    rc = 1;
                } else {
                    rc = 0;
                    break;
                }
            }
        } else {
            break;
        }
    }

    return rc;
}


/* Starter states for UTF8 */
static int States[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
};

/* Initial byte masks for UTF8 */
static int StateMask[5] = {0, 0, 0x1F, 0x0F, 0x07};

/* Check for valid second byte of UTF-8 - copied from utils */
static int validSecond(int state, int byte1, int byte2, int isName) {
    int ret = 1;

    if (byte2 < 0x80 || byte2 > 0xbf)
        return 0;

    switch (state) {
    case 2:
        if (byte1 < 2)
            ret = 0;
        break;
    case 3:
        if ((byte1== 0 && byte2 < 0xa0) || (byte1 == 13 && byte2 > 0x9f))
            ret = 0;
        break;
    case 4:
        if (byte1 == 0 && byte2 < 0x90)
            ret = 0;
        else if (byte1 == 4 && byte2 > 0x8f)
            ret = 0;
        else if (byte1 > 4)
            ret = 0;
        break;
    }
    return ret;
}

/* Validate names and other items for valid UTF8 and specifial characters */
static int ismqmc_validateNameSpecialCases(const char * s, int len, int checkFirst, int isName) {
    int  byte1 = 0;
    int  state = 0;
    int  count = 0;
    int  inputsize = 0;
    uint8_t * sp = (uint8_t *)s;
    uint8_t * endp = (uint8_t *)(s+len);
    int firstDone = 0;

    while (sp < endp) {
        if (state == 0) {
            /* Fast loop in single byte mode */
            for (;;) {
                if (*sp >= 0x80) {
                    firstDone = 1;
                    break;
                }

                if ( isName == 1 ) {
                    if ( checkFirst && !firstDone &&  *sp < 0x41 ) {
                        return -1;
                    }
                    firstDone = 1;
                    if (*sp < ' ' || *sp == '=' || *sp == ',' || *sp == 0x22 || *sp == 0x5C ) {
                        return -1;
                    }
                } else if ( isName == 2 ) {
                    int allow = 0;
                    if ( (*sp > 0x2F && *sp < 0x3A) || (*sp > 0x40 && *sp < 0x5B) || (*sp > 0x60 && *sp < 0x7B) ) {
                        allow = 1;
                    } else {
                        /* allow  -, _, ., and + */
                        if ( *sp == '-' || *sp == '_' || *sp == '.' || *sp == '+') {
                            allow = 1;
                        }else  {
                            allow = 0;
                        }
                    }
                    if ( allow == 0 ) {
                        return 0;
                    }
                } else if ( isName == 3 ) {
                    if ( *sp < 0x20 ) {
                        return 0;
                    }
                } else if ( isName == 4 ) {
                    int allow = 0;
                    if ( (*sp > 0x2F && *sp < 0x3A) || (*sp > 0x40 && *sp < 0x5B) || (*sp > 0x60 && *sp < 0x7B) ) {
                        allow = 1;
                    } else {
                        /* allow  -, _, ., and + */
                        if ( *sp == '#' || *sp == '_' || *sp == '.' || *sp == '+' || *sp == '/' || *sp == '%') {
                            allow = 1;
                        } else  {
                            allow = 0;
                        }
                    }
                    if ( allow == 0 ) {
                        return 0;
                    }
                } else {
                    if (*sp < ' ') {
                        return -1;
                    }
                }

                count++;
                if (++sp >= endp)
                    return count;
            }

            count++;
            state = States[*sp >>3];
            byte1 = *sp & StateMask[state];
            sp++;
            inputsize = 1;
            if (state == 1)
                return -1;
        } else {
            int byte2 = *sp++;
            if ((inputsize==1 && !validSecond(state, byte1, byte2, isName)) ||
                (inputsize > 1 && (byte2 < 0x80 || byte2 > 0xbf)))
                return -1;
            if (inputsize+1 >= state) {
                state = 0;
            } else {
                inputsize++;
            }
        }
    }
    if (state)
        return -1;        /* Incomplete character */
    return count;
}


/*
 * Item: QueueManagerConnection
 * Component: MQConnectivity
 *
 * Description:
 *
 * Schema:
 * {
 * 		"QueueManagerConnection": {
 *      	"Name":"string",
 *			"Force":"boolean",
 *			"QueueManagerName":"string",
 *			"ConnectionName":"string",
 *			"ChannelName":"string",
 *			"SSLCipherSpec":"string",
 *			"ChannelUserName":"string",
 *			"ChannelUserPassword":"string"
 *		}
 *	}
 *
 *
 * Force: Force delete or update of the Queue Manager Connection. This might cause the XA IDs to be orphaned.
 * QueueManagerName: The Queue Manager to connect to.
 * ConnectionName: Comma delimited list of Connections.
 * ChannelName: The Channel to use while connecting to the MQ Queue Manager.
 * SSLCipherSpec: SSL Cipher specification.
 *
 * Component callback(s):
 * - MQConnectivity
 * Validation rules:
 *
 * User defined Configuration Item(s):
 * ----------------------------------
 *
 * Internal Configuration Item(s):
 * ------------------------------
 *
 * Unused configuration Item(s):
 * ----------------------------
 *
 * Dependent validation rules:
 * --------------------------
 *
 */
XAPI int32_t ism_config_validate_QueueManagerConnection(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int hasPassword = 0;
    char *encPassword = NULL;

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

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    json_t *cobj = json_object_get(currPostObj, "QueueManagerConnection");
    if (cobj && json_typeof(cobj) == JSON_OBJECT) {
        json_t *inst = json_object_get(cobj, name);
        if ( inst && json_typeof(inst) == JSON_OBJECT ) {
            json_t *passwordObj = json_object_get(inst, "ChannelUserPassword");
            if (passwordObj) {
                TRACE(9, "%s: The new configuration has ChannelUserPassword specified.\n", __FUNCTION__);
                hasPassword = 1;
            }
        }
    }

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);

        if (rc != ISMRC_OK) goto VALIDATION_END;

		if ( !strcmp(key, "QueueManagerName") || !strcmp(key, "ChannelName")) {
		    int otype = json_typeof(value);
		    if ( otype == JSON_STRING ) {
		        const char *svalue = json_string_value(value);
		        if (!svalue || *svalue == '\0' || ismqmc_validateNameSpecialCases(svalue, strlen(svalue), 0, 4) <= 0 ) {
		            rc = ISMRC_BadPropertyValue;
		            ism_common_setErrorData(rc, "%s%s", key, svalue);
		            goto VALIDATION_END;
		        }
		    } else {
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "QueueManagerConnection", name, key, ism_config_json_typeString(otype));
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
		    }
		}

	    /* validate special cases related to this object here*/

        /* Encrypt password */
		if (!strcmp(key, "ChannelUserPassword") && hasPassword == 1) {
			const char *svalue = json_string_value(value);
			if ( svalue && *svalue != '\0') {
	            if (!strcmp(svalue, "XXXXXX")) {
	                rc = ISMRC_BadPropertyValue;
	                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "XXXXXX");
	                goto VALIDATION_END;
	            }
			}
		}

        if (!strcmp(key, "ConnectionName")) {
            const char *svalue = json_string_value(value);
            if (!svalue || *svalue == '\0' || !ismqmc_validateConnectionName(svalue)) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", key, svalue);
                goto VALIDATION_END;
            }
        }
        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);

VALIDATION_END:

    if (encPassword) ism_common_free(ism_memory_admin_misc,encPassword);

	TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;

}
