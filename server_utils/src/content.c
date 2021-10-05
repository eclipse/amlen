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

#include <ismutil.h>
#include <imacontent.h>
#include <fendian.h>

/*
 * Define JSON type designation for conversion from MTYPE_MQTT_TextArray
 * to JMS StreamMessage.
 */
#define JSON_MQTT_TEXTARRAY 0x9E

/*
 * Define JSON type designation for conversion from MTYPE_MQTT_TextObject
 * to JMS MapMessage.
 */
#define JSON_MQTT_TEXTOBJECT 0x9F

/*
 * Mask for values and max lengths
 */
enum S_Mask {
    S_BooleanValue  = 0x01,
    S_FloatValue    = 0x01,
    S_DoubleValue   = 0x01,
    S_CharValue     = 0x03,
    S_ShortValue    = 0x03,
    S_ByteValue     = 0x0f,
    S_IntValue      = 0x07,
    S_ByteArrayValue= 0x07,
    S_LongValue     = 0x0f,
    S_NameValue     = 0x1f,

    S_NameMax       = 30,
    S_StringMax     = 60,
    S_ByteMax       = 20
};


/*
 * Type enumerations
 */
enum S_Type {
    STYPE_Bad,
    STYPE_Null,
    STYPE_String,
    STYPE_StrLen,
    STYPE_Boolean,
    STYPE_Byte,
    STYPE_UByte,
    STYPE_Short,
    STYPE_UShort,
    STYPE_Char,
    STYPE_Int,
    STYPE_UInt,
    STYPE_Long,
    STYPE_ULong,
    STYPE_Float,
    STYPE_Double,
    STYPE_BArray,
    STYPE_Name,
    STYPE_NameLen,
    STYPE_ID,
    STYPE_Array,
    STYPE_Time,
    STYPE_User,
    STYPE_Map,
    STYPE_Xid
};


/*
 * Map from the type byte to the type.
 * The data value is the offset of this type byte from the start of the type range.
 *
 * The User type actually starts at 0x20, but the zero length user type is invalid since the user type
 * always has a type byte.  This makes the 0x20 encoding invalid as that is the ASCII space.
 */
enum S_Type FieldTypes [] = {
    STYPE_Bad,     STYPE_Null,    STYPE_Boolean, STYPE_Boolean, STYPE_Float,   STYPE_Float,   STYPE_Double,  STYPE_Double,  /* 00 - 07 */
    STYPE_Time,    STYPE_Time,    STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 08 - 0F */
    STYPE_Byte,    STYPE_UByte,   STYPE_Bad,     STYPE_Bad,     STYPE_ID,      STYPE_ID,      STYPE_ID,      STYPE_ID,      /* 10 - 17 */
    STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Char,    STYPE_Char,    STYPE_Char,    STYPE_Char,    /* 17 - 1F */
    STYPE_Bad,     STYPE_User,    STYPE_User,    STYPE_User,    STYPE_User,    STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 20 - 27 */
    STYPE_StrLen,  STYPE_StrLen,  STYPE_StrLen,  STYPE_StrLen,  STYPE_StrLen,  STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 28 - 2F */
    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    /* 30 - 37 */
    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    /* 38 - 3F */
    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Byte,    STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 40 - 47 */
    STYPE_Map,     STYPE_Map,     STYPE_Map,     STYPE_Map,     STYPE_Map,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 48 - 4F */
    STYPE_Short,   STYPE_Short,   STYPE_Short,   STYPE_Bad,     STYPE_UShort,  STYPE_UShort,  STYPE_UShort,  STYPE_Bad,     /* 50 - 57 */
    STYPE_NameLen, STYPE_NameLen, STYPE_NameLen, STYPE_NameLen, STYPE_NameLen, STYPE_Bad,     STYPE_Xid,     STYPE_Bad,     /* 58 - 5F */
    STYPE_Int,     STYPE_Int,     STYPE_Int,     STYPE_Int,     STYPE_Int,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 60 - 67 */
    STYPE_UInt,    STYPE_UInt,    STYPE_UInt,    STYPE_UInt,    STYPE_UInt,    STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 68 - 6F */
    STYPE_Long,    STYPE_Long,    STYPE_Long,    STYPE_Long,    STYPE_Long,    STYPE_Long,    STYPE_Long,    STYPE_Long,    /* 70 - 77 */
    STYPE_Long,    STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 78 - 7F */
    STYPE_ULong,   STYPE_ULong,   STYPE_ULong,   STYPE_ULong,   STYPE_ULong,   STYPE_ULong,   STYPE_ULong,   STYPE_ULong,   /* 80 - 87 */
    STYPE_ULong,   STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 88 - 8F */
    STYPE_BArray,  STYPE_BArray,  STYPE_BArray,  STYPE_BArray,  STYPE_BArray,  STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 90 - 97 */
    STYPE_Array,   STYPE_Array,   STYPE_Array,   STYPE_Array,   STYPE_Array,   STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* 98 - 9F */
    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    /* A0 - A7 */
    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    /* A8 - AF */
    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    /* B0 - B7 */
    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Name,    STYPE_Bad,     /* B8 - BF */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* C0 - C7 */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* C8 - CF */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* D0 - D7 */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* D8 - DF */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* E0 - E7 */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* E8 - EF */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  /* F0 - F7 */
    STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_String,  STYPE_Bad,     STYPE_Bad,     STYPE_Bad,     /* F8 - FF */
};


/*
 * The table gives the length to skip over to find the next item.  This includes
 * the length of the type byte itself.  The value of zero indicates an invalid type.
 * Note the for types User, StrLen, BArray, Xid, and Map it is necessary to skip over the content length as well.
 */
