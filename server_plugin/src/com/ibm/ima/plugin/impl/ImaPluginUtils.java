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
 * Note: Any changes to this class probably need to be made in com.ibm.ima.jms.ImaUtils as well.
 * This is similar to the JMS class except for the exception handling.
 */

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.transaction.xa.Xid;

import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaPluginException;

/*
 * Define the content of the message bb
 */
public class ImaPluginUtils implements ImaPluginConstants{
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    static final ThreadLocal < byte[] > tempByte = 
        new ThreadLocal < byte[] > () {
            protected byte[] initialValue() {
                return new byte[1000];
        }
    };
    static final ThreadLocal < char[] > tempChar = 
        new ThreadLocal < char[] > () {
            protected char[] initialValue() {
                return new char[1000];
        }
    };

    private ImaPluginUtils() {}
      
   
    /*
     * Make sure the buffer is large enough.
     * When writing anything we check there is enough room for small objects, but
     * larger objects must check again when the actual size is known.
     */
    
    static final ByteBuffer ensureBuffer(ByteBuffer bb, int len) {
    	if (bb != null) {
            if (bb.remaining() >= len) {
            	return bb;
            }
            int newlen = bb.capacity() + len + 1024;
            ByteBuffer newbb = (bb.isDirect()) ? ByteBuffer.allocateDirect(newlen) : ByteBuffer.allocate(newlen);
            bb.flip();
            newbb.put(bb);
            return newbb;
    	}
		bb = ByteBuffer.allocate(len+1024);
		return bb;                
    }

    
    /*
     * Get the type of the object.  
     * This also checks that the bb is readable and if we are at the end of the stream.
     * 
     * At other times we return JMSException if there is not enough data, as this indicates
     * a system error.
     */
    static final int getObjectType(ByteBuffer bb) {
        if (bb.remaining() == 0) {
        	ImaPluginException jex = new ImaPluginException("CWLNA0013", "The end of the message has been reached.");
        	// ImaTrace.traceException(2, jex);
        	throw jex;
        }
        return bb.get()&0xff;
    }
    
    /*
     * Check that the bb is long enough to read the item.
     */
   static  final void checkBodyLength(ByteBuffer bb, int len) {
        if ( bb == null || bb.remaining() < len) {
        	ImaPluginException jex = new ImaPluginException("CWLNA0105", "The message data is not valid");
            // ImaTrace.traceException(2, jex);
            throw jex;
        }
    }    
    
    /*
     * Get a string value.
     * For short strings, the length is kept in the type byte.  For longer strings, the
     * length is kept as an integer.  A null value is considered a special case of String.
     */
   static  final String getStringValue(ByteBuffer bb, int otype) {
        S_Type type = FieldTypes[otype];
        int len;
        switch (type) {
        case StrLen:
            len = getIntValue(bb,otype-S_StrLen);
            break;
        case String:
            len = otype&0x3f;
            break;
        default:
            return null;
        }
       
        checkBodyLength(bb,len);
        String s = fromUTF8(bb, len-1, true);
        if (bb.get() != 0) {
        	//TODO
        }
        return s;
    }
    
    /*
     * Get a name value.
     * For short names, the length is kept in the type byte.  For longer names, the
     * length is kept as an integer.  The name can also be an ID which is converted
     * to its string value. 
     */
    static final String getNameValue(ByteBuffer bb, int otype) {
        S_Type type = FieldTypes[otype];
        int id = 0;
        int len;
        switch (type) {
        case Name:                     /* Length in type byte */
            len = otype&0x1F;
            break;
        case NameLen:                  /* Length is stored as an integer */    
            len = getIntValue(bb,otype);
            break;
        case ID:                       /* Convert an ID to the string form */
            len = otype&3;
            if (len == 3)
                id = -getShortValue(bb,2);
            else
                id = getShortValue(bb,len);
            return ""+id;
        default:
        	ImaPluginException jex = new ImaPluginException("CWLNA0105", "The message content data is not valid.");
            // ImaTrace.traceException(2, jex);
            throw jex;
        }
        checkBodyLength(bb,len);
        String s = fromUTF8(bb, len-1, true);
        if (bb.get() != 0) {
        	//TODO
        }
        return s;
    }
    
    
    /*
     * Get a byte value.
     * A byte with a value between 0 and 12 is stored in the type byte.
     * Otherwise it is stored in the following byte.
     */
    static final byte getByteValue(ByteBuffer bb, int otype) {
        if (otype >= S_Byte)
            return (byte) (otype - S_Byte);
        checkBodyLength(bb,1);
        return bb.get();
    }
    
