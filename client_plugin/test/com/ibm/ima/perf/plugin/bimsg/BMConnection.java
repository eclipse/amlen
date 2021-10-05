/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
 * Connection listener for   
 */

package com.ibm.ima.perf.plugin.bimsg;

import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.HashMap;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaReliability;
import com.ibm.ima.plugin.ImaSubscription;
import com.ibm.ima.plugin.util.ImaJson;
//import com.ibm.ima.plugin.ImaShareType;
//import com.ibm.ima.plugin.impl.ImaSubscriptionImpl;

public class BMConnection implements ImaConnectionListener {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaConnection  connect;
    ImaPlugin      plugin;
    byte[]         savestr = null;
    int            livenesTimeoutCounter = 0;
    int            MaxLengthFieldByte = 4;
    int            AVAIL = 0, PUBLISH = 1, PUBREC =2;
    int            msgrmLen = 0, msgType, rlenField;
    int            nextAvail = 1;  // next available msgId
    static Charset utf8 = Charset.forName("UTF-8");
    HashMap<String,ImaSubscription> SubMap = new HashMap<String,ImaSubscription>();
    HashMap<Integer, ImaMessage>MsgIdMap = new HashMap<Integer, ImaMessage>();
   /*   		
    * Removing qos1 support due to msgidstatus large memory requirements 
    * 
    int            MsgIdStatus[] = new int[65536];
    */
    
    /*
     * Constructor
     */
    BMConnection(ImaConnection connect, ImaPlugin plugin) {
        this.connect = connect;
        this.plugin  = plugin;
    }

    /*
     * @see com.ibm.ima.plugin.ImaConnectionListener#onClose(int, java.lang.String)
     */
    public void onClose(int rc, String reason) {
    	//System.err.println("onClose");
    }
    
    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.plugin.ImaConnectionListener#onLivenessCheck()
     */
    public boolean onLivenessCheck() {
        if (livenesTimeoutCounter > 5)
            return false;
        livenesTimeoutCounter++;
        connect.sendData("{\"Action\": \"Ping\"}");
        return true;
    }
   