static char FieldLen [] = {
    0, 1, 1, 1, 1, 5, 1, 9, 1, 9, 0, 0, 0, 0, 0, 0,  /* 00 - 0F */
    2, 2, 0, 0, 0, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,  /* 10 - 1F */
    0, 2, 3, 4, 5, 0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 0,  /* 20 - 2F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 30 - 3F */
    1, 1, 1, 1, 1, 0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 0,  /* 40 - 4F */
    1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 0, 2, 0,  /* 50 - 5F */
    1, 2, 3, 4, 5, 0, 0, 0, 0, 1, 2, 3, 4, 5, 0, 0,  /* 60 - 6F */
    1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0,  /* 70 - 7F */
    1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0,  /* 80 - 8F */
    1, 2, 3, 4, 5, 0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 0,  /* 90 - 9F */
    1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,  /* A0 - AF */
   17,18,19,20,21,22,23,24,25,26,27,28,29,30,31, 0,  /* B0 - BF */
    1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,  /* C0 - CF */
   17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,  /* D0 - DF */
   33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,  /* E0 - EF */
   49,50,51,52,53,54,55,56,57,58,59,60,61, 0, 0, 0,  /* F0 - FF */
};


/*
 * Forward references within this source module
 */
extern int ism_protocol_getByteValue(ism_actionbuf_t * action, int otype);
extern int ism_protocol_getShortValue(ism_actionbuf_t * action, int otype);
extern int ism_protocol_getIntValue(ism_actionbuf_t * action, int otype);
extern int ism_protocol_getInteger(ism_actionbuf_t * action, int otype);
extern int64_t ism_protocol_getLongValue(ism_actionbuf_t * action, int otype);
extern float ism_protocol_getFloatValue(ism_actionbuf_t * action, int otype);
extern double ism_protocol_getDoubleValue(ism_actionbuf_t * action, int otype);
extern const char * ism_protocol_getStringValue(ism_actionbuf_t * action, int otype);
extern int ism_protocol_getByteArrayValue(ism_actionbuf_t * action, ism_field_t * var, int otype);
extern int ism_protocol_getUserValue(ism_actionbuf_t * action, ism_field_t * var, int otype);
extern int ism_protocol_getMapValue(ism_actionbuf_t * action, ism_field_t * var, int otype);
extern int ism_protocol_getXidValue(ism_actionbuf_t * action, ism_field_t * var);
extern const char * ism_protocol_getNameValue(ism_actionbuf_t * action, int otype);
extern void ism_protocol_ensureBuffer(ism_actionbuf_t * buf, int len);
static void ensureBuffer(ism_actionbuf_t * buf, int len);

/*
 * Exported interface to ensure buffer
 */
void ism_protocol_ensureBuffer(ism_actionbuf_t * buf, int len) {
    ensureBuffer(buf, len);
}

/*
 * Exported interface to reserve space in the buffer and return position of that space
 */
int ism_protocol_reserveBuffer(ism_actionbuf_t * buf, int len) {
    int pos = buf->used;
    ensureBuffer(buf, len);
    buf->used += len;
    return pos;
}

/*
 * Get a byte value
 */
int ism_protocol_getByteValue(ism_actionbuf_t * action, int otype) {
    if (otype >= S_Byte)
        return otype - S_Byte;

    if ((action->pos) >= action->used) {
        action->pos = action->used;
        return 0;
    } else {
        return action->buf[action->pos++];
    }
}


/*
 * Get a short value
 */
int ism_protocol_getShortValue(ism_actionbuf_t * action, int otype) {
    int len = otype&S_ShortValue;
    int val = 0;
    if ((action->pos + len) > action->used) {
        action->pos = action->used;
    } else {
        while (len-- > 0) {
            val = (val<<8) + (uint8_t)(action->buf[action->pos++]);
        }
    }
    return val;
}


/*
 * Get an integer value
 */
int ism_protocol_getIntValue(ism_actionbuf_t * action, int otype) {
    int len = otype&S_IntValue;
    int val = 0;
    if ((action->pos + len) > action->used) {
        action->pos = action->used;
    } else {
        while (len-- > 0) {
            val = (val<<8) + (uint8_t)(action->buf[action->pos++]);
        }
    }
    return val;
}


/*
 * Get an integer.
 * The integer can be stored in any of the types which convert to integer
 */
int ism_protocol_getInteger(ism_actionbuf_t * action, int otype) {
    int val;
    int xtype = FieldTypes[otype];
    switch (xtype) {
    case STYPE_Boolean: val = otype&1;                                    break;
    case STYPE_Byte:    val = ism_protocol_getByteValue(action, otype);            break;
    case STYPE_UByte:   val = (uint8_t)ism_protocol_getByteValue(action, otype);   break;
    case STYPE_Short:   val = ism_protocol_getShortValue(action, otype);           break;
    case STYPE_UShort:  val = (uint16_t)ism_protocol_getShortValue(action, otype); break;
    case STYPE_Char:    val = ism_protocol_getShortValue(action, otype);           break;
    case STYPE_UInt:
    case STYPE_Int:     val = ism_protocol_getIntValue(action, otype);             break;
    default:            val = -1;
    break;
    }
    return val;
}


/*
 * Get a long value
 */
int64_t ism_protocol_getLongValue(ism_actionbuf_t * action, int otype) {
    int len = otype&S_LongValue;
    int64_t val = 0;
    if ((action->pos + len) > action->used) {
        action->pos = action->used;
    } else {
        while (len-- > 0) {
            val = (val<<8) + (uint8_t)(action->buf[action->pos++]);
        }
    }
    return val;
}


/*
 * Get a float value
 */
float ism_protocol_getFloatValue(ism_actionbuf_t * action, int otype) {
    if ((otype&S_FloatValue) == 0) {
        return (float)0.0;
    } else {
        ism_field_t var;
        var.val.i = ism_protocol_getIntValue(action, 4);
        return var.val.f;
    }
}


/*
 * Get a double value
 */
double ism_protocol_getDoubleValue(ism_actionbuf_t * action, int otype) {
    if ((otype&S_DoubleValue) == 0) {
        return 0.0;
    } else {
        ism_field_t var;
        var.val.l = ism_protocol_getLongValue(action, 8);
        return var.val.d;
    }
}


/*
 * Get a string value
 */
const char * ism_protocol_getStringValue(ism_actionbuf_t * action, int otype) {
    const char * ret;
    int  len;
    int  xtype = FieldTypes[otype];
    switch (xtype) {
    case STYPE_StrLen:
        len = ism_protocol_getIntValue(action, otype-S_StrLen);
        break;
    case STYPE_String:
        len = otype&0x3f;
        break;
    default:
        return NULL;
    }
    if ((action->pos + len) > action->used) {
        action->pos = action->used;
        return NULL;
    } else {
        ret = action->buf+action->pos;
        action->pos += len;
        return ret;
    }
}


