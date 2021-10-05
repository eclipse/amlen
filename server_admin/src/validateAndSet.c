/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <config.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <admin.h>
#include "adminInternal.h"
#include "validateInternal.h"

#define MAXINTERFACES 20

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */

/**
 * Get a count of object item
 */
static int cfgval_findObject(ism_config_t *handle, char *object, char *name) {
    int count = 0;
    const char *pName = NULL;
    int i;
    ism_prop_t *p = ism_config_getProperties(handle, object, name);
    if ( p ) {
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            int len = strlen(object) + strlen(name) + 7;
            char *key = alloca(len);
            sprintf(key, "%s.Name.%s", object, name);
            key[len - 1] = 0;
            if ( strcmp(key, pName) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && *value == *name ) {
                     count += 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    return count;
}

/**
 * Get a particular field of a component
 * The caller need to free the memory of the returned value.
 */
char * ism_config_getComponentItemField(ism_prop_t * allProps, char *item, char * name, const char * fieldName)
{

	char *cfgname = NULL;
    cfgname = alloca(strlen(name) + 64);
    sprintf(cfgname, "%s.",item);
    int pos = strlen(item)+1;

    sprintf(cfgname + pos, "%s.%s",fieldName, name);

    char * value = (char *) ism_common_getStringProperty(allProps, cfgname);

    if(value!=NULL){
    	return ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
    }else{
    	return NULL;
    }
}

#if 0
static int cfgval_validateConnectionPolicy(ism_config_t *handle, char *name) {
    int count = 0;
    const char *pName = NULL;
    int i;
    ism_prop_t *p = ism_config_getProperties(handle, "Endpoint", NULL);
    if ( p ) {
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if ( strncmp(pName, "Endpoint.SecurityPolicies.", 26) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && strstr(value, name ))  {
                     count += 1;
                     break;
                }
            } else if ( strncmp(pName, "Endpoint.ConnectionPolicies.", 28) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && strstr(value, name ))  {
                     count += 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    return count;
}

static char * cfgval_getIPv4InterfaceName(char *IPAddress, char *name) {
    char *ifcName = NULL;
    char *ifcIP = NULL;
    int sock;
    struct ifconf ifconf;
    struct ifreq ifreq[MAXINTERFACES];
    int interfaces;
    int i;

    /* either IPAddress or name is allowed */
    if ( IPAddress && name ) 
        return NULL;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        TRACE(6, "cfgval_getInterfaceName: socket error\n");
        return ifcName;
    }

    ifconf.ifc_buf = (char *) ifreq;
    ifconf.ifc_len = sizeof ifreq;
    if (ioctl(sock, SIOCGIFCONF, &ifconf) == -1) {
        TRACE(6, "cfgval_getInterfaceName: ioctl error\n");
        return ifcName;
    }

    interfaces = ifconf.ifc_len / sizeof(ifreq[0]);
    for (i = 0; i < interfaces; i++) {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *address = (struct sockaddr_in *) &ifreq[i].ifr_addr;
        if (!inet_ntop(AF_INET, &address->sin_addr, ip, sizeof(ip))) {
            TRACE(6, "cfgval_getInterfaceName: ioctl error\n");
             return ifcName;
        }

        /* if IPAddress is specified - return matching interface name */
        if ( IPAddress && !strcmp(ip, IPAddress)) {
            ifcName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ifreq[i].ifr_name);
            break;
        }
        /* if name is specified - return matching IP address */
        if ( name && !strcmp(ifreq[i].ifr_name, name)) {
            ifcIP = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ip);
            break;
        } 
    }

    if ( name )
        return ifcIP;

    return ifcName;
}
#endif

int32_t ism_config_updateEndpointInterface(ism_prop_t * props, char * name, int mode) {
    int rc = ISMRC_OK;
    int update = 0;
    int nameLen = strlen(name);
    int l;

    /* Get interface name from Interface and add InterfaceName
     * in properties
     */
    int ifcPropLen = nameLen + 20;
    char *ifcPropName = alloca(ifcPropLen);
    sprintf(ifcPropName, "Endpoint.Interface.%s", name);
    char *ifcStr = (char *)ism_common_getStringProperty(props, ifcPropName);
    if ( !ifcStr || (ifcStr && *ifcStr == '\0')) {
        TRACE(5, "No Interface is specified for the Endpoint \"%s\" UpdateMode=%d\n", name, mode);
    } else {
    	update = 1;
    }

    int ifcNamePropLen = nameLen + 24;
    char *ifcNamePropName = alloca(ifcNamePropLen);
    sprintf(ifcNamePropName, "Endpoint.InterfaceName.%s", name);
    char *ifcNameStr = (char *)ism_common_getStringProperty(props, ifcNamePropName);
   	if ( !ifcNameStr || ( ifcNameStr && *ifcNameStr == '\0')) {
       	TRACE(5, "No InterfaceName is specified for the Endpoint \"%s\" UpdateMode=%d\n", name, mode);
    } else {
       	update = 1;
    }

    TRACE(5, "UpdateInterface: Update:%d Mode:%d IP:%s NAME:%s\n", update, mode, ifcStr?ifcStr:"",
        ifcNameStr?ifcNameStr:"");

    if ( update == 0 ) {
    	return rc;
    }

    int pNameLen = 512;
    char *pName = alloca(pNameLen);

    /* 
     * On primary node mode = 1
     * Get interface name from IP address and add update interface name
     */
    if ( mode == 1 ) {
        if ( !ifcStr || !strcmpi(ifcStr, "all") || !strcmpi(ifcStr, "*")) {
            ism_field_t f;
            f.type = VT_String;
            f.val.s = "All";
            l = snprintf(pName, pNameLen, "Endpoint.InterfaceName.%s", name);
            if (l + 1 > pNameLen) {
            	pNameLen = l + 1;
            	pName = alloca(pNameLen);
            	sprintf(pName, "Endpoint.InterfaceName.%s", name);
            }
            ism_common_setProperty(props, pName, &f);
            return rc;
        } else {
           	rc = ism_config_validate_IPAddress(ifcStr, 1);
           	return rc;
        }
    } else {
        /* On standby node - get IP address from interface name and 
         * update IP address
         */
        if ( ifcNameStr && ( !strcmpi(ifcNameStr, "all") || !strcmpi(ifcNameStr, "*")) ) {
            ism_field_t f;
            f.type = VT_String;
            f.val.s = "All";
            l = snprintf(pName, pNameLen, "Endpoint.Interface.%s", name);
            if (l + 1 > pNameLen) {
            	pNameLen = l + 1;
            	pName = alloca(pNameLen);
            	sprintf(pName, "Endpoint.Interface.%s", name);
            }
            ism_common_setProperty(props, pName, &f);
            return rc;
        }

        //char *ifcIP = cfgval_getIfaNameOrAddress(ifcStr, ifcNameStr, 2);
        char *ifcIP = ism_common_ifmapip(ifcNameStr);
        if (ifcIP) {
            /* match with stored interface and replace if required */
            if ( !ifcStr || (ifcStr && strcmp(ifcIP, ifcStr))) {
                ism_field_t f;
                f.type = VT_String;
                f.val.s = ifcIP;
                l = snprintf(pName, pNameLen, "Endpoint.Interface.%s", name);
                if (l + 1 > pNameLen) {
                	pNameLen = l + 1;
                	pName = alloca(pNameLen);
                	sprintf(pName, "Endpoint.Interface.%s", name);
                }
                ism_common_setProperty(props, pName, &f);
            }
            ism_common_free(ism_memory_admin_misc,ifcIP);
            return rc;
        } else {
        	return ISMRC_IPNotValid;
        }
    }
    
    return ISMRC_PropertiesNotValid;
}

