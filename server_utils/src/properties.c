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

#include <ismutil.h>
#include <fendian.h>
#include <ctype.h>
#include <imacontent.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct suballoc_t {
    struct suballoc_t * next;
    uint32_t   size;
    uint32_t   pos;
};

/*
 * Internal definition of encapsulated field
 */
struct ism_prop_t {
    int             type;
    int             propcount;
    int             alloccount;
    int             basealloc;
    int             basesize;
    int             resv;
    ism_propent_t * props;
    struct suballoc_t suballoc;         /* Must be last */
};


void ism_protocol_putPropertyValue(ism_actionbuf_t * map, const char * name, ism_field_t * var);

/*
 * Simple properties object constructor.
 * @param The initial property count
 * @return A properties object
 */
ism_prop_t * ism_common_newProperties(int count) {
    ism_prop_t * ret;
    int  bufsize;
    int  size;
    if (count<20)
        count = 20;
    bufsize = count*40;
    if (bufsize < 1000)
        bufsize = 1000;
    size = sizeof(ism_prop_t) + count * sizeof(ism_propent_t) + bufsize;
    ret = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_props,128),size);
    memset(ret, 0, sizeof(ism_prop_t));
    ret->props = (ism_propent_t *)(ret+1);
    ret->suballoc.size = size-sizeof(ism_prop_t);
    ret->suballoc.pos = count*sizeof(ism_propent_t);
    ret->alloccount = count;
    ret->basealloc = count;
    ret->basesize = ret->suballoc.size;
    return ret;
}

static char * allocPropertyBytes(ism_prop_t * props, int len, int align) {
    struct suballoc_t * suba = &props->suballoc;
    int  pad = 0;
    for (;;) {
        if (align) {
            pad = 8-(suba->pos&7);
            if (pad == 8)
                pad = 0;
        }
        if (suba->size-suba->pos > len+pad) {
            char * ret = ((char *)(suba+1))+suba->pos+pad;
            suba->pos += len+pad;
            return ret;
        }
        if (!suba->next) {
            int newlen = ((len+1200+sizeof(struct suballoc_t)) & ~0x3ff) - 16; /* 16 -s the malloc overhead. for small allocations the ideal size is power of 2 - 16 */
            suba->next = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_props,129),newlen);
            suba->next->next = 0;
            suba->next->size = newlen-sizeof(struct suballoc_t);
            suba->next->pos  = 0;
            continue;
        }
        suba = suba->next;
    }
}



/*
 * Free a properties object.
 * @param props  A properties object
 */
void ism_common_freeProperties(ism_prop_t * props) {
    if (props) {
        struct suballoc_t * suba = props->suballoc.next;
        while (suba) {
            struct suballoc_t * freesub = suba;
            suba = suba->next;
            freesub->next = NULL;
            ism_common_free(ism_memory_utils_props,freesub);
        }
        ism_common_free(ism_memory_utils_props,props);
    }
}


/*
 * Get a property as a field.
 * @param props A properties object
 * @param name  The name of the property to get
 * @param field The field in which to return the property
 */
int  ism_common_getProperty(ism_prop_t * props, const char * name, ism_field_t * field) {
    int  i;
    if (!props || !name) {
        field->type = VT_Null;
        return 2;
    }
    for (i=0; i<props->propcount; i++) {
        if (!strcmp(name, props->props[i].name)) {
            *field = props->props[i].f;
            return 0;
        }
    }
    field->type = VT_Null;
    return 1;
}

/*
 * Get property count.
 * @param props A properties object
 */
 int  ism_common_getPropertyCount(ism_prop_t * props) {
    if (!props) {
        return -1;
    }
    return props->propcount;
}


/*
 * Get a property by index
 */
int  ism_common_getPropertyIndex(ism_prop_t * props, int index, const char * * name) {
    if (index < 0 || index >= props->propcount) {
        *name = NULL;
        return 1;
    }
    *name = props->props[index].name;
    return 0;
}


/*
 * Find a field by name
 */
static int findField(ism_prop_t * props, const char * name) {
    int  i;
    for (i=0; i<props->propcount; i++) {
        if (!strcmp(name, props->props[i].name)) {
            return i;
        }
    }
    return -1;
}


/*
 * Add a string to the buffer, extending if required
 */