/*
 * Get a byte array
 */
int ism_protocol_getByteArrayValue(ism_actionbuf_t * action, ism_field_t * var, int otype) {
    int  len;
    int  xtype = FieldTypes[otype];
    switch (xtype) {
    case STYPE_BArray:
        len = ism_protocol_getIntValue(action, otype-S_ByteArray);
        break;
    case STYPE_Null:
        var->type = VT_Null;
        return 0;
    default:
        var->type = VT_Null;
        return 1;
    }
    if ((action->pos + len) > action->used) {
        action->pos = action->used;
        var->type = VT_Null;
        return 2;
    } else {
        var->type = VT_ByteArray;
        var->val.s = action->buf+action->pos;
        var->len = len;
        action->pos += len;
        return 0;
    }
}


/*
 * Get a user value
 */
int ism_protocol_getUserValue(ism_actionbuf_t * action, ism_field_t * var, int otype) {
    int  len;
    int  xtype = FieldTypes[otype];
    if (xtype != STYPE_User) {
        var->type = VT_Null;
        return 1;
    }
    len = ism_protocol_getIntValue(action, otype&S_IntValue);
    var->type = VT_ByteArray;
    var->val.s = action->buf+action->pos+1;
    var->len = len-1;
    action->pos += len;
    return 0;
}


/*
 * Get a map value
 */
int ism_protocol_getMapValue(ism_actionbuf_t * action, ism_field_t * var, int otype) {
    int  len;
    int  xtype = FieldTypes[otype];
    if (xtype != STYPE_Map) {
        var->type = VT_Null;
        return 1;
    }
    len = ism_protocol_getIntValue(action, otype&S_IntValue);
    var->type = VT_Map;
    var->val.s = action->buf+action->pos;
    var->len = len;
    action->pos += len;
    return 0;
}


/*
 * Get a xid value in wire format
 */
int ism_protocol_getXidValue(ism_actionbuf_t * action, ism_field_t * var) {
    int  len;
    len = action->buf[action->pos++];
    var->type = VT_Xid;
    var->val.s = action->buf+action->pos;
    var->len = len;
    action->pos += len;
    return 0;
}


/*
 * Get a name value
 */
const char * ism_protocol_getNameValue(ism_actionbuf_t * action, int otype) {
    const char * ret;
    int  len;
    int  xtype = FieldTypes[otype];
    switch (xtype) {
    case STYPE_NameLen:
        len = ism_protocol_getIntValue(action, otype-S_StrLen);
        break;
    case STYPE_Name:
        len = otype&S_NameValue;
        break;
    default:
        return NULL;
    }
    if ((action->pos + len) > action->used) {
        action->pos = action->used;
        return NULL;
    } else {
        ret = action->buf+action->pos;
        action->pos += len;
        return ret;
    }
}


/*
 * Get an object value.
 * This is the only externally defined get function and uses the other get
 * functions.
 */
int ism_protocol_getObjectValue(ism_actionbuf_t * action, ism_field_t * f) {
    int otype, xtype, alen;
    if (action->pos >= action->used) {
        f->type = VT_Null;
        return -1;
    }
    otype = (uint8_t)action->buf[action->pos++];
    xtype = FieldTypes[otype];
    switch (xtype) {
    case STYPE_Null:    f->type = VT_Null;                                                            break;
    case STYPE_Boolean: f->type = VT_Boolean;  f->val.i = otype&1;                                    break;
    case STYPE_Byte:    f->type = VT_Byte;     f->val.i = ism_protocol_getByteValue(action, otype);            break;
    case STYPE_UByte:   f->type = VT_UByte;    f->val.i = (uint8_t)ism_protocol_getByteValue(action, otype);   break;
    case STYPE_Short:   f->type = VT_Short;    f->val.i = ism_protocol_getShortValue(action, otype);           break;
    case STYPE_UShort:  f->type = VT_UShort;   f->val.i = (uint16_t)ism_protocol_getShortValue(action, otype); break;
    case STYPE_Char:    f->type = VT_Char;     f->val.i = ism_protocol_getShortValue(action, otype);           break;
    case STYPE_UInt:    f->type = VT_UInt;     f->val.i = ism_protocol_getIntValue(action, otype);             break;
    case STYPE_Int:     f->type = VT_Integer;  f->val.i = ism_protocol_getIntValue(action, otype);             break;
    case STYPE_Time:
    case STYPE_ULong:   f->type = VT_ULong;    f->val.l = ism_protocol_getLongValue(action, otype);            break;
    case STYPE_Long:    f->type = VT_Long;     f->val.l = ism_protocol_getLongValue(action, otype);            break;
    case STYPE_Float:   f->type = VT_Float;    f->val.f = ism_protocol_getFloatValue(action, otype);           break;
    case STYPE_Double:  f->type = VT_Double;   f->val.d = ism_protocol_getDoubleValue(action, otype);          break;
    case STYPE_StrLen:
    case STYPE_String:  f->type = VT_String;   f->val.s = (char *)ism_protocol_getStringValue(action, otype);  break;
    case STYPE_BArray:  ism_protocol_getByteArrayValue(action, f, otype);                                      break;
    case STYPE_NameLen:
    case STYPE_Name:    f->type = VT_Name;     f->val.s = (char *)ism_protocol_getNameValue(action, otype);    break;
    case STYPE_ID:      f->type = VT_NameIndex;f->val.i = ism_protocol_getShortValue(action, otype);           break;
    case STYPE_User:    ism_protocol_getUserValue(action, f, otype);                                           break;
    case STYPE_Map:     ism_protocol_getMapValue(action, f, otype);                                            break;
    case STYPE_Xid:     ism_protocol_getXidValue(action, f);                                                   break;
    case STYPE_Array:
        /* Skip over the array and return null */
        alen = ism_protocol_getIntValue(action, otype);
        while (alen-- > 0) {
            ism_field_t var;
            ism_protocol_getObjectValue(action, &var);
        }
        f->type = VT_Null;
        break;
    default:
        f->type = VT_Null;
        return -2;
    }
    if (action->pos < 0)
        return -3;
    return 0;
}


