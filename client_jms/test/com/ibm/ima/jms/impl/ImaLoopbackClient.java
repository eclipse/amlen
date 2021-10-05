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
package com.ibm.ima.jms.impl;

import java.nio.ByteBuffer;
import java.util.Enumeration;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import javax.jms.Connection;

import com.ibm.ima.jms.impl.ImaAction;
import com.ibm.ima.jms.impl.ImaClient;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaCreateConsumerAction;
import com.ibm.ima.jms.impl.ImaMessage;
import com.ibm.ima.jms.impl.ImaMessageConsumer;
import com.ibm.ima.jms.impl.ImaProducerAction;


/*
 * This class is a fake client used in unit test.
 *   
 * When an action is sent, it returns a good return code.
 * When a message is sent, it sends an ack response (if requested) and queues the message to
 * be returned on the next receive call.  When using this code you should always ask for the 
 * reply after doing an action without any intervening requests.  
 * 
 * UPDATE: messages are placed on the sync delivery list in the ImaConnection
 * to replace the functionality currently missing in the engine - sync message delivery.
 * Receive call will retrieve the next message on the list for this session. This overrides
 * the above-listed restriction until the engine restriction is lifted.
 * 
 * This code is very stupid and is designed only for unit test to avoid using a server.
 */
public class ImaLoopbackClient extends ImaClient {
	static final AtomicInteger idCount = new AtomicInteger(1);
    ImaMessage message;
    AtomicLong msg_seqnum = new AtomicLong();
    ByteBuffer body = ByteBuffer.allocate(32*1024);
    ImaLoopbackClient() {
        super(null);
    }
    
    public ImaLoopbackClient(Connection conn) {
        this();
        connect = (ImaConnection) conn;
        connect.isloopbackclient = true;
		connect.instanceHash = instanceHash(System.currentTimeMillis());
    }

    protected void closeImpl() {
    }
    
    public void run() {       
    }
    
    /*
     * Send an action
     */
    public void send(ByteBuffer bb, boolean flush) {
        long respid = 0;
        byte item_type = bb.get(ImaAction.ITEM_TYPE_POS);
        @SuppressWarnings("unused")
		int session_id = -1;
        @SuppressWarnings("unused")
        int consumer_id = -1;
        
        if (item_type == ImaAction.ItemTypes.None.value()) {

        } else if (item_type == ImaAction.ItemTypes.Thread.value()) {
            respid = bb.getLong(ImaAction.MSG_ID_POS);
        } else if (item_type == ImaAction.ItemTypes.Session.value()) {
            session_id = bb.getInt(ImaAction.ITEM_POS);
            respid = bb.getLong(ImaAction.MSG_ID_POS);
        } else if (item_type == ImaAction.ItemTypes.Consumer.value()) {
            consumer_id = bb.getInt(ImaAction.ITEM_POS);
            respid = bb.getLong(ImaAction.MSG_ID_POS);
        } else if (item_type == ImaAction.ItemTypes.Producer.value()) {
            respid = bb.getLong(ImaAction.MSG_ID_POS);
        }
        int actionType = bb.get(ImaAction.ACTION_POS);
        if (!flush)
        	return;
        
        ImaAction act = ImaAction.getAction(respid);

        if (actionType == ImaAction.message || actionType == ImaAction.messageWait) {
//            try {
//                message = (ImaAction) act.clone();
//                body.put(act.outBB);
//                body.flip();
////                System.arraycopy(act.body, 0, message.body, 0, message.bodylen);
//            } catch (CloneNotSupportedException e) {
//                e.printStackTrace();
//                throw new RuntimeException(e);
//            }   /* clone the message */
            
            try {
              ImaMessageConsumer consumer = null;
              Enumeration <?> cEn = connect.consumerMap.elements();
              while (cEn.hasMoreElements()) {
            	  consumer = (ImaMessageConsumer)cEn.nextElement();
                  if (consumer != null && consumer.dest != null && act instanceof ImaProducerAction) { 
                	  String destname = consumer.dest.toString();
                	  if (destname == null) 
                		  destname = "";
                	  if (destname.equals(((ImaProducerAction)act).senddestname)) {
                          message = ImaMessage.makeMessage(bb, consumer.session, consumer);
                          message.ack_sqn = msg_seqnum.incrementAndGet();
                          consumer.receivedMessages.add(message);
                          break;
                	  }
                  }
              }
            } catch (Exception e) {
                e.printStackTrace();
            }

            if (act != null) {
	            act.rc = 0;
	            act.action = ImaAction.reply;
	            act.complete();
            }

        }
        else if (act.action == ImaAction.receiveMsg || act.action == ImaAction.receiveMsgNoWait) {
            if (message == null) {
                act.rc = -1;
                act.action = ImaAction.reply;
            } else {
                act.rc = 0;
                act.action = ImaAction.message;
                if ((act.inBB == null) || (act.inBB.capacity() < body.limit()))
                    act.inBB = ByteBuffer.allocate(body.capacity());
                act.inBB.clear();
                act.inBB.put(body);
                act.inBB.flip();
//                act.bodylen = message.bodylen;
                act.hdrpos = ImaAction.HEADER_START;
//                act.bodypos = ImaAction.HEADER_START+ImaAction.HEADER_LENGTH;           /* skip over header and length */
                message = null;
                body.clear();
            }
            act.complete();
        }
        else if (act.action == ImaAction.createDurable) {
            act.rc = 0;
            if ((act.inBB == null) || (act.inBB.capacity() < act.outBB.limit()))
                act.inBB = ByteBuffer.allocate(act.outBB.capacity());
            act.inBB.clear();
            act.inBB.put(act.outBB);
            act.inBB.flip();
    		ImaMessageConsumer consumer = ((ImaCreateConsumerAction)act).consumer;
    		consumer.consumerid = idCount.incrementAndGet();
    		connect.consumerMap.put(Integer.valueOf(consumer.consumerid),consumer);
            act.complete();
        }
        else {
        	if (act.action == ImaAction.createConsumer) {
        		ImaMessageConsumer consumer = ((ImaCreateConsumerAction)act).consumer;
        		consumer.consumerid = idCount.incrementAndGet();
        		connect.consumerMap.put(Integer.valueOf(consumer.consumerid),consumer);
        	}
            act.rc = 0;
            act.action = ImaAction.reply;
            if ((act.inBB == null) || (act.inBB.capacity() < act.outBB.limit()))
                act.inBB = ByteBuffer.allocate(act.outBB.capacity());
            act.inBB.clear();
            act.inBB.put(act.outBB);
            act.inBB.flip();
            act.complete();
        }
    }
}
