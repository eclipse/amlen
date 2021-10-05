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
#include <validateInternal.h>
#include <ismregex.h>
#include <arpa/inet.h>
#include <selector.h>

/*
 * This file contains functions to validate generic configuration data types
 * specified in configuration and monitoring schema, such as:
 * Name - name of object
 * Number
 * Boolean
 * String
 * Enum
 * IP Address (IPv4 and IPv6)
 */
 

/*
 * NOTES for developers:
 *
 * 1. To add a validation function, use the following naming convention for function prototype:
 *
 *    int32_t ism_config_validateDataType_<name>(args ...)
 *
 *    In typical signature, arguments will include name of the configuration item,
 *    value to get validated, and data fields specified to define validation rules
 *    in the configuration schema for the configuration item.
 *
 *    Function should returns ISMRC_OK on success, ISMRC_* on error
 *
 * 2. Add function prototype in include/validateConfigData.h file.
 *    Add function validation rules in this file.
 *
 * 3. If there are any internal data, functions etc, define these in
 *    validate_internal.c and validate_internal.h
 *
 * 4. Create CUNIT test in validate_genericData_test.c file in test directory
 *
 * 5. Update CUNIT header file admin_test.h to include generic data cunit tests
 *
 */

/* check if the string is digit*/
static int isInteger(char *tmpstr) {
    if ( !tmpstr )
        return 0;
    while (*tmpstr) {
        if(!isdigit((int)*tmpstr))
            return 0;
        tmpstr++;
    }
    return 1;
}

/*
 * Validate data type number.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  min                    The minimum allowed number
 * @param  max                    The maximum allowed number
 * @param  sizeQualifierOptions   Comma-separated list of allowed size qualifiers
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_number( char *name, char *value, char *min, char *max, char *sizeQualifierOptions ) {
    int32_t rc = ISMRC_OK;

    //TRACE(9, "Entry %s: name: %s, value: %s, min: %s, max: %s, sizeQualifierOptions: %s\n",
    //    		__FUNCTION__, name?name:"null", value?value:"null",
    //    		min?min:"null", max?max:"null", sizeQualifierOptions?sizeQualifierOptions:"null");

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    char *tmpstr = value;
    if ( !tmpstr || *tmpstr == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "NULL");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    /* check if option includes any qualifier */
    int data;
    char * qualif = NULL;
    data = strtoul(value, &qualif, 10);

    if ( qualif ) {
        while ( *qualif == ' ' )
            qualif++;

        /* check if qualifier is in list of sizeQualifierOptions */
        if ( sizeQualifierOptions && *sizeQualifierOptions != '\0' ) {
            char *opttoken, *optnexttoken = NULL;

            int len = strlen(sizeQualifierOptions);
            char *ioption = (char *)alloca( len+1 );
            memcpy(ioption, sizeQualifierOptions, len);
            ioption[len] = 0;
            int qualifierIsValid = 0;

            /* remove all spaces from ioptions before processing */
            int i = 0, j = 0;
            char *tmpstr2 = (char *)alloca(len + 1);
            while (ioption[i] != '\0')
            {
                if (!(ioption[i] == ' ' && ioption[i + 1] == ' ')) {
                    tmpstr2[j] = ioption[i];
                    j++;
                }
                i++;
            }
            tmpstr2[j] = '\0';

            for (opttoken = strtok_r(tmpstr2, ",", &optnexttoken); opttoken != NULL;
                    opttoken = strtok_r(NULL, ",", &optnexttoken))
            {
                if ( !strcmpi(qualif, opttoken )) {
                    qualifierIsValid = 1;
                    break;
                }
            }

            /* if set qualifier is invalid, return error */
            if ( qualifierIsValid == 0 ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
                rc = ISMRC_BadPropertyValue;
                goto VALIDATION_END;
            }
        }

        /* set data based on qualifier */
        if (*qualif == 'K' || *qualif == 'k')
            data *= 1024;
        else if (*qualif == 'M' || *qualif == 'm')
            data *= (1024 * 1024);
        else if (*qualif == 'G' || *qualif == 'g')
            data *= (1024 * 1024 * 1024);
        else if (*qualif) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
            rc = ISMRC_BadPropertyValue;
            goto VALIDATION_END;
        }
    }

    if (min && *min != '\0') {
        /* Validate data for min */
        int minval = atoi(min);
        if ( data < minval ) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
            rc = ISMRC_BadPropertyValue;
            goto VALIDATION_END;
        }
    }

    if (max && *max != '\0') {
        /* Validate data for max */
         int maxval = atoi(max);
        if ( data > maxval ) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
            rc = ISMRC_BadPropertyValue;
        }
    }
    
VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;

}

/*
 * Validate data type buffer size.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  min                    The minimum allowed number
 * @param  max                    The maximum allowed number
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_bufferSize( char *name, char *value, char *min, char *max) {
    int32_t rc = ISMRC_OK;

    //TRACE(9, "Entry %s: name: %s, value: %s, min: %s, max: %s, sizeQualifierOptions: %s\n",
    //    		__FUNCTION__, name?name:"null", value?value:"null",
    //    		min?min:"null", max?max:"null", sizeQualifierOptions?sizeQualifierOptions:"null");

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    char *tmpstr = value;
    if ( !tmpstr || *tmpstr == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "NULL");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    /* check if option includes any qualifier */
    /* check if option includes any qualifier */
    uint64_t data = 0;


    /* Skip leading white space */
    while (*tmpstr && isspace(*tmpstr)) tmpstr++;
    /* Grab a base 10 integer */
    if (*tmpstr < '0' || *tmpstr > '9') {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }
    while (*tmpstr >= '0' && *tmpstr <= '9') {
    	data = data * 10 + (*tmpstr - '0');
    	tmpstr++;
    }

    /* Skip intervening white space */
    while (*tmpstr && isspace(*tmpstr)) tmpstr++;

    /* Check for size qualifier */
    if (*tmpstr && (*tmpstr == 'K' || *tmpstr == 'M') && *(tmpstr+1) == 'B') {
    	if (*tmpstr == 'K') data *= 1024;
    	else if (*tmpstr == 'M') data *= 1024 * 1024;
    	tmpstr += 2;
        /* Skip trailing white space */
        while (*tmpstr && isspace(*tmpstr)) tmpstr++;
    }
    if (*tmpstr) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }


    if (min && *min != '\0') {
        /* Validate data for min */
        long minval = atol(min);
        if ( data < minval ) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
            rc = ISMRC_BadPropertyValue;
            goto VALIDATION_END;
        }
    }

    if (max && *max != '\0') {
        /* Validate data for max */
        long maxval = atol(max);
        if ( data > maxval ) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
            rc = ISMRC_BadPropertyValue;
        }
    }

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;

}