static char * addToBufLen(ism_prop_t * props, const char * str, int len) {
    char * ret = allocPropertyBytes(props, len, 0);
    memcpy(ret, str, len);
    return ret;
}


/*
 * Add a zero terminated string to the buffer, extending if required
 */
static char * addToBuf(ism_prop_t * props, const char * str) {
    if (!str)
        return NULL;
    return addToBufLen(props, str, (int)strlen(str)+1);
}

/*
 * Convert a field to a string to return the value
 */
const char * ism_common_convertToString(ism_prop_t * props, ism_field_t * f) {
	char tbuf[64];
	switch (f->type) {
    case VT_Null:                      /* Null value                       */
    case VT_ByteArray:                 /* Type of Byte Array               */
    case VT_Xid:
    default:
        return NULL;

    case VT_String:                    /* Type of String (must be zero)    */
        return f->val.s;

    case VT_Boolean:                   /* Type of boolean                  */
        return f->val.i ? "true" : "false";

    case VT_Byte:                      /* Signed 8 bit value               */
    case VT_UByte:                     /* Unsigned 8 bit value             */
    case VT_Short:                     /* Signed 16 bit value              */
    case VT_UShort:                    /* Unsigned 16 bit value            */
    case VT_Integer:                   /* Type of Integer                  */
        ism_common_itoa(f->val.i, tbuf);
        return addToBuf(props, tbuf);

    case VT_UInt:                      /* Type of Integer                  */
        ism_common_uitoa(f->val.u, tbuf);
        return addToBuf(props, tbuf);

    case VT_Long:                      /* Type of Long Integer             */
        ism_common_ltoa(f->val.l, tbuf);
        return addToBuf(props, tbuf);

    case VT_ULong:                     /* Type of Long Integer             */
        ism_common_ultoa((uint64_t)f->val.l, tbuf);
        return addToBuf(props, tbuf);

    case VT_Float:                     /* Type of Float                    */
        ism_common_ftoa(f->val.f, tbuf);
        return addToBuf(props, tbuf);

    case VT_Double:                   /* Type of Double                   */
        ism_common_dtoa(f->val.d,tbuf);
        return addToBuf(props, tbuf);
    }
}

/*
 * Convert a field to a string to return the value
 */
const char * ism_common_fieldToString(ism_field_t * f, char tbuf[64]) {
    switch (f->type) {
    case VT_Null:                      /* Null value                       */
    case VT_ByteArray:                 /* Type of Byte Array               */
    case VT_Xid:
    default:
        return NULL;

    case VT_String:                    /* Type of String (must be zero)    */
        return f->val.s;

    case VT_Boolean:                   /* Type of boolean                  */
        return f->val.i ? "true" : "false";

    case VT_Byte:                      /* Signed 8 bit value               */
    case VT_UByte:                     /* Unsigned 8 bit value             */
    case VT_Short:                     /* Signed 16 bit value              */
    case VT_UShort:                    /* Unsigned 16 bit value            */
    case VT_Integer:                   /* Type of Integer                  */
        ism_common_itoa(f->val.i, tbuf);
        return tbuf;

    case VT_UInt:                      /* Type of Integer                  */
        ism_common_uitoa(f->val.u, tbuf);
        return tbuf;

    case VT_Long:                      /* Type of Long Integer             */
        ism_common_ltoa(f->val.l, tbuf);
        return tbuf;

    case VT_ULong:                     /* Type of Long Integer             */
        ism_common_ultoa((uint64_t)f->val.l, tbuf);
        return tbuf;

    case VT_Float:                     /* Type of Float                    */
        ism_common_ftoa(f->val.f, tbuf);
        return tbuf;

    case VT_Double:                   /* Type of Double                   */
        ism_common_dtoa(f->val.d,tbuf);
        return tbuf;
    }
}

/*
 * Get a property as a string.
 * If the object is not a string, it is converted as required to a string.
 * The memory is associated with the properties object and has the lifetime of
 * the properties object.
 * @param props  A properties object
 * @param name   The name of the object to return.
 */
const char * ism_common_getStringProperty(ism_prop_t * props, const char * name) {
    ism_field_t f;
    ism_common_getProperty(props, name, &f);
    return ism_common_convertToString(props, &f);
}


