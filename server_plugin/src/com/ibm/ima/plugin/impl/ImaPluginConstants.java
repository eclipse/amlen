/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.plugin.impl;


/*
 * Define the content of the message bb
 */
public interface ImaPluginConstants {   
    /*
     * Within a stream, each data item is introduced by a type byte which is interpreted as an unsigned byte.
     * The type byte encodes the object type, and a data field.  The data field is the offset from the start
     * of the type range to this byte value.  Type ranges always start at an offset so that the data field
     * can be derived by masking.
     * <p>
     * The data field is either a length, or directly encodes the data of the field.  The length of a scalar
     * field can be smaller than the length for the type since leading zero bytes are suppressed.
     * <p>
     * A number of types are represented in this encoding which do not map to JMS stream and map data types.
     * This interchange format is designed to allow for storing more types than JMS uses.
     * <p>
     * A stream is introduced with the 0x9F type byte.  A map is introduced with the 0x9E type byte.
     * About 20% of the possible type byte values are invalid.  These are used to detect bad streams.
     * <p>
     * For maps, each entry is a pair of name and value.  The name is either an name string or an integer ID.
     * The value is either a single value, or an array count followed by the number of items specified in 
     * the array header.  This allows for non-homogeneous arrays.  Arrays are not supported by JMS.
     * <p>
     */
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    static final int S_Null          = 0x01;
    static final int S_Boolean       = 0x02;
    static final int S_Float         = 0x04;
    static final int S_Double        = 0x06;
    static final int S_Char          = 0x1C;
    static final int S_Short         = 0x50;
    static final int S_Byte          = 0x30;
    static final int S_ByteLen       = 0x10;
    static final int S_UByteLen      = 0x11;
    static final int S_Map           = 0x48;
    static final int S_Int           = 0x60;
    static final int S_ByteArray     = 0x90;
    static final int S_User          = 0x20;
    static final int S_Long          = 0x70;
    static final int S_Time          = 0x08;
    static final int S_NameLen       = 0x58;
    static final int S_Xid           = 0x5E;
    static final int S_Name          = 0xA0;
    static final int S_String        = 0xC0;
    static final int S_StrLen        = 0x28;
    static final int S_NullString    = 0x01;
    static final int S_ID            = 0x14;

    static final int S_BooleanValue  = 0x01;
    static final int S_FloatValue    = 0x01;
    static final int S_DoubleValue   = 0x01;
    static final int S_CharValue     = 0x03;
    static final int S_ShortValue    = 0x03;
    static final int S_ByteValue     = 0x0f;
    static final int S_IntValue      = 0x07;
    static final int S_ByteArrayValue= 0x07;
    static final int S_LongValue     = 0x0f;
    static final int S_NameValue     = 0x1f;
    static final int S_UserValue     = 0x07;
    
    static final int S_NameMax       = 30;
    static final int S_StringMax     = 60;
    static final int S_ByteMax       = 20;
    static final byte S_BooleanFalse = (byte)0x02;    /* For boolean, the type byte encodes the value */
    static final byte S_BooleanTrue  = (byte)0x03;
    
    static final int  ID_Timestamp    = 2;
    static final int  ID_Expire       = 3;
    static final int  ID_MsgID        = 4;
    static final int  ID_CorrID       = 5;
    static final int  ID_JMSType      = 6;
    static final int  ID_ReplyToQ     = 7; 
    static final int  ID_ReplyToT     = 8;
    static final int  ID_Topic        = 9;
    static final int  ID_DeliveryTime = 10;
    static final int  ID_Queue        = 11;
    static final int  ID_UserID       = 12;    /* The user ID of the originator of the message */
    static final int  ID_ClientID     = 13;    /* The client ID of the originator of the message */
    static final int  ID_Domain       = 14;    /* The domain of the client ID */
    static final int  ID_Token        = 15;    /* A security token authenticating this message */
    static final int  ID_DeviceID     = 16;    /* The device identifier of the originator */
    static final int  ID_AppID        = 17;    /* The application identifier of the originator */
    static final int  ID_Protocol     = 18;    /* The protocol which originated the message */
    static final int  ID_ObjectID     = 19;    /* An identifier of an object associated with the message */
    static final int  ID_GroupID      = 20;    /* An identifier of the group of a message */
    static final int  ID_GroupSeq     = 21;    /* The sequence within a group */
    static final int  ID_ServerTime   = 22;    /* The originating server timestamp */
    static final int  ID_OriginServer = 23;    /* The originating server */
    
    enum S_Type { Bad, Null, String, StrLen, Boolean, Byte, UByte, Short, UShort, Char, Int, UInt, Long, ULong, Float, Double, BArray, Name, NameLen, 
    	ID, Array, Time, User, Map, Xid };
    