/*
 * Find a property by name
 */
int ism_findPropertyName(ism_actionbuf_t * props, const char * name, ism_field_t * f) {
    int   otype;
    int   xtype;
    concat_alloc_t action;
    ism_field_t field;
    char * fname;

    if ((uint64_t)(uintptr_t)name < 0x10000)
        return ism_findPropertyNameIndex(props, (int)(uintptr_t)name, f);

    if (!props || !props->buf)
        return 1;
    action = *props;
    action.pos = 0;

    memset(f, 0, sizeof(ism_field_t));

    while (action.pos < action.used) {
        otype = (uint8_t)action.buf[action.pos++];
        xtype = FieldTypes[otype];
        switch (xtype) {
        case STYPE_Bad:
            return 1;
        case STYPE_Name:
        case STYPE_NameLen:
            fname = (char *)ism_protocol_getNameValue(&action, otype);
            if (!strcmp(name, fname)) {
                ism_protocol_getObjectValue(&action, f);
                return 0;
            }
            break;
        case STYPE_StrLen:
            ism_protocol_getStringValue(&action, otype);
            break;
        case STYPE_BArray:
            ism_protocol_getByteArrayValue(&action, &field, otype);
            break;
        case STYPE_Map:
            ism_protocol_getMapValue(&action, &field, otype);
            break;
        case STYPE_Xid:
            ism_protocol_getXidValue(&action, &field);
            break;
        case STYPE_User:
            ism_protocol_getUserValue(&action, &field, otype);
            break;
        default:
            action.pos += FieldLen[otype]-1;
            break;
        }
    }
    return 1;
}


/*
 * Find a property by name index
 */
int ism_findPropertyNameIndex(ism_actionbuf_t * props, int index, ism_field_t * f) {
    int   otype;
    int   xtype;
    ism_field_t field;
    concat_alloc_t action;
    int   findex;

    if (!props || !props->buf)
        return 1;
    action = *props;
    action.pos = 0;

    memset(f, 0, sizeof(ism_field_t));

    while (action.pos < action.used) {
        otype = (uint8_t)action.buf[action.pos++];
        xtype = FieldTypes[otype];
        switch (xtype) {
        case STYPE_Bad:
            return 1;
        case STYPE_ID:
            findex = ism_protocol_getShortValue(&action, otype);
            if (findex == index) {
                ism_protocol_getObjectValue(&action, f);
                return 0;
            }
            break;
        case STYPE_StrLen:
            ism_protocol_getStringValue(&action, otype);
            break;
        case STYPE_BArray:
            ism_protocol_getByteArrayValue(&action, &field, otype);
            break;
        case STYPE_Map:
            ism_protocol_getMapValue(&action, &field, otype);
            break;
        case STYPE_Xid:
            ism_protocol_getXidValue(&action, &field);
            break;
        case STYPE_User:
            ism_protocol_getUserValue(&action, &field, otype);
            break;
        default:
            action.pos += FieldLen[otype]-1;
            break;
        }
    }
    return 1;
}


/*
 * Put out a null value.
 */
void ism_protocol_putNullValue(ism_actionbuf_t * map) {
    map->buf[map->used++] = S_NullString;
}


/*
 * Put a boolean value.
 * Two two values of boolean each have a byte encoding them.
 */
void ism_protocol_putBooleanValue(ism_actionbuf_t * map, int val) {
    map->buf[map->used++] = val ? (char)(S_Boolean+1) : (char)S_Boolean;
}


/*
 * Put out a byte value
 * The values 0-20 have special encodings so that type, length, and value
 * are all encoded in a single byte.
 */
void ism_protocol_putByteValue(ism_actionbuf_t * map, int val) {
    if (val >= 0 && val <= S_ByteMax) {
        map->buf[map->used++] = (char)(S_Byte + val);
    } else {
        map->buf[map->used++] = (char)(S_ByteLen);
        map->buf[map->used++] = (char)val;
    }
}


/*
 * Put out an unsigned byte value.
 * There is no special enooding for this as it is not used in Java.
 */
void ism_protocol_putUByteValue(ism_actionbuf_t * map, int val) {
        map->buf[map->used++] = (char)(S_UByteLen);
        map->buf[map->used++] = (char)val;
}


/*
 * Put a short value.
 */
void ism_protocol_putShortValue(ism_actionbuf_t * map, int val) {
    ism_protocol_putSmallValue(map, val&0xffff, S_Short);
}


/*
 * Put out a small value.
 * This is used to implement several of the types up to the size of an integer.
 * Leading zero bytes are suppressed
 */
void ism_protocol_putSmallValue(ism_actionbuf_t * map, int val, int otype) {
    int count = 0;
    int savepos = map->used++;
    if (val != 0) {
        int shift = 24;
	    while (shift >= 0) {
	        int bval = (int)(val >> shift);
	        if (count == 0 && bval != 0)
	            count = (shift >> 3) + 1;
	        if (count > 0)
	            map->buf[map->used++] = (char)bval;
	        shift -= 8;
	    }
    }
    map->buf[savepos] = (char)(otype + count);
}

/*
 * Put out an int value.
 * Leading zero byte are suppressed.
 */
void ism_protocol_putIntValue(ism_actionbuf_t * map, int val) {
    int count = 0;
    int savepos = map->used++;

    if (val != 0) {
        int shift = 24;
	    while (shift >= 0) {
	        int bval = (int)(val >> shift);
	        if (count == 0 && bval != 0)
	            count = (shift >> 3) + 1;
	        if (count > 0)
	            map->buf[map->used++] = (char)bval;
	        shift -= 8;
	    }
    }
    map->buf[savepos] = (char)(S_Int + count);
}

/*
 * Put out a long value.
 * Leading zero bytes are suppressed.
 */
void ism_protocol_putLongValue(ism_actionbuf_t * map, int64_t val) {
    int count = 0;
    int savepos = map->used++;

    if (val != 0) {
        int shift = 56;
	    while (shift >= 0) {
	        int bval = (int)(val >> shift);
	        if (count == 0 && bval != 0)
	            count = (shift >> 3) + 1;
	        if (count > 0)
	            map->buf[map->used++] = (char)bval;
	        shift -= 8;
	    }
    }
    map->buf[savepos] = (char)(S_Long + count);
}


