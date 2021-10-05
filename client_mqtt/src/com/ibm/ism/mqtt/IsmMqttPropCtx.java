/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.mqtt;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * MQTT property context
 */
public class IsmMqttPropCtx {
    int  version;
    IsmMqttProperty [] fields;
    int  max_id = 0;

    /**
     * Construct the MQTT context for a version
     * @param idtbl  An array of field definitions
     * @param version The MQTT version number
     */
    IsmMqttPropCtx(IsmMqttProperty [] idtbl, int version) {
        for (int i=0; i<idtbl.length; i++) {
            if (idtbl[i].id >= max_id && idtbl[i].version >= version)
                max_id = idtbl[i].id+1;
        }

        fields = new IsmMqttProperty [max_id + 1];
        
        for (int i=0; i<idtbl.length; i++) {
            int id = idtbl[i].id;
            if (idtbl[i].version >= version && 
                    (fields[id] == null || idtbl[i].version > fields[id].version))
                fields[id] = idtbl[i];
        }
        this.version = version;
    }
    
    /**
     * Get the MQTT version for this context
     * @return The MQTT version
     */
    int getVersion() {
        return version;
    }
    
    /**
     * Check that the MQTT properties are valid
     * @param bb     The byte buffer containing the properties
     * @param valid  A bitmask of where these extended fields are valid
     * @param bsm  An object containing check methods
     * 
     * #throws A RuntimeException or IOException of the fields are not valid 
     */
    void check(ByteBuffer bb, int valid, IsmMqttPropChecker bsm) throws IOException {
        byte [] foundtab = new byte[max_id];
        bb = bb.duplicate();       /* Do not consume from the input ByteBuffer */

        while (bb.remaining()  > 0) {
            int datalen;
            int value;
            int id = bb.get()&0xff;
            if (id > max_id || fields[id]==null) {
                throw new RuntimeException("The Mqttfield: " + id);
            }
            if ((fields[id].valid & 0x10000) == 0 && foundtab[id] != 0) {
                throw new RuntimeException("Duplicate field: " + fields[id].name);
            }
            foundtab[id] = 1;

            if ((fields[id].valid & valid) == 0) {
                throw new RuntimeException("Field not in valid packet: " + fields[id].name);
            }

            switch (fields[id].type) {
            case IsmMqttProperty.Int1:
                value = bb.get()&0xff;
                bsm.check(this, fields[id], value);
                break;
            case IsmMqttProperty.Int2:
                value = bb.getShort()&0xffff;
                bsm.check(this, fields[id], value);
                break;
            case IsmMqttProperty.Int4:
                value = bb.getInt();
                bsm.check(this, fields[id], value);
                break;
            case IsmMqttProperty.Bytes:
                datalen = bb.getShort()&0xffff;
                byte [] ba = new byte[datalen];
                bb.get(ba);
                bsm.check(this, fields[id], ba);
                break;
            case IsmMqttProperty.String:
                datalen = bb.getShort()&0xffff;
                ba = new byte[datalen];
                bb.get(ba);
                String str = IsmMqttConnection.fromUTF8(ba, 0, datalen, IsmMqttConnection.CHECK_CC_NON);
                bsm.check(this, fields[id], str);
                break;
            case IsmMqttProperty.NamePair:
                datalen = bb.getShort()&0xffff;
                ba = new byte[datalen];
                bb.get(ba);
                String name = IsmMqttConnection.fromUTF8(ba, 0, datalen, IsmMqttConnection.CHECK_CC_NON);   /* Check valid UTF-8 */
                datalen = bb.getShort()&0xffff;
                ba = new byte[datalen];
                bb.get(ba);
                String propval = IsmMqttConnection.fromUTF8(ba, 0, datalen, IsmMqttConnection.CHECK_CC_NON);   /* Check valid UTF-8 */
                bsm.check(this, fields[id], name, propval);
                break;
            case IsmMqttProperty.Boolean:
                bsm.check(this, fields[id], 1);
                break;
            case IsmMqttProperty.VarInt:
                value = getVarInt(bb);
                bsm.check(this, fields[id], value);
                break;
            default:
                throw new RuntimeException("Unknown type: " + fields[id].name);
            }

        }
    }
    

    
    /**
     * Put an MQTT property scalar value
     * @param bb    The byte buffer 
     * @param id    The ID of the field
     * @param value The value of the field as an integer
     * @return The ByteBuffer which might have been updated
     */
    ByteBuffer put(ByteBuffer bb, int id, int value) {
        if (id > max_id || fields[id] == null)
            throw new RuntimeException("The MQTT property is not known: " + id);
        IsmMqttProperty fld = fields[id];
        bb = IsmMqttConnection.ensure(bb, 9);
        if (value != 0 || fld.type != IsmMqttProperty.Boolean)
            bb.put((byte)id);  
        
        switch (fld.type) {
        case IsmMqttProperty.Int1:   bb.put((byte)value);        break;   
        case IsmMqttProperty.Int2:   bb.putShort((short)value);  break;
        case IsmMqttProperty.Int4:   bb.putInt(value);           break;
        case IsmMqttProperty.Boolean:                            break;
        case IsmMqttProperty.VarInt: putVarInt(bb, value);       break;
        default:
            bb.position(bb.position()-1);
            throw new RuntimeException("The MQTT property is not a scalar: " + fld.name);
        }
        return bb;
    }
    