static void cfgval_removeFiles(char *path) {
	struct dirent *d;
	DIR *dir;
	char filepath[2048];

    dir = opendir(path);
    if (dir == NULL)
    	return;

    while((d = readdir(dir)) != NULL) {
        //fprintf(stderr, "%s\n",d->d_name);
        memset(filepath, '\0', sizeof(filepath));
        snprintf(filepath, sizeof(filepath), "%s/%s", path, d->d_name);
        unlink(filepath);
    }
    closedir(dir);
    return;
}

int32_t ism_config_valEndpointObjects(ism_prop_t * props, char * name, int isCreate) {
    int rc = ISMRC_OK;
    int nameLen = strlen(name);
	int count = 0;
    int pollen = 0;
	char *token, *nexttoken = NULL;
    char *msgPolicies = NULL;
    char *conPolicies = NULL;
    char *tmpstr = NULL;

    int msgPropLen = nameLen + 28;
    char *msgPropName = alloca(msgPropLen);

	int conPropLen = nameLen + 29;
	char *conPropName = alloca(conPropLen);

    if (isCreate == 1 ) {
		/* check for valid port */
		int portPropLen = nameLen + 15;
		char *portPropName = alloca(portPropLen);
		sprintf(portPropName, "Endpoint.Port.%s", name);
		char *portStr = (char *)ism_common_getStringProperty(props, portPropName);
		if ( !portStr || (portStr && *portStr == '\0')) {
			TRACE(8, "No Port is specified for the Endpoint \"%s\"\n", name);
			ism_common_setError(ISMRC_NoPort);
			return ISMRC_NoPort;
		}
		int port = atoi(portStr);
		if ( port <= 0 || port >= 65536 ) {
			TRACE(8, "Invalid Port %d is specified for the Endpoint \"%s\"\n",
				port, name);
			ism_common_setError(ISMRC_InvalidPort);
			return ISMRC_InvalidPort;
		}



		/* check if message hub is specified - "Endpoint.MessageHub." */
		int mHubPropLen = nameLen + 21;
		char *mHubPropName = alloca(mHubPropLen);
		sprintf(mHubPropName, "Endpoint.MessageHub.%s", name);
		char *msgHub = (char *)ism_common_getStringProperty(props, mHubPropName);
		if ( !msgHub || (msgHub && *msgHub == '\0')) {
			TRACE(8, "No MessageHub is specified for the Endpoint \"%s\"\n", name);
			ism_common_setError(ISMRC_NoMessageHub);
			return ISMRC_NoMessageHub;
		}
		int comptype = ism_config_getCompType("Transport");
		ism_config_t * thandle = ism_config_getHandle(comptype, NULL);
		if ( !thandle ) {

			thandle = ism_config_getHandle(comptype, "Endpoint");
			if ( !thandle ) {
				TRACE(7, "Could not find config handle of Transport component\n");
					ism_common_setError(ISMRC_InvalidComponent);
					return ISMRC_InvalidComponent;
			}
		}
		if ( cfgval_findObject(thandle, "MessageHub", msgHub) == 0 ) {
			rc = ISMRC_InvalidMessageHub;
		}

		if ( rc != ISMRC_OK ) {
			TRACE(8, "Invalid MessageHub \"%s\" is specified for the Endpoint \"%s\"\n", msgHub, name);
			ism_common_setError(rc);
			return rc;
		}

		/* Check for valid connection policies - "Endpoint.ConnectionPolicies." */
		sprintf(conPropName, "Endpoint.ConnectionPolicies.%s", name);
		conPolicies = (char *)ism_common_getStringProperty(props, conPropName);
		if ( !conPolicies || (conPolicies && *conPolicies == '\0')) {
			TRACE(8, "No ConnectionPolicies are specified for the Endpoint \"%s\"\n", name);
			ism_common_setError(ISMRC_NoConnectionPolicy);
			// return ISMRC_NoSecurityPolicies;
			return ISMRC_NoConnectionPolicy;
		}

		/* Check for valid policies - "Endpoint.TopicPolicies." */
		sprintf(msgPropName, "Endpoint.TopicPolicies.%s", name);
		msgPolicies = (char *)ism_common_getStringProperty(props, msgPropName);
		if ( !msgPolicies || (msgPolicies && *msgPolicies == '\0')) {
			TRACE(8, "No TopicPolicies are specified for the Endpoint \"%s\"\n", name);
			ism_common_setError(ISMRC_NoMessagingPolicy);
			// return ISMRC_NoSecurityPolicies;
			return ISMRC_NoMessagingPolicy;
		}

		/* got policy list - check for valid connection and messaging policy */
		int connPol = 0;
		int messPol = 0;
		int found = 0;
		/* get security component config handle */
		comptype = ism_config_getCompType("Security");
		ism_config_t * handle = ism_config_getHandle(comptype, NULL);
		if ( !handle ) {
			TRACE(7, "Could not find config handle of security component\n");
			ism_common_setError(ISMRC_InvalidComponent);
			return ISMRC_InvalidComponent;
		}

		// Check for Connection Policy
		pollen = strlen(conPolicies);
		tmpstr = (char *)alloca(pollen+1);
		tmpstr[pollen] = 0;
		memcpy(tmpstr, conPolicies, pollen);
		for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken)) {
			if ( !token || (token && *token == '\0')) {
				continue;
			} else {
				/* check valid connection policy */
				found = cfgval_findObject(handle, "ConnectionPolicy", token);
				connPol += found;
			}
		}

		// Check for Messaging policies
		pollen = strlen(msgPolicies);
		token = nexttoken = NULL;
		tmpstr = (char *)alloca(pollen+1);
		tmpstr[pollen] = 0;
		memcpy(tmpstr, msgPolicies, pollen);
		for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken)) {
			if ( !token || (token && *token == '\0')) {
				continue;
			} else {
				/* check valid messaging policy */
				found = cfgval_findObject(handle, "TopicPolicy", token);
				messPol += found;
			}
		}

		if (connPol == 0) {
			/* a valid connection policy is required for an endpoint */
			TRACE(7, "Could not find a valid connection policy for the Endpoint: \"%s\"\n", conPolicies? conPolicies:"");
			ism_common_setError(ISMRC_NoConnectPolicies);
			return ISMRC_NoConnectPolicies;
		}
		if (messPol == 0) {
			/* a valid messaging policy is required for an endpoint */
			TRACE(7, "Could not find a valid messaging policy for the Endpoint: \"%s\"\n", msgPolicies? msgPolicies:"");
			ism_common_setError(ISMRC_NoMesssgePolicies);
			return ISMRC_NoMesssgePolicies;
		}

    } else {   // for update case
		int comptype = ism_config_getCompType("Security");
		ism_config_t * handle = ism_config_getHandle(comptype, NULL);
		if ( !handle ) {
			TRACE(7, "Could not find config handle of security component\n");
			ism_common_setError(ISMRC_InvalidComponent);
			return ISMRC_InvalidComponent;
		}

    	/* get "Endpoint.TopicPolicies." if existing for later pending action checking*/
		sprintf(msgPropName, "Endpoint.TopicPolicies.%s", name);
		msgPolicies = (char *)ism_common_getStringProperty(props, msgPropName);
		if ( !msgPolicies || (msgPolicies && *msgPolicies == '\0')) {
			TRACE(8, "No update TopicPolicies are specified for the Endpoint \"%s\"\n", name);
		}else {
			pollen = strlen(msgPolicies);
			tmpstr = (char *)alloca(pollen+1);
			tmpstr[pollen] = 0;
			memcpy(tmpstr, msgPolicies, pollen);
			for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken)) {
				if ( !token || (token && *token == '\0')) {
					continue;
				} else {
					/* check valid messaging policy */
					if (cfgval_findObject(handle, "TopicPolicy", token) == 0) {
						TRACE(3, "Could not find the topic policy %s specified for the Endpoint: \"%s\"\n", token, msgPolicies);
						ism_common_setError(ISMRC_NoMesssgePolicies);
						return ISMRC_NoMesssgePolicies;
					} else {
						count++;
					}
				}
			}
			if (count == 0) {
				TRACE(3, "No valid TopicPolicy has been specified for the Endpoint: \"%s\"\n", msgPolicies);
				ism_common_setError(ISMRC_NoMesssgePolicies);
				return ISMRC_NoMesssgePolicies;
			}
		}

		count = 0;
    	/* get "Endpoint.ConnectionPolicies." if existing for later pending action checking*/
		sprintf(conPropName, "Endpoint.ConnectionPolicies.%s", name);
	    conPolicies = (char *)ism_common_getStringProperty(props, conPropName);
		if ( !conPolicies || (conPolicies && *conPolicies == '\0')) {
			TRACE(8, "No update ConnectionPolicies are specified for the Endpoint \"%s\"\n", name);
		}else {
			pollen = strlen(conPolicies);
			tmpstr = (char *)alloca(pollen+1);
			tmpstr[pollen] = 0;
			memcpy(tmpstr, conPolicies, pollen);
			for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken)) {
				if ( !token || (token && *token == '\0')) {
					continue;
				} else {
					/* check valid messaging policy */
					if (cfgval_findObject(handle, "ConnectionPolicy", token) == 0) {
						TRACE(3, "Could not find the connection policy %s specified for the Endpoint: \"%s\"\n", token, conPolicies);
						ism_common_setError(ISMRC_NoConnectPolicies);
						return ISMRC_NoConnectPolicies;
					} else {
						count++;
					}
				}
			}
			if (count == 0) {
				TRACE(3, "No valid ConnectionPolicy has been specified for the Endpoint: \"%s\"\n", conPolicies);
				ism_common_setError(ISMRC_NoConnectPolicies);
				return ISMRC_NoConnectPolicies;
			}
		}
    }

    /* If security is enabled, and UseClientCertificate is set in the SecurityProfile,
     * then check if trusted certificates uploaded
     */
    int secProfPropLen = nameLen + 26;
    char *secProfPropName = alloca(secProfPropLen);
    sprintf(secProfPropName, "Endpoint.SecurityProfile.%s", name);
    char *secProf = (char *)ism_common_getStringProperty(props, secProfPropName);
    if ( !secProf || (secProf && *secProf == '\0')) {
        return rc;
    } else {
    	/* check UseClientCertificate flag */
    	ism_prop_t * transProps = ism_config_getComponentProps(ISM_CONFIG_COMP_TRANSPORT);
    	char *useCLCert = ism_config_getComponentItemField(transProps, "SecurityProfile", secProf, "UseClientCertificate");
        if ( !useCLCert || (useCLCert && *useCLCert == '\0')) {
            return rc;
        } else {
        	if ( *useCLCert == 'T' || *useCLCert == 't' ) {
				/* check secProf_cafile.pem exist in truststore */
				char *trustCertDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"TrustedCertificateDir");
				int cafileLen = strlen(trustCertDir) + strlen(secProf) + 14;
				char *cafileName = alloca(cafileLen);
				sprintf(cafileName, "%s/%s_cafile.pem", trustCertDir, secProf);
				struct stat sb;
				if (stat(cafileName, &sb) == -1) {
					TRACE(7, "Could not find a valid trusted certificate associated with SecurityProfile \"%s\" for the Endpoint \"%s\".\n", secProf, name);
					rc = ISMRC_NoTrustedCertificate;
				}
        	}
        }
       	ism_common_free(ism_memory_admin_misc,useCLCert);
    }

    return rc;
}