/*
 * Validate data type Enum.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  options                Comma-separated list of allowed enum options
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_enum( char *name, char *value, char *options, int type ) {
    int32_t rc = ISMRC_ArgNotValid;
    ism_common_list valueList;
    ism_common_list_init(&valueList, 0, NULL);

    //TRACE(9, "Entry %s: name: %s, value: %s, options: %s\n",
    //		__FUNCTION__, name?name:"null", value?value:"null", options?options:"null");

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    if ( !value || *value == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value ? "\"\"" :"NULL");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

	/* check for NULL options */
    if ( !options || *options == '\0' ) {
        ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", name, "options", "NULL");
        rc = ISMRC_BadOptionValue;
        goto VALIDATION_END;
    }

    int vlen = strlen(value);
    char *valueStr = (char *)alloca(vlen+1);
    memcpy(valueStr, value, vlen);
    valueStr[vlen] = 0;

    char *vtoken, *vnexttoken = NULL;
    char *opttoken, *optnexttoken = NULL;
    int found;
    int count = 0;
    for (vtoken = strtok_r(valueStr, ",", &vnexttoken);
    		vtoken != NULL;
    		vtoken = strtok_r(NULL, ",", &vnexttoken))
    {
    	/* reset found */
    	found = 0;

    	if (!vtoken || *vtoken == '\0') {
    		continue;
    	}
    	/* Check duplication in the value */
    	rc = ism_config_addValueToList(&valueList, vtoken, 0);
		if(rc != ISMRC_OK ){
			TRACE(3, "%s: The value: %s of %s is duplicated. it can only be used once.", __FUNCTION__, value, name);
	        rc = ISMRC_BadPropertyValue;
	        ism_common_setErrorData(rc, "%s%s", name, value);
	        goto VALIDATION_END;
		}

		/* check if the vtoken is part of the options */
		int len = strlen(options);
		char *ioption = (char *)alloca(len+1);
		memcpy(ioption, options, len);
		ioption[len] = 0;

		for (opttoken = strtok_r(ioption, ",", &optnexttoken);
			 opttoken != NULL;
			 opttoken = strtok_r(NULL, ",", &optnexttoken))
		{
			if (!opttoken || *opttoken == '\0') {
			    		continue;
			}

			if ( strcmp(vtoken, opttoken) == 0 ) {
					found = 1;
					count++;
					break;
			}
		}

		if (found == 0 ) {
	        //ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", vtoken, "The value can not be found from the list");
			TRACE(3, "%s: The value: %s of the configuration item: %s is not valid.\n", __FUNCTION__, value, name);
	        rc = ISMRC_BadPropertyValue;
	        ism_common_setErrorData(rc, "%s%s", name, value);
			goto VALIDATION_END;
		}
		if ( count > 1 && ISM_CONFIG_PROP_ENUM == type ) {
	        //ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", vtoken, "The value can not be found from the list");
			TRACE(3, "%s: The value: %s of the configuration item: %s is not valid.\n", __FUNCTION__, value, name);
	        rc = ISMRC_BadPropertyValue;
	        ism_common_setErrorData(rc, "%s%s", name, value);
			goto VALIDATION_END;
		}
    }


VALIDATION_END:
    ism_common_list_destroy(&valueList);
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);
    
    return rc;
}

/*
 * Validate data type boolean.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_boolean( char *name, char *value ) {
    int32_t rc = ISMRC_ArgNotValid;

    //TRACE(9, "Entry %s: name: %s, value: %s\n",
    //		__FUNCTION__, name?name:"null", value?value:"null" );
    		
    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    if ( !value || *value == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "NULL");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    if ( strcmpi(value, "true") == 0 || strcmpi(value, "false") == 0 ) {
    	rc = ISMRC_OK;
    } else {
    	rc = ISMRC_BadPropertyValue;
    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
    }

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 * Validate an configuration object name.
 *
 * @param  name                   Configuration object name
 * @param  value                  Object value to be validated
 * @param  maxlen                 Maximum length of the object name
 * @param  item                   Configuration object type
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 */
XAPI int32_t ism_config_validateDataType_name( char *name, char *value, char *maxlen, char *item ) {
    int32_t rc = ISMRC_OK;
    int count = 0;
    int checkFirstChar = 1;
    int isName = 1;
    int len;

    //TRACE(9, "Entry %s: name: %s, value: %s, maxlen: %s\n",
    //		__FUNCTION__, name?name:"null", value?value:"null", maxlen?maxlen:"null" );

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    if ( !value ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "NULL");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    /* check for empty value */
    if ( *value == '\0' ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "\"\"");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    len = strlen(value);

    count = ism_config_validate_UTF8(value, len, checkFirstChar, isName);
    if (count<1) {
        ism_common_setError(ISMRC_UnicodeNotValid);
        rc = ISMRC_UnicodeNotValid;
        goto VALIDATION_END;
    }

    if (value[len-1] == ' ') {
    	ism_common_setErrorData(ISMRC_ArgNotValid, "%s", value);
    	rc = ISMRC_ArgNotValid;
    	goto VALIDATION_END;
    }

    if (maxlen) {
		int mlen = atoi(maxlen);
		if (count > mlen) {
			TRACE(3, "%s: Name length check failed. len=%d maxlen=%s\n", __FUNCTION__, count, maxlen);
			rc = ISMRC_NameLimitExceed;
			ism_common_setErrorData(rc, "%s%s%s", item, name, value);

			goto VALIDATION_END;
		}
    }

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 * Validate ClientAddress:
 * Valid IPv4 or IPv6 address
 * Valid range - low-high
 */

static int cmp_in6addr(struct in6_addr * low, struct in6_addr * high) {
    int ndx;
    int cmp = 0;
    for (ndx = 0; ndx < 16; ndx++) {
        if (low->s6_addr[ndx] < high->s6_addr[ndx]) {
            cmp = -1;
            break;
        }
        if (low->s6_addr[ndx] > high->s6_addr[ndx]) {
            cmp = 1;
            break;
        }
    }
    return cmp;
}

/*
 * Validate a single IP address.
 *
 * @param  value                  An IP address to be validated.
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_IPAddress(char *value) {
    int ipv6 = 0;
    int rc;
    char str[INET_ADDRSTRLEN];
    char * p = value + strlen(value) - 1;
    /* remove leading space */
    while ( *value == ' ' )
        value++;
    if ( *value == 0 )
        return ISMRC_BadPropertyValue;

    /* remove trailing space */
    while ( *p == ' ' )
        p--;
    *(p+1) = 0;

    if ( strstr(value, ":"))
            ipv6 = 1;
    if(ipv6) {
        struct in6_addr ipaddr;
        if (*value == '[') {
            value++;
            p = value + strlen(value) - 1;
            if(*p != ']')
                return ISMRC_BadPropertyValue;
            *p = 0;
        }
        rc = inet_pton(AF_INET6, value, &ipaddr);
        if (rc != 1 )  {
            return ISMRC_BadPropertyValue;
        }
        inet_ntop(AF_INET6, &ipaddr, str, INET_ADDRSTRLEN);
    } else {
        struct in_addr ipaddr;
        rc = inet_pton(AF_INET, value, &ipaddr);
        if (rc != 1 )  {
            return ISMRC_BadPropertyValue;
        }
        inet_ntop(AF_INET, &ipaddr, str, INET_ADDRSTRLEN);
    }
    TRACE(9, "%s: The IPaddress %s is converted as: %s\n", __FUNCTION__, value, str);
    return ISMRC_OK;
}