    /**
     * Put an MQTT property as a string
     * @param bb   The byte buffer
     * @param id   Th ID of the field 
     * @param value  The value of the field as a String
     * @return The ByteBuffer which might have been updated
     */
    ByteBuffer put(ByteBuffer bb, int id, String value) {
        bb = IsmMqttConnection.ensure(bb, 3);
        if (id > max_id || fields[id] == null)
            throw new RuntimeException("The MQTT property is not known: " + id);
        IsmMqttProperty fld = fields[id];
        if (fld.type != IsmMqttProperty.String) 
            throw new RuntimeException("The MQTT property is not a string: " + fld.name);
        bb.put((byte)id);
        bb = IsmMqttConnection.putString(bb, value);
        return bb;
    }
    
    /*
     * Put an MQTT property byte array
     * @param bb    The byte buffer
     * @param id    The ID of the field
     * @param value The value of the field as an array of bytes
     */
    ByteBuffer put(ByteBuffer bb, int id, byte [] value) {
        if (id > max_id || fields[id] == null)
            throw new RuntimeException("The MQTT property is not known: " + id);
        IsmMqttProperty fld = fields[id];
        if (fld.type != IsmMqttProperty.Bytes) 
            throw new RuntimeException("The MQTT property is not a byte array: " + fld.name);
        bb = IsmMqttConnection.ensure(bb, value.length + 3);
        bb.put((byte)id);
        bb.putShort((short)value.length);
        bb.put(value);
        return bb;
    }
    
    /*
     * Put an MQTT name pair property
     * @param bb    The byte buffer
     * @param id    The ID of the field
     * @param name  The name of the named pair
     * #param value The value of the named pair
     */
    ByteBuffer put(ByteBuffer bb, int id, String name, String value) {
        bb = IsmMqttConnection.ensure(bb, 3);
        if (id > max_id || fields[id] == null)
            throw new RuntimeException("The MQTT property is not knwon: " + id);
        IsmMqttProperty fld = fields[id];
        if (fld.type != IsmMqttProperty.NamePair) 
            throw new RuntimeException("The MQTT property is not name pair: " + fld.name);
        bb.put((byte)id);
        bb = IsmMqttConnection.putString(bb, name);
        bb = IsmMqttConnection.putString(bb, value);
        return bb;    
    }
    
    /** 
     * Put a variable length int at a specified position
     * 
     *  @param bb    The ByteBuffer
     *  @param pos   The position to write
     *  @param value The value to write
     *  @return The ByteBuffer which might have been updated 
     */
    ByteBuffer putVarInt(ByteBuffer bb, int pos, int value) {
        bb = IsmMqttConnection.ensure(bb, 4);
        if (value <= 127) {
            bb.put(pos, (byte)(value&0x7f));
        } else if (value <= 16383) {
            bb.put(pos,   (byte)((value&0x7f)|0x80));
            bb.put(pos+1, (byte)(value>>7));
        } else if (value <= 2097151) {
            bb.put(pos, (byte)((value&0x7f)|0x80));
            bb.put(pos+1, (byte)((value>>7)|0x80));
            bb.put(pos+2, (byte)(value>>14));
        } else {
            bb.put(pos,   (byte)((value&0x7f)|0x80));
            bb.put(pos+1, (byte)((value>>7)|0x80));
            bb.put(pos+2, (byte)((value>>14)|0x80));
            bb.put(pos+3, (byte)((value>>21)&0x7f));
        }
        return bb;
    }
    
    /** 
     * Put a variable length int at the current position and update the position
     * 
     *  @param bb    The ByteBuffer
     *  @param value The value to write
     *  @return The ByteBuffer which might have been updated
     */
    ByteBuffer putVarInt(ByteBuffer bb, int value) {
        bb = IsmMqttConnection.ensure(bb, 4);
        if (value <= 127) {
            bb.put((byte)(value&0x7f));
        } else if (value <= 16383) {
            bb.put((byte)((value&0x7f)|0x80));
            bb.put((byte)(value>>7));
        } else if (value <= 2097151) {
            bb.put((byte)((value&0x7f)|0x80));
            bb.put((byte)((value>>7)|0x80));
            bb.put((byte)(value>>14));
        } else {
            bb.put((byte)((value&0x7f)|0x80));
            bb.put((byte)((value>>7)|0x80));
            bb.put((byte)((value>>14)|0x80));
            bb.put((byte)((value>>21)&0x7f));
        }
        return bb;
    }
    
    /**
     * Get a variable int from a ByteBuffer
     * @param bb   The byte buffer
     * @return The value of the variable length int or -1 to indicate an error
     */
    int   getVarInt(ByteBuffer bb) {
        int  len;
        int  count = 1;
        int  buflen = bb.remaining();
        int  multshift = 7;

        if (buflen < 1)
            return -1;
        len =  bb.get()&0xFF;
        /* Handle a multi-byte length */
        if ((len & 0x80) != 0) {
            int got = 0;
            len &= 0x7f;
            do {
                count++;
                buflen--;
                if (count > 4 || buflen <= 0) {
                    return -1;
                }
                got = bb.get()&0xff;
                len += (got&0x7f) << multshift;
                multshift += 7;
            } while ((got&0x80) != 0);
        }
        return len;
    }
}    