    /*
     * Map from the type byte to the type.
     * The data value is the offset of this type byte from the start of the type range.
     * 
     * The User type actually starts at 0x20, but the zero length user type is invalid since the user type
     * always has a type byte.  This makes the 0x20 encoding invalid as that is the ASCII space.    
     */
    static final S_Type [] FieldTypes = {
        S_Type.Bad,     S_Type.Null,    S_Type.Boolean, S_Type.Boolean, S_Type.Float,   S_Type.Float,   S_Type.Double,  S_Type.Double,  /* 00 - 07 */
        S_Type.Time,    S_Type.Time,    S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 08 - 0F */
        S_Type.Byte,    S_Type.UByte,   S_Type.Bad,     S_Type.Bad,     S_Type.ID,      S_Type.ID,      S_Type.ID,      S_Type.ID,      /* 10 - 17 */
        S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Char,    S_Type.Char,    S_Type.Char,    S_Type.Char,    /* 17 - 1F */ 
        S_Type.Bad,     S_Type.User,    S_Type.User,    S_Type.User,    S_Type.User,    S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 20 - 27 */ 
        S_Type.StrLen,  S_Type.StrLen,  S_Type.StrLen,  S_Type.StrLen,  S_Type.StrLen,  S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 28 - 2F */
        S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    /* 30 - 37 */
        S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    /* 38 - 3F */
        S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Byte,    S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 40 - 47 */
        S_Type.Map,     S_Type.Map,     S_Type.Map,     S_Type.Map,     S_Type.Map,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 48 - 4F */
        S_Type.Short,   S_Type.Short,   S_Type.Short,   S_Type.Bad,     S_Type.UShort,  S_Type.UShort,  S_Type.UShort,  S_Type.Bad,     /* 50 - 57 */
        S_Type.NameLen, S_Type.NameLen, S_Type.NameLen, S_Type.NameLen, S_Type.NameLen, S_Type.Bad,     S_Type.Xid,     S_Type.Bad,     /* 58 - 5F */
        S_Type.Int,     S_Type.Int,     S_Type.Int,     S_Type.Int,     S_Type.Int,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 60 - 67 */
        S_Type.UInt,    S_Type.UInt,    S_Type.UInt,    S_Type.UInt,    S_Type.UInt,    S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 67 - 6F */
        S_Type.Long,    S_Type.Long,    S_Type.Long,    S_Type.Long,    S_Type.Long,    S_Type.Long,    S_Type.Long,    S_Type.Long,    /* 70 - 77 */
        S_Type.Long,    S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 78 - 7F */
        S_Type.ULong,   S_Type.ULong,   S_Type.ULong,   S_Type.ULong,   S_Type.ULong,   S_Type.ULong,   S_Type.ULong,   S_Type.ULong,   /* 80 - 87 */
        S_Type.ULong,   S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 88 - 8F */
        S_Type.BArray,  S_Type.BArray,  S_Type.BArray,  S_Type.BArray,  S_Type.BArray,  S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 90 - 97 */
        S_Type.Array,   S_Type.Array,   S_Type.Array,   S_Type.Array,   S_Type.Array,   S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* 98 - 9F */
        S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    /* A0 - A7 */
        S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    /* A8 - AF */
        S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    /* B0 - B7 */
        S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Name,    S_Type.Bad,     /* B8 - BF */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* C0 - C7 */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* C8 - CF */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* D0 - D7 */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* D8 - DF */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* E0 - E7 */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* E8 - EF */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  /* F0 - F7 */
        S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.String,  S_Type.Bad,     S_Type.Bad,     S_Type.Bad,     /* F8 - FF */
    };
    
    /*
     * The table gives the length to skip over to find the next item.  This includes
     * the length of the type byte itself.  The value of zero indicates an invalid type.
     * Note the for types User, StrLen, and BArray it is necessary to skip over the content length as well.
     */
    static final byte [] FieldLen = {
        0, 1, 1, 1, 1, 5, 1, 9, 1, 9, 0, 0, 0, 0, 0, 0,  /* 00 - 0F */
        2, 2, 0, 0, 1, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,  /* 10 - 1F */
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
       33,34,35,36,37,38,39,40,41,42,43,44,45,45,47,48,  /* E0 - EF */
       49,50,51,52,53,54,55,56,56,57,59,60,61, 0, 0, 0,  /* F0 - FF */
    };


    /*
     * Bit constants for debug toString()
     */
    public static final int TS_Class      = 1;    /* Show the class name  */
    public static final int TS_HashCode   = 2;    /* Show the hash code   */
    public static final int TS_Info       = 4;    /* Show header info     */
    public static final int TS_Details    = 8;    /* Show object details  */
    public static final int TS_Links      = 16;   /* Show linked objects  */
    public static final int TS_Properties = 32;   /* Show properties      */
    public static final int TS_Body       = 64;   /* Show body            */
    
    public static final int TS_Common     = 7;    /* class@hash header    */
    public static final int TS_All        = 63;   /* Show everything      */
    
    /*
     * Build constants
     */
    static final String Client_build_id = "BUILD_ID";
    static final String Client_build_version = "VERSION_ID";
}