/*
 * Modified version of strtol which handles hex and decimal, but not octal
 */
static int str2l(const char * str, char * * endp) {
    const char * cp = str;
    int  ret;
    while (*cp==' ' || *cp=='\t') cp++;
    if (*cp == '0' && cp[1]=='x')
        ret = strtol(cp+2, endp, 16);
    else
        ret = strtol(cp, endp, 10);
    if (endp && *endp > str && **endp) {
        cp = *endp;
        while (*cp==' ' || *cp=='\t') cp++;
        *endp = (char *)cp;
    }
    return ret;
}

/*
 * Get a property as an integer.
 * If the object is not an integer, it is converted to an integer.  If it does not
 * exist or cannot be converted to an integer, the default value is returned.
 */
int  ism_common_getIntProperty(ism_prop_t * props, const char * name, int deflt) {
    char * endp;
    int    val;
    ism_field_t f;
    ism_common_getProperty(props, name, &f);
    switch (f.type) {
    case VT_Null:                      /*<< Null value                       */
    case VT_ByteArray:                 /**< Type of Byte Array               */
    default:
        return deflt;

    case VT_String:                    /**< Type of String                   */
        if (!f.val.s)
            return deflt;
        val = str2l(f.val.s, &endp);
        if (*endp) {
            if (!strcmpi(f.val.s, "false"))
                return 0;
            if (!strcmpi(f.val.s, "true"))
                return 1;
            return deflt;
        }
        return val;

    case VT_Boolean:                   /* Type of boolean                  */
        return f.val.i ? 1 : 0;

    case VT_Byte:                      /* Signed 8 bit value               */
    case VT_UByte:                     /* Unsigned 8 bit value             */
    case VT_Short:                     /* Signed 16 bit value              */
    case VT_UShort:                    /* Unsigned 16 bit value            */
    case VT_Integer:                   /* Type of Integer                  */
    case VT_UInt:                      /* Type of Integer                  */
        return f.val.i;

    case VT_Long:                      /* Type of Long Integer             */
    case VT_ULong:                     /* Type of Long Integer             */
        return (int32_t)f.val.l;

    case VT_Float:                     /* Type of Float                    */
        return (int32_t)f.val.f;

    case VT_Double:                    /* Type of Double                   */
        return (int32_t)f.val.d;
    };
}

/*
 * Modified version of strtoul which handles hex and decimal, but not octal
 */
static uint32_t str2ul(const char * str, char * * endp) {
    const char * cp = str;
    int  ret;
    while (*cp==' ' || *cp=='\t') cp++;
    if (*cp == '0' && cp[1]=='x')
        ret = strtoul(cp+2, endp, 16);
    else
        ret = strtoul(cp, endp, 10);
    if (endp && *endp > str && **endp) {
        cp = *endp;
        while (*cp==' ' || *cp=='\t') cp++;
        *endp = (char *)cp;
    }
    return ret;
}

/*
 * Get a property as an unsigned integer.
 * If the object is not an unsigned integer, it is converted to an unsigned integer.  If it does not
 * exist or cannot be converted to an unsigned integer, the default value is returned.
 */
uint32_t ism_common_getUintProperty(ism_prop_t * props, const char * name, uint32_t deflt) {
    char * endp;
    uint32_t val;
    ism_field_t f;
    ism_common_getProperty(props, name, &f);
    switch (f.type) {
    case VT_Null:                      /*<< Null value                       */
    case VT_ByteArray:                 /**< Type of Byte Array               */
    default:
        return deflt;

    case VT_String:                    /**< Type of String                   */
        if (!f.val.s)
            return deflt;
        val = str2ul(f.val.s, &endp);
        if (*endp) {
            if (!strcmpi(f.val.s, "false"))
                return 0;
            if (!strcmpi(f.val.s, "true"))
                return 1;
            return deflt;
        }
        return val;

    case VT_Boolean:                   /* Type of boolean                  */
        return f.val.u ? 1 : 0;

    case VT_Byte:                      /* Signed 8 bit value               */
    case VT_UByte:                     /* Unsigned 8 bit value             */
        return (uint32_t)(f.val.u&0xff);

    case VT_Short:                     /* Signed 16 bit value              */
    case VT_UShort:                    /* Unsigned 16 bit value            */
        return (uint32_t)(f.val.u&0xffff);

    case VT_Integer:                   /* Type of Integer                  */
    case VT_UInt:                      /* Type of Integer                  */
        return f.val.u;

    case VT_Long:                      /* Type of Long Integer             */
    case VT_ULong:                     /* Type of Long Integer             */
        return (uint32_t)f.val.l;

    case VT_Float:                     /* Type of Float                    */
        return (uint32_t)f.val.f;

    case VT_Double:                    /* Type of Double                   */
        return (uint32_t)f.val.d;
    };
}