/*
 * Validate an IP addresses.
 *
 * @param  name                   Object name
 * @param  value                  A comma separated list of IP address to be validated.
 *                                It also support IP address range. Such as "9.3.179.85-9.3.179.87"
 * @param  mode                   1, validate IP address as an host, allow "*" and "all".
 *                                0, validate IP address as IP.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_IPAddresses( char *name, char *value, int mode ) {
    int32_t rc = ISMRC_OK;
    int len = 0;

    //TRACE(9, "Entry %s: name: %s, value: %s, mode: %d\n",
    //		__FUNCTION__, name?name:"null", value?value:"null", mode );

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    if ( !value  ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "NULL");
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    if ( mode != 0 && mode != 1 ) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%d", "mode", mode);
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    if ( (strcmp(value, "*") == 0 || strcasecmp(value, "all") == 0) )  {
    	if ( mode == 0 )
    	    goto VALIDATION_END;
    	else {
    		rc = ISMRC_BadPropertyValue;
    		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
        	goto VALIDATION_END;
    	}
    }

    len = strlen(value);
    /* allow to set it as empty*/
    if (len== 0)
    	goto VALIDATION_END;


    char *tmpstr0 = (char *)alloca(len+1);
    memcpy(tmpstr0, value, len);
    tmpstr0[len] = 0;

    if ( !strcmp(tmpstr0, "0.0.0.0") || !strcmp(tmpstr0, "[::]") )
    {
    	goto VALIDATION_END;
    }


    char *p, *token, *nexttoken = NULL;
    for (token = strtok_r(tmpstr0, ",", &nexttoken); token != NULL;
            token = strtok_r(NULL, ",", &nexttoken))
    {
		int isPair = 0;
		int type = 1;

        /* remove leading space */
        while ( *token == ' ' )
            token++;
        if ( *token == 0 )
            continue;

        /* remove trailing space */
        p = token + strlen(token) - 1;
        while ( p > token && *p == ' ' )
            p--;
        *(p+1) = 0;

        if ( strstr(token, "-"))
				isPair = 1;

		if ( strstr(token, ":"))
				type = 2;

		char *addstr = token;
		char *tmptok;
		char *IPlow = strtok_r(addstr, "-", &tmptok);
		char *IPhigh = strtok_r(NULL, "-", &tmptok);

		if ( type == 1 ) {
			struct sockaddr_in ipaddr;
			int rc4 = 0;
			rc4 = inet_pton(AF_INET, IPlow, &ipaddr.sin_addr);
			if (rc4 != 1 )  {
			    rc = ISMRC_BadPropertyValue;
			    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value?value:"null");
				goto VALIDATION_END;
			}

			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &ipaddr.sin_addr, str, INET_ADDRSTRLEN);
			TRACE(9, "%s: The IPaddress %s is converted as: %s\n", __FUNCTION__, IPlow, str);

			if (isPair ) {
				rc4 = inet_pton(AF_INET, IPhigh, &ipaddr.sin_addr);
				if (rc4 != 1 )  {
	                rc = ISMRC_BadPropertyValue;
	                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value?value:"null");
					goto VALIDATION_END;
				}
				unsigned long low = ntohl(inet_addr(IPlow));
				unsigned long high = ntohl(inet_addr(IPhigh));
				if ( low > high ) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value?value:"null");
					goto VALIDATION_END;
				}
			 }
		} else {
			int rc1=0, rc2=0;
			char buf[64];
			int i = 0;
			char *tmpstr;
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
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value?value:"null");
				goto VALIDATION_END;
			}

			if (isPair ) {
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
	                rc = ISMRC_BadPropertyValue;
	                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value?value:"null");
					goto VALIDATION_END;
				}

				if ( cmp_in6addr(&low6, &high6) > 0 ) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value?value:"null");
					goto VALIDATION_END;
				}
			}
		}
    }  /* End of for loop */

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}


/*
 * Validate data type string
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  type                   Type of the string
 *                                0 - Description
 *                                1 - Topic
 * @param  maxlen                 Maximum length of the string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Empty string is a valid string for Description.
 */