/*
 * Put out a float value.
 * All four bytes of a float value are put out.
 */
void ism_protocol_putFloatValue(ism_actionbuf_t * map, float val) {
    union {int32_t i; float f;} temp;
    int   ival;

    if (val == 0.0) {
        map->buf[map->used++] = S_Float;
    } else {
        map->buf[map->used++] = S_Float+1;
    }
    temp.f = val;
    ival = temp.i;
    map->buf[map->used++] = (char)(ival>>24);
    map->buf[map->used++] = (char)(ival>>16);
    map->buf[map->used++] = (char)(ival>>8);
    map->buf[map->used++] = (char)(ival);
}

/*
 * Put out a double value.
 * All 8 bytes of a double are put out
 */
void ism_protocol_putDoubleValue(ism_actionbuf_t * map, double val) {
    union {int64_t l; double d;} temp;
    int64_t   ival;

    if (val == 0.0) {
        map->buf[map->used++] = S_Double;
    } else {
        map->buf[map->used++] = S_Double+1;
    }
    temp.d = val;
    ival = temp.l;
    map->buf[map->used++] = (char)(ival>>56);
    map->buf[map->used++] = (char)(ival>>48);
    map->buf[map->used++] = (char)(ival>>40);
    map->buf[map->used++] = (char)(ival>>32);
    map->buf[map->used++] = (char)(ival>>24);
    map->buf[map->used++] = (char)(ival>>16);
    map->buf[map->used++] = (char)(ival>>8);
    map->buf[map->used++] = (char)(ival);
}


/*
 * Put out a string value.
 * The value is kept with its trailing null
 */
void ism_protocol_putStringValue(ism_actionbuf_t * map, const char * str) {
    if (str == NULL) {
        ism_protocol_putNullValue(map);
    } else {
        int  len = (int)strlen(str)+1;
        ensureBuffer(map, len+2);
        if (len <= S_StringMax) {
            map->buf[map->used++] = (char)(S_String+len);
        } else {
            ism_protocol_putSmallValue(map, len, S_StrLen);
        }
        memcpy(map->buf + map->used, str, len);
        map->used += len;
    }
}


/*
 * Put out a string value with a length.
 * A trailing null is added.
 */
void ism_protocol_putStringLenValue(ism_actionbuf_t * map, const char * str, int len) {
    if (str == NULL || len<0) {
        ism_protocol_putNullValue(map);
    } else {
        ensureBuffer(map, len+1);
        if (len < S_StringMax) {
            map->buf[map->used++] = (char)(S_String+len+1);
        } else {
            ism_protocol_putSmallValue(map, len+1, S_StrLen);
        }
        memcpy(map->buf + map->used, str, len);
        map->used += len;
        map->buf[map->used++] = 0;
    }
}

/*
 * Put a name value.
 * A name value is just like a string, but is the name of a map or property
 */
void ism_protocol_putNameValue(ism_actionbuf_t * map, const char * str) {
    int  len = (int)strlen(str)+1;
    ensureBuffer(map, len+3);
    if (len <= S_NameMax) {
        map->buf[map->used++] = (char)(S_Name+len);
    } else {
        ism_protocol_putSmallValue(map, len, S_NameLen);
    }
    memcpy(map->buf + map->used, str, len);
    map->used += len;
}

/*
 * Put a name value with name which is not null terminated.
 * A name value is just like a string, but is the name of a map or property
 */
void ism_protocol_putNameLenValue(ism_actionbuf_t * map, const char * str, int len) {
    int flen = len+1;
    ensureBuffer(map, len+4);
    if (flen <= S_NameMax) {
        map->buf[map->used++] = (char)(S_Name+flen);
    } else {
        ism_protocol_putSmallValue(map, flen, S_NameLen);
    }
    memcpy(map->buf + map->used, str, len);
    map->buf[map->used+len] = 0;
    map->used += flen;
}


/*
 * Simple form of put name index.
 * This assume we have a single byte ID and we have checked for capasity.
 */
void ism_protocol_putNameIndex(ism_actionbuf_t * map, int id) {
    map->buf[map->used++] = S_ID+1;
    map->buf[map->used++] = (char)id;
}

/*
 * Extend the buffer if required
 */
static void ensureBuffer(ism_actionbuf_t * buf, int len) {
    if (buf->used + len + 7 > buf->len) {
        int newsize = 32*1024;
        while (newsize < buf->used + len + 7)
            newsize *= 2;
        if (buf->inheap) {
            buf->buf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,6),buf->buf, newsize);
        } else {
            char * tmpbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,7),newsize);
            if (tmpbuf && buf->used) {
                memcpy(tmpbuf, buf->buf, buf->used > buf->len ? buf->len : buf->used);
            }
            buf->buf = tmpbuf;
        }
        buf->inheap = 1;
        buf->len = newsize;
    }
}


/*
 * Put a byte array into the content
 */
void ism_protocol_putByteArrayValue(ism_actionbuf_t * map, const char * str, int len) {
    ensureBuffer(map, len);
    ism_protocol_putSmallValue(map, len, S_ByteArray);
    memcpy(map->buf + map->used, str, len);
    map->used += len;
}

/*
 * Put a byte array into the content
 */
void ism_protocol_putXidValueF(ism_actionbuf_t * map, const char * str, int len) {
    ensureBuffer(map, len+2);
    map->buf[map->used++] = S_Xid;
    map->buf[map->used++] = (uint8_t)len;
    memcpy(map->buf + map->used, str, len);
    map->used += len;
}

/*
 * Put a byte array into the content
 */