/*
 * Modified version of strtol which handles hex and decimal, but not octal
 */
static int64_t str2ll(const char * str, char * * endp) {
    const char * cp = str;
    int64_t  ret;
    while (*cp==' ' || *cp=='\t') cp++;
    if (*cp == '0' && cp[1]=='x')
        ret = strtoll(cp+2, endp, 16);
    else
        ret = strtoll(cp, endp, 10);
    if (endp && *endp > str && **endp) {
        cp = *endp;
        while (*cp==' ' || *cp=='\t') cp++;
        *endp = (char *)cp;
    }
    return ret;
}

/*
 * Get a property as a long.
 * If the object is not an integer, it is converted to an integer.  If it does not
 * exist or cannot be converted to an integer, the default value is returned.
 */
int64_t  ism_common_getLongProperty(ism_prop_t * props, const char * name, int64_t deflt) {
    char * endp;
    int64_t  val;
    ism_field_t f;
    ism_common_getProperty(props, name, &f);
    switch (f.type) {
    case VT_Null:                      /*<< Null value                       */
    case VT_ByteArray:                 /**< Type of Byte Array               */
    default:
        return deflt;

    case VT_String:                    /**< Type of String                   */
        if (!f.val.s)
            return deflt;
        val = str2ll(f.val.s, &endp);
        if (*endp) {
            if (strcmpi(f.val.s, "false"))
                return 0;
            if (strcmpi(f.val.s, "true"))
                return 1;
            return deflt;
        }
        return val;

    case VT_Boolean:                   /* Type of boolean                  */
        return f.val.i ? 1 : 0;

    case VT_Byte:                      /* Signed 8 bit value               */
    case VT_UByte:                     /* Unsigned 8 bit value             */
    case VT_Short:                     /* Signed 16 bit value              */
    case VT_UShort:                    /* Unsigned 16 bit value            */
    case VT_Integer:                   /* Type of Integer                  */
        return (int64_t)f.val.i;
    case VT_UInt:                      /* Type of Integer                  */
        return (int64_t)(uint32_t)f.val.i;

    case VT_Long:                      /* Type of Long Integer             */
    case VT_ULong:                     /* Type of Long Integer             */
        return (int64_t)f.val.l;

    case VT_Float:                     /* Type of Float                    */
        return (int64_t)f.val.f;

    case VT_Double:                    /* Type of Double                   */
        return (int64_t)f.val.d;
    }
}


/*
 * Get a property as an boolean.
 * If the object is not an boolean, it is converted to a boolean.  If it does not
 * exist or cannot be converted to a boolean, the default value is returned.
 */
int  ism_common_getBooleanProperty(ism_prop_t * props, const char * name, int deflt) {
    int ret = ism_common_getIntProperty(props, name, -999);
    if (ret == -999)
        return deflt;
    return ret != 0;
}


/*
 * Clear all properties
 */
int ism_common_clearProperties(ism_prop_t * props) {
    /* If we have extended, go back to the original definition */
    struct suballoc_t * suba = props->suballoc.next;
    while (suba) {
        struct suballoc_t * freesub = suba;
        suba = suba->next;
        freesub->next = NULL;
        ism_common_free(ism_memory_utils_props,freesub);
    }
    props->suballoc.next = NULL;
	props->propcount = 0;
	props->props = (ism_propent_t *)(props+1);
	props->suballoc.size = props->basesize;
	props->suballoc.pos = props->basealloc*sizeof(ism_propent_t);
    props->alloccount = props->basealloc;
	return 0;
}


/*
 * Set a property
 */