/**
 * Check if the object name is valid.
 * Currently, strict the object name to be Not NULL and 256 in len.
 * @return If valid, return ISMRC_OK, otherwise, an error code will return.
 */
static int ism_config_validateObjectName(const char * name)
{
    if (name != NULL) {
        int namelen= strlen(name);
        int count = ism_common_validUTF8(name, namelen);
        if( count < 0 || count > MAX_OBJ_NAME_LEN ) {
            ism_common_setError(ISMRC_InvalidNameLen);
            return ISMRC_InvalidNameLen;
        }
    } else {
        ism_common_setError(ISMRC_InvalidNameLen);
        return ISMRC_InvalidNameLen;
    }
    return ISMRC_OK;
}

int32_t ism_config_validate_set_data(ism_json_parse_t *json, int *mode) {
    int rc = ISMRC_OK;
    int composite = 0;
    int fflag = 0;

    *mode = ISM_CONFIG_CHANGE_PROPS;

    /* get some basic data from set string */
    char *item = (char *)ism_json_getString(json, "Item");
    char *comp = (char *)ism_json_getString(json, "Component");
    if (comp == NULL) {
        ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &comp, NULL);
        fflag = 1;
    }
    char *name = (char *)ism_json_getString(json, "Name");
    char *ObjectIdField = (char *)ism_json_getString(json, "ObjectIdField");
    // char *indexStr = (char *)ism_json_getString(json, "Index");

    
    /* Get object type */
    char *type = (char *)ism_json_getString(json, "Type");
    if ( type && !strncasecmp(type,"composite",9))
        composite = 1;
        
    /*Validate the Name*/
    if( composite && !ObjectIdField && strcmpi(item, "ClientSet")
    		&& (rc = ism_config_validateObjectName((const char*)name)) != ISMRC_OK ) {
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
    	return rc;
    }
    

    if (!comp || !item || (composite == 1 && !ObjectIdField && !name && strcmpi(item, "ClientSet")) ) {
        TRACE(7, "Invalid items in set request: comp=%s item=%s name=%s\n",
                comp?comp:"", item?item:"", name?name:"");
        ism_common_setError(ISMRC_InvalidComponent);
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
        return ISMRC_InvalidComponent;
    }

    /* check if component is valid and get its handle */
    int comptype = ism_config_getCompType(comp);

    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        /* try for object specific handle */
        handle = ism_config_getHandle(comptype, item);
        if ( !handle ) {
            /* Invalid component - return error */
            TRACE(7, "Invalid component %s\n", comp);
            ism_common_setError(ISMRC_InvalidComponent);
            if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
            return ISMRC_InvalidComponent;
        }
    }

    /* return ok if single object */
    if ( composite == 0 ) {
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
        return rc;
    }

    if ( ObjectIdField ) {
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
        return rc;
    }

    /* check if object exist */
    int objectFound = 0;
    const char *pName = NULL;
    int i;
    ism_prop_t *p = ism_config_getProperties(handle, item, name);
    if ( p ) {
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            int len = strlen(name) + strlen(item) + 7;
            char *key = alloca(len);
            sprintf(key, "%s.Name.%s", item, name);
            if ( strncmp(key, pName, len-1) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && *value == *name ) {
                    objectFound = 1;
                    // TRACE(8, "key=%s pName=%s value=%s\n", key, pName, value?value:"");
                    break;
                }
            }
        }
        ism_common_freeProperties(p);
    }