void ism_protocol_putXidValue(ism_actionbuf_t * map, const ism_xid_t * xid) {
    int len = xid->gtrid_length + xid->bqual_length + 6;
    int memint;
    if (len > 256) {
        ism_protocol_putNullValue(map);
        return;
    }
    ensureBuffer(map, len+2);
    map->buf[map->used++] = S_Xid; /* BEAM suppression: accessing beyond memory */
    map->buf[map->used++] = (uint8_t)len; /* BEAM suppression: accessing beyond memory */
    memint = endian_int32(xid->formatID);
    memcpy(map->buf + map->used, &memint, 4); /* BEAM suppression: accessing beyond memory */
    map->buf[map->used+4] = (uint8_t)xid->gtrid_length; /* BEAM suppression: accessing beyond memory */
    map->buf[map->used+5] = (uint8_t)xid->bqual_length; /* BEAM suppression: accessing beyond memory */
    memcpy(map->buf + map->used + 6, xid->data, len-6); /* BEAM suppression: accessing beyond memory */
    map->used += len;
}


/*
 * Put an existing map object into the content
 */
void ism_protocol_putMapValue(ism_actionbuf_t * map, const char * str, int len) {
    ensureBuffer(map, len);
    ism_protocol_putSmallValue(map, len, S_Map);
    memcpy(map->buf + map->used, str, len);
    map->used += len;
}



/**
 * Put a map value from properties.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map    The map or message to put to
 * @param props  The properties object
 */
XAPI void ism_protocol_putMapProperties(ism_actionbuf_t * map, ism_prop_t * props) {
	int i;
	int sizepos = map->used;
	int len;
	const char * name;
	ism_field_t field;

	/* Ensure we have some buffer */
	ism_protocol_ensureBuffer(map, 16);

	/*
	 * Reserve space for data size
	 * TODO: Once size is known, we should check that it fits in the allotted space.
	 */
	map->buf[map->used] = (char)(S_Map+3);
	map->used += 4;
	for (i=0; ism_common_getPropertyIndex(props, i, &name)==0; i++) {
	    ism_common_getProperty(props, name, &field);
	    ism_protocol_putNameValue(map, name);
	    ism_protocol_putObjectValue(map, &field);
	}

	len = map->used - sizepos - 4;
	map->buf[sizepos+1] = (char)(len >> 16);
	map->buf[sizepos+2] = (char)((len >> 8) & 0xff);
	map->buf[sizepos+3] = (char)(len & 0xff);
}

/*
 * Put out a JSON parsed object as a map.
 * This is used when converting MQTT JSON objects to JMS map or stream objects
 */
void ism_protocol_putJSONMapValue(ism_actionbuf_t * map, ism_json_entry_t * ent, int count, int inarray) {
    int entnum;
    int sizepos = map->used;
    int len;

    /* Ensure we have some buffer */
    ism_protocol_ensureBuffer(map, 16);

    /*
     * Reserve space for data size
     * TODO: Once size is known, we should check that it fits in the allotted space.
     */
    map->buf[map->used] = (char)(S_Map+3);
    map->used += 4;
    map->buf[map->used++] = inarray ? JSON_MQTT_TEXTARRAY : JSON_MQTT_TEXTOBJECT;

    for (entnum = 1; entnum < count; entnum++, ent++) {
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array) {
            entnum += ent->count;
            ent += ent->count;
        } else {
            ism_field_t field;
            if (ent->name) {
                ism_protocol_putNameValue(map, ent->name);
            }
            switch (ent->objtype) {
	        case JSON_String:      /* JSON string, value is UTF-8              */
	            field.type = VT_String;
	            field.val.s = (char *)ent->value;
	            break;
	        case JSON_Integer:     /* Number with no decimal point             */
	            field.type = VT_Integer;
	            field.val.i = ent->count;
	            break;
	        case JSON_Number:      /* Number which is too big or has a decimal */
	            field.type = VT_Double;
	            field.val.d = strtod(ent->value, NULL);
	            break;
	        case JSON_True:        /* JSON true, value is not set              */
	            field.type = VT_Boolean;
	            field.val.i = 1;
	            break;
	        case JSON_False:       /* JSON false, value is not set             */
	            field.type = VT_Boolean;
	            field.val.i = 0;
	            break;
	        case JSON_Null:        /* JSON null, value is not set              */
	        default:
	            field.type = VT_Null;
	            break;
	        }
	        ism_protocol_putObjectValue(map, &field);
        }
    }
    len = map->used - sizepos - 4;
    map->buf[sizepos+1] = (char)(len >> 16);
    map->buf[sizepos+2] = (char)((len >> 8) & 0xff);
    map->buf[sizepos+3] = (char)(len & 0xff);
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
int ism_protocol_serializeProperties(ism_prop_t * props, concat_alloc_t * mapbuf) {
    int i;
    const char * name;
    ism_field_t field;

    for (i=0; ism_common_getPropertyIndex(props, i, &name)==0; i++) {
        ism_common_getProperty(props, name, &field);
        ism_protocol_putNameValue(mapbuf, name);
        ism_protocol_putObjectValue(mapbuf, &field);
    }
    return 0;
}

/*
 * Convert serialized properties to a properties object.
 *
 * Put all fields in the specified buffer (from pos to used) are put into the properties object.
 * The properties object must be created before calling this function.
 * @param  mapbuf The serialized properties
 * @param  props  The properties object
 * @return A return code: 0=good, ISMRC_PropertiesNotValid if bad
 */
int ism_protocol_deserializeProperties(concat_alloc_t * mapbuf, ism_prop_t * props) {
    ism_field_t obj;
    while (mapbuf->pos < mapbuf->used) {
        int rc = ism_protocol_getObjectValue(mapbuf, &obj);
        if (!rc && obj.type == VT_Name) {
            const char * key = obj.val.s;
            ism_protocol_getObjectValue(mapbuf, &obj);
            ism_common_setProperty(props, key, &obj);
        }
    }

    if (mapbuf->pos != mapbuf->used) {
        ism_common_setError(ISMRC_PropertiesNotValid);
        return ISMRC_PropertiesNotValid;
    }
    return 0;
}

/*
 * Put a property
 */
void ism_protocol_putPropertyValue(ism_actionbuf_t * map, const char * name, ism_field_t * var) {
    if (name) {
        if ((uint64_t)(uintptr_t)name < 0x10000)
            ism_protocol_putNameIndex(map, (int)(uintptr_t)name);
        else
            ism_protocol_putNameValue(map, name);
    }
    ism_protocol_putObjectValue(map, var);
}



/*
 * Put an object into the content
 */
