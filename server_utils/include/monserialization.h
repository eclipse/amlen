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
/*********************************************************************/
/*                                                                   */
/* Module Name: monserialization.h                                   */
/*                                                                   */
/* Description: IMA Monitoring Data JSON serialization header file   */
/*                                                                   */
/*********************************************************************/
#ifndef __MON_SERIALIZATION_DEFINED
#define __MON_SERIALIZATION_DEFINED

#define ISMOBJD_I        1     /* int32_t */
#define ISMOBJD_U        2     /* uint32_t */
#define ISMOBJD_L        3     /* int64_t */
#define ISMOBJD_S        4     /* String */
#define ISMOBJD_CNT      5     /* Count */
#define ISMOBJD_SP       6     /* String pointer */
#define ISMOBJD_VP       7     /* void pointer */
#define ISMOBJD_D        8     /* Double */
#define ISMOBJD_UL       9     /* uint64_t */
#define ISMOBJD_ISMTIME  10    /* ism_time_t */
#define ISMOBJD_BOOL     11    /* bool */
#define ISMOBJD_PERCENT  12    /* Percent Type with 1 decimal point precision (rounded) */
#define ISMOBJD_XID      13    /* ism_xid_t */
#define ISMOBJD_HIST     14    /* String pointer for history data. */
#define ISMOBJD_RSETID   15    /* String pointer for resource set identifier */
#define ISMOBJD_SKIPSP   16    /* String pointer skipped if null */

#define OffsetOf(s,m) offsetof(s,m)

/**
 * Monitoring Object Definition
 */
typedef struct ism_mon_obj_def_t {
    short   type;				/**< The type of the field			*/
    short   offset;				/**< Offset of the field			*/
    short   wtype;				/**< The wire format type			*/
    char  * name;				/**< The name of the field			*/
    char  * descr;				/**< The description of the field	*/
    int     version;			/**< The least version that this field is supported*/
    int		displayable;		/**< If set to 1, then the value can be display in the specified output format. if 0,
							         the field will be hidden */
	char  * oid;				/**< The object identifier string */
	char  * displayname	;		/**< The display name */
    int     reserved;
} ism_mon_obj_def_t;

/**
 * Macro to define each monitoring field.
 */
#define ISMMONOBJ(objtype,typ, wtype, nam, displayname, oid, version,  displayble, descr) \
{ ISMOBJD_##typ, OffsetOf(objtype,nam), ISMOBJD_##wtype, #nam, #descr, version, displayble, #oid, #displayname},

/**
 * Serializer Data
 */
typedef struct
{
	void	* serializer_userdata;				/*The user data for this serializer*/
}ismSerializerData;

/**
 * JSON Serializer Data
 */
typedef struct
{
	concat_alloc_t * outbuf;
	int isExternalMonitoring;
	char * prefix;
	
}ismJsonSerializerData;

/**
 * Structure to hold the the historical data
 */
typedef struct  ismMonitoringHistoryData {
	char *typeStr;
	int type;
	int64_t *darray;   // The array of numbers
	char * orgDataStr;   //Original String which contain the comma separated list of value
}ismMonitoringHistoryData_t;

/**
 * Serialize the Monitoring Data
 * @param objdef the monitoring object definition
 * @param obj structure/object that define the the monitoring object
 * @param buf the buffer that contain the data
 * @param version version of the product 
 * @param serData the serialization data
 * @return the String that the data is serialized.
 */
char * ism_common_serializeMonJson(const ism_mon_obj_def_t *objdef, const void * obj, char * buf, int version, ismSerializerData *serData);

#endif