#if 0
    /* check for rename option */
    char *newname = (char *)ism_json_getString(json, "NewName");
    if ( newname && *newname != '\0') {
        /* check if object exist, if not throw error */
        if ( objectFound == 0 ) {
            TRACE(7, "Cannot rename an non-existing object %s\n", name);
            ism_common_setError(ISMRC_PropertiesNotValid);
            return ISMRC_PropertiesNotValid;
        }
        *mode = ISM_CONFIG_CHANGE_NAME;
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
        return rc;
    }
#endif

    /* check for update or create */
    char *update = (char *)ism_json_getString(json, "Update");
    if ( update && strncasecmp(update, "true", 4) == 0 ) {
        if ( objectFound == 0 ) {
            TRACE(8,"Cannot update a non-exiting object: %s - %s.\n",item,name);
            ism_common_setError(ISMRC_PropEditError);
            if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
            return ISMRC_PropEditError;
        }
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
        return rc;
    }

    /* check for delete - only composite objects can be deleted */
    char *delete = (char *)ism_json_getString(json, "Delete");
    if ( delete && strncasecmp(delete, "true", 4) == 0 ) {
        if (composite == 0) {
            TRACE(8, "Cannot delete a singleton item: %s - %s.\n", item, name);
            ism_common_setError(ISMRC_SingltonDeleteError);
            return ISMRC_SingltonDeleteError;
        }
        *mode = ISM_CONFIG_CHANGE_DELETE;
        if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
        return rc;
    }

    /* check if connection policy is reused */


    /* Trying to create a new object */
    if ( objectFound == 1 ) {
        TRACE(8,"Cannot create an exiting object: %s - %s.\n",item,name);
        ism_common_setError(ISMRC_PropCreateError);
        return ISMRC_PropCreateError;
    }
    if (comp && fflag) ism_common_free(ism_memory_admin_misc,comp);
    return rc;
}