int  ism_common_setProperty(ism_prop_t * props, const char * name, ism_field_t * field) {
    int  which;

    if (!props || !name  )
        return 2;

    which = findField(props, name);

    /* TODO: validate type */
    if (!field) {
        if (which >= 0) {                                    /* Remove the property */
            props->propcount--;
            while (which < props->propcount) {
                props->props[which] = props->props[which+1];
                which++;
            }
        }
    } else {
        if (which < 0) {
            which = props->propcount++;
            /* Extend properties */
            if (which >= props->alloccount) {
                int newcount = props->alloccount*2 + 10;
                ism_propent_t * newprops = (ism_propent_t *)allocPropertyBytes(props, newcount * sizeof(ism_propent_t), 1);
                memcpy(newprops, props->props, props->alloccount * sizeof(ism_propent_t));
                props->props = newprops;
                props->alloccount = newcount;
            }
            props->props[which].name = addToBuf(props, name);
        }
        props->props[which].f = *field;
        switch (field->type) {
        default:
            break;
        case VT_String:
            props->props[which].f.val.s = addToBuf(props, field->val.s);
            break;
        case VT_ByteArray:
        case VT_Xid:
        case VT_Map:
            props->props[which].f.val.s = addToBufLen(props, field->val.s, field->len);
            break;
        }
    }
    return 0;
}

/*
 * Add a property to an ism_prop_t (no duplicate checking or deletion)
 */
int ism_common_addProperty(ism_prop_t * props, const char * name, ism_field_t * field) {
    int which;
    if (!props || !name  || !field)
        return 2;

    which = props->propcount++;
    /* Extend properties */
    if (which >= props->alloccount) {
        int newcount = props->alloccount*2 + 10;
        ism_propent_t * newprops = (ism_propent_t *)allocPropertyBytes(props, newcount * sizeof(ism_propent_t), 1);
        memcpy(newprops, props->props, props->alloccount * sizeof(ism_propent_t));
        props->props = newprops;
        props->alloccount = newcount;
    }
    props->props[which].name = addToBuf(props, name);
    props->props[which].f = *field;
    switch (field->type) {
        default:
            break;
        case VT_String:
            props->props[which].f.val.s = addToBuf(props, field->val.s);
            break;
        case VT_ByteArray:
        case VT_Xid:
        case VT_Map:
            props->props[which].f.val.s = addToBufLen(props, field->val.s, field->len);
            break;
        }

    return 0;
}

/*
 * Convert a VT_Xid to a XID structure
 */
int  ism_common_toXid(ism_field_t * f, ism_xid_t * xid) {
    int  memint;
    memset(xid, 0, XID_HDRSIZE);
    if (f->type != VT_Xid || f->len < 6)
        return ISMRC_Error;
    memmove(xid->data, f->val.s+6, f->len-6);
    memcpy(&memint, f->val.s, 4);
    xid->formatID = endian_int32(memint);
    xid->gtrid_length = f->val.s[4];
    xid->bqual_length = f->val.s[5];
    return 0;
}

/*
 * Return the buffer size need to convert a XID to VT_Xid
 * @param xid  The input XID structure
 * @return The length in bytes of the allocation required for the XID
 */
int  ism_common_xidFieldLen(ism_xid_t * xid) {
    int   len;
    if (!xid)
        return 0;
    len = xid->gtrid_length + xid->bqual_length + 6;
    if (len > XID_DATASIZE+6)
        return 0;
    return len;
}

/*
 * Convert a XID structure to VT_Xid field
 *
 * @param xid   The input XID structure
 * @param field The output field allocated by the caller
 * @param buf   The output buffer for the XID which must be at least as large as specified by calling
 *              ism_protocol_xidFieldLen() with this xid.
 */
int  ism_common_fromXid(ism_xid_t * xid, ism_field_t * field, char * buf) {
    int  len;
    int  memint;

    len = xid->gtrid_length + xid->bqual_length + 6;
    if (len > XID_DATASIZE+6) {
        field->type = VT_Xid;
        return ISMRC_Error;
    }
    field->type = VT_Xid;
    field->val.s = buf;
    memint = endian_int32(xid->formatID);
    memcpy(buf, &memint, 4);
    buf[4] = (uint8_t)xid->gtrid_length;
    buf[5] = (uint8_t)xid->bqual_length;
    memmove(buf+6, xid->data, len-6);
    field->len = len;
    return 0;
}