XAPI int32_t ism_config_validateDataType_string( char *name, char *value, int type, char *maxlen, char *item ) {
    int32_t rc = ISMRC_OK;

    //TRACE(9, "Entry %s: name: %s, value: %s, type: %d, maxlen: %s\n",
    //		__FUNCTION__, name?name:"null", value?value:"null", type, maxlen?maxlen:"null" );
    		
    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    if ( !value || *value == '\0' ) {
    	goto VALIDATION_END;
    }

    int len = strlen(value);
    /* check for leading and trailing space if not description*/
    if ( len && (type != 0 && (*value == ' ' || value[len-1] == ' ' ))) {
    	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    /* Check valid UTF8 */
    int count = ism_common_validUTF8(value, len);
    if (count < 1) {
        TRACE(3, "%s: Invalid UTF8 string\n", __FUNCTION__);
        rc = ISMRC_ObjectNotValid;
        goto VALIDATION_END;
    }    

    if (maxlen) {
		int mlen = atoi(maxlen);
		if ( mlen < 0 || count > mlen ) {
			TRACE(3, "%s: String length check failed. len=%d maxlen=%s\n", __FUNCTION__, count, maxlen);
			if ( item ) {
			    rc = ISMRC_LenthLimitExceed;
			    ism_common_setErrorData(rc, "%s%s%s", item?item:"", name, value);
			} else {
			    rc = ISMRC_LenthLimitSingleton;
			    ism_common_setErrorData(rc, "%s%s", name, value);
			}
		}
    }
       
VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}    

/*
 * Validate data type regular expression
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  maxlen                 Maximum length of the string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Empty string is a valid string for Description.
 */
XAPI int32_t ism_config_validateDataType_regex( char *name, char *value, char *maxlen, char *item ) {
    int32_t rc = ISMRC_OK;

    //TRACE(9, "Entry %s: name: %s, value: %s, type: %d, maxlen: %s\n",
    //		__FUNCTION__, name?name:"null", value?value:"null", type, maxlen?maxlen:"null" );

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    if ( !value || *value == '\0' ) {
        goto VALIDATION_END;
    }

    int len = strlen(value);

    /* Check valid UTF8 */
    int count = ism_common_validUTF8(value, len);
    if (count < 1) {
        TRACE(3, "%s: Invalid UTF8 string\n", __FUNCTION__);
        rc = ISMRC_ObjectNotValid;
        goto VALIDATION_END;
    }

    if (maxlen) {
        int mlen = atoi(maxlen);
        if ( mlen < 0 && count > mlen ) {
            TRACE(3, "%s: String length check failed. len=%d maxlen=%s\n", __FUNCTION__, count, maxlen);
            if ( item ) {
                rc = ISMRC_LenthLimitExceed;
                ism_common_setErrorData(rc, "%s%s%s", item?item:"", name, value);
            } else {
                rc = ISMRC_LenthLimitSingleton;
                ism_common_setErrorData(rc, "%s%s", name, value);
            }
            goto VALIDATION_END;
        }
    }

    /* Check for a valid regular expression */
    ism_regex_t regex;

    if (ism_regex_compile(&regex, value)) {
        TRACE(3, "%s: Error compiling regular expression \"%s\"\n", __FUNCTION__, value);
        rc = ISMRC_RegularExpression;
        ism_common_setErrorData(rc, "%s", value);
        goto VALIDATION_END;
    } else {
        ism_regex_free(regex);
    }

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Validate data type regular expression with at least one subexpression
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  maxlen                 Maximum length of the string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Empty string is a valid string for Description.
 */
XAPI int32_t ism_config_validateDataType_regex_subexpr( char *name, char *value, char *maxlen, char *item ) {
    int32_t rc = ISMRC_OK;

    TRACE(1, "Entry %s: name: %s, value: %s, maxlen: %s, item: %s\n",
          __FUNCTION__, name?name:"null", value?value:"null", maxlen?maxlen:"null", item?item:"null" );

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    if ( !value || *value == '\0' ) {
        goto VALIDATION_END;
    }

    int len = strlen(value);

    /* Check valid UTF8 */
    int count = ism_common_validUTF8(value, len);
    if (count < 1) {
        TRACE(3, "%s: Invalid UTF8 string\n", __FUNCTION__);
        rc = ISMRC_ObjectNotValid;
        goto VALIDATION_END;
    }

    if (maxlen) {
        int mlen = atoi(maxlen);
        if ( mlen < 0 && count > mlen ) {
            TRACE(3, "%s: String length check failed. len=%d maxlen=%s\n", __FUNCTION__, count, maxlen);
            if ( item ) {
                rc = ISMRC_LenthLimitExceed;
                ism_common_setErrorData(rc, "%s%s%s", item?item:"", name, value);
            } else {
                rc = ISMRC_LenthLimitSingleton;
                ism_common_setErrorData(rc, "%s%s", name, value);
            }
            goto VALIDATION_END;
        }
    }

    /* Check for a valid regular expression with at least one subexpression */
    ism_regex_t regex;
    int subexprcnt;

    if (ism_regex_compile_subexpr(&regex, &subexprcnt, value)) {
        TRACE(3, "%s: Error compiling regular expression \"%s\"\n", __FUNCTION__, value);
        rc = ISMRC_RegularExpression;
        ism_common_setErrorData(rc, "%s", value);
        goto VALIDATION_END;
    } else {
        ism_regex_free(regex);

        if (subexprcnt == 0) {
            TRACE(3, "%s: Error no subexpressions in regular expression \"%s\"\n", __FUNCTION__, value);
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", name, value);
            goto VALIDATION_END;
        }
    }

VALIDATION_END:
    TRACE(1, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Validate data type Selector
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  maxlen                 Maximum length of the string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Empty string is valid
 */
XAPI int32_t ism_config_validateDataType_Selector( char *name, char *value, char *maxlen, char *item ) {
    int32_t rc = ISMRC_OK;

    //TRACE(9, "Entry %s: name: %s, value: %s, type: %d, maxlen: %s\n",
    //      __FUNCTION__, name?name:"null", value?value:"null", type, maxlen?maxlen:"null" );

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    if ( !value || *value == '\0' ) {
        goto VALIDATION_END;
    }

    int len = strlen(value);

    /* Check valid UTF8 */
    int count = ism_common_validUTF8(value, len);
    if (count < 1) {
        TRACE(3, "%s: Invalid UTF8 string\n", __FUNCTION__);
        rc = ISMRC_ObjectNotValid;
        goto VALIDATION_END;
    }

    if (maxlen) {
        int mlen = atoi(maxlen);
        if ( mlen > 0 && count > mlen ) {
            TRACE(3, "%s: String length check failed. len=%d maxlen=%s\n", __FUNCTION__, count, maxlen);
            if ( item ) {
                rc = ISMRC_LenthLimitExceed;
                ism_common_setErrorData(rc, "%s%s%s", item?item:"", name, value);
            } else {
                rc = ISMRC_LenthLimitSingleton;
                ism_common_setErrorData(rc, "%s%s", name, value);
            }
            goto VALIDATION_END;
        }
    }

    /* Check for a valid selector by compiling it */
    ismRule_t *pSelectionRule = NULL;
    int32_t SelectionRuleLen = 0;

    rc = ism_common_compileSelectRuleOpt(&pSelectionRule,
                                         (int *)&SelectionRuleLen,
                                         value,
                                         SELOPT_Internal);

     if (rc != OK) {
        TRACE(3, "%s: Error %d compiling selector \"%s\"\n", __FUNCTION__, rc, value);
        ism_common_setErrorData(rc, value);
        goto VALIDATION_END;
    } else if (pSelectionRule) {
        ism_common_freeSelectRule(pSelectionRule);
    }

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Validate data type URL
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  maxlen                 Maximum length of the string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * URL allows different protocols based upon the property name.
 */
XAPI int32_t ism_config_validateDataType_URL( char *name, char *value, char * maxlen, char *options, char *item ) {
    int32_t rc = ISMRC_OK;
    char *tmpstr = NULL;

    //TRACE(9, "Entry %s: name: %s, value: %s, maxlen: %s, options: %s\n",
    //		__FUNCTION__, name?name:"null", value?value:"null", maxlen?maxlen:"null", options?options:"null" );

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    /* check for NULL value */
    if ( !value || *value == '\0' ) {
        goto VALIDATION_END;
    }

    int len = strlen(value);
    /* check for leading and trailing space*/
    if ( *value == ' ' || value[len] == ' ') {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
        rc = ISMRC_BadPropertyValue;
        goto VALIDATION_END;
    }

    if (maxlen) {
		int mlen = atoi(maxlen);
		if(len > mlen){
			if ( item ) {
			    rc = ISMRC_LenthLimitExceed;
			    ism_common_setErrorData(rc, "%s%s%s", item?item:"", name, value);
			} else {
			    rc = ISMRC_LenthLimitSingleton;
			    ism_common_setErrorData(rc, "%s%s", name, value);
			}
			goto VALIDATION_END;
		}
    }

    if (!options) {
    	TRACE(5, "%s: no URL options specified for name: %s, value: %s\n", __FUNCTION__, name, value);
    	goto VALIDATION_END;
    }

    tmpstr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),options);

    int valueInList = 0;
    char *token, *nexttoken = NULL;


    for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
    {
    	if ( !token || (token && *token == '\0')) {
    	    continue;
    	}else {
			if (!strncasecmp(value, token, strlen(token))) {
				valueInList = 1;
				break;

			}
    	}
    }

    if ( valueInList == 0) {
		 ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
		 rc = ISMRC_BadPropertyValue;
		 goto VALIDATION_END;
	}

    /* check for ascii and control characters */
    char *pstr = value;
    while (*pstr) {
        if( !isascii((int)*pstr) || *pstr < 32 ) {
       	    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
       	    rc = ISMRC_BadPropertyValue;
       	    goto VALIDATION_END;
        }
        pstr++;
    }


VALIDATION_END:
    if (tmpstr)  ism_common_free(ism_memory_admin_misc,tmpstr);
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

#define MAX_SSHHOST 100

/*
 * Validate that given IP address is valid IP address for this system.
 */
int ism_config_validate_IPAddress(char *ip, int checkLocal) {
	int i;
	int count = 0;
	int rc = ISMRC_ArgNotValid;
	int af = AF_INET;
	uint32_t inet4, rinet4;
	struct in6_addr inet6, rinet6;
	int ptonrc;
	void * result;
	char **ipr = NULL;

    TRACE(9, "Entry %s: ip: %s\n", __FUNCTION__, ip );
	if (strstr(ip, ":")) {
		af = AF_INET6;
		result = &rinet6;
		ptonrc = inet_pton(af, ip, &inet6);
	} else {
		result = &rinet4;
		ptonrc = inet_pton(af, ip, &inet4);
	}
	if (1 != ptonrc) {
		return rc;
	}

	if ( checkLocal == 0 ) {
		rc = ISMRC_OK;
	} else {
		ipr = (char **)alloca(MAX_SSHHOST*sizeof(char*));

		for (i = 0; i < MAX_SSHHOST; i++ ) {
			ipr[i] = NULL;
		}

		// Following routine is in adminHA.c.... Do we need to move it?
		if ( ism_admin_getIfAddresses(ipr, &count, 1) != 0 && count > 0) {
			for (i=0; i<count; i++) {
				int offset = 0;
				// getIfAddresses returns IP6 addresses enclosed in "[...]"
				// inet_pton does not like that. Need to remove.
				TRACE(8, "%s: Checking against ip: %s\n", __FUNCTION__, ipr[i] );
				if ('[' == ipr[i][0]) {
					ipr[i][strlen(ipr[i])-1] = 0;
					offset = 1;
				}
				ptonrc = inet_pton(af, ipr[i]+offset, result);
				if (1 == ptonrc) {
					if (AF_INET == af) {
						if (inet4 == rinet4) {
							// Found a match
							rc = ISMRC_OK;
							break;
						}
					} else {
						if (!memcmp(&inet6, &rinet6, sizeof(inet6))) {
							// Found a match
							rc = ISMRC_OK;
							break;
						}
					}
				} else {
					rc = ISMRC_IPNotValid;
				}
			}
			for (i = 0; i<count; i++) {
				if (ipr[i]) ism_common_free(ism_memory_admin_misc,ipr[i]);
			}
		}
	}

    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);
 	return rc;

}


/*
 * Generic validation of a Singleton item.
 * @param name                    The name of the item
 * @param value                   The value to validate
 * @param action                  The flag indicates the configuration string is 0 - create, 1 - update, 2 - delete.
 * @param newValue                If a default value is substituted, pointer will be placed here
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 */
XAPI int ism_config_validate_singletonItem( char *name, char *value, int action, char **newValue)
{
	int rc = ISMRC_OK;
	char *val = NULL;

	ism_json_parse_t *json = ism_config_getSchema(ISM_CONFIG_SCHEMA);
	if (NULL == json) {
		rc = ISMRC_NotImplemented;
		ism_common_setError(rc);
        goto VALIDATE_ITEM_END;
	}
	/* Delete not allowed for singleton objects */
	if (action == 2) {
		rc = ISMRC_SingltonDeleteError;
		ism_common_setError(rc);
        goto VALIDATE_ITEM_END;
	}

    /* Find position of the object in JSON object */
    int pos = ism_json_get(json, 0, name);
    if ( pos == -1 )
    {
        ism_common_setErrorData(ISMRC_BadPropertyName, "%s", name);
        rc = ISMRC_BadPropertyName;
        goto VALIDATE_ITEM_END;
    }
    char *settable = ism_config_validate_getAttr("Settable", json, pos);
    if (settable && (*settable=='n' || *settable=='N')) {
    	rc = 6208;
        ism_common_setErrorData(rc, "%s", name);
        goto VALIDATE_ITEM_END;
    }
    char *type = ism_config_validate_getAttr("Type", json, pos);
    if (type) {
    	char *defval = ism_config_validate_getAttr("Default", json, pos);
    	char *maxlen = ism_config_validate_getAttr("MaxLength", json, pos);
        if (!strcmpi(type, "Number")) {
            char *minval = ism_config_validate_getAttr("Minimum", json, pos);
            char *maxval = ism_config_validate_getAttr("Maximum", json, pos);
 		    if ((!value || *value == '\0') &&  defval) {
			    /* allow create to use default value if not specified */
				val = defval;
			} else {
 		        rc = ism_config_validateDataType_number(name, value, minval, maxval, NULL);
 		    }
        } else if (!strcmpi(type, "BufferSize")) {
        	char *minval = ism_config_validate_getAttr("Minimum", json, pos);
        	char *maxval = ism_config_validate_getAttr("Maximum", json, pos);
        	if (!value &&  defval) {
        		/* allow create to use default value if not specified */
        		val = defval;
 			} else {
 				rc = ism_config_validateDataType_bufferSize(name, value, minval, maxval);
 	 		}
        } else if (!strcmpi(type, "Enum") || !strcmpi(type, "List")) {
            char *options = ism_config_validate_getAttr("Options", json, pos);
		    if (!value &&  defval) {
			    /* allow create to use default value if not specified */
				val = defval;
			} else {
 		        rc = ism_config_validateDataType_enum(name, value, options,
 		        		!strcmpi(type, "List") ? ISM_CONFIG_PROP_LIST : ISM_CONFIG_PROP_ENUM);
			}
        } else if (!strcmpi(type, "String") || !strcmpi(type, "StringBig")) {
			if (!value) {
				if (defval) {
					/* allow to use default value if not specified */
					val = defval;
				} else {
					/* If no default is given, then an empty value is not allowed */
					ism_common_setError(ISMRC_NullArgument);
					rc = ISMRC_NullArgument;
				}
			} else {
				rc = ism_config_validateDataType_string(name, value, 1, maxlen, NULL);
			}
        } else if (!strcmpi(type, "Boolean")) {
			if (!value &&  defval) {
			    /* allow create to use default value if not specified */
			    val = defval;
 		    } else {
 		        rc = ism_config_validateDataType_boolean(name, value);
 		    }
/*
  *  Not URL or IPAdrees supported in singletons
  *
 	 	 	} else if (!strcmpi(type, "IPAddress")) {
            int mode = 1;
            if (defval && ( !strcmpi(defval, "all") || ( *defval == '*'))) {
                mode = 0;
            }
			if ((!value || *value == '\0') &&  defval) {
			    // allow create to use default value if not specified
			    val = defval;
			    value = val;
			}
		    rc = ism_config_validateDataType_IPAddresses(name, value, mode);
        } else if (!strcmpi(type, "URL")) {
            char *options = ism_config_validate_getAttr("Options", json, pos);
 		    rc = ism_config_validateDataType_URL( name, value, maxlen, options );
 		    if (rc == ISMRC_BadPropertyValue && (value == NULL || *value == '\0') && defval) {
 		    	rc = ISMRC_OK;
 		    	val = defval;
 		    }
*/
        } else if (!strcmpi(type, "Regex")) {
            rc = ism_config_validateDataType_regex(name, value, maxlen, NULL);
        } else if (!strcmpi(type, "RegexSub")) {
            rc = ism_config_validateDataType_regex_subexpr(name, value, maxlen, NULL);
        }else if (!strcmpi(type, "Selector")) {
            rc = ism_config_validateDataType_Selector(name, value, maxlen, NULL);
        } else {
            TRACE(3, "%s: Unsupported property type %s for singleton object.", __FUNCTION__, type);
            rc = ISMRC_BadPropertyName;
            ism_common_setErrorData(ISMRC_BadPropertyName, "%s", type);
        }

    }


VALIDATE_ITEM_END:
	if (newValue && val) {
		*newValue = val;
	}
	return rc;
}

/*
 * Generic validation for each passing configuration item.
 * @param  list                   A complete list of defined items of a composite object defined in the JSON schema
 * @param  name                   A configuration item to be validated
 * @param  value                  The value of the configuration item specified by name
 * @param  dupName                The value of "Name" item to be returned.
 * @param  isGet                  The flag to be assigned if "Action" item is "get".
 * @param  action                 The flag indicates the configuraton string is 0 - create, 1 - update, 2 - delete.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * NOTE: PLEASE do not add any special cases inside this function.
 * This validation should be ONLY used for generic validating the item against its type,
 * and range defined in the schema. For example, "ConnectionName" is defined as String, this
 * function will only verify the string. To validate each of the
 * IPAddess:port in the string, have special code piece inside valid_QueueManagerConnection API.
 */
XAPI int ism_config_validate_checkItemDataType( ism_config_itemValidator_t *list, char *name, char *value, char **dupName, int *isGet, int action, ism_prop_t *props)
{
	int i = 0;
	int rc = ISMRC_OK;

    if (!name || !list) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    if ( list->total < 1) {
    	TRACE(5, "%s: validation list is empty.\n", __FUNCTION__);
    	goto VAL_CHECK_END;
    }

    /* ALL items in the json configuration string but not in the schema should check first*/
	/* Validate Action */
	if ( !strcmpi(name, "Action")) {
		if (value && !strcasecmp(value, "Get")) {
			*isGet = 1;
		    goto VAL_CHECK_END;
		} else if (value && !strcasecmp(value, "Set")) {
			goto VAL_CHECK_END;
		} else if (value && (!strcasecmp(value, "Delete"))) {
			goto VAL_CHECK_END;
		} else {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(rc, "%s%s", name, value?value:"null");
			goto VAL_CHECK_END;
		}
    }

	/* No check required for Version, User, Locale, Component, and Item */
    if ( !strcmpi(name, "Version") || !strcmpi(name, "User") || !strcmpi(name, "Locale") || !strcmpi(name, "Component") || !strcmpi(name, "Item")) {
		goto VAL_CHECK_END;
	}

	/* Item type */
    if ( !strcmpi(name, "Type")) {
    	if (value && (!strcmpi(value, "Composite") || !strcmpi(value, "Singleton")))
	        goto VAL_CHECK_END;
    	else {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(rc, "%s%s", name, value?value:"null");
			goto VAL_CHECK_END;
    	}
    }

    /* ALL items from the json string but not in the schema should NOT allow after this line*/
    /* check the item name specified on the schema*/
    int nameInList = 0;
    ism_field_t f;
    for (i = 0; i < list->total; i++) {
       if ( list->name[i] && !strcmp(list->name[i], name) ) {
    	   /* mark the found flag */
    	   nameInList = 1;
    	   list->assigned[i] = 1;
    	   int tempflag = list->tempflag[i];

    	   f.type = VT_Null;

           /* validate the value by type*/
    	   switch(list->type[i]) {
    	   case ISM_CONFIG_PROP_NUMBER:
    	   {
    		   char *val;
    		   if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
    			   /* allow update to remove optinal configure item by set it to empty or null */
				   val = value;
			   } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
				   /* allow create to use default value if not specified */
				   val = list->defv[i];
			   } else {
    		       rc = ism_config_validateDataType_number(name, value, list->min[i], list->max[i], NULL);
    		       val = value;
    		   }
    		   if (rc == ISMRC_OK) {
				   if ( *val != '\0' && isInteger(val) == 1 ) {
					   f.type = VT_Integer;
					   f.val.i = atoi(val);
				   } else {
					   f.type = VT_String;
					   f.val.s = val;
				   }
    		   }
    	       break;
    	   }
    	   case ISM_CONFIG_PROP_ENUM:
    	   case ISM_CONFIG_PROP_LIST:
    	   {
               char *val;
			   if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
				   /* allow update to remove optinal configure item by set it to empty or null */
				   val = value;
			   } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
				   /* allow create to use default value if not specified */
				   val = list->defv[i];
			   } else {
    		       rc = ism_config_validateDataType_enum(name, value, list->options[i], list->type[i]);
    		       val = value;
			   }
    		   if (rc == ISMRC_OK) {
    			   f.type = VT_String;
    			   f.val.s = val;
    		   }
    	       break;
    	   }
    	   case ISM_CONFIG_PROP_BOOLEAN:
    	   {
    		   char *val;
			   if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
				   /* allow update to remove optinal configure item by set it to empty or null */
				   val = value;
			   } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
				   /* allow create to use default value if not specified */
				   val = list->defv[i];
    		   } else {
    		       rc = ism_config_validateDataType_boolean(name, value);
    		       val = value;
    		   }
    		   if (rc == ISMRC_OK )  {
    			   if (strcmp(name, "Delete") && strcmp(name, "Update")){
				       f.type = VT_String;
				       f.val.s = val;
    		       }
    		   }
    	       break;
    	   }
    	   case ISM_CONFIG_PROP_NAME:
    	   {
    		   rc = ism_config_validateDataType_name(name, value, list->max[i], list->item);
    		   if (rc == ISMRC_OK) {
    			   if (dupName) {
    				   *dupName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
    			   }
    			   f.type = VT_String;
    			   f.val.s = value;
    		   }
    	       break;
    	   }
    	   case ISM_CONFIG_PROP_STRING:
    	   {
    		   char *val;
			   if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
				   /* allow update to remove optinal configure item by set it to empty or null */
				   val = value;
			   } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
				   /* allow create to use default value if not specified */
				   val = list->defv[i];
			   } else {
			       int namelen = strlen(name);
				   if (!strcmpi(name, "Description") || (namelen > 8 && !strcmp(name+namelen-8, "Policies"))) {
					   rc = ism_config_validateDataType_string(name, value, 0, list->max[i], list->item);
				   } else {
					   rc = ism_config_validateDataType_string(name, value, 1, list->max[i], list->item);
				   }
				   val = value;
			   }

    		   if (rc == ISMRC_OK ) {
    			   //if ( value && *value != '\0' ) {
    			       f.type = VT_String;
    			       f.val.s = val;
                       if (list->minoneopt[i] && val && *val) list->minonevalid[i] = 1;
   			   //}
    		   }
    		   break;
    	   }
    	   case ISM_CONFIG_PROP_IPADDRESS:
    	   {
    	           int mode = 1;
    	           if ( tempflag == 1 ) mode = 0;
    		   rc = ism_config_validateDataType_IPAddresses(name, value, mode);
    		   if (rc == ISMRC_OK) {
    			   if ( value && *value != '\0' ) {
					   f.type = VT_String;
					   f.val.s = value;
				   } else if (list->defv[i]) {
					   f.type = VT_String;
					   f.val.s = value;
				   }
    		   }
    	       break;
    	   }
    	   case ISM_CONFIG_PROP_URL:
    	   {
    		   rc = ism_config_validateDataType_URL( name, value, list->max[i], list->options[i], list->item );
    		   if (rc == ISMRC_OK) {
    			   f.type = VT_String;
    			   f.val.s = value;
    		   }
    	       break;
    	   }
    	   case ISM_CONFIG_PROP_REGEX:
    	   {
    		   rc = ism_config_validateDataType_regex(name, value, list->max[i], list->item);
    		   if (rc == ISMRC_OK) {
    			   f.type = VT_String;
    			   f.val.s = value;
    		   }
    	       break;
    	   }

    	   case ISM_CONFIG_PROP_REGEX_SUBEXP:
    	   {
    	       rc = ism_config_validateDataType_regex_subexpr(name, value, list->max[i], list->item);
    	       if (rc == ISMRC_OK) {
    	           f.type = VT_String;
    	           f.val.s = value;
    	       }
    	       break;
    	   }

    	   case ISM_CONFIG_PROP_SELECTOR:
    	   {
    	       rc = ism_config_validateDataType_Selector(name, value, list->max[i], list->item);
    	       if (rc == ISMRC_OK) {
    	           f.type = VT_String;
    	           f.val.s = value;
    	       }
    	       break;
    	   }

    	   default:
    	   {
    		   /* Invalid item is found in the json object */
    		   //rc = ISMRC_BadPropertyName;
    		   //ism_common_setErrorData(ISMRC_BadPropertyName, "%s", name);
    		   TRACE(3, "%s: An unsupported property type: %d has been used for property: %s. The property is ignored.\n", __FUNCTION__, list->type[i], name?name:"null" );
    	       break;
    	   }
    	   } /* End of switch */

    	   if (rc == ISMRC_OK) {
    	       ism_common_setProperty(props, name, &f);
    	   }
       } /* End of for loop */
    }

    // ISMRC_BadAdminPropName error code is used by RESTAPI monitor code to
    // determine if a query parameter should be ignored.
    if (nameInList == 0) {
 	   rc = ISMRC_BadAdminPropName;
       ism_common_setErrorData(ISMRC_BadAdminPropName, "%s%s%s", list->item, "UNKNOWN", name);
 	   ism_common_setErrorData(rc, "%s", name);
    }

