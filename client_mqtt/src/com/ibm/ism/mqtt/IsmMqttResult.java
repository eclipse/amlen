/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;



/*
 * Define the result of an MQTT operation 
 */
public class IsmMqttResult implements IsmMqttPropChecker {
    IsmMqttConnection connect;
    int     command;
    int     rc;
    int     packetID;
    boolean sync;
    volatile boolean complete;
    String  reason;
    List<IsmMqttUserProperty> props;
    byte []  rcList;
    Throwable ex;
    ByteBuffer payload;

    /**
     * Create a result object snd set the operation
     */
    IsmMqttResult(IsmMqttConnection connect, int command, ByteBuffer payload, int packetID, boolean sync) {
        this.connect  = connect;
        this.command  = command;
        this.payload  = payload;
        this.packetID = packetID;
        this.sync     = sync;
    }
    
    /**
     * Get the connection
     * @return The connection
     */
    public IsmMqttConnection getConnect() {
        return connect;
    }
    
    /**
     * Get the operation which is completed
     * @return The operation 
     */
    public int getOperation() {
        return command >> 4;
    }
    
    /**
     * Determine if the operation is complete
     * @return true=complete
     */
    public boolean isComplete() {
        return complete;
    }
    
    /**
     * Get the MQTT reason code
     * @return the return code
     */
    public int getReasonCode() {
        return rc;
    }
    
    /**
     * Set the MQTT return code
     * @param rc the return code to set
     */
    public void setReasonCode(int rc) {
        this.rc = rc;
    } 
    
    /* Legacy name for ReasonCode */
    public int getReturnCode() {
        return rc;
    }
    
    /**
     * Get the packet identifier
     * @return the PacketID
     */
    public int getPacketID() {
        return packetID;
    }
    
    /**
     * Get the reason string
     * @return The reason string which might be null
     */
    public String getReason() {
        return reason;
    }

    /**
     * Get a user property
     * @param name  The name of the property to get
     * @return the value of the named property or null to indicate it does not exist
     */
    public String getProperty(String name) {
        if (props == null || name == null)
            return null;
        Iterator<IsmMqttUserProperty> it = props.iterator();
        while (it.hasNext()) {
            IsmMqttUserProperty up = it.next();
            if (name.equals(up.name())) 
                return up.value();
        }    
        return null;
    }
    
    /*
     * Add a user property
     */
    public synchronized void addUserProperty(String name, String value) {
        if (props == null) {
            props = (List<IsmMqttUserProperty>)new Vector<IsmMqttUserProperty>();
        }
        if (name == null)
            name = "";
        if (value == null)
            value = "";
        props.add(new IsmMqttUserProperty(name, value));
    }
    
    /**
     * Get the list of user properties
     * @return a list of user properties
     */
    public List<IsmMqttUserProperty> getProperties() {
        return props;
    }
    
    /**
     * Get an array of return codes.
     * This is used for subscribe and unsubscribe when multiple topic filters are given
     * @return An array of return codes
     */
    public byte [] getReasonCodes() {
        return rcList;
    }  
    
    /**
     * Returns payload
     * @return MQTT message payload
     */
    public ByteBuffer getPayload() {
        return payload;
    }

    /**
     * Set an exception associated with the result
     * @param e The exception
     */
    public void setThrowable(Throwable e) {
        ex = e;
    }
    
    /**
     * Get an exception associated with the result
     * @return The exception
     */
    public Throwable getThrowable() {
        return ex;
    }
    
    /**
     * Set the reason string
     * @param reason The reason string
     */
    public void setReason(String reason) {
        this.reason = reason;
    }

    /**
     * Get the command byte value.
     * The operation is the upper 4 bits
     * @return
     */
    public int getCommand() {
        return command;
    }    

    /*
     * Check an integer property
     * @see com.ibm.ism.mqtt.IsmMqttPropChecker#check(com.ibm.ism.mqtt.IsmMqttPropCtx, com.ibm.ism.mqtt.IsmMqttProperty, int)
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, int value) {
    }

    /*
     * Check a string property
     * @see com.ibm.ism.mqtt.IsmMqttPropChecker#check(com.ibm.ism.mqtt.IsmMqttPropCtx, com.ibm.ism.mqtt.IsmMqttProperty, java.lang.String)
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String str) {
        switch (fld.id) {
        case IsmMqttConnection.MPI_Reason:
            this.reason = str;
            break;
        }    
    }
    
    /*
     * Check a byte array property
     * @see com.ibm.ism.mqtt.IsmMqttPropChecker#check(com.ibm.ism.mqtt.IsmMqttPropCtx, com.ibm.ism.mqtt.IsmMqttProperty, byte[])
     */
    public void check(IsmMqttPropCtx cts, IsmMqttProperty fld, byte [] ba) {
    }
    
    /*
     * Check a name pair property
     * @see com.ibm.ism.mqtt.IsmMqttPropChecker#check(com.ibm.ism.mqtt.IsmMqttPropCtx, com.ibm.ism.mqtt.IsmMqttProperty, java.lang.String, java.lang.String)
     */
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String name, String value) {
        addUserProperty(name, value);     
    }
    
    /**
     * Get the name of the operation for debug purposes
     * @return
     */
    public String getOperationName() {
        switch (command>>4) {
        case IsmMqttConnection.CONNECT:     return "CONNECT";  
        case IsmMqttConnection.CONNACK:     return "CONNACK"; 
        case IsmMqttConnection.PUBLISH:     return "PUBLISH";
        case IsmMqttConnection.PUBACK:      return "PUBACK";
        case IsmMqttConnection.PUBREC:      return "PUBREC";
        case IsmMqttConnection.PUBREL:      return "PUBREL"; 
        case IsmMqttConnection.PUBCOMP:     return "PUBCOMP";
        case IsmMqttConnection.SUBSCRIBE:   return "SUBSCRIBE";
        case IsmMqttConnection.SUBACK:      return "SUBACK";
        case IsmMqttConnection.UNSUBSCRIBE: return "UNSUBSCRIBE";
        case IsmMqttConnection.UNSUBACK:    return "UNSUBACK";
        case IsmMqttConnection.PINGREQ:     return "PINGREQ"; 
        case IsmMqttConnection.PINGRESP:    return "PINGRESP";
        case IsmMqttConnection.DISCONNECT:  return "DISCONNECT";
        case IsmMqttConnection.AUTH:        return "AUTH";
        default:                            return "UNKNOWN";
        }
    }    
    
    public String toString() {
        String ret = "IsmMqttResult cmd=" + getOperationName() + " connect=" + connect.getClientID() +  " complete=" + complete;
        if (complete) {
            if (rcList != null) {
                if (rcList.length == 1) {
                    ret += " rc=" + rc;
                } else {
                    String rcs = " rc=[";
                    for (int i=0; i<rcList.length; i++) {
                        rcs += ((i==0)?"":",") + (rcList[i]&0xff);
                    }  
                    ret += rcs + "]";
                }
            } else {
                ret += " rc=" + rc;
            }
        }        
        if (packetID != 0)
            ret += " packetID=" + packetID;
        if (reason != null)
            ret += " reason=" + reason;
        if (ex != null)
            ret += "\n   except=" + ex;
        if (props != null) {
            Iterator<IsmMqttUserProperty> it = props.iterator();
            while (it.hasNext()) {
                IsmMqttUserProperty prp = it.next();
                ret += "\n   "+prp.name()+" = "+prp.value();
            }
        }
        return ret;
    }
}