int32_t ism_config_valDeleteEndpointObject(char *object, char * name) {
    int rc = ISMRC_OK;

    if ( !object || !name )
        return ISMRC_NotFound;

    int comptype = ism_config_getCompType("Transport");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }

    /* Get list of endpoints, check if the specified endpoint is referenced */
    int count = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "Endpoint", NULL);
    if ( !p ) {
        rc = ISMRC_NoEndpoint;
    } else {
        const char *pName = NULL;
        int i;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            char objstr[64];
            sprintf(objstr, "Endpoint.%s.", object);
            int len = strlen(objstr);
            if (strncmp(pName, objstr, len) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' ) {
                    int plen = strlen(value);
                    char *token, *nexttoken = NULL;
                    char *tmpstr = (char *)alloca(plen+1);
                    tmpstr[plen] = 0;
                    memcpy(tmpstr, value, plen);
                    for (token = strtok_r(tmpstr, ",", &nexttoken); 
                        token != NULL; 
                        token = strtok_r(NULL, ",", &nexttoken)) 
                    {
                        if ( token && *token != '\0' && !strcmp(token, name)) {
                            count = 1;
                            break;
                        }
                    }
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if ( count ) {
        if ( strcmp(object, "MessageHub") == 0 ) {
            rc = ISMRC_MessageHubInUse;
        } else if ( strcmp(object, "ConnectionPolicies") == 0 ) {
            rc = ISMRC_SecPolicyInUse;
        } else if ( strcmp(object, "TopicPolicies") == 0 ) {
            rc = ISMRC_SecPolicyInUse;
        } else if ( strcmp(object, "SecurityProfile") == 0 ) {
            rc = ISMRC_SecProfileInUse;
        }
        if (ISMRC_SecPolicyInUse == rc) ism_common_setErrorData(ISMRC_SecPolicyInUse, "%s", name);
        else ism_common_setError(rc);
    }
    return rc;
}

int32_t ism_config_validateDeleteCertificateProfile(char * name) {
    int rc = ISMRC_OK;

    /* Sanity check for MessageHub object */
    int comptype = ism_config_getCompType("Transport");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int count = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "SecurityProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        const char *pName = NULL;
        int i;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName, "SecurityProfile.CertificateProfile.", 35) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, name)) {
                     count = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if ( count ) {
        rc = ISMRC_CertProfileInUse;
        ism_common_setError(rc);
    }
    return rc;
}
int32_t ism_config_validateDeleteLTPAProfile(char * name) {
    int rc = ISMRC_OK;
    int comptype = -1;

    /* Sanity check for MessageHub object */
    comptype = ism_config_getCompType("Security");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int found = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "LTPAProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        const char *pName = NULL;
        int i;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName, "LTPAProfile.Name.", 17) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, name)) {
                     found = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if (found == 0) {
    	rc = ISMRC_NotFound;
    }

    /* Now check if the specified profile has been used */
    comptype = ism_config_getCompType("Transport");
    ism_config_t * shandle = ism_config_getHandle(comptype, NULL);
    if ( !shandle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int count = 0;
    ism_prop_t *sp = ism_config_getProperties(shandle, "SecurityProfile", NULL);
    if ( !sp ) {
        rc = ISMRC_NotFound;
    } else {
        const char *spName = NULL;
        int j;
        for (j=0; ism_common_getPropertyIndex(sp, j, &spName) == 0; j++) {
            if (strncmp(spName, "SecurityProfile.LTPAProfile.", 28) == 0 ) {
                char *svalue = (char *)ism_common_getStringProperty(sp, spName);
                if ( svalue && *svalue != '\0' && !strcmp(svalue, name)) {
                     count = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(sp);
    }
    if ( count ) {
        rc = ISMRC_LTPAProfileInUse;
        ism_common_setError(rc);
    }
    return rc;
}

int32_t ism_config_validateDeleteOAuthProfile(char * name) {
    int rc = ISMRC_OK;
    int comptype = -1;

    /* Sanity check for MessageHub object */
    comptype = ism_config_getCompType("Security");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int found = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "OAuthProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        const char *pName = NULL;
        int i;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName, "OAuthProfile.Name.", 18) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, name)) {
                     found = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if (found == 0) {
    	rc = ISMRC_NotFound;
    }

    /* Now check if the specified profile has been used */
    comptype = ism_config_getCompType("Transport");
    ism_config_t * shandle = ism_config_getHandle(comptype, NULL);
    if ( !shandle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int count = 0;
    ism_prop_t *sp = ism_config_getProperties(shandle, "SecurityProfile", NULL);
    if ( !sp ) {
        rc = ISMRC_NotFound;
    } else {
        const char *spName = NULL;
        int j;
        for (j=0; ism_common_getPropertyIndex(sp, j, &spName) == 0; j++) {
            if (strncmp(spName, "SecurityProfile.OAuthProfile.", 29) == 0 ) {
                char *svalue = (char *)ism_common_getStringProperty(sp, spName);
                if ( svalue && *svalue != '\0' && !strcmp(svalue, name)) {
                     count = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(sp);
    }
    if ( count ) {
        rc = ISMRC_OAuthProfileInUse;
        ism_common_setError(rc);
    }
    return rc;
}

int32_t ism_config_validateCertificateProfileExist(ism_json_parse_t *json, char * name, int isUpdate) {
    int rc = ISMRC_OK;
    int i;
    const char *pName = NULL;
    char *tlsEnabled = (char *)ism_json_getString(json, "TLSEnabled");
    char *certName = (char *)ism_json_getString(json, "CertificateProfile");

	/*  check is not needed with TLSEnabled = false and no CertificateProfile is specified */
	if ( tlsEnabled && !strcasecmp(tlsEnabled,"false") && ( !certName || *certName == '\0' ) ) {
	    return rc;
	}

	/* check if the certificate already exist inside the security profile *
	 * This situation is introduced by TLSEnabled.
	 * When TLSEnabled is false, CertificateProfile is disabled but still exist
	 * inside SecurityProfile configure object.
	 * We need to check if the CertificateProfile is still there when TLSEnable update to true
	 *
	 * We also need to check TLSEnabled value during update to make sure we do NOT check
	 * CertificateProfile is the value is ture or no value.
	 */

    int comptype = ism_config_getCompType("Transport");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }

    /*  create case */
    if ( isUpdate == 0 ) {
	    if ( !certName || *certName == '\0' ) {
			TRACE(3, "%s: No CertificateProfile specified for the SecurityProfile \"%s\"\n", __FUNCTION__, name);
			ism_common_setError(ISMRC_NoCertProfile);
			return ISMRC_NoCertProfile;
	    }
    } else {    // update case
    	ism_prop_t *sprops = ism_config_getPropertiesDynamic(handle, "SecurityProfile", name);
    	char *p1 = alloca(35 + strlen(name) + 1);
    	sprintf(p1, "%s.%s.%s", "SecurityProfile", "CertificateProfile", name);
    	char *p2 = alloca(27 + strlen(name) + 1);
    	sprintf(p2, "%s.%s.%s", "SecurityProfile", "TLSEnabled", name);

		for (i=0; ism_common_getPropertyIndex(sprops, i, &pName) == 0; i++) {
			if (pName == NULL)
				continue;
			if (strncmp(pName, p1, strlen(p1)) == 0 ) {
				if ( !certName ) {
				    certName = (char *)ism_common_getStringProperty(sprops, pName);
				    TRACE(9, "%s: Found certName=%s\n", __FUNCTION__, certName?certName:"NULL");
				}
			}else if (strncmp(pName, p2, strlen(p2)) == 0 ) {
				if (tlsEnabled == NULL) {
					tlsEnabled = (char *)ism_common_getStringProperty(sprops, pName);
					TRACE(9, "%s: Found TLSEnabled=%s in config\n",  __FUNCTION__, tlsEnabled?tlsEnabled:"NULL");
				}
			}
			if (certName && tlsEnabled)
				break;

			pName = NULL;
		}

		/*  check is not needed with TLSEnabled = false and without CertificateProfile change*/
		if ( tlsEnabled && !strcasecmp(tlsEnabled,"false") && ( !certName || *certName == '\0' ) ) {
			return rc;
		}

        if ( ( !certName || *certName == '\0' ) && ( tlsEnabled == NULL || !strcasecmp(tlsEnabled,"true") )) {
        	TRACE(3, "%s: No CertificateProfile specified for the SecurityProfile \"%s\"\n", __FUNCTION__, name);
        	ism_common_setError(ISMRC_NoCertProfile);
            return ISMRC_NoCertProfile;
        }

    }

    TRACE(8, "%s: certificateProfile name is [%s]\n", __FUNCTION__, certName);

    /* Sanity check for CertitifaceProfile object */
    /* Get list of endpoints, check if the specified policy is referenced */
    int count = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "CertificateProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        pName = NULL;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName, "CertificateProfile.Name.", 24) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, certName)) {
                	TRACE(8, "Find certificateProfile.\n");
                     count = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if ( count == 0 ) {
        rc = ISMRC_NoCertProfileFound;
        ism_common_setError(rc);
    }
    return rc;
}

int32_t ism_config_validateCertificateProfileKeyCertUnique(ism_json_parse_t *json, char * name) {
    int rc = ISMRC_OK;

    char *certName = (char *)ism_json_getString(json, "Certificate");
    if ( !certName || (certName && *certName == '\0')) {
        TRACE(8, "No Certificate specified for the CertificateProfile \"%s\"\n", name);
        ism_common_setError(ISMRC_NotFound);
        return ISMRC_NotFound;
    }
    TRACE(8, "Certificate name is [%s]\n", certName);

    char *keyName = (char *)ism_json_getString(json, "Key");
    if ( !keyName || (keyName && *keyName == '\0')) {
        TRACE(8, "No key specified for the CertificateProfile \"%s\"\n", name);
        ism_common_setError(ISMRC_NotFound);
        return ISMRC_NotFound;
    }
    TRACE(8, "key name is [%s]\n", keyName);


    /* Sanity check for MessageHub object */
    int comptype = ism_config_getCompType("Transport");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Transport component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    ism_prop_t *p = ism_config_getProperties(handle, "CertificateProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        const char *pName = NULL;
        int i;
        const char CERT_PROFILE_CERTIFICATE[] = "CertificateProfile.Certificate.";
        const char CERT_PROFILE_KEY[]         = "CertificateProfile.Key.";
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName,
            			CERT_PROFILE_CERTIFICATE,
            			sizeof(CERT_PROFILE_CERTIFICATE) - 1) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, certName)) {
                	char *ctmpstr = (char *)pName+strlen(CERT_PROFILE_CERTIFICATE);
                	if (ctmpstr != NULL && !strcmp(ctmpstr, name)) {
                		TRACE(8, "The certificate %s is assigned to the CertificateProfile %s.\n", certName, name);
                	} else {
                	    TRACE(8, "The certificate %s can not be assigned to CertificateProfile %s because it is already assigned to a different CertificateProfile.\n", certName, name);
                        rc = ISMRC_CertNameInUse;
                	}
                     break;
                }
            }
            if (strncmp(pName, CERT_PROFILE_KEY, sizeof(CERT_PROFILE_KEY) - 1) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, keyName)) {
                	char *ktmpstr = (char *)pName + strlen(CERT_PROFILE_KEY);
                	if (ktmpstr != NULL && !strcmp(ktmpstr, name)) {
                		TRACE(8, "The key %s is assigned to the CertificateProfile %s.\n", keyName, name);
                	} else {
                	    TRACE(8, "The key %s can not be assigned to CertificateProfile %s because it is already assigned to a different CertificateProfile.\n", keyName, name);
                        rc = ISMRC_KeyNameInUse;
                        break;
                	}
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if ( rc != ISMRC_OK ) {
        ism_common_setError(rc);
    }
    return rc;
}

//int32_t ism_config_valPolicyOptionalFilter(ism_prop_t * props, char * name) {
//    int rc = ISMRC_OK;
//    int nameLen = strlen(name);
//
//    /* check if one of the following optional filters are set:
//     * ID, Group, ClientID, ClientAddress, Protocol, CommonNames
//     */
//    int prPropLen = nameLen + 27;
//    char *prPropName = alloca(prPropLen);
//    sprintf(prPropName, "ConnectionPolicy.ID.%s", name);
//    char *prStr = (char *)ism_common_getStringProperty(props, prPropName);
//    if ( prStr && *prStr == '\0' )
//    	return rc;
//
//    sprintf(prPropName, "ConnectionPolicy.GroupID.%s", name);
//    prStr = (char *)ism_common_getStringProperty(props, prPropName);
//    if ( prStr && *prStr == '\0' )
//    	return rc;
//
//    sprintf(prPropName, "ConnectionPolicy.ClientID.%s", name);
//    prStr = (char *)ism_common_getStringProperty(props, prPropName);
//    if ( prStr && *prStr == '\0' )
//    	return rc;
//
//    sprintf(prPropName, "ConnectionPolicy.ClientAddress.%s", name);
//    prStr = (char *)ism_common_getStringProperty(props, prPropName);
//    if ( prStr && *prStr == '\0' )
//    	return rc;
//
//    sprintf(prPropName, "ConnectionPolicy.CommonNames.%s", name);
//    prStr = (char *)ism_common_getStringProperty(props, prPropName);
//    if ( prStr && *prStr == '\0' )
//    	return rc;
//
//    sprintf(prPropName, "ConnectionPolicy.Protocol.%s", name);
//    prStr = (char *)ism_common_getStringProperty(props, prPropName);
//    if ( prStr && *prStr == '\0' )
//    	return rc;
//
//    TRACE(8, "Policy \"%s\" is not valid. At least one of the optional filters is required.\n", name);
//    return ISMRC_NoOptPolicyFilter;
//}

int32_t ism_config_deleteCertificateProfileKeyCert(char * cert, char *key, int needDeleteCert, int needDeleteKey) {
    int rc = ISMRC_OK;
    char filepath[1024];
    int isSameFile = 0;

    if (cert && key && !strcmp(cert, key)) {
    	isSameFile = 1;
    }
	char *certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
			, "KeyStore" );
    if (cert && needDeleteCert)  {
        memset(filepath, '\0', sizeof(filepath));
        snprintf(filepath, sizeof(filepath), "%s/%s", certDir, cert);
        TRACE(9, "remove %s\n", filepath);
        unlink(filepath);
    }

    if (key && isSameFile != 1 && needDeleteKey)  {
        memset(filepath, '\0', sizeof(filepath));
        snprintf(filepath, sizeof(filepath), "%s/%s", certDir, key);
        TRACE(9, "remove %s\n", filepath);
        unlink(filepath);
    }
    return rc;
}

int32_t ism_config_deleteSecurityProfile(char * name) {
    int rc = ISMRC_OK;
    char filepath[1024];

	char *certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
			, "TrustedCertificateDir" );
    /* remove *_capath first*/
    memset(filepath, '\0', sizeof(filepath));
    snprintf(filepath, sizeof(filepath), "%s/%s_capath", certDir, name);
    cfgval_removeFiles(filepath);
    rmdir(filepath);

    memset(filepath, '\0', sizeof(filepath));
    snprintf(filepath, sizeof(filepath), "%s/%s", certDir, name);
    cfgval_removeFiles(filepath);
    rmdir(filepath);

    memset(filepath, '\0', sizeof(filepath));
    snprintf(filepath, sizeof(filepath), "%s/%s_cafile.pem", certDir, name);
    unlink(filepath);
    return rc;
}