    /*
     * Get a character value.
     */
    static final char getCharValue(ByteBuffer bb, int otype) {
        return (char)getShortValue(bb, otype);
    }
    
    /*
     * Get a short value.
     */
    static final short getShortValue(ByteBuffer bb, int otype) {
        int len = otype&S_ShortValue;
        int val = 0;
        checkBodyLength(bb,len);
        while (len-- > 0) {
            val = (val<<8) | (bb.get()&0xff);
        }
        return (short)val;
    }
 
    /*
     * Get an int value.
     * All integer type values are stored in big endian without leading zero bytes.
     * Thus the value zero is stored with just the type byte.  Negative values require
     * the full size encoding.
     */
    static final int getIntValue(ByteBuffer bb, int otype) {
        int len = otype&S_IntValue;
        int val = 0;
        checkBodyLength(bb,len);
        while (len-- > 0) {
            val = (val<<8) | (bb.get()&0xff);
        }
        return val;
    }
    
    /*
     * Get a long value.
     */
    static final long getLongValue(ByteBuffer bb, int otype) {
        int len = otype&S_LongValue;
        long val = 0;
        checkBodyLength(bb,len);
        while (len-- > 0) {
            val = (val<<8) | (bb.get()&0xff);
        }
        return val;
    }
    
    /*
     * Get a float value.
     * A float value is stored in either 0 (if the value is 0.0) or 4 bytes.
     */
    static final float getFloatValue(ByteBuffer bb, int otype) {
        if ((otype&S_FloatValue) == 0) {
            return (float)0.0;
        } else {
            return Float.intBitsToFloat(getIntValue(bb,4));
        }    
    }
    
    /*
     * Get a double value.
     * A double value is tored in either 0 (if the value is 0.0) or 8 bytes.
     */
    static final double getDoubleValue(ByteBuffer bb, int otype) {
        if ((otype&S_DoubleValue)== 0) {
            return 0.0;
        } else {
            return Double.longBitsToDouble(getLongValue(bb,8));
        }    
    }
    
    /*
     * The byte array is stored as a encoded integer length, followed by the 
     * specified number of bytes.
     */
    static final ByteBuffer getByteBufferValue(ByteBuffer bb, int otype) {
        int len = getIntValue(bb,otype);
        checkBodyLength(bb,len);
        byte[] ba = new byte[len];
        bb.get(ba);
        ByteBuffer result = ByteBuffer.wrap(ba);
        return result;
    }
    static final byte[] getByteArrayValue(ByteBuffer bb, int otype) {
        int len = getIntValue(bb,otype);
        checkBodyLength(bb,len);
        byte[] ba = new byte[len];
        bb.get(ba);
        return ba;
    }
    
    /*
     * Get the value of a user type object. 
     * The object type is interpreted as a String if it has a user type in the range 0x60 - 0x7F.
     * User types in the range 0x00 to 0x6F are reserved for system use.
     */
    static final Object getUserValue(ByteBuffer bb, int otype) {
        int len = getIntValue(bb,otype);
        Object ret = null;
        
        checkBodyLength(bb,len);
        int kind = bb.get()&0xff;
        
        if (kind >= 0x60 && kind < 0x80) {
            ret = fromUTF8(bb, len-1, true);
        } else {
            byte [] ba = new byte[len-1];
            bb.get(ba);
            ret = ByteBuffer.wrap(ba);
        } 
        return ret;
    }
    
    /*
     * Get the value of a user type object. 
     * The object type is interpreted as a String if it has a user type in the range 0x60 - 0x7F.
     * User types in the range 0x00 to 0x6F are reserved for system use.
     */
    static final Object getXidValue(ByteBuffer bb, int otype)  {
        byte [] branchq;
        byte [] globalid;
        int     formatid = 0;
        int     len = bb.get()&0xff;
        int     branchq_len = 0;
        int     globalid_len = 0;
        checkBodyLength(bb, len);
        if (len >= 6) {
            formatid = bb.getInt();
            globalid_len = bb.get()&0xff;
            branchq_len = bb.get()&0xff;
        }
        if (len == (branchq_len + globalid_len + 6)) {
            branchq = new byte[branchq_len];
            bb.get(branchq);
            globalid = new byte[globalid_len];
            bb.get(globalid);
        } else {
            ImaPluginException jex = new ImaPluginException("CWLNA0105", "The message data is not valid");
            // ImaTrace.traceException(2, jex);
            throw jex;
        }

        Xid ret = new ImaXidImpl(formatid, globalid, branchq);
        return ret;
    }
    
