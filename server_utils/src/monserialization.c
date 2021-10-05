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
/*
 */
#define TRACE_COMP Util
#include <ismutil.h>
#include <ismxmlparser.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ismjson.h>
#include <monserialization.h>

/*
 * Serialize the boolean value
 */
static char * serializeBoolean(int32_t ival, char * buf ) {
	char * outp = buf;
	char * trueStr = "true";
	char * falseStr = "false";
	int len = 5;

    if ( ival == 1 ) {
	  	len = 4;
	    memcpy(buf, trueStr, len);
	} else {
	  	memcpy(buf, falseStr, len);
	}
	buf[len] = 0;
	return outp;
}

/*
 * Serialize the Percent value with 1 decimal point precision (rounded).
 */
static char * serializePercent(double pval, char * buf ) {
	char * outp = buf;

	if (buf != NULL) {
		sprintf(buf, "%.1lf", pval);
	}
    return outp;
}

/*
 * Serialize the null value.
 */
static char * serializeNull(char * buf) {
	char * outp = buf;

	if (buf != NULL) {
		strcpy(buf, "null");
	}
    return outp;
}

/*
 * Serialize the Monitoring Data
 * @param objdef the monitoring object definition
 * @param obj structure/object that define the the monitoring object
 * @param buf the buffer that contain the data
 * @param version version of the product
 * @param serData the serialization data
 * @return the String that the data is serialized.
 */
char * ism_common_serializeMonJson(const ism_mon_obj_def_t *objdef, const void * obj, char * buf, int version, ismSerializerData *serData) {
	ismJsonSerializerData * serUserData = (ismJsonSerializerData *)serData->serializer_userdata;
	char tbuf[2048];
	char * val = NULL;
	int ival=0;
	char * lobj =(char *)obj;
	int needquote=0;
	char tbuffer[80];
	uint64_t t;
	ism_ts_t * ts = NULL;
	ism_mon_obj_def_t * thisdef = (ism_mon_obj_def_t *)objdef;

	if (obj == NULL || serUserData->outbuf == NULL){
		return NULL;
	}

	ism_common_allocBufferCopyLen(serUserData->outbuf, "{", 1);

	/*Set the message prefix for external monitoring*/
	if (serUserData->isExternalMonitoring == 1) {
		ism_common_allocBufferCopyLen(serUserData->outbuf, serUserData->prefix, strlen(serUserData->prefix) );
		ism_common_allocBufferCopyLen(serUserData->outbuf, ",", 1);
	}

	while (thisdef->wtype) {
	    int skip = 0;
		switch (thisdef->wtype) {
        case ISMOBJD_CNT:     ival = *(int32_t *)(lobj+thisdef->offset); val = ism_common_itoa(ival, tbuf); needquote=0;   break;
        case ISMOBJD_I:	      ival = *(int32_t *)(lobj+thisdef->offset); val = ism_common_itoa(ival, tbuf); needquote=0;   break;
        case ISMOBJD_U:	      val = ism_common_itoa(*(uint32_t *)(lobj+thisdef->offset), tbuf);  needquote=0;              break;
        case ISMOBJD_D:       val = ism_common_dtoa(*(double *)(lobj+thisdef->offset), tbuf);   needquote=0;               break;
        case ISMOBJD_L:	      val = ism_common_ltoa(*(int64_t *)(lobj+thisdef->offset), tbuf); needquote=0;                break;
        case ISMOBJD_S:	  	  val = lobj+thisdef->offset;   needquote=1;  												  break;
        case ISMOBJD_HIST:
        case ISMOBJD_RSETID:
        case ISMOBJD_SP:
        	val = *(char **)(lobj+thisdef->offset);
        	needquote=1;
        	break;
        case ISMOBJD_SKIPSP:
            val = *(char **)(lobj+thisdef->offset);
            needquote=1;
            if (val == NULL) skip = 1;
            break;
        case ISMOBJD_UL:
            val = ism_common_ultoa(*(uint64_t *)(lobj+thisdef->offset), tbuf);
            needquote=0;
            break;
        case ISMOBJD_ISMTIME:
        	/* Convert valid time to ISO8601 or set field to null, if 0 */
        	t = *(uint64_t *)(lobj+thisdef->offset);
        	if (t == 0) {
        		val = serializeNull(tbuf);
        		needquote=0;
        	} else {
				ts = ism_common_openTimestamp(ISM_TZF_UTC);
				if ( ts != NULL) {
					ism_common_setTimestamp(ts, *(uint64_t *)(lobj+thisdef->offset));
					ism_common_formatTimestamp(ts, tbuffer, 80, 0, ISM_TFF_ISO8601);
					val = tbuffer;
					ism_common_closeTimestamp(ts);
				}
				needquote=1;
        	}

            break;
        case ISMOBJD_XID:
        	memset(tbuf, '\0', sizeof(tbuf));
        	ism_xid_t * xid = (ism_xid_t *)(lobj+thisdef->offset); val = ism_common_xidToString(xid, tbuf, 278); needquote=1;   break;
        	break;
        case ISMOBJD_BOOL:    ival = (int)*(bool *)(lobj+thisdef->offset); val = serializeBoolean(ival, tbuf);needquote=0; break;
        case ISMOBJD_PERCENT:
        	val = serializePercent(*(double *)(lobj+thisdef->offset), tbuf);
        	needquote=0;
        	break;
        default:
            val = NULL;  needquote=1;                                                                   break;
		};

		if (thisdef->type && thisdef->displayable>0) {
		    if (!skip) {
		        ism_common_allocBufferCopyLen(serUserData->outbuf, "\"", 1);
		        ism_common_allocBufferCopyLen(serUserData->outbuf, thisdef->displayname, strlen(thisdef->displayname));
		        ism_common_allocBufferCopyLen(serUserData->outbuf, "\":", 2);

		        if (needquote==1)
		            ism_common_allocBufferCopyLen(serUserData->outbuf, "\"", 1);
		        if (val) {
		            ism_json_putEscapeBytes(serUserData->outbuf, val, strlen(val));
		        }

		        if (needquote==1 )
		            ism_common_allocBufferCopyLen(serUserData->outbuf, "\"", 1);
		    } else if (thisdef != objdef) {
		        serUserData->outbuf->used--;
		    }
		}

		thisdef++;

		if (thisdef->type && thisdef->displayable>0) {
            ism_common_allocBufferCopyLen(serUserData->outbuf, ",", 1);
		}
		val = NULL;
	}

	ism_common_allocBufferCopy(serUserData->outbuf, "}");  /* End the object and null terminate it */
	serUserData->outbuf->used--;                           /* Back up before null terminator       */
	// TRACE(9, "monitoring string=[%s]", serUserData->outbuf->buf);

	return serUserData->outbuf->buf;
}