int32_t ism_config_deleteLTPAKeyFile(char * name) {
    int rc = ISMRC_OK;
    char filepath[1024];

    memset(filepath, '\0', sizeof(filepath));
	char *certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(), "LTPAKeyStore" );
    snprintf(filepath, sizeof(filepath), "%s/%s", certDir, name);
    unlink(filepath);
    return rc;
}

int ism_config_deleteOAuthKeyFile(char * name) {
    int rc = ISMRC_OK;
    char filepath[1024];

    memset(filepath, '\0', sizeof(filepath));
    char *certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(), "OAuthCertificateDir" );
    snprintf(filepath, sizeof(filepath), "%s/%s", certDir, name);
    unlink(filepath);
    return rc;
}

int ism_config_deleteMQCerts(void) {
    int rc = ISMRC_OK;
    char filepath[2048];
    char *certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(), "MQCertificateDir" );

    memset(filepath, '\0', sizeof(filepath));
    snprintf(filepath, sizeof(filepath), "%s/mqconnectivity.kdb", certDir);
    unlink(filepath);

    memset(filepath, '\0', sizeof(filepath));
    snprintf(filepath, sizeof(filepath), "%s/mqconnectivity.sth", certDir);
    unlink(filepath);

    return rc;
}

/*
 * Validate ClientAddress:
 * Valid IPv4 or IPv6 address
 * Valid range - low-high
 */

static int ism_config_cmp_in6addr(struct in6_addr * high, struct in6_addr * low) {
    int ndx;
    int cmp = 0;
    for (ndx = 0; ndx < 16; ndx++) {
        if (high->s6_addr[ndx] < low->s6_addr[ndx]) {
            cmp = -1;
            break;
        }
    }
    return cmp;
}