    /*
     *  Get an integer value
     */
    static final int getInteger(ByteBuffer bb) {
    	int ret;
        int otype = getObjectType(bb);
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Int:
        case UInt:     ret = getIntValue(bb,otype);           break;
        case Byte:    ret = getByteValue(bb,otype);          break;
        case UByte:   ret = getByteValue(bb,otype)&0xff;     break;
        case Short:   ret = getShortValue(bb,otype);         break;
        case UShort:  ret = getShortValue(bb,otype)&0xffff;  break;
        case Char:    ret = getCharValue(bb,otype);          break;
        case Null:    ret = 0;                               break;
        case Boolean: ret = otype&1;                         break;
        default:      ret = -1;
        }
        return ret;
    }
    
    /*
     *  Get a long value
     */
    static final long getLong(ByteBuffer bb) {
    	long ret;
        int otype = getObjectType(bb);
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Long:
        case ULong:  ret = getLongValue(bb,otype);           break;
        case Int:
        case UInt:    ret = getIntValue(bb,otype);           break;
        case Byte:    ret = getByteValue(bb,otype);          break;
        case UByte:   ret = getByteValue(bb,otype)&0xff;     break;
        case Short:   ret = getShortValue(bb,otype);         break;
        case UShort:  ret = getShortValue(bb,otype)&0xffff;  break;
        case Char:    ret = getCharValue(bb,otype);          break;
        case Null:    ret = 0;                               break;
        case Boolean: ret = otype&1;                         break;
        default:      ret = -1;
        }
        return ret;
    }
    
    /*
     * Get the next object from the stream and return it as an Object.
     * This has support for all items including those not used by JMS.
     * If an array is found, skip over the array and return null.
     */
    static final Object getObjectValue(ByteBuffer bb) {
        Object obj;
        int otype = getObjectType(bb);
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Null:    obj = null;                                        break;
        case Boolean: obj = Boolean.valueOf((otype&1) == 1);             break;
        case Byte:    obj = Byte.valueOf(getByteValue(bb,otype));           break;
        case UByte:   obj = Integer.valueOf(getByteValue(bb,otype)&0xff);   break;
        case Short:   obj = Short.valueOf(getShortValue(bb,otype));         break;
        case UShort:  obj = Integer.valueOf(getShortValue(bb,otype)&0xffff);break;
        case Char:    obj = Character.valueOf(getCharValue(bb,otype));      break;
        case UInt:
        case Int:     obj = Integer.valueOf(getIntValue(bb,otype));         break; 
        case Time:
        case ULong:   
        case Long:    obj = Long.valueOf(getLongValue(bb,otype));           break;
        case Float:   obj = Float.valueOf(getFloatValue(bb,otype));         break;
        case Double:  obj = Double.valueOf(getDoubleValue(bb,otype));       break;
        case StrLen:
        case String:  obj = getStringValue(bb,otype);                       break;
        case BArray:  obj = getByteArrayValue(bb,otype);                    break;
        case User:    obj = getUserValue(bb,otype);                         break;
        case Xid:     obj = getXidValue(bb, otype);                         break;
        case Map:
            bb.position(bb.position()-1);
            obj = getMapValue(bb, null);                          
            break;
        case Array:   
            /* Skip over the array and return null */
            int alen = getIntValue(bb,otype);
            while (alen-- > 0)
                getObjectValue(bb);
            obj = null;
            break;
        default:
        	ImaPluginException jex = new ImaPluginException("CWLNA0105", "The message content data is not valid");
            // ImaTrace.traceException(2, jex);
            throw jex;
        }
        return obj;
    }
    
    /*
     * Get a map value
     */
    static final HashMap<String,Object> getMapValue(ByteBuffer bb, ImaMessageImpl msg) {
    	int otype = getObjectType(bb);
    	if (otype == S_Map || otype == S_Null)
    		return null;
    	if (otype<S_Map || otype>S_Map+4) {
    		ImaPluginException jex = new ImaPluginException("CWLNA0118", "Message properties not valid, otype="+otype);
    		// ImaTrace.traceException(2, jex);
    		throw jex;
    	}
    	int maplen = getIntValue(bb,otype);
    	checkBodyLength(bb,maplen);
    	HashMap<String,Object> map = new HashMap<String,Object>();
       	int endpos = bb.position()+maplen;
    		
    	while (bb.position() < endpos) {
    		Object value;
    		otype = getObjectType(bb);
    		if (otype == S_ID+1) {
    			int which = getObjectType(bb);
    			value = getObjectValue(bb);
    			if (msg != null) {
        			switch (which) {
        			case ID_Topic:      
        			    msg.dest       = (String)value;   
        			    msg.destType   = ImaDestinationType.topic;
        			    break;
        			}    
    			} 
    			map.put(""+which, value);
    		} else if (otype == S_ID || otype == S_ID+2 || otype == S_ID+3) {
     		    int which = getShortValue(bb, otype);
                value = getObjectValue(bb);
     		    map.put(""+which, value);
    		} else {
    		    String key = getNameValue(bb,otype);
    		    value = getObjectValue(bb);
    		    map.put(key, value);
    		}    
    	}
    	if (bb.position() != endpos) {
    		ImaPluginException jex = new ImaPluginException("CWLNA0118", "The message properties length is not valid");
    		// ImaTrace.traceException(2, jex);
    		throw jex;
    	}
    	return map;
    }
    
    /*
     * Put the null value.
     * This is a special case of a string.
     */
    static final ByteBuffer putNullValue(ByteBuffer bb) {
    	bb = ensureBuffer(bb,16);
        bb.put((byte)S_NullString);
        return bb;
    }
    
    /*
     * Put a boolean value.
     * The boolean value is stored in the type byte.
     */
    static final ByteBuffer putBooleanValue(ByteBuffer bb, boolean val) {
    	bb = ensureBuffer(bb,16);
        byte b = val ? (byte)(S_Boolean+1) : (byte)S_Boolean;
        bb.put(b);
        return bb;
    }
    
    /*
     * Put a byte value.
     */
    static final ByteBuffer putByteValue(ByteBuffer bb, byte val) {
    	bb = ensureBuffer(bb,16);
        if (val >= 0 && val <= S_ByteMax) {
            bb.put((byte)(S_Byte + val));
        } else {
        	bb.put((byte)(S_ByteLen));
        	bb.put(val);
        }
        return bb;
    }
    
    /*
     * Put a short value.
     */
    static final ByteBuffer putShortValue(ByteBuffer bb, short val) {
        return putSmallValue(bb, (int)val&0xffff, S_Short);
    }

    /* 
     * Put a char value.
     */
    static final ByteBuffer putCharValue(ByteBuffer bb, char val) {
        return putSmallValue(bb,(int)val, S_Char);
    }
    
    /*
     * Put a small integer value.  This is common code used for short, char, byte array, and string.
     */
    static final ByteBuffer putSmallValue(ByteBuffer bb, int val, int otype) {
        int shift = 24;
        int count = 0;
        int savepos = bb.position();
        bb = ensureBuffer(bb,16);
    	bb.position(savepos+1);
        while (shift >= 0) {
            int bval = (int)(val >> shift);
            if (count == 0 && bval != 0) 
                count = (shift >> 3) + 1;
            if (count > 0)
                bb.put((byte)bval);
            shift -= 8;
        }
        bb.put(savepos, (byte)(otype + count));    
        return bb;
    }
    
    /*
     * Put an integer value.
     * All integer type values are stored in big endian without leading zero bytes.
     * Thus the value zero is stored with just the type byte.  Negative values require
     * the full size encoding.A
     */
    static final ByteBuffer putIntValue(ByteBuffer bb, int val) {
        int shift = 24;
        int count = 0;
        int savepos = bb.position();
        bb = ensureBuffer(bb,16);
    	bb.position(savepos+1);
        while (shift >= 0) {
            int bval = (int)(val >> shift);
            if (count == 0 && bval != 0) 
                count = (shift >> 3) + 1;
            if (count > 0)
                bb.put((byte)bval);
            shift -= 8;
        }
        bb.put(savepos, (byte)(S_Int + count));
        return bb;
    }
    
    /*
     * Put a long value.
     */
    static final ByteBuffer putLongValue(ByteBuffer bb, long val) {
        int shift = 56;
        int count = 0;
        int savepos = bb.position();
        bb = ensureBuffer(bb,16);
    	bb.position(savepos+1);
        while (shift >= 0) {
            int bval = (int)(val >> shift);
            if (count == 0 && bval != 0) 
                count = (shift >> 3) + 1;
            if (count > 0)
                bb.put((byte)bval);
            shift -= 8;
        }
        bb.put(savepos, (byte)(S_Long + count)); 
        return bb;
    }
    
    /*
     * Put a float value.
     */
    static final ByteBuffer putFloatValue(ByteBuffer bb, float val) {
    	bb = ensureBuffer(bb,16);
        if (val == 0.0f) { 
            bb.put((byte)S_Float);
        } else {    
        	bb.put((byte)(S_Float+1));    
            int ival = Float.floatToRawIntBits(val);
            bb.putInt(ival);
        }
        return bb;
    }
    
    /*
     * Put a double value.
     */
    static final ByteBuffer putDoubleValue(ByteBuffer bb, double val) {
    	bb = ensureBuffer(bb,16);
        if (val == 0.0) { 
            bb.put((byte)S_Double);
        } else {    
            bb.put((byte)(S_Double+1));
	        long lval = Double.doubleToRawLongBits(val);
	        bb.putLong(lval);
        } 
        return bb;
    }
    
    /*
     * Put a string value.
     * 
     * If the length of the string is less than 60 the length is in the type byte, otherwise 
     * the length is stored as an encoded integer.
     * The string is kept as a UTF-8 string.
     * @param val
     * @throws MessageFormatException  If the string is not valid
     */
    static final ByteBuffer putStringValue(ByteBuffer bb, String val) {
        if (val == null) {
            return putNullValue(bb);
        } else {
            int len = sizeUTF8(val);
            if (len < 0) {
            	ImaPluginException jex = new ImaPluginException("CWLNA0122", "The UTF-16 string encoding is not valid");
                // ImaTrace.traceException(2, jex);
                throw jex;
            }
            len++;
            if (len <= S_StringMax) {
                bb.put((byte)(S_String + len));
            } else {
                bb = putSmallValue(bb,len, S_StrLen);
            }
            if (len > 0) {
                bb = ensureBuffer(bb,len + 6);
                len--;
                makeUTF8(val, len, bb);
            }   
            bb.put((byte)0);
        }  
        return bb;
    }
    
    /*
     * Put a Xid value.
     * 
     * @param val
     * @throws MessageFormatException  If the string is not valid
     */
    static final ByteBuffer putXidValue(ByteBuffer bb, Xid val) {
        byte [] branchq = val.getBranchQualifier();
        byte [] globalid = val.getGlobalTransactionId();
        int     len = branchq.length + globalid.length + 6;
        if (len > 256) {
            return putNullValue(bb);
        }
        bb = ensureBuffer(bb, len+2);
        bb.put((byte)S_Xid);
        bb.put((byte)len); 
        bb.putInt(val.getFormatId());
        bb.put((byte)globalid.length);
        bb.put((byte)branchq.length);
        bb.put(branchq);
        bb.put(globalid);
        return bb;
    }
    
    /*
     * Check if the name can be reduced to a small number.
     */
    static final int getSimpleValue(String s) {
        int val = 0;
        int len = s == null ? -1 : s.length();
        if (len > 0 && (s.charAt(0)!='0' || len==1)) {
            for (int i=0; i<len; i++) {
                char ch = s.charAt(i);
                if (ch < '0' || ch > '9') {
                    val = -1;
                    break;
                }
                val = val * 10 + (ch-'0');
            }
        } else {
            val = -1;    
        }
        return val > 65535 ? -1 : val;
    }
    
    
    /*
     * Put out a name value.
     * 
     * A name is kept like a string, but the size range is smaller.
     * The name type is used to increase redundancy when storing a map.
     */
    static final ByteBuffer putNameValue(ByteBuffer bb, String val) {
        int id = getSimpleValue(val);
        if (id >= 0) {
            bb = putSmallValue(bb,id, S_ID);    
        } else {
            int len = sizeUTF8(val);
            if (len < 0) {
            	ImaPluginException jex = new ImaPluginException("CWLNA0122", "The UTF-16 encoding is not valid in name: " + val);
                // ImaTrace.traceException(2, jex);
                throw jex;
            }
            len++;  
            bb = ensureBuffer(bb,64+len);
            if (len < S_NameMax) {
                bb.put((byte)(S_Name + len));
            } else {
                bb = putSmallValue(bb,len, S_NameLen);
            }
            if (len > 0) {
                len--;
                bb = makeUTF8(val, len, bb);
                bb.put((byte)0);
            } 
        } 
        return bb;
    }
    
    
    /*
     * Put out a byte array value.
     * A byte array is stored as an encoded integer length, followed by the specified bytes.
     */
    static final ByteBuffer putByteArrayValue(ByteBuffer bb, byte [] val, int pos, int len) {
    	bb = ensureBuffer(bb,len + 5);
        bb = putSmallValue(bb,len, S_ByteArray);
        bb.put(val, pos, len);
        return bb;
    }
    

    /*
     * Put out a map.  
     * Since we do not precheck the length, we always assume a 3 byte length giving 
     * a maximum map size of 16MB.
     */
    static final ByteBuffer putMapValue(ByteBuffer bb, Map<String,Object> props, ImaMessageImpl msg, ImaDestinationType desttype, String dest) {
    	bb = ensureBuffer(bb,32);
    	
    	int savepos = bb.position();
        bb.put(savepos,(byte)(S_Map+3));
        bb.position(savepos+4);
            
        /* For topics, put in the topic name so that we know it when using wildcards */
        if (desttype == ImaDestinationType.topic) {
        	if (dest != null) {
        		if (dest.length() > 0) {
        	    	bb.put((byte)(S_ID+1));
        		    bb.put((byte)ID_Topic);
            	    bb = putStringValue(bb, dest);
        		}    
        	}
        	if (msg.retain != 0) {
                bb.put((byte)(S_ID+1));
                bb.put((byte)ID_ServerTime);
                bb = putLongValue(bb, System.currentTimeMillis()*1000000);
                bb.put((byte)(S_ID+1));
                bb.put((byte)ID_OriginServer);
                bb = putStringValue(bb, ImaPluginMain.getStringConfig("ServerUID")); 
        	}
        }
        
        /*
         * If we have a null map an no header fields, put in a null map entry
         */
        if (props == null || props.size() == 0) {
        	if (bb.position() == savepos+4) {
    		    bb.put(savepos,(byte)S_Map);
    		    bb.position(savepos+1);
    		    return bb;
        	}    
    	} else {
	        /* Put in all property entries */
	        Iterator <String> it = props.keySet().iterator();
	        while (it.hasNext()) {
	            String key = it.next();
	            if (key.length() > 0 && key.charAt(0)<='9' && key.charAt(1)>='0') {
	                int val = 0;
	                for (int digit=0; digit < key.length(); digit++) {
	                    char ch = key.charAt(digit);
	                    if (ch < '0' || ch > '9') {
	                        val = -1;
	                        break;
	                    }
	                    val = (val * 10) + (ch - '0');
	                }
	                if (val >= 16*1024*1024)
	                    val = -1;
	                if (val >= 0) {
	                    bb = putSmallValue(bb, val, S_ID);
	                } else {
	                    bb = putNameValue(bb, key);
	                }
	            } else {
	                bb = putNameValue(bb, key);
	            }    
	            bb = putObjectValue(bb, props.get(key));
	        }
    	}
        
        /* Put out the length */
        int len = bb.position()-savepos-4;
        bb.put(savepos+1,(byte)(len >> 16));
        bb.put(savepos+2,(byte)((len >> 8) & 0xff));
        bb.put(savepos+3, (byte)(len&0xff));
        return bb;
    }
    
    /*
     * Put out a map.  
     * Since we do not precheck the length, we always assume a 3 byte length giving 
     * a maximum map size of 16MB.
     */
    static final ByteBuffer putMapValue(ByteBuffer bb, Map<String,Object> props) {
        bb = ensureBuffer(bb,32);
        
        int savepos = bb.position();
        bb.put(savepos,(byte)(S_Map+3));
        bb.position(savepos+4);
        
        /*
         * If we have a null map an no header fields, put in a null map entry
         */
        if (props == null || props.size() == 0) {
            bb.put(savepos,(byte)S_Map);
            bb.position(savepos+1);
            return bb;
        }    
            /* Put in all property entries */
        Iterator <String> it = props.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            bb = putNameValue(bb, key);
            bb = putObjectValue(bb,props.get(key));
        }
        
        /* Put out the length */
        int len = bb.position()-savepos-4;
        bb.put(savepos+1,(byte)(len >> 16));
        bb.put(savepos+2,(byte)((len >> 8) & 0xff));
        bb.put(savepos+3, (byte)(len&0xff));
        return bb;
    }
    
      
    /*
     * Put an object value.
     */
    static final ByteBuffer putObjectValue(ByteBuffer bb, Object value) {
        if (value == null) {
            bb = putNullValue(bb);
        } else if (value instanceof String) {
        	bb = putStringValue(bb,(String)value);
        } else if (value instanceof Integer) {
        	bb = putIntValue(bb,((Integer)value).intValue());
        } else if (value instanceof Boolean) {
        	bb = putBooleanValue(bb,((Boolean)value).booleanValue());
        } else if (value instanceof Byte) {
        	bb = putByteValue(bb,((Byte)value).byteValue());            
        } else if (value instanceof Long) {
        	bb = putLongValue(bb,((Long)value).longValue());
        } else if (value instanceof byte[]) {
        	bb = putByteArrayValue(bb,(byte[])value, 0, ((byte[])value).length);
        } else if (value instanceof Double) {
        	bb = putDoubleValue(bb,((Double)value).doubleValue());
        } else if (value instanceof Float) {
        	bb = putFloatValue(bb,((Float)value).floatValue());
        } else if (value instanceof Character) {
            putCharValue(bb,((Character)value).charValue());
        } else if (value instanceof Short) {
        	bb = putShortValue(bb,((Short)value).shortValue());
        } else if (value instanceof Xid) {
            bb = putXidValue(bb, ((Xid)value));
        } else {
        	bb = putStringValue(bb,String.valueOf(value));
        }
        return bb;
    }    
    
    /*
     * Make a modified UTF-8 byte array.
     * Before making this call you must call sizeModifiedUTF8() to find the size of the utf8 byte array.
     * This is separated into two calls since it is hard to return multiple values in Java, and some
     * invokers need to put a check between the two calls.
     * 
     * The modified UTF-8 bytes encode the \u0000 character as 0xc080 rather than 0x00, and does not
     * process surrogates.  This makes it invalid UTF-8 for a converter.
     * 
     * @param str     The string to convert.
     * @param utflen  The length of the output byte array required.  This must have been created using sizeModifiedUTF8 on the same string.
     * @param ba      A byte array for output.  If this is null or too short, a byte array is created.  It is intended that this is used
     *                to have a temporary conversion buffer.
     */
    static byte [] makeModifiedUTF8(String str, int utflen, byte [] ba, int pos) {
        int  strlen = str.length();
        int  c;
        int  i;
        
        if (ba == null || utflen > ba.length)
            ba = new byte[utflen];
        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if ((c >= 0x0001) && (c <= 0x007F)) {
                ba[pos++] = (byte) c;
            } else if (c > 0x07FF) {
                ba[pos++] = (byte) (0xE0 | ((c >> 12) & 0x0F));
                ba[pos++] = (byte) (0x80 | ((c >>  6) & 0x3F));
                ba[pos++] = (byte) (0x80 | ((c >>  0) & 0x3F));
            } else {
                ba[pos++] = (byte) (0xC0 | ((c >>  6) & 0x1F));
                ba[pos++] = (byte) (0x80 | ((c >>  0) & 0x3F));
            }
        }
        return ba;
    }
    
    /*
     * Determine the size of a modified UTF-8 string
     */
    static int sizeModifiedUTF8(String str) {
        int  strlen = str.length();
        int  utflen = 0;
        int  c;
        int  i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if ((c >= 0x0001) && (c <= 0x007F)) {
                utflen++;
            } else if (c > 0x07FF) {
                utflen += 3;
            } else {
                utflen += 2;
            }
        }
        return utflen;
    }   
    
    /*
     * Make a valid UTF-8 byte array from a string.  
     * Before invoking this method you must call sizeUTF8 using the same string to find the length of the UTF-8
     * buffer and to check the validity of the UTF-8 string.
     */
    static ByteBuffer makeUTF8(String str, int utflen, ByteBuffer ba) {
        int  strlen = str.length();
        int  c;
        int  i;
        
        if (ba == null || utflen > ba.capacity())
            ba = ByteBuffer.allocate(utflen);
        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c <= 0x007F) {
                ba.put((byte) c);
            } else if (c > 0x07FF) {
                if (c >= 0xd800 && c <= 0xdbff) {
                    c = ((c&0x3ff)<<10) + (str.charAt(++i)&0x3ff) + 0x10000;
                    ba.put((byte) (0xF0 | ((c >> 18) & 0x07)));
                    ba.put((byte) (0x80 | ((c >> 12) & 0x3F)));
                    ba.put((byte) (0x80 | ((c >>  6) & 0x3F)));
                    ba.put((byte) (0x80 | (c & 0x3F)));
                } else {    
                	 ba.put((byte) (0xE0 | ((c >> 12) & 0x0F)));
                	 ba.put((byte) (0x80 | ((c >>  6) & 0x3F)));
                	 ba.put((byte) (0x80 | (c & 0x3F)));
                }
            } else {
            	ba.put((byte) (0xC0 | ((c >>  6) & 0x1F)));
            	 ba.put((byte) (0x80 | (c & 0x3F)));
            }
        }
        return ba;
    }
    
    /* Starter states for UTF8 */
    private static final int States[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
    };
    
    /* Initial byte masks for UTF8 */
    private static final int StateMask[] = {0, 0, 0x1F, 0x0F, 0x07};
    
    /*
     * Check if the second and subsequent bytes of a UTF8 string are valid.
     */
    private static boolean validSecond(boolean checkValid, int state, int byte1, int byte2) {
        if (byte2 < 0x80 || byte2 > 0xbf)
            return false;
        boolean ret = true;
        if (checkValid) {
            switch (state) {
            case 2:
                if (byte1 < 2)
                    ret = false;
                break;
            case 3:
                if (byte1== 0 && byte2 < 0xa0)
                    ret = false;
                if (byte1 == 0x0d && byte2 > 0x9f)
                    ret = false;
                break;
            case 4:
                if (byte1 == 0 && byte2 < 0x90)
                    ret = false;
                else if (byte1 == 4 && byte2 > 0x8f)
                    ret = false;
                else if (byte1 > 4)
                    ret = false;
            }
        }
        return ret;
    }
    
    /*
     * Fast UTF-8 to char conversion.
     * This code does a fast conversion from a UTF-8 byte array to a String.
     * If checkValid is specified, the function matches the Unicode specification for checking
     * encoding, but does not guarantee that there are no unmatched surrogates.  The modified
     * UTF-8 will fail when checkValid is specified if there is a zero byte encoded.
     */
    static String fromUTF8(ByteBuffer buf, int len, boolean checkValid) {
    	char [] tbuf = tempChar.get(); 
        if (tbuf.length < len*2) {
            tbuf = new char[len*2];
            tempChar.set(tbuf);
        }
        int  byte1 = 0;
        int  byteend = buf.position()+len;
        int  charoff = 0;
        int  state = 0;
        int  value = 0;
        int  inputsize = 0;
        
        while (buf.position() < byteend) {
            if (state == 0) {
                /* Fast loop in single byte mode */
                for (;;) {
                    byte1 = buf.get();
                    if (byte1 < 0)
                        break;
                    tbuf[charoff++] = (char)byte1;
                    if (buf.position() >= byteend)
                        return new String(tbuf, 0, charoff);
                }                
                state = States[(byte1&0xff) >>3];
                value = byte1 = byte1 & StateMask[state];
                inputsize = 1;
                if (state == 1) {
                	ImaPluginException jex = new ImaPluginException("CWLNA0122", "The UTF-8 text encoding is not valid.");
                    // ImaTrace.traceException(2, jex);
                    throw jex;
                }
            } else {
                int byte2 = buf.get()&0xff;
                if ((inputsize==1 && !validSecond(checkValid, state, byte1, byte2)) ||
                    (inputsize > 1 && byte2 < 0x80 || byte2 > 0xbf)) {
                	ImaPluginException jex = new ImaPluginException("CWLNA0122", "The UTF-8 text encoding is not valid.");
                	// ImaTrace.traceException(2, jex);
                	throw jex;
                }
                value = (value<<6) | (byte2&0x3f);
                if (inputsize+1 >= state) {
                    if (value < 0x10000) {
                        tbuf[charoff++] = (char)value;
                    } else {
                        tbuf[charoff++] = (char)(((value-0x10000) >> 10) | 0xd800);
                        tbuf[charoff++] = (char)(((value-0x10000) & 0x3ff) | 0xdc00);
                    }
                    state = 0;
                } else {
                    inputsize++;
                }
            }
        }    
        return new String(tbuf, 0, charoff);
    }
    
    
    /* 
     * Return the length of a valid UTF-8 string including surrogate handling.
     * @param str  The string to find the UTF8 size of
     * @returns The length of the string in UTF-8 bytes, or -1 to indicate an input error
     */
    static int sizeUTF8(String str) {
        int  strlen = str.length();
        int  utflen = 0;
        int  c;
        int  i;

        for (i = 0; i < strlen; i++) {
            c = str.charAt(i);
            if (c <= 0x007f) {
                utflen++;
            } else if (c <= 0x07ff) {
                utflen += 2;
            } else if (c >= 0xdc00 && c <= 0xdfff) {   /* High surrogate is an error */
                return -1;
            } else if (c >= 0xd800 && c <= 0xdbff) {   /* Low surrogate */
                 if ((i+1) >= strlen) {      /* At end of string */
                     return -1;
                 } else {                    /* Check for valid surrogate */
                     c = str.charAt(++i);
                     if (c >= 0xdc00 && c <= 0xdfff) {
                         utflen += 4;
                     } else {
                         return -1;
                     }    
                 }
            } else {     
                 utflen += 3;
            }
        }
        return utflen;
    }  

    /*
     * Check for a valid UTF16 string
     */
    static boolean isValidUTF16(String str) {
    	return sizeUTF8(str) >= 0;
    }
    
}