VAL_CHECK_END:
    if (rc != ISMRC_OK) {
    	TRACE(5, "%s: Validation failed: name: %s, value: %s, isGet: %d, action: %d\n",
			__FUNCTION__, name?name:"null", value?value:"null", *isGet, action );
    }
    //TRACE(9, "Exit %s: dupName: %s, isGet: %d, isDeleted: %d, isUpdated: %d, rc: %d\n",
    //		__FUNCTION__, *dupName?*dupName:"null", *isGet, *isDeleted, *isUpdated, rc);

    return rc;
}

/*
 * Generic configuration item using schema.
 * @param  list      A complete list of defined items of a composite object defined in the JSON schema
 * @param  name      A configuration item to be validated
 * @param  value     The value of the configuration item specified by name
 * @param  action    Set 0 for create, 1 for update
 * @param  props     Set items in props after validating the item
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * NOTE: PLEASE do not add any special cases inside this function.
 * This validation should be ONLY used for generic validating the item against its type,
 * and range defined in the schema. For example, "ConnectionName" is defined as String, this
 * function will only verify the string. To validate each of the
 * IPAddess:port in the string, have special code piece inside valid_QueueManagerConnection API.
 */
XAPI int ism_config_validateItemData( ism_config_itemValidator_t *list, char *name, char *value, int action, ism_prop_t *props )
{
    int i = 0;
    int rc = ISMRC_OK;

    if (!name || !list) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    if ( list->total < 1) {
        TRACE(5, "%s: validation list is empty.\n", __FUNCTION__);
        goto VAL_CHECK_END;
    }

    /* No check required for Type, Action, Version, User, Locale, Component, and Item */
    if ( !strcmpi(name, "Type") || !strcmpi(name, "Action") || !strcmpi(name, "Version") || !strcmpi(name, "User") || !strcmpi(name, "Locale") || !strcmpi(name, "Component") || !strcmpi(name, "Item")) {
        goto VAL_CHECK_END;
    }

    /* ALL items from the json string but not in the schema should NOT allow after this line*/
    /* check the item name specified on the schema*/
    int nameInList = 0;
    ism_field_t f;
    for (i = 0; i < list->total; i++) {
       if ( list->name[i] && !strcmp(list->name[i], name) ) {
           /* mark the found flag */
           nameInList = 1;
           list->assigned[i] = 1;
           int tempflag = list->tempflag[i];

           f.type = VT_Null;

           /* validate the value by type*/
           switch(list->type[i]) {
           case ISM_CONFIG_PROP_NUMBER:
           {
               char *val;
               if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
                   /* allow update to remove optional configure item by set it to empty or null */
                   val = value;
               } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
                   /* allow create to use default value if not specified */
                   val = list->defv[i];
               } else {
                   rc = ism_config_validateDataType_number(name, value, list->min[i], list->max[i], NULL);
                   val = value;
               }
               if (rc == ISMRC_OK) {
                   if ( *val != '\0' && isInteger(val) == 1 ) {
                       f.type = VT_Integer;
                       f.val.i = atoi(val);
                   } else {
                       f.type = VT_String;
                       f.val.s = val;
                   }
               }
               break;
           }
           case ISM_CONFIG_PROP_BUFFERSIZE:
           {
               char *val;
               if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
                   /* allow update to remove optional configure item by set it to empty or null */
                   val = value;
               } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
                   /* allow create to use default value if not specified */
                   val = list->defv[i];
               } else {
                   rc = ism_config_validateDataType_bufferSize(name, value, list->min[i], list->max[i]);
                   val = value;
               }
               if (rc == ISMRC_OK) {
                   if ( *val != '\0' && isInteger(val) == 1 ) {
                       f.type = VT_Long;
                       f.val.i = atol(val);
                   } else {
                       f.type = VT_String;
                       f.val.s = val;
                   }
               }
               break;
           }
           case ISM_CONFIG_PROP_ENUM:
           case ISM_CONFIG_PROP_LIST:
           {
               char *val;
               if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
                   /* allow update to remove optional configure item by set it to empty or null */
                   val = value;
               } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
                   /* allow create to use default value if not specified */
                   val = list->defv[i];
               } else {
                   rc = ism_config_validateDataType_enum(name, value, list->options[i], list->type[i]);
                   val = value;
               }
               if (rc == ISMRC_OK) {
                   f.type = VT_String;
                   f.val.s = val;
               }
               break;
           }
           case ISM_CONFIG_PROP_BOOLEAN:
           {
               char *val;
               if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
                   /* allow update to remove optional configure item by set it to empty or null */
                   val = value;
               } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
                   /* allow create to use default value if not specified */
                   val = list->defv[i];
               } else {
                   rc = ism_config_validateDataType_boolean(name, value);
                   val = value;
               }
               if (rc == ISMRC_OK )  {
                   if (strcmp(name, "Delete") && strcmp(name, "Update")){
                       f.type = VT_String;
                       f.val.s = val;
                   }
               }
               break;
           }
           case ISM_CONFIG_PROP_NAME:
           {
               rc = ism_config_validateDataType_name(name, value, list->max[i], list->item);
               if (rc == ISMRC_OK) {
                   f.type = VT_String;
                   f.val.s = value;
               }
               break;
           }
           case ISM_CONFIG_PROP_STRING:
           {
               char *val;
               if ( (!value || *value == '\0') && action == 1 && list->reqd[i] == 0 ) {
                   /* allow update to remove optional configure item by set it to empty or null */
                   val = value;
               } else if ((!value || *value == '\0') &&  action == 0 && list->defv[i]) {
                   /* allow create to use default value if not specified */
                   val = list->defv[i];
               } else {
                   int namelen = strlen(name);
                   if (!strcmpi(name, "Description") || (namelen > 8 && !strcmp(name+namelen-8, "Policies"))) {
                       rc = ism_config_validateDataType_string(name, value, 0, list->max[i], list->item);
                   } else {
                       rc = ism_config_validateDataType_string(name, value, 1, list->max[i], list->item);
                   }
                   val = value;
               }

               if (rc == ISMRC_OK ) {
                   f.type = VT_String;
                   f.val.s = val;
                   if (list->minoneopt[i] && val && *val) list->minonevalid[i] = 1;
               }
               break;
           }
           case ISM_CONFIG_PROP_IPADDRESS:
           {
                   int mode = 1;
                   if ( tempflag == 1 ) mode = 0;
               rc = ism_config_validateDataType_IPAddresses(name, value, mode);
               if (rc == ISMRC_OK) {
                   if ( value && *value != '\0' ) {
                       f.type = VT_String;
                       f.val.s = value;
                   } else if (list->defv[i]) {
                       f.type = VT_String;
                       f.val.s = value;
                   }
               }
               break;
           }
           case ISM_CONFIG_PROP_URL:
           {
               rc = ism_config_validateDataType_URL( name, value, list->max[i], list->options[i], list->item );
               if (rc == ISMRC_OK) {
                   f.type = VT_String;
                   f.val.s = value;
               }
               break;
           }
           case ISM_CONFIG_PROP_REGEX:
           {
               rc = ism_config_validateDataType_regex(name, value, list->max[i], list->item);
               if (rc == ISMRC_OK) {
                   f.type = VT_String;
                   f.val.s = value;
               }
               break;
           }

           case ISM_CONFIG_PROP_REGEX_SUBEXP:
           {
               rc = ism_config_validateDataType_regex_subexpr(name, value, list->max[i], list->item);
               if (rc == ISMRC_OK) {
                   f.type = VT_String;
                   f.val.s = value;
               }
               break;
           }


           case ISM_CONFIG_PROP_SELECTOR:
           {
               rc = ism_config_validateDataType_Selector(name, value, list->max[i], list->item);
               if (rc == ISMRC_OK) {
                   f.type = VT_String;
                   f.val.s = value;
               }
               break;
           }

           default:
           {
               /* Invalid item is found in the json object */
        	   TRACE(3, "%s: Schema JSON contains bad type: %d for property: %s in object: %s"
        			   , __FUNCTION__, list->type[i], name, list->item);
               ism_common_setErrorData(ISMRC_BadPropertyName, "%s", name);
                break;
           }
           } /* End of switch */

           if (rc == ISMRC_OK && props) {
               ism_common_setProperty(props, name, &f);
           }
       } /* End of for loop */
    }

    if (nameInList == 0) {
       rc = ISMRC_BadAdminPropName;
       ism_common_setErrorData(rc, "%s%s%s", list->item, "UNKNOWN", name);
    }