/*
 * Return the integer value of a hex digit
 */
static int hexCharToInt(char c) {
	if ((c >= '0') && (c <= '9')) {
		return c - '0';
	}
	if ((c >= 'A') && (c <= 'F')) {
		return c - 'A' + 10;
	}
    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }
	return -1;
}

/**
 * Convert string to XID.
 *
 * The format is three hex strings separated by colons where the first is the formatID,
 * the second is the branch qualifier, and the third is the global transaction ID.
 * WAS uses a similar string but commonly does not show the formatID.
 * @param xidStr input string
 * @param xid   The XID structure
 * @return 0 if OK
 */
int ism_common_StringToXid(const char * xidStr, ism_xid_t * xid) {
	if (xidStr && xid) {
		char * formatStr;
		char * bqStr;
		char * gtStr;
		size_t len = strlen(xidStr)+1;

		formatStr = alloca(len);
		memcpy(formatStr, xidStr, len);
		bqStr = strchr(formatStr, ':');
		if (bqStr) {
		    *bqStr++ = 0;
		    gtStr = strchr(bqStr, ':');
		    if (gtStr) {
		        *gtStr++ = 0;
		    }
		}

		if (bqStr && gtStr) {
		    if (!strcmp(formatStr, "fwd")) {    /* XID format for forwarder */
		        xid->formatID = ISM_FWD_XID;
		        xid->bqual_length = strlen(bqStr);
		        xid->gtrid_length = strlen(gtStr);
		        memcpy(xid->data, bqStr, xid->bqual_length);
		        memcpy(xid->data + xid->bqual_length, gtStr, xid->gtrid_length);
		    } else {
                int  i;
                int  j = 0;
                len = strlen(formatStr);
                xid->formatID = 0;
                for (i = 0; i < len; i++) {
                    int c = hexCharToInt(toupper(formatStr[i]));
                    if (c < 0)
                        return ISMRC_InvalidParameter;
                    xid->formatID = (xid->formatID << 4) | c;
                }
                len = strlen(bqStr);
                if ((len == 0) || (len & 0x1))
                    return ISMRC_InvalidParameter;
                for (i = 0; i < len;) {
                    int c1 = hexCharToInt(toupper(bqStr[i++]));
                    int c2 = hexCharToInt(toupper(bqStr[i++]));
                    if ((c1 < 0) || (c2 < 0))
                        return ISMRC_InvalidParameter;
                    xid->data[j++] = (uint8_t)((c1 << 4) | c2);
                }
                xid->bqual_length = j;
                len = strlen(gtStr);
                if ((len == 0) || (len & 0x1))
                    return ISMRC_InvalidParameter;
                for (i = 0; i < len;) {
                    int c1 = hexCharToInt(toupper(gtStr[i++]));
                    int c2 = hexCharToInt(toupper(gtStr[i++]));
                    if ((c1 < 0) || (c2 < 0))
                        return ISMRC_InvalidParameter;
                    xid->data[j++] = (uint8_t)((c1 << 4) | c2);
                }
                xid->gtrid_length = j - xid->bqual_length;
                return 0;
		    }
		}
	}
	return ISMRC_InvalidParameter;
}

/**
 * Convert a XID to a printable string.
 *
 * The format is three hex strings separated by colons where the first is the formatID,
 * the second is the global transaction ID, and the third is the branch qualifier.
 * WAS uses a similar string but commonly does not show the formatID.
 *
 * @param xid   The XID structure
 * @param buf   The output buffer
 * @param len   The length of the buffer (should be 278 byes for max size)
 * @return The buffer with the xid filled in or NULL for an error
 */