    /*
     * parse msg header to get MSGTYPE and remaining length
     */
    public void parseMsgHeader(byte[] data) {
    	int len = data.length;
    	msgrmLen = 0;
    	if (len >= 2){
    		msgType = (int)(data[0] & 0xF0) / 16;
    		//System.err.println("msgType: " + msgType);
    		int i = 1, multiplier = 1;
    		while ((i < MaxLengthFieldByte +1) && i < len){
    			//System.err.println("rlfield: " + (int)(data[i]));
    			msgrmLen += ((data[i] & 127) * multiplier);
    			multiplier *= 128;
    			if ((data[i] & 128) == 0){   // full remaining length field
    				rlenField = i;
    				//System.err.println("remainLen: " + msgrmLen);
    				return;
    			}
    			i++;
    		}
    		if ( i < MaxLengthFieldByte +1 ){ // partial remaining length field
    			msgrmLen = 0;
    		}
    		else msgrmLen = -1;   // Bad data
    	}
    	//System.err.println("remainLen: " + msgrmLen);
    	return;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnectionListener#onData(byte[])
     */
    public int onData(byte[] data, int offset, int length) {
    	//System.err.println("onData: " + ((int) data[offset]));
        try {
            byte[] jstr = null;
            /* TODO: check for the data not ending at a character boundary */
            if (savestr != null) {
            	jstr = new byte[length + savestr.length];
                System.arraycopy(savestr, 0, jstr, 0, savestr.length);
                System.arraycopy(data, offset, jstr, savestr.length, length);
                savestr = null;
            }
            else {
                jstr = new byte[length];
                System.arraycopy(data, offset, jstr, 0, length);
            }
            //System.err.println("onData: " + ((int)jstr[0]));
            while (jstr != null) {
            	//parse msg header to get remaining length, msgType
	            parseMsgHeader(jstr);
	            int currLen = jstr.length;
	            
	            if (msgrmLen == -1){   // Bad data
	            	connect.close(217, "Bad MQTT data");
	                jstr = null;
	                break;
	            }
	            else if ((msgrmLen == 0) || (jstr.length < msgrmLen + 1 + rlenField)){   // partial header or msg
	            	//System.err.println("Partial msg length: " + jstr.length + " needed:" + msgrmLen);
	            	savestr = jstr;
	                jstr = null;
	                break;
	            }
	            else{  // at least one full msg
	            	//System.err.println("Get a full msg: " + jstr[0]);
	            	byte[] msgdata = new byte[msgrmLen];
	            	int qos = (jstr[0] >> 1) & 0x03;
	            	//System.err.println("QoS in onData: "+qos);
	            	System.arraycopy(jstr, 1 + rlenField, msgdata, 0, msgrmLen);
	            	jstr = Arrays.copyOfRange(jstr, 1+rlenField +msgrmLen, currLen);    //data left in jstr
		             
		            if (msgType == 1) {  //connect
		                doConnect(msgdata);
		            } else if (msgType == 2) {  //connack 
		                doAck(msgdata, 1);
		            } else if (msgType == 3) {   //publish
		                doSend(msgdata, qos);
		            } else if (msgType == 4) {  //puback 
		                doAck(msgdata, 3);
		            } else if (msgType == 5) {  //pubrec
		                doAck(msgdata, 4);
		            } else if (msgType == 6) {  //pubrel
		                doAck(msgdata, 5);
		            } else if (msgType == 7) {  //pubcomp
		                doAck(msgdata, 6);
		            } else if (msgType == 8) {   //subscription
		                doSubscribe(msgdata);
		            } else if (msgType == 9) {  //suback 
			            doAck(msgdata, 2);
		            } else if (msgType == 10) {  //unsubscription
		                doCloseSub(msgdata);
		            } else if (msgType == 11) {  //unsuback
		                doAck(msgdata, 7);
		            } else if (msgType == 12) {  //ping
		                doPing(msgdata);
		            } else if (msgType == 13) {  //pingresp
		                doAck(msgdata, 8);
		            } else if (msgType == 14) {  //disconnect
		                doClose(msgdata);    
		            } else {
		                connect.close(217, "Bad MQTT data");
		                jstr = null;
		                break;
		            }
		            msgdata = null;
	            }
            }
        } catch (Exception e) {
            e.printStackTrace(System.err);
            connect.close();
        }
        return length;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnectionListener#onConnected(int, java.lang.String)
     */
    public void onConnected(int rc, String reason) {
    	//System.err.println("onConnected: rc=" + rc + " reason=" + reason);
    	if (rc < 6) {
	    	//create CONNACK msg
			byte[] connack = new byte[4];
			connack[0] = (byte)0x20;
			connack[1] = (byte)0x02;
			connack[2] = (byte)0x00;
			connack[3] = (byte)rc;  // return code
	    	connect.sendData(connack);
    	}
    }
    
    /*
     * Process a connect
     */
    void doConnect(byte[] data) {
    	//System.err.println("doConnect: " + data[0]);
    	int index = 0;
    	
    	// Get protocol
    	int protoLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
    	index += 2;
    	//String protoName = msg.substring(index, index+protoLen);
    	index += protoLen + 1;  
    	// Don't check WILL_FLAG, add here if needed
    	int usrnameflag = data[index] & 128;
    	int pwflag = data[index] & 64;
        String user     = null;
        String password = null;
        // Don't check Alive Timer, add here if needed
        index += 3;
        
        // Get client_ID
        int clntIDLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
        index += 2;
        String clientID = new String(data, index, clntIDLen, utf8);
        index += clntIDLen;
        
        // Get Username
        if (usrnameflag == 1){
        	int unLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
            index += 2;
            user = new String(data, index, unLen, utf8);
            index += unLen;
        }
        
        // Get password
        if (pwflag == 1){
        	int pwLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
            index += 2;
            password = new String(data, index, pwLen, utf8);
            index += pwLen;
        }
        
        connect.setIdentity(clientID, user, password);
    }
    
    /*
     * Process a publish
     */
    void doSend(byte[] data, int qos) {
    	//System.err.println("doSend: " + data[0]);
    	int index = 0;
    	int tpcLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
    	index += 2;
        String topic = new String(data, index, tpcLen, utf8);
        index += tpcLen;  // topic length
        int msgId = 0, idLen = 0;
        
        //for QoS 1/2 it needs 2 bytes msgID
        if (qos == 1 | qos == 2){
        	msgId = (data[index]&0xFF)*256 + (data[index+1]&0xFF);
        	index += 2;
        	idLen = 2;
        }
        int bdLen = data.length - 2 - tpcLen - idLen;
        byte[] body  = new byte[bdLen];
        System.arraycopy(data, index, body, 0, bdLen);
        // System.out.println("doSend: topic=" + topic + " body=" + body);
        if (topic != null) {
            ImaMessage mesg = plugin.createMessage(ImaMessageType.bytes);
            mesg.setBodyBytes(body);
            if (qos == 0)
	        	mesg.setReliability(ImaReliability.qos0);
	        else if (qos == 1)
	        	mesg.setReliability(ImaReliability.qos1);
	        else if (qos == 2)
	        	mesg.setReliability(ImaReliability.qos2);
            if (qos == 1 | qos == 2){
            	connect.send(mesg, ImaDestinationType.topic, topic, (Integer)msgId);
            }
            else
            	connect.send(mesg, ImaDestinationType.topic, topic, null);
        }
    }
    
    /*
     * Process an ACK
     */
    void doAck(byte[] data, int acktype) {
    	if (acktype == 3) { //puback
    		int id = (data[0]& 0xFF) * 256 + (data[1]& 0xFF);
    		//System.err.println("id: " + id);
    		
    		/*   		
    		 * Removing qos1 support due to msgidstatus large memory requirements 
    		 * 
    		MsgIdStatus[id] = AVAIL;
    		*/
    		
    		ImaMessage msg = MsgIdMap.get((Integer)id);
    		msg.acknowledge(0);
    	}
    }
    
    /*
     * Send ACK message for Send, Subscribe, CloseSubscription, DestroySubscription
     * @param id 	client action ID.
     * @param rc	return code
     */
    void sendAck(int msgid, int rc, int type, int qos){
    	if(msgid > 0 && rc == 0){
    		if (type == 0){ 
	    		//create SUBACK msg
    			//System.err.println("do SUBACK: " + msgid);
				byte[] suback = new byte[5];
				suback[0] = (byte)0x90;
				suback[1] = (byte)0x03;
				suback[2] = (byte)(msgid / 256);
				suback[3] = (byte)(msgid % 256);
				suback[4] = (byte)qos;  // granted qos
		    	connect.sendData(suback);
    		}
    		else if (type == 1){ 
	    		//create UNSUBACK msg
    			//System.err.println("do UNSUBACK: " + msgid);
				byte[] suback = new byte[4];
				suback[0] = (byte)0xB0;
				suback[1] = (byte)0x02;
				suback[2] = (byte)(msgid / 256);
				suback[3] = (byte)(msgid % 256);
				connect.sendData(suback);
    		}
    		else if (type == 2){
    			// create PUBACK msg
    			byte[] puback = new byte[4];
    			puback[0] = (byte)0x40;
				puback[1] = (byte)0x02;
				puback[2] = (byte)(msgid / 256);
				puback[3] = (byte)(msgid % 256);
				connect.sendData(puback);
    		}
    	}
    }
    
    /*
     * Process a subscribe 
     */
    void doSubscribe(byte[] data) {
    	//System.err.println("doSubscribe: " + data[0]);
    	int index = 0, tpcLen;
    	
    	int msgid = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
    	//System.err.println("msgId: " + msgid);
    	index += 2;
    	
    	/* for each subscribed topic */
    	while (index < data.length){
    	    tpcLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF);
    		index += 2;
	        String topic = new String(data, index, tpcLen, utf8);
	        //System.err.println("topic: " + topic);
	        index += tpcLen;
	        int qos = data[index];
	        //System.err.println("qos: " + qos);
	        index += 1;
	        
	        if(topic.length() == 0){
	        	//sendAck(msgid, -1);
	        	return;
	        }
	        
	        ImaSubscription sub = connect.newSubscription(topic);
	        sub.setUserData(new Integer(msgid));
	        if (qos == 0)
	        	sub.setReliability(ImaReliability.qos0);
	        else if (qos == 1)
	        	sub.setReliability(ImaReliability.qos1);
	        else if (qos == 2)
	        	sub.setReliability(ImaReliability.qos2);
            SubMap.put(topic, sub);
	        if(msgid != 0){
	        	sub.subscribe();
	        }else{
	        	sub.subscribeNoAck();
	        }
    	}
    }
    
    /*
     * Process a unsubscription
     */
    void doCloseSub(byte[] data) {
    	//System.err.println("closeSub: " + data[0]);
    	int index = 0, tpcLen;
    	
    	int msgid = (data[index]&0xFF)*256 + (data[index+1]&0xFF) ;
    	//System.err.println("msgId: " + msgid);
    	index += 2;
    	
    	/* for each subscribed topic */
    	while (index < data.length){
    	    tpcLen = (data[index]&0xFF)*256 + (data[index+1]&0xFF);
    		index += 2;
	        String topic = new String(data, index, tpcLen, utf8);
	        //System.err.println("topic: " + topic);
	        index += tpcLen;
	        if (tpcLen == 0)
	        	return;	        
	        ImaSubscription sub = SubMap.get(topic);
	        if(msgid != 0){
                sub.close();
	        }
    	}
    }
    
    /*
     * Process a destroy subscription
     */
    void doDestroySub(byte[] data) {
    }
    
    /*
     * Process a disconnect
     */
    void doClose(byte[] data) {
        
    }
    
    /*
     * Process an ping
     */
    void doPing(byte[] data) {
        //connect.sendData("{\"Action\": \"Pong\"}");
    }
    
    /*
     * Process an pong
     */
    void doPong(ImaJson jobj) {
    	livenesTimeoutCounter--;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaConnectionListener#onMessage(com.ibm.ima.plugin.ImaSubscription, com.ibm.ima.plugin.ImaMessage)
     */
    public void onMessage(ImaSubscription sub, ImaMessage msg) {
    	byte[] body = msg.getBodyBytes();
    	String topic = msg.getDestination();
    	int tpcLen = topic.length();
    	int rLen = tpcLen + 2 + body.length;
    	int subqos = 0, msgqos = 0, qos = 0;
    	
    	if (msg.getReliability() == ImaReliability.qos1)
    		msgqos = 1;
    	else if (msg.getReliability() == ImaReliability.qos2)
    		msgqos = 2;
    	
    	if (sub.getReliability() == ImaReliability.qos1)
    		subqos = 1;
    	else if (sub.getReliability() == ImaReliability.qos2)
    		subqos = 2;
    	
    	if (msgqos <= subqos)
    		qos = msgqos;
    	else
    		qos = subqos;
    	
    	// create MQTT header
    	byte[] header = new byte[1+MaxLengthFieldByte];
    	if (qos == 1){  // 2 byte msgId field
    		rLen += 2;
    		header[0] = 0x32;
    	}
    	else if (qos == 2){
    		rLen += 2;
    		header[0] = 0x34;
    	}
    	else
    		header[0] = 0x30;
    	
    	int len = rLen, digit, i=1;
    	// generate remain length field
    	while (len > 0){
    		digit = len % 128;
    		len = len / 128;
    		if (len > 0)
    			digit = digit | 0x80;
    		header[i] = (byte)digit;
    		i++;
    	}
    	byte[] data = new byte[i+rLen];
    	System.arraycopy(header, 0, data, 0, i);
    	header = null;
    	//generate topicLen field
    	data[i] = (byte)(tpcLen / 256);
    	data[i+1] = (byte)(tpcLen % 256);
    	System.arraycopy(topic.getBytes(), 0, data, i+2, tpcLen);
    	if (qos == 1){
    		int trytime = 0;
 /*   		
  * Removing qos1 support due to msgidstatus large memory requirements 
  * 
  * while (MsgIdStatus[nextAvail] != AVAIL ){
    			nextAvail = (nextAvail + 1) % 65536;
    			if (nextAvail == 0)
    				nextAvail ++;
    			trytime ++;
    			if (trytime == 65536) {//no available msgID
    				System.err.println("No available msgId.");
    				return;
    			}
    		}
    		MsgIdStatus[nextAvail] = PUBLISH;
    		MsgIdMap.put((Integer)nextAvail, msg);
    		i += 2;
    		data[i+tpcLen] = (byte)(nextAvail / 256);
    		data[i+1+tpcLen] = (byte)(nextAvail % 256);
    		nextAvail = (nextAvail + 1) % 65536;
			if (nextAvail == 0)
				nextAvail ++; 
	*/
    	}
    	else if (qos == 2){
    		
    	}
    		
    	System.arraycopy(body, 0, data, i+2+tpcLen, body.length);
        connect.sendData(data);
    }

    /*
     * Completion event.
     * @see com.ibm.ima.plugin.ImaConnectionListener#onComplete(java.lang.Object, int, java.lang.String)
     */
    public void onComplete(Object obj, int rc, String message) {
        //System.out.println("onComplete: " + obj + " rc=" + rc + " msg=" + message);
        if (obj != null) {
            if (obj instanceof ImaMessage) {           	
            } 
            else if (obj instanceof Integer){
            	Integer msgid = (Integer)obj;
            	sendAck(msgid.intValue(),rc, 2, 0);  // send PUBACK to producer
            }
            else if (obj instanceof ImaSubscription) {
            	ImaSubscription sub = (ImaSubscription)obj;
            	Integer msgid=(Integer)sub.getUserData();
            	int qos = 0;  // granted QoS
            	if ( sub.getReliability() == ImaReliability.qos1){
            		qos = 1;
            	}
            	else if ( sub.getReliability() == ImaReliability.qos2){
            		qos = 2;
            	}
            	if (sub.isSubscribed()) {
            		//Subscribe Action
                    sendAck(msgid.intValue(), rc, 0, qos);
            	}else{
            		sendAck(msgid.intValue(), rc, 1, qos);  
                    SubMap.remove(sub.getDestination());
            	}
            } 
            else if (obj == this) {
            	//connect.sendData("{ \"Action\": \"Connected\" }");
            }
        } else {
            System.out.println("onComplete unknown object");
        }
    }

    /*
     * HTTP is not supported.
     * @see com.ibm.ima.plugin.ImaConnectionListener#onHttpData(java.lang.String, java.lang.String, java.lang.String, java.lang.String, byte[])
     */
    public void onHttpData(String op, String path, String query, String content_type, byte[] data) {
        connect.close();   /* Should never be called */
    }

	@Override
	public void onGetMessage(Object correlate, int rc, ImaMessage message) {
		// TODO Auto-generated method stub
		
	}

}