int32_t ism_config_validateClientAddress(char *addr) {
        if ( !addr )
                return 0;

        TRACE(5, "Validate ClientAddress: %s\n", addr);

        int len = strlen(addr);
        char *tmpstr = (char *)alloca(len+1);
        memcpy(tmpstr, addr, len);
        tmpstr[len] = 0;

        if ( *tmpstr == '*' || !strcmp(tmpstr, "0.0.0.0") || !strcmp(tmpstr, "[::]") )
        {
                return 1;
        }


        char *token, *nexttoken = NULL;
        for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL;
                token = strtok_r(NULL, ",", &nexttoken))
        {
                int isPair = 0;
                int type = 1;

                if ( strstr(token, "-"))
                        isPair = 1;

                if ( strstr(token, ":"))
                        type = 2;

                char *addstr = token;
                char *tmptok;
                char *IPlow = strtok_r(addstr, "-", &tmptok);
                char *IPhigh = strtok_r(NULL, "-", &tmptok);

                if ( !isPair )
                        IPhigh = IPlow;

                if ( type == 1 ) {
                    struct sockaddr_in ipaddr;
                    int rc = 0;
                    rc = inet_aton(IPlow, &ipaddr.sin_addr);
                    //fprintf(stderr, "inet_pton return rc: %d\n", rc);
                    if (rc == 0 )  {
                        return 0;
                    }

                    if ( isPair ) {
                    	rc = inet_aton(IPhigh, &ipaddr.sin_addr);
                    	if (rc == 0 )  {
                    	   return 0;
                    	}
                        unsigned long low = ntohl(inet_addr(IPlow));
                        unsigned long high = ntohl(inet_addr(IPhigh));
                        if ( low > high ) return 0;
                    }

                } else {
                        int rc1=0, rc2=0;
                        char buf[64];
                        int i = 0;
                        struct in6_addr low6;
                        struct in6_addr high6;

                        memset(buf, 0, 64);
                        i = 0;
                        tmpstr = IPlow;
                        while(*tmpstr) {
                                if ( *tmpstr == '[' || *tmpstr == ']' ) {
                                        tmpstr++;
                                        continue;
                                }
                                buf[i++] = *tmpstr;
                                tmpstr++;
                        }

                        rc1 = inet_pton(AF_INET6, buf, &low6);
                        if ( rc1 != 1 ) {
                                return 0;
                        }

                        memset(buf, 0, 64);
                        i = 0;
                        tmpstr = IPhigh;
                        while(*tmpstr) {
                                if ( *tmpstr == '[' || *tmpstr == ']' ) {
                                        tmpstr++;
                                        continue;
                                }
                                buf[i++] = *tmpstr;
                                tmpstr++;
                        }

                        rc2 = inet_pton(AF_INET6, buf, &high6);
                        if ( rc2 != 1 ) {
                                return 0;
                        }

                        if ( ism_config_cmp_in6addr(&high6, &low6) < 0 )
                                return 0;
                }
        }

        return 1;
}

int32_t ism_config_validateLTPAProfileExist(ism_json_parse_t *json, char * name) {
    int rc = ISMRC_OK;

    char *ltpaName = (char *)ism_json_getString(json, "LTPAProfile");
    if ( !ltpaName || (ltpaName && *ltpaName == '\0')) {
        TRACE(8, "No LTPAProfile specified for the SecurityProfile \"%s\"\n", name);
        return rc;
    }
    TRACE(8, "LTPAProfile name is [%s]\n", ltpaName);


    /* Sanity check for MessageHub object */
    int comptype = ism_config_getCompType("Security");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Security component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int count = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "LTPAProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        const char *pName = NULL;
        int i;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName, "LTPAProfile.Name.", 17) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, ltpaName)) {
                	TRACE(8, "Find LTPAProfile.\n");
                     count = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if ( count == 0 ) {
    	TRACE(5, "LTPAProfile %s is not found.", ltpaName);
        rc = ISMRC_NoLTPAProfileFound;
        ism_common_setError(rc);
    }
    return rc;
}

int32_t ism_config_validateOAuthProfileExist(ism_json_parse_t *json, char * name) {
    int rc = ISMRC_OK;

    char *oauthName = (char *)ism_json_getString(json, "OAuthProfile");
    if ( !oauthName || (oauthName && *oauthName == '\0')) {
        TRACE(8, "No LTPAProfile specified for the SecurityProfile \"%s\"\n", name);
        return rc;
    }
    TRACE(8, "OAuthProfile name is [%s]\n", oauthName);


    /* Sanity check for MessageHub object */
    int comptype = ism_config_getCompType("Security");
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        TRACE(7, "Could not find config handle of Security component\n");
        ism_common_setError(ISMRC_InvalidComponent);
        return ISMRC_InvalidComponent;
    }
    /* Get list of endpoints, check if the specified policy is referenced */
    int count = 0;
    ism_prop_t *p = ism_config_getProperties(handle, "OAuthProfile", NULL);
    if ( !p ) {
        rc = ISMRC_NotFound;
    } else {
        const char *pName = NULL;
        int i;
        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if (strncmp(pName, "OAuthProfile.Name.", 18) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && !strcmp(value, oauthName)) {
                	TRACE(8, "Find OAuthProfile.\n");
                     count = 1;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }
    if ( count == 0 ) {
    	TRACE(5, "OAuthProfile %s is not found.", oauthName);
        rc = ISMRC_NoOAuthProfileFound;
        ism_common_setError(rc);
    }
    return rc;
}

/*
 *
 * Check if Certificate and Key file specified in payload
 * is unique to the CertificateProfile.
 * If the Certificate and Key file name has been used by the other CertificateProfile,
 * this checking will fail.
 *
 * @param  name         The CertificateProfile name.
 * @param  certName     The Certificate file name.
 * @param  keyName      The Key file name
 *
 * Returns ISMRC_OK on passing the checking, ISMRC_* on error
 * */