void ism_protocol_putObjectValue(ism_actionbuf_t * map, ism_field_t * var) {
    ensureBuffer(map, 8);
    switch (var->type) {
    case VT_Null:       ism_protocol_putNullValue(map);                               break;  /*<< Null value                       */
    case VT_String:     ism_protocol_putStringValue(map, var->val.s);                 break;  /**< Type of String (must be zero)    */
    case VT_ByteArray:  ism_protocol_putByteArrayValue(map, var->val.s, var->len);    break;  /**< Type of Byte Array               */
    case VT_Boolean:    ism_protocol_putBooleanValue(map, var->val.i);                break;  /**< Type of boolean                  */
    case VT_Byte:       ism_protocol_putByteValue(map, var->val.i);                   break;  /**< Signed 8 bit value               */
    case VT_UByte:      ism_protocol_putUByteValue(map, var->val.i&0xff);             break;  /**< Unsigned 8 bit value             */
    case VT_Short:      ism_protocol_putShortValue(map, var->val.i);                  break;  /**< Signed 16 bit value              */
    case VT_UShort:     ism_protocol_putSmallValue(map, var->val.i&0xffff, S_UShort); break;  /**< Unsigned 16 bit value            */
    case VT_Integer:    ism_protocol_putIntValue(map, var->val.i);                    break;  /**< Type of Integer                  */
    case VT_UInt:       ism_protocol_putSmallValue(map, var->val.i, S_UInt);          break;  /**< Type of Integer                  */
    case VT_Long:       ism_protocol_putLongValue(map, var->val.l);                   break;  /**< Type of Long Integer             */
    case VT_ULong:      ism_protocol_putLongValue(map, var->val.l);                   break;  /**< Type of Long Integer             */
    case VT_Float:      ism_protocol_putFloatValue(map, var->val.f);                  break;  /**< Type of Float                    */
    case VT_Double:     ism_protocol_putDoubleValue(map, var->val.d);                 break;  /**< Type of Double                   */
    case VT_Name:       ism_protocol_putNameValue(map, var->val.s);                   break;  /**< Name string                      */
    case VT_NameIndex:  ism_protocol_putSmallValue(map, var->val.i&0xffff, S_ID);     break;  /**< Name integer                     */
    case VT_Char:       ism_protocol_putSmallValue(map, var->val.i&0x1ffff, S_Char);  break;  /**< Character value                  */
    case VT_Map:        ism_protocol_putMapValue(map, var->val.s, var->len);          break;  /**< Value is a map                   */
    case VT_Xid:        ism_protocol_putXidValueF(map, var->val.s, var->len);         break;  /**< Value is a XID field             */
    default:
        break;
    };
}


/*
 * Dump a field for debugging
 */
char * ism_protocol_dumpField(ism_field_t * f, char * buf, int len) {
    switch (f->type) {
    case VT_Null:      snprintf(buf, len, "Null");                                          break;
    case VT_String:    snprintf(buf, len, "String %s", f->val.s);                           break;
    case VT_ByteArray: snprintf(buf, len, "ByteArray len=%u", f->len);                      break;
    case VT_Boolean:   snprintf(buf, len, "Boolean %d", f->val.i);                          break;
    case VT_Byte:      snprintf(buf, len, "Byte %d", (int8_t)f->val.i);                     break;
    case VT_UByte:     snprintf(buf, len, "UByte %u", (uint8_t)f->val.i);                   break;
    case VT_Short:     snprintf(buf, len, "Short %d", (int16_t)f->val.i);                   break;
    case VT_UShort:    snprintf(buf, len, "UShort %u", (uint16_t)f->val.i);                 break;
    case VT_Integer:   snprintf(buf, len, "Int %d", f->val.i);                              break;
    case VT_UInt:      snprintf(buf, len, "UInt %u", f->val.i);                             break;
    case VT_Long:      snprintf(buf, len, "Long %lld", (long long)f->val.l);                break;
    case VT_ULong:     snprintf(buf, len, "ULong %llu", (unsigned long long)f->val.l);      break;
    case VT_Float:     snprintf(buf, len, "Float %g", f->val.f);                            break;
    case VT_Double:    snprintf(buf, len, "Double %g", f->val.d);                           break;
    case VT_Name:      snprintf(buf, len, "Name %s", f->val.s);                             break;
    case VT_NameIndex: snprintf(buf, len, "NameX %d", f->val.i);                            break;
    case VT_Char:      snprintf(buf, len, "Char %u", f->val.i);                             break;
    case VT_Map:       snprintf(buf, len, "Map len=%u", f->len);                            break;
    case VT_Unset:     snprintf(buf, len, "Unset");                                         break;
    case VT_Xid:       snprintf(buf, len, "Xid len=%u", f->len);                            break;
    default:           snprintf(buf, len, "Unknown");                                       break;
    }
    return buf;
}

/**
 * Get a 4 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 4 byte integer at the current location or 0 if past the end
 */
int ism_kafka_getInt4(concat_alloc_t * buf) {
    int32_t val;
    if (buf->pos + 4 > buf->used) {
        buf->pos += 4;
        return 0;
    }
    memcpy(&val, buf->buf + buf->pos, 4);
    buf->pos += 4;
    return (int)endian_int32(val);
}

/**
 * Get a 1 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 1 byte integer at the current location or 0 if past the end
 */
int ism_kafka_getInt1(concat_alloc_t * buf) {
    if (buf->pos >= buf->used) {
        buf->pos++;
        return 0;
    }
    return (int)buf->buf[buf->pos++];
}

/**
 * Get a 2 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 2 byte integer at the current location or 0 if past the end
 */
int ism_kafka_getInt2(concat_alloc_t * buf) {
    int16_t val;
    if (buf->pos + 2 > buf->used) {
        buf->pos += 2;
        return 0;
    }
    memcpy(&val, buf->buf + buf->pos, 2);
    buf->pos += 2;
    return (int)(int16_t)endian_int16(val);
}

/**
 * Return the remaining bytes in the buffer
 * @param buf  The buffer
 * @returns    0 if at end of the buffer, >0 more bytes available, <0 past end of buffer
 */
int ism_kafka_more(concat_alloc_t * buf) {
    return buf->used-buf->pos;
}