char * ism_common_xidToString(const ism_xid_t * xid, char * buf, int len) {
    char   out[278];
    char * outp = out;
    uint8_t * in;
    int    i;
    int    shift = 28;
    static char myhex [18] = "0123456789ABCDEF";

    if (!xid || (uint32_t)xid->bqual_length>64 || (uint32_t)xid->gtrid_length>64) {
        if (len)
            *buf = 0;
        return NULL;
    }
    if (xid->formatID == ISM_FWD_XID) {
        strcpy(out, "fwd:");
        memcpy(out+4, xid->data, xid->bqual_length);
        out[4+xid->bqual_length] = ':';
        memcpy(out+5+xid->bqual_length, xid->data+xid->bqual_length, xid->gtrid_length);
        out[5+xid->bqual_length+xid->gtrid_length] = 0;
    } else {

        /* Put out the format ID in hex with no leading zeros */
        for (i=0; i<7; i++) {
            if ((xid->formatID>>shift)&0xf)
                break;
            shift -= 4;
        }
        for (; i<7; i++) {
            *outp++ = myhex[(xid->formatID>>shift)&0xf];
            shift -= 4;
        }
        *outp++ = myhex[xid->formatID&0xf];
        *outp++ = ':';

        /* Put out the branch qualifier */
        in = (uint8_t *)xid->data;
        for (i=0; i<xid->bqual_length; i++) {
            *outp++ = myhex[(*in>>4)&0xf];
            *outp++ = myhex[*in++&0xf];
        }
        *outp++ = ':';

        /* Put out the global transaction ID */
        for (i=0; i<xid->gtrid_length; i++) {
            *outp++ = myhex[(*in>>4)&0xf];
            *outp++ = myhex[*in++&0xf];
        }
        *outp = 0;
    }

    /* Copy into the output buffer */
    ism_common_strlcpy(buf, out, len);
    return buf;
}

/*
 * Look up the enum value given a name.
 * @param enumlist  A enumerated value list
 * @param value     A name to search for.
 * @return  The value matching the specified name, or INVALID_ENUM to indicate not found.
 */
int32_t ism_common_enumValue(ism_enumList * enumlist, const char * name) {
    int  i;
    if (name) {
        for (i=1; i<=enumlist[0].value; i++) {
            if (!strcmpi(name, enumlist[i].name))
                return enumlist[i].value;
        }
    }
    return INVALID_ENUM;
}

/*
 * Look up the enum name given the value.
 * @param enumlist  An enumerated value list
 * @param value     A value to search for.  The name of the first matching value is returned.
 * @return  The name matching the specified value, or NULL to indicate not found.
 */
const char * ism_common_enumName(ism_enumList * enumlist, int32_t value) {
    int  i;
    for (i=1; i<=enumlist[0].value; i++) {
        if (value == enumlist[i].value)
            return enumlist[i].name;
    }
    return NULL;
}

typedef struct propname_t {
    const char * name;
    const char * validator;
} propname_t;

static propname_t known_props [] = {
    { "AdminLog",                  "E:enum_auxlogsettings" },
    { "AdminLogFile",              "S"                 },
    { "Affinity.",                 "S"                 },
    { "Alias.",                    "S"                 },
    { "AuthName",                  "S"                 },
    { "CertificateProfile.Certificate.",    "S"        },
    { "CertificateProfile.Key.",            "S"        },
    { "CertificateProfile.Name.",           "S"        },
    { "Channel.",                  "S"                 },
    { "ConfigDir",                 "S"                 },
    { "ConnectionLog",             "E:enum_auxlogsettings" },
    { "ConnectionLogFile",         "S"                 },
    { "DelayStrategy",             "I:0:2"             },
    { "FIPS",                      "B"                 },
    { "KeyStore",                  "S"                 },
    { "Listener.",                 "S"                 },  /* Deprecated */
    { "Endpoint.Enabled.",         "B"                 },
    { "Endpoint.Interface.",       "S"                 },
    { "Endpoint.Port.",            "I:0:65535"         },
    { "Endpoint.Protocol.",        "S"                 },
    { "Endpoint.Security.",        "B"                 },  /* Deprecated */
    { "Endpoint.SecurityProfile.", "S"                 },
    { "Endpoint.ConnectionPolicy", "S"                 },
    { "FileLimit"                  "I"                 },
    { "LogFile",                   "S"                 },
    { "LogLevel",                  "S"                 },
    { "LogFilter",                 "S"                 },
    { "MessageHub.Description.",   "S"                 },
    { "MessageHub.Name.",          "S"                 },
    { "Name",                      "S"                 },
    { "NetworkInterface.",         "S"                 },
    { "PolicyDir",                 "S"                 },
    { "Processor."                 "S"                 },
    { "Port.",                     "I:0:65635"         },
    { "RecvBuffer.",               "S"                 },
    { "Redirect.",                 "S"                 },
    { "Security.",                 "S"                 },
    { "SecurityLog",               "E:enum_auxlogsettings" },
    { "SecurityLogFile",           "S"                 },
    { "SecurityProfile.CertificateProfile.",     "S"   },
    { "SecurityProfile.Ciphers.",                "S"   },
    { "SecurityProfile.MinimumProtocolMethod.",  "S"   },
    { "SecurityProfile.Name.",                   "S"   },
    { "SecurityProfile.UseClientCertificate.",   "B"   },
    { "SendBuffer.",               "S"                 },
    { "StoreMode",                 "S"                 },
    { "TimeZone",                  "S"                 },
    { "TcpImpl",                   "S"                 },
    { "TcpThreads",                "I:0:8"             },
    { "TraceFile",                 "S"                 },
    { "TraceFilter",               "S"                 },
    { "TraceLevel",                "I:0:9"             },
    { "TraceMax",                  "S"                 },
    { "TraceOptions",              "S"                 },
    { "Transport.",                "E:enum_transports" },
    { NULL,                        NULL                },
};