int32_t ism_config_json_validateCertificateProfileKeyCertUnique(char * name, char *certName, char *keyName) {
    int rc = ISMRC_OK;
    int found = 0;

    if ( !name || *name == '\0') {
		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "CertificateProfile", name?name:"null");
		rc =ISMRC_BadPropertyValue;
		goto VALIDATE_CERT_END;
	}

    if ( !certName || *certName == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Certificate", certName?certName:"null");
        rc =ISMRC_BadPropertyValue;
        goto VALIDATE_CERT_END;
    }

    if ( !keyName || *keyName == '\0') {
    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Key", keyName?keyName:"null");
    	rc =ISMRC_BadPropertyValue;
    	goto VALIDATE_CERT_END;
    }


    /* find object */
    json_t * objroot = json_object_get(srvConfigRoot, "CertificateProfile");
    if ( objroot ) {
        found = 1;
    }

    if ( found ) {
    	/* Iterate thru the node */
    	void *itemIter = json_object_iter(objroot);
    	while(itemIter) {
			const char *instName = json_object_iter_key(itemIter);
			json_t * instObj = json_object_iter_value(itemIter);
			int objType = json_typeof(instObj);

			if (!instName) {
				itemIter = json_object_iter_next(objroot, itemIter);
				continue;
			}

			if (objType != JSON_OBJECT) {
				/* Return error */
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "CertificateProfile", instName, "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto VALIDATE_CERT_END;
			}

			//skip the checking if
			if (!strcmp(instName, name)) {
				itemIter = json_object_iter_next(objroot, itemIter);
				continue;
			}

			//check certificate file name
			json_t *item1 = json_object_get(instObj, "Certificate");
			if (item1->type != JSON_STRING) {
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "Certificate", "Name", "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto VALIDATE_CERT_END;
			}
			const char *cert = json_string_value(item1);
			if (cert && !strcmp(cert, certName)) {
				TRACE(3, "%s: The certificate %s can not be assigned to CertificateProfile %s. It is already assigned to the CertificateProfile %s.\n", __FUNCTION__, certName, name, instName);
				ism_common_setError(ISMRC_CertNameInUse);
				rc = ISMRC_CertNameInUse;
				goto VALIDATE_CERT_END;
			}

			//check key file name
			json_t *item2 = json_object_get(instObj, "Key");
			if (!item2 || item2->type != JSON_STRING) {
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "Key", "Name", "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto VALIDATE_CERT_END;
			}
			const char *key = json_string_value(item2);
			if (key && !strcmp(key, keyName)) {
				TRACE(3, "%s: The Key file %s can not be assigned to CertificateProfile %s. It is already assigned to the CertificateProfile %s.\n", __FUNCTION__, keyName, name, instName);
				ism_common_setError(ISMRC_KeyNameInUse);
				rc = ISMRC_KeyNameInUse;
				goto VALIDATE_CERT_END;

			}
			itemIter = json_object_iter_next(objroot, itemIter);
    	}  // end of while loop
    }

VALIDATE_CERT_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}


/*
 *
 * Check if LTPA Certificate specified in payload
 * is unique to the LTPAProfile.
 * If the Certificate  file name has been used by the other LTPAProfile,
 * this checking will fail.
 *
 * @param  name         The CertificateProfile name.
 * @param  certName     The Certificate file name.
 *
 * Returns ISMRC_OK on passing the checking, ISMRC_* on error
 * */
int32_t ism_config_json_validateLTPACertUnique(char * name, char *certName) {
    int rc = ISMRC_OK;
    int found = 0;

    if ( !name || *name == '\0') {
		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "LTPAProfile", name?name:"null");
		rc =ISMRC_BadPropertyValue;
		goto VALIDATE_CERT_END;
	}

    if ( !certName || *certName == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Certificate", certName?certName:"null");
        rc =ISMRC_BadPropertyValue;
        goto VALIDATE_CERT_END;
    }

    /* find object */
    json_t * objroot = json_object_get(srvConfigRoot, "LTPAProfile");
    if ( objroot ) {
        found = 1;
    }

    if ( found ) {
    	/* Iterate thru the node */
    	void *itemIter = json_object_iter(objroot);
    	while(itemIter) {
			const char *instName = json_object_iter_key(itemIter);
			json_t * instObj = json_object_iter_value(itemIter);
			int objType = json_typeof(instObj);

			if (!instName) {
				itemIter = json_object_iter_next(objroot, itemIter);
				continue;
			}

			if (objType != JSON_OBJECT) {
				/* Return error */
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "LTPAProfile", instName, "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto VALIDATE_CERT_END;
			}

			//skip the checking if
			if (!strcmp(instName, name)) {
				itemIter = json_object_iter_next(objroot, itemIter);
				continue;
			}

			//check certificate file name
			json_t *item = json_object_get(instObj, "KeyFileName");
			if (item->type != JSON_STRING) {
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "KeyFileName", "Name", "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto VALIDATE_CERT_END;
			}
			const char *cert = json_string_value(item);
			if (cert && !strcmp(cert, certName)) {
				TRACE(3, "%s: The certificate %s can not be assigned to CertificateProfile %s. It is already assigned to the CertificateProfile %s.\n", __FUNCTION__, certName, name, instName);
				ism_common_setError(ISMRC_CertNameInUse);
				rc = ISMRC_CertNameInUse;
				goto VALIDATE_CERT_END;
			}

			itemIter = json_object_iter_next(objroot, itemIter);
    	}  // end of while loop
    }

VALIDATE_CERT_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 *
 * Check if LTPA Certificate specified in payload
 * is unique to the LTPAProfile.
 * If the Certificate  file name has been used by the other LTPAProfile,
 * this checking will fail.
 *
 * @param  name         The CertificateProfile name.
 * @param  certName     The Certificate file name.
 *
 * Returns ISMRC_OK on passing the checking, ISMRC_* on error
 * */
int32_t ism_config_json_validateOAuthCertUnique(char * name, char *certName) {
    int rc = ISMRC_OK;
    int found = 0;

    if ( !name || *name == '\0') {
		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "OAuthProfile", name?name:"null");
		rc =ISMRC_BadPropertyValue;
		goto VALIDATE_CERT_END;
	}

    if ( !certName || *certName == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KeyFileName", certName?certName:"null");
        rc =ISMRC_BadPropertyValue;
        goto VALIDATE_CERT_END;
    }

    /* find object */
    json_t * objroot = json_object_get(srvConfigRoot, "OAuthProfile");
    if ( objroot ) {
        found = 1;
    }

    if ( found ) {
    	/* Iterate thru the node */
    	void *itemIter = json_object_iter(objroot);
    	while(itemIter) {
			const char *instName = json_object_iter_key(itemIter);
			json_t * instObj = json_object_iter_value(itemIter);
			int objType = json_typeof(instObj);

			if (!instName) {
				itemIter = json_object_iter_next(objroot, itemIter);
				continue;
			}

			if (objType != JSON_OBJECT) {
				/* Return error */
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "OAuthProfile", instName, "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto VALIDATE_CERT_END;
			}

			//skip the checking if
			if (!strcmp(instName, name)) {
				itemIter = json_object_iter_next(objroot, itemIter);
				continue;
			}

			//check certificate file name
			json_t *item = json_object_get(instObj, "KeyFileName");
            if ( item ) {
                if (item->type != JSON_STRING) {
                    ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "KeyFileName", "Name", "InvalidType");
                    rc = ISMRC_BadOptionValue;
                    goto VALIDATE_CERT_END;
                }

                const char *cert = json_string_value(item);
                if (cert && !strcmp(cert, certName)) {
                    TRACE(3, "%s: The certificate %s can not be assigned to CertificateProfile %s. It is already assigned to the CertificateProfile %s.\n", __FUNCTION__, certName, name, instName);
                    ism_common_setError(ISMRC_CertNameInUse);
                    rc = ISMRC_CertNameInUse;
                    goto VALIDATE_CERT_END;
                }
			}

			itemIter = json_object_iter_next(objroot, itemIter);
    	}  // end of while loop
    }

VALIDATE_CERT_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}