/**
 * Get an 8 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 8 byte integer at the current location or 0 if past the end
 */
int64_t ism_kafka_getInt8(concat_alloc_t * buf) {
    int64_t val;
    if (buf->pos + 8 > buf->used) {
        buf->pos += 8;
        return 0;
    }
    memcpy(&val, buf->buf + buf->pos, 8);
    buf->pos += 8;
    return (int64_t)endian_int64(val);
}

/**
 * Get a string in kafka format
 * @param buf  The buffer
 * @param str  (output) The location of the string or NULL if none exists.  This is not null terminated.
 * @returns    The length of the string
 */
int ism_kafka_getString(concat_alloc_t * buf, char * * str) {
    int16_t len;
    if (buf->pos + 2 > buf->used) {
        buf->pos += 2;
        *str = NULL;
        return 0;
    }
    len = (((int)(uint8_t)(buf->buf)[buf->pos]<<8) | (uint8_t)(buf->buf)[buf->pos+1]);
    buf->pos += 2;
    if (len < 0) {
        *str = NULL;
        return 0;
    }
    if (buf->pos + len > buf->used) {
        buf->pos += len;
        *str = NULL;
        return 0;
    }
    *str = buf->buf + buf->pos;
    buf->pos += len;
    return (int)len;
}

/**
 * Get a byte array in kafka format
 * @param buf  The buffer
 * @param bytes (output) The location of the byte array or NULL if none exists.
 * @returns    The length of the byte array
 */
int ism_kafka_getBytes(concat_alloc_t * buf, char * * bytes) {
    int32_t len;
    if (buf->pos + 4 > buf->used) {
        buf->pos += 4;
        *bytes = NULL;
        return 0;
    }
    memcpy(&len, buf->buf + buf->pos, 4);
    len = endian_int32(len);
    buf->pos += 4;
    if (len < 0) {
        *bytes = NULL;
        return 0;
    }
    if (buf->pos + len > buf->used) {
        buf->pos += len;
        *bytes = NULL;
        return 0;
    }
    *bytes = buf->buf + buf->pos;
    buf->pos += len;
    return len;
}

/**
 * Put a 4 byte integer in kafka format
 * @param buf  The buffer
 * @param value The value to put
 */
void ism_kafka_putInt4(concat_alloc_t * buf, int value) {
    uint32_t val;
    if (buf->used + 4 > buf->len)
        ism_protocol_ensureBuffer(buf, 4);
    val = endian_int32(value);
    memcpy(buf->buf+buf->used, &val, 4);
    buf->used += 4;
}

/**
 * Put a 4 byte integer in kafka format at the specified location.
 * This is used to fill in a length value unknown when it is originally created.
 * @param buf  The buffer
 * @param offset The position in the buffer.
 * @param value The value to put
 */
void ism_kafka_putInt4At(concat_alloc_t * buf, int offset, int value) {
    uint32_t val;
    if (offset < 0)
        return;
    if (offset + 4 > buf->len)
        ism_protocol_ensureBuffer(buf, (offset+4) - buf->len);
    val = endian_int32(value);
    memcpy(buf->buf+offset, &val, 4);
}

/**
 * Put a 1 byte integer in kafka format
 * @param buf  The buffe
 * @param value The value to put
 */
void ism_kafka_putInt1(concat_alloc_t * buf, int value) {
    if (buf->used >= buf->len)
        ism_protocol_ensureBuffer(buf, 1);
    buf->buf[buf->used++] = (char)value;
}

/**
 * Put a 2 byte integer in kafka format
 * @param buf  The buffer
 * @param value The value to put
 */
void ism_kafka_putInt2(concat_alloc_t * buf, int value) {
    uint16_t val;
    if (buf->used + 2 > buf->len)
        ism_protocol_ensureBuffer(buf, 2);
    val = endian_int16((uint16_t)value);
    memcpy(buf->buf+buf->used, &val, 2);
    buf->used += 2;
}

/**
 * Put an 8 byte integer in kafka format
 * @param buf  The buffer
 * @returns    The value of a 4 byte integer at the current location or 0 if past the end
 */
void ism_kafka_putInt8(concat_alloc_t * buf, int64_t value) {
    uint64_t val;
    if (buf->used + 8 > buf->len)
        ism_protocol_ensureBuffer(buf, 8);
    val = endian_int64(value);
    memcpy(buf->buf+buf->used, &val, 8);
    buf->used += 8;
}

/**
 * Put a 8 byte integer in kafka format at the specified location.
 * This is used to fill in a length value unknown when it is originally created.
 * @param buf  The buffer
 * @param offset The position in the buffer.
 * @param value The value to put
 */
void ism_kafka_putInt8At(concat_alloc_t * buf, int offset, uint64_t val) {
    if (offset < 0)
        return;
    if (offset + 8 > buf->len)
        ism_protocol_ensureBuffer(buf, (offset+8) - buf->len);
    val = endian_int64(val);
    memcpy(buf->buf+offset, &val, 8);
}

/**
 * Put a string in kafka format
 * @param buf  The buffer
 * @param value The location of the string
 * @param len  The length of the string or -1 to indicate it is null terminated
 */
void ism_kafka_putString(concat_alloc_t * buf, const char * value, int len) {
    if (!value) {
        ism_kafka_putInt2(buf, -1);
    } else {
        if (len < 0)
            len = (int) strlen(value);
        ism_kafka_putInt2(buf, len);
        if (buf->used + len > buf->len) {
            ism_protocol_ensureBuffer(buf, len);
        }
        memcpy(buf->buf+buf->used, value, len);
        buf->used += len;
    }
}

/**
 * Put a byte array in kafka format
 * @param buf  The buffer
 * @param value The location of the byte array
 * @param len   The length of the byte array
 */
void ism_kafka_putBytes(concat_alloc_t * buf, const char * value, int len) {
    if (!value || len < 0) {
        ism_kafka_putInt4(buf, -1);
    } else {
        ism_kafka_putInt4(buf, len);
        if (buf->used + len > buf->len) {
            ism_protocol_ensureBuffer(buf, len);
        }
        memcpy(buf->buf+buf->used, value, len);
        buf->used += len;
    }
}