VAL_CHECK_END:
    if (rc != ISMRC_OK) {
        TRACE(5, "%s: Validation failed: name: %s, value: %s, action: %d\n",
            __FUNCTION__, name?name:"null", value?value:"null", action );
    }

    return rc;
}


/*
 * Validate data type number.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated (json_t)
 * @param  min                    The minimum allowed number
 * @param  max                    The maximum allowed number
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataTypeJson_number( char *name, long value, char *min, char *max ) {
    int32_t rc = ISMRC_OK;

    //TRACE(9, "Entry %s: name: %s, value: %s, min: %s, max: %s, sizeQualifierOptions: %s\n",
    //    		__FUNCTION__, name?name:"null", value?value:"null",
    //    		min?min:"null", max?max:"null", sizeQualifierOptions?sizeQualifierOptions:"null");

    /* check for NULL pointer */
    if ( !name || *name == '\0' ) {
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }

    if (min && *min != '\0') {
        /* Validate data for min */
        long minval = atol(min);
        if ( value < minval ) {
            rc = ISMRC_BadPropertyValue;
            goto VALIDATION_END;
        }
    }

    if (max && *max != '\0') {
        /* Validate data for max */
         long maxval = atol(max);
        if ( value > maxval ) {
            rc = ISMRC_BadPropertyValue;
        }
    }