/*
 * Convert a name to its canonical name.
 *
 * The name is converted in place which requires that the canonical name be the same length or
 * shorter that the original name.  In almost all cases, the canonical name is used only to
 * allow any case of a property to be specified.
 * <p>
 * It the property name is not known it is still allowed, but the case cannot be corrected.
 *
 * @param name  The property name to convert.
 * @return A return code: 0=property is known, 1=property is unknown
 */
int ism_common_canonicalName(char * name) {
    int  len;
    int  rc = 1;
    char * pos = strchr(name, '.');
    char * pos2;
    propname_t * prop = known_props;

    if (pos) {
        pos2 = strchr(pos+1, '.');
        if (pos2)
            pos = pos2;
        len = (int)(pos-name)+1;
    } else {
        len = (int)strlen(name);
    }
    while (prop->name) {
        if (!strncasecmp(prop->name, name, len)) {
            memcpy(name, prop->name, len);
            rc = 0;
            break;
        }
        prop++;
    }
    return rc;
}


/*
 * Convert a properties object to a serialized properties map.
 *
 * Properties are written to the buffer at the used position in the buffer which is updated.
 *
 * The properties are written to a single
 * @param props   The properties object to serialize
 * @param mapbuf  The buffer to place the map.
 * @return A return code 0=good
 */
int ism_common_serializeProperties(ism_prop_t * props, concat_alloc_t * mapbuf) {
    return ism_protocol_serializeProperties(props, mapbuf);
}

/*
 * Convert serialized properties to a properties object.
 *
 * Put all fields in the specified buffer (from pos to used) are put into the properties object.
 * The properties object must be created before calling this function.
 * <p>
 * The map field contains its own length
 * @param  The address of the map
 * @return An ISMRC return code: 0=good
 */
XAPI int ism_common_deserializeProperties(concat_alloc_t * mapbuf, ism_prop_t * props) {
    return ism_protocol_deserializeProperties(mapbuf, props);
}

/*
 * Find a property by name in serialized properties.
 *
 * Search a set of properties for the named property, and return a field containing its value.
 * If the named property is not found return a non-zero return code and set the field to
 * a null field.
 * @param mapbuf  The serialized properties buffer
 * @param name    The property name to search for
 * @param field   The field to return
 * @return A return code 0=found, 1=not found
 */
XAPI int ism_common_findPropertyName(concat_alloc_t * mapbuf, const char * name, ism_field_t * field) {
    return ism_findPropertyName(mapbuf, name, field);
}

/*
 * Thunk to proxy implementation
 */
XAPI int ism_common_findPropertyID(concat_alloc_t * mapbuf, int id, ism_field_t * field) {
    return ism_findPropertyNameIndex(mapbuf, id, field);
}

/*
 * Thunk to proxy implementation
 */
XAPI void ism_common_putProperty(concat_alloc_t * mapbuf, const char * name, ism_field_t * field) {
    ism_protocol_putPropertyValue(mapbuf, name, field);
}

/*
 * Thunk to proxy implementation
 */
XAPI void ism_common_putPropertyID(concat_alloc_t * mapbuf, int id, ism_field_t * field) {
    ism_protocol_putPropertyValue(mapbuf, (const char *)(uintptr_t)id, field);
}