VALIDATION_END:
    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;

}

char *valueof(json_t *value, char *result) {
    json_type objType = json_typeof(value);
	if (objType == JSON_TRUE) {
		return "true";
	} else if (objType == JSON_FALSE) {
		return "false";
	} else if (objType == JSON_STRING) {
		return (char *)json_string_value(value);
	} else if (objType == JSON_INTEGER) {
		return ism_common_ltoa((long)json_integer_value(value), result);
	}
	return "null";
}

/*
 * Generic configuration item using schema.
 * @param  list      A complete list of defined items of a composite object defined in the JSON schema
 * @param  item      Name of the object
 * @param  name      A configuration item to be validated
 * @param  value     The json_t value of the configuration item specified by name
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * NOTE: PLEASE do not add any special cases inside this function.
 * This validation should be ONLY used for generic validating the item against its type,
 * and range defined in the schema. For example, "ConnectionName" is defined as String, this
 * function will only verify the string. To validate each of the
 * IPAddess:port in the string, have special code piece inside valid_QueueManagerConnection API.
 */
XAPI int ism_config_validateItemDataJson( ism_config_itemValidator_t *list, char *item, char *name, json_t *value )
{
    int i = 0;
    int rc = ISMRC_OK;
    char * object = list->item ? list->item : "UNKNOWN";

    if (!name || !list) {
        rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    if ( list->total < 1) {
        TRACE(5, "%s: validation list is empty.\n", __FUNCTION__);
        goto VAL_CHECK_END;
    }

    /* No check required for Type, Action, Version, User, Locale, Component, and Item */
    if ( !strcmpi(name, "Type") || !strcmpi(name, "Action") || !strcmpi(name, "Version") || !strcmpi(name, "User") || !strcmpi(name, "Locale") || !strcmpi(name, "Component") || !strcmpi(name, "Item")) {
        goto VAL_CHECK_END;
    }

    /* Should not get Update or Delete from RESTAPI. This can be removed once they are removed from the schema */
    if (!strcmp(name,"Update") || !strcmp(name, "Delete")) {
        rc = ISMRC_BadAdminPropName;
        ism_common_setErrorData(rc, "%s%s%s", object, item, name);
        goto VAL_CHECK_END;
    }

    /* ALL items from the json string but not in the schema should NOT allow after this line*/
    /* check the item name specified on the schema*/
    int nameInList = 0;
    for (i = 0; i < list->total && !nameInList; i++) {
       if ( list->name[i] && !strcmp(list->name[i], name) ) {
    	   char *val;
           /* mark the found flag */
           nameInList = 1;
           list->assigned[i] = 1;
           int tempflag = list->tempflag[i];

           json_type objType = json_typeof(value);

           if ( objType == JSON_NULL && list->defv[i] ) {
               /* allow create/update to use default value if not specified */
               val = list->defv[i];
           } else if ( objType == JSON_NULL && list->reqd[i] == 0 ) {
               /* allow update to remove optional configure item by set it to empty or null */
               val = NULL;
           } else {
               /* validate the value by type*/
               switch(list->type[i]) {
               case ISM_CONFIG_PROP_NUMBER:
               {
                   if ((objType != JSON_INTEGER)) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   } else {
                	   long lvalue = json_integer_value(value);
                       rc = ism_config_validateDataTypeJson_number(name, lvalue, list->min[i], list->max[i]);
                       if (rc == ISMRC_NullPointer) {
                           ism_common_setError(rc);
                       } else if (rc == ISMRC_BadPropertyValue) {
                           ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%ld", name, (long)lvalue);
                       }
                   }
                    break;
               }
               case ISM_CONFIG_PROP_BUFFERSIZE:
               {
                   if (objType != JSON_STRING ) {
                       /* Return error */

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                       goto VAL_CHECK_END;
                   } else {
                	   char * svalue = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_bufferSize(name, svalue, list->min[i], list->max[i]);
//                       val = svalue;
                   }
                   break;
               }
               case ISM_CONFIG_PROP_ENUM:
               case ISM_CONFIG_PROP_LIST:
               {
            	   if (objType == JSON_NULL && list->reqd[i] == 1 && list->defv[i] == NULL) {
                       ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, "null");
                       rc = ISMRC_BadPropertyValue;
                       goto VAL_CHECK_END;
                   }else if (objType != JSON_STRING ) {
                       /* Return error */

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   } else {
                	   char * svalue = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_enum(name, svalue, list->options[i], list->type[i]);
//                       val = svalue;
                   }
                   break;
               }
               case ISM_CONFIG_PROP_BOOLEAN:
               {
            	   if (objType != JSON_TRUE && objType != JSON_FALSE) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
            	   }
                   break;
               }
               case ISM_CONFIG_PROP_NAME:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
            	   } else if (objType == JSON_STRING) {
            		   char * svalue = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_name(name, svalue, list->max[i], list->item);
                       if (rc == ISMRC_OK) {
                           val = svalue;
                       }
            	   }
                   break;
               }
               case ISM_CONFIG_PROP_STRING:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   } else {
                	   char * svalue = "";
                	   if ( objType == JSON_STRING )
                	       svalue = (char *)json_string_value(value);
                       int namelen = strlen(name);
                       if (!strcmpi(name, "Description") || (namelen > 8 && !strcmp(name+namelen-8, "Policies"))) {
                           rc = ism_config_validateDataType_string(name, svalue, 0, list->max[i], list->item);
                       } else {
                           rc = ism_config_validateDataType_string(name, svalue, 1, list->max[i], list->item);
                       }
                       val = svalue;
                   }

                   if (rc == ISMRC_OK ) {
                       if (list->minoneopt[i] && val && *val) list->minonevalid[i] = 1;
                   }
                   break;
               }
               case ISM_CONFIG_PROP_IPADDRESS:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL ) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   }
                   if (objType == JSON_STRING) {
                       char *lvalue = (char *)json_string_value(value);
                       int mode = 1;
                       if ( tempflag == 1 ) mode = 0;
                       rc = ism_config_validateDataType_IPAddresses(name, lvalue, mode);
                   }
                   break;
               }
               case ISM_CONFIG_PROP_URL:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   }
                   if (objType == JSON_STRING) {
                       char *lvalue  = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_URL( name, lvalue, list->max[i], list->options[i], list->item );
                   }
                   break;
               }
               case ISM_CONFIG_PROP_REGEX:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL ) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   }
                   if (objType == JSON_STRING) {
                       char *lvalue  = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_regex(name, lvalue, list->max[i], list->item);
                       if (rc == ISMRC_OK) {
                           if (list->minoneopt[i] && lvalue && *lvalue) list->minonevalid[i] = 1;
                       }
                   }
                   break;
               }
               case ISM_CONFIG_PROP_REGEX_SUBEXP:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL ) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   }
                   if (objType == JSON_STRING) {
                       char *lvalue  = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_regex_subexpr(name, lvalue, list->max[i], list->item);
                       if (rc == ISMRC_OK) {
                           if (list->minoneopt[i] && lvalue && *lvalue) list->minonevalid[i] = 1;
                       }
                   }
                   break;
               }
               case ISM_CONFIG_PROP_SELECTOR:
               {
                   if ( objType != JSON_STRING && objType != JSON_NULL ) {

                       ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object, item, name, ism_config_json_typeString(objType));
                       rc = ISMRC_BadPropertyType;
                       goto VAL_CHECK_END;
                   }
                   if (objType == JSON_STRING) {
                       char *lvalue  = (char *)json_string_value(value);
                       rc = ism_config_validateDataType_Selector(name, lvalue, list->max[i], list->item);
                   }
                   break;
               }
               default:
               {
                   /* Invalid item is found in the json object */
                   rc = ISMRC_BadAdminPropName;
                   ism_common_setErrorData(ISMRC_BadAdminPropName, "%s%s%s", object, item, name);
                    break;
               }
               } /* End of switch */
           }

           break;
       } /* End of for loop */
    }

    if (nameInList == 0) {
       rc = ISMRC_BadAdminPropName;
       ism_common_setErrorData(rc, "%s%s%s", object, item, name);
    }

VAL_CHECK_END:
    if (rc != ISMRC_OK) {
    	char buff[25];
    	char *svalue = valueof(value, buff);
        TRACE(5, "%s: Validation failed: name: %s, value: %s, rc: %d\n",
            __FUNCTION__, name?name:"null", svalue, rc );
    }

    return rc;
}

