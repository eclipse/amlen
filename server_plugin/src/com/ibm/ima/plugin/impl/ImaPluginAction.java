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

package com.ibm.ima.plugin.impl;

import java.io.IOException;
import java.lang.management.ManagementFactory;
import java.lang.management.MemoryMXBean;
import java.lang.management.ThreadMXBean;
import java.nio.ByteBuffer;
import java.util.Map;

import com.ibm.ima.plugin.ImaConnection;

import java.lang.management.GarbageCollectorMXBean;

/*
 * The action defines a communications between server and plugin processes.
 *
 * The action consists of a fixed six byte frame, a set of header fields,
 * properties, and a body.
 *
 * The frame consists of a one byte command, a one byte options field, and a four byte big-endian
 * length of the action not including the frame.
 *
 * The lower four bits of the options byte give the number of header fields.  Each field
 * follows the id and is encoded in concise encoding.  Optionally following this is a
 * map object which represents a set of properties, and a byte array which represents
 * the body.  Either the properties or body may be null or missing (and are for most actions).
 *
 * Configuration actions from the server to the plugin a synchronous and wait for a SyncReply.
 * These are run in the control channel except for InitChannel which is run in processing channels.
 *
 * For asynchronous global requests, the sequence number is
 */
public class ImaPluginAction {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /* Server to plug-in - configuration */
    public static final int Initialize              = 1;   /* h0=id p=config */
    public static final int StartMessaging          = 2;   /* h0=role */
    public static final int Terminate               = 3;   /* h0=rc, h1=reason */
    public static final int DefinePlugin            = 4;   /* p=properties */
    public static final int Endpoint                = 5;   /* p=properties */
    public static final int InitChannel             = 6;   /* h0=id */

    /* Server to plug-in - global async */

    public static final int Authenticate            = 10;  /* h0=seqnum h1=conntype, h1=connid, p=connection */
    public static final int GetStats                = 13;  /* */
    public static final int InitConnection          = 0x0F;/* Do not change this value */

    /* Server to plug-in - connection */
    public static final int OnConnection            = 20;  /* h0=connect h1=seqnum h2=conntype p=props b=bytes */
    public static final int OnClose                 = 21;  /* h0=connect h1=seqnum h2=rc, h3=reason */
    public static final int OnComplete              = 22;  /* h0=connect h1=seqnum h2=rc h3=data */
    public static final int OnData                  = 23;  /* h0=connect h1=seqnum, b=bytes */
    public static final int OnMessage               = 24;  /* h0=connect h1=seqnum, h2=msgtype, h3=flags, h4=subname, h5=dest, p=properties, b=body */
    public static final int OnConnected             = 26;  /* h0=connect h1=seqnum h2=rc, h3=reason */
    public static final int OnLivenessCheck         = 27;  /* h0=connect */
    public static final int OnHttpData              = 28;  /* h0=connect, h1=op, h2=path, h3=query, p=headers+cookies, b=data */
    public static final int SuspendDelivery         = 29;  /* h0=connect */
    public static final int OnGetMessage            = 30;  /* h0=connect h1=seqnum, h2=rc, h3=msgtype, h4=flags, h5=dest, p=properties, b=body */

    /* Plug-in to server - global */
    public static final int NewConnection           = 41;  /* h0=seqnum, h1=prococol */
    public static final int Stats                   = 44;  /* h0=seqnum, p=stats */
    public static final int Reply                   = 45;  /* h0=rc */
    public static final int Log                     = 46;  /* h0=msgid, h2=sev, h2=category, h3=file, h4=line, h5=msg, p=repl */

    /* Plug-in to server - connection */
    public static final int SendData                = 50;  /* h0=connect b=bytes */
    public static final int Accept                  = 51;  /* h0=connect h1=protocol h2=protfamily h3=plugin_name */
    public static final int Identify        	    = 52;  /* h0=connect h1=seqnum h2=rc h3=keepalive h4=maxMsgInFlight p=connection */
    public static final int Subscribe               = 53;  /* h0=connect h1=seqnum h2=subid, h3=flages =4=share h5=dest h6=name */
    public static final int CloseSub                = 55;  /* h0=connect h1=seqnum h2=dest h3=name h4=flags */
    public static final int DestroySub              = 56;  /* h0=connect h1=seqnum h2=subname h3=share */
    public static final int Send                    = 57;  /* h0=connect h1=seqnum h2=msgtype, h3=flsgs, p4=dest p=props b=body */
    public static final int Close                   = 58;  /* h0=connect h1=seqnum h2=rc h3=reason */
    public static final int AsyncReply              = 59;  /* h0=connect h1=seqnum h2=rc h3=string p=props */
    public static final int SetKeepAlive            = 60;  /* h0=connect h1=keepAliveTimeout */
    public static final int Acknowledge             = 61;  /* h0=connect h1=seqnum h2=rc */
    public static final int DeleteRetain            = 62;  /* h0=connect h1=topic */
    public static final int SendHttp                = 63;  /* h0=connect h1=rc h2=content_type, p=map, b-body */
    public static final int ResumeDelivery          = 64;  /* h0=connect h1=msgCount */
    public static final int GetMessage		        = 65;  /* h0=connect h1=seqnum h2=topic */
    public static final int CreateTransaction       = 66;  /* h0=connect h1=seqnum */
    public static final int CommitTransaction       = 67;  /* h0=connect h1=seqnum h2=txID */
    public static final int RollbackTransaction     = 68;  /* h0=connect h1=seqnum h2=txID */
    public static final int UpdateProperties	    = 69; 

    ByteBuffer              bb;
    int                     action;
    ImaChannel              channel;
    Object                  obj;

    static ImaPluginImpl[]  plugins         = new ImaPluginImpl[50];
    static int              plugin_count    = 0;

    /*
     * Constructor for an action.
     * @param action The action
     * @param size The initial size of the action
     * @param headers The count of headers
     */
    public ImaPluginAction(int action, int size, int headers) {
        if (size < 256)
            size = 256;
        this.action = action;
        bb = ByteBuffer.allocate(size);
        bb.put(0, (byte) action);
        bb.put(1, (byte) headers);
        bb.position(6);
    }

    /*
     * Simple constructor for an action
     */
    public ImaPluginAction(int act) {
        this(act, 256, 0);
    }

    /*
     * Create an incoming action
     */
    public ImaPluginAction(ImaChannel channel, byte[] b, int len) {
        this.channel = channel;
        bb = ByteBuffer.wrap(b, 0, len);
        if (len > 0) {
            action = b[0];
        }
    }

    /*
     * Set the header count for an action
     */
    public void setHeaders(int headers) {
        bb.put(1, (byte) headers);
    }

    public void setObject(Object obj) {
        this.obj = obj;
    }

    /*
     * Send the action to a specified channel
     */
    public void send(ImaChannel channel) {
        bb.putInt(2, bb.position() - 6);
        bb.flip();
        channel.send(bb);
    }

    /*
     * Send the action with a byte array to the specified channel
     */
    public void send(ImaChannel channel, byte[] b, int len) throws IOException {
        if (b != null && len > 0) {
            bb = ImaPluginUtils.ensureBuffer(bb, len + 32);
            ImaPluginUtils.putSmallValue(bb, len, ImaPluginConstants.S_ByteArray);
            bb.put(b);
        }
        send(channel);
    }

    /*
     * Send a sync reply
     */
    public void doReply(int rc) {
        ImaPluginAction action = new ImaPluginAction(Reply, 32, 1);
        action.bb = ImaPluginUtils.putIntValue(action.bb, rc);
        if (ImaPluginMain.trace.isTraceable(7)) {
            ImaPluginMain.trace.trace("Reply channel " + channel.id + ": " + rc);
        }
        action.send(channel);
    }

    /*
     * Process an incoming action
     */
    public void process() {
        int i;
        Object[] hdr = null;
        Map<String, Object> props = null;
        byte[] body = null;
        ImaMessageImpl msg = null;
        Object obj;
        int connectid;
        int seqnum;
        int conntype;
        ImaConnectionImpl connect;
        int rc;

        int headers = bb.get(1);
        /*
         * System.out.println("action=" + action + " channel=" + channel.id + " headers=" + headers + " length=" +
         * bb.remaining());
         */
        bb.position(6);
        if (headers > 0) {
            hdr = new Object[headers];
            for (i = 0; i < headers; i++) {
                hdr[i] = ImaPluginUtils.getObjectValue(bb);
            }
        }
        if (bb.remaining() > 0) {
            props = ImaPluginUtils.getMapValue(bb, msg);
            if (bb.remaining() > 0) {
                obj = ImaPluginUtils.getObjectValue(bb);
                if (obj instanceof byte[]) {
                    body = (byte[]) obj;
                }
            }
        }
        switch (action) {
        case OnData: /* h0=connect h1=seqnum, b=bytes */
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                connect.onData(body);
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnData: connection not found: " + connectid);
                }    
            }
            break;

        case OnMessage: /* h0=connect h1=seqnum, h2=msgtype, h3=flags, h4=subname, h5=dest, p=properties, b=body */
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                seqnum = ((Number) hdr[1]).intValue();
                int msgtype = 0;
                if (hdr[2] instanceof Number)
                    msgtype = ((Number) hdr[2]).intValue();
                int flags = 0;
                if (hdr[3] instanceof Number)
                    flags = ((Number) hdr[3]).intValue();
                String subname = (String) hdr[4];
                String dest = (String) hdr[5];
                msg = new ImaMessageImpl(connect, seqnum, msgtype, flags, dest, props, body);
                connect.onMessage(subname, msg);                
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnMessage: connection not found: " + connectid);
                }  
            }
            /* TODO: ack non-zero seqnum */
            break;
            
        case OnGetMessage: /* h0=connect h1=seqnum, h2=rc, h3=msgtype, h4=flags, h5=dest, p=properties, b=body */
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            rc = ((Number) hdr[2]).intValue();
            if (ImaPluginMain.trace.isTraceable(8)) {
                 ImaPluginMain.trace.trace("OnGetMessage: connect=" + connectid + " rc=" + rc);
            }     
            if (connect != null) {
                seqnum = ((Number) hdr[1]).intValue();
                if (rc == 0) {
	                int msgtype = 0;
	                if (hdr[3] instanceof Number)
	                    msgtype = ((Number) hdr[3]).intValue();
	                int flags = 0;
	                if (hdr[4] instanceof Number)
	                    flags = ((Number) hdr[4]).intValue();
	                String dest = (String) hdr[5];
	                msg = new ImaMessageImpl(connect, seqnum, msgtype, flags, dest, props, body);
                }
                /*Get Correlate Data*/
                ImaPluginAction workact = null;
                Object correlate = null;
                if (seqnum != 0 && seqnum != 1) {
                    workact = connect.getWork(seqnum);
                    correlate = workact.obj;
                }
                connect.listener.onGetMessage( correlate, rc, msg);                
            } else {
                ImaPluginMain.trace.trace("OnGetMessage: Connection not found: " + connectid);
            }
            break;

        case OnHttpData:
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                connect.onHttpData(body, (char) ((Number) hdr[1]).intValue(), (String) hdr[2], (String) hdr[3], props);
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnHttpData: connection not found: " + connectid);
                }  
            }
            break;
            
        case OnComplete: /* h0=connect h1=seqnum h2=rc h3=data */
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);

            if (connect != null) {
                connect.onComplete(hdr);
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnComplete: connection not found: " + connectid);
                }  
            }
            break;

        /* Server to plug-in - global async */
        case OnConnection:     /* h0=connect h1=seqnum h2=conntype ?h3=new_connect p=props b=bytes */ 
            connectid = ((Number)hdr[0]).intValue();
            seqnum = ((Number)hdr[1]).intValue();
            conntype = ((Number)hdr[2]).intValue();
            if (conntype == 3) {
                int newconnid = ((Number)hdr[3]).intValue();
                connect = ImaConnectionImpl.connects.get(connectid);
                if (connect != null && newconnid != 0) {
                    ImaConnectionImpl.connects.remove(connectid);
                    ImaConnectionImpl.connects.put(newconnid, connect);
                    connect.connectID = newconnid;
                    connect.index = newconnid;
                    connect.channel = channel;
                    connect.state = ImaConnection.State_Accepted;
                    Object pobj = props.get("Protocol");
                    String protocol = connect.plugin.alias;
                    if (pobj != null && pobj instanceof String) {
                        protocol = (String)pobj;
                    }
                    connect.listener = connect.plugin.plugin.onConnection(connect, protocol);
                    if (connect.listener == null) {
                        connect.close(0, "Connection is rejected by the plugin");
                        break;
                    }
                    connect.listener.onData(new byte[0], 0, 0);
                    break;
                } else {
                    if (ImaPluginMain.trace.isTraceable(1)) {
                        ImaPluginMain.trace.trace("OnConnection: virtual connection not found: " + connectid);
                    }  
                }
            } else {     
            	connect = ImaConnectionImpl.connects.get(connectid);
            	if (connect == null)
                connect = new ImaConnectionImpl(channel, connectid, props);
            }    
            if (connect != null) {
                if (connect.getEndpoint() != null) {
                    switch (conntype) {
                    /*
                     * Handle connection type 0 (TCP) where we have a buffer of data
                     */
                    case 0:         /* TCP */
                        connect.onData(body);
                        break;
                    /*
                     * Handle connection type 1 (WebSockets) and type 2 (HTTP) where the protocol is known
                     */
                    case 1:         /* WebSockets */  
                    case 2:         /* HTTP       */
                        connect.onData(new byte[0]);
                        break;
                    }
                } else {
                    connect.close(351, "The Endpoint is not configured");
                }
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnConnection: connection not found: " + connectid);
                }  
            }
            break;
                
        case SuspendDelivery:
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                connect.suspendMessageDelivery();
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("SuspendDelivery: connection not found: " + connectid);
                }
            }    
            break;
            
        case ResumeDelivery:
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                connect.resumeMessageDelivery();
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("ResumeDelivery: connection not found: " + connectid);
                }  
            }
            break;
            
        case Initialize: /* p=ext-props */
            if (ImaPluginMain.trace.isTraceable(2)) {
                ImaPluginMain.trace.trace("Initialize: " + props);
            }
            ImaPluginMain.setConfig(props);
            channel.setID((Integer) hdr[0]);
            doReply(0);
            break;

        case StartMessaging: /* h0=role */
            int role = 0;
            if (hdr[0] instanceof Number)
                role = ((Number) hdr[0]).intValue();
            if (ImaPluginMain.trace.isTraceable(2)) {
                ImaPluginMain.trace.trace("Start Messaging: " + role);
            }
            for (i = 0; i < plugin_count; i++) {
                plugins[i].plugin.startMessaging(role != 0);
            }
            doReply(0);
            break;

        case Terminate: /* h0=rc, h1=message */
            rc = 0;
            if (hdr[0] instanceof Number)
                rc = ((Number) hdr[0]).intValue();
            for (i = 0; i < plugin_count; i++) {
                plugins[i].plugin.terminate(rc);
            }
            ImaPluginMain.terminate(rc, (String) hdr[1]);
            doReply(0);
            break;

        case DefinePlugin: /* p=properties */
            try {
                if (ImaPluginMain.trace.isTraceable(2)) {
                    ImaPluginMain.trace.trace("Define plug-in: " + props);
                }
                ImaPluginImpl pii = new ImaPluginImpl(props, channel);
                if (plugin_count >= plugins.length) {
                    doReply(101); /* fix rc */
                } else {
                    plugins[plugin_count++] = pii;
                    doReply(0);
                }
            } catch (Exception e) {
                e.printStackTrace(System.out);
                doReply(100);
            }
            break;

        case Endpoint: /* p=properties */
            if (ImaPluginMain.trace.isTraceable(2)) {
                ImaPluginMain.trace.trace("Define endpoint: " + props);
            }
            try {
                new ImaEndpointImpl(props);
                doReply(0);
            } catch (Exception e) {
                doReply(100);
            }
            break;

        case InitChannel: /* h0=id */
            channel.setID((Integer) hdr[0]);
            if (ImaPluginMain.trace.isTraceable(2)) {
                ImaPluginMain.trace.trace("initChannel: " + (Integer) hdr[0]);
            }
            doReply(0);
            break;

        case Authenticate: /* h0=seqnum h1=conntype, h1=connid, p=connection */
            /* TODO */
            break;

        case GetStats: /* */
            int stattype = ((Number) hdr[0]).intValue();
            MemoryMXBean mm = ManagementFactory.getMemoryMXBean();    
            {
                ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Stats, 512, 5);
                ImaPluginUtils.putIntValue(action.bb, stattype);
                ImaPluginUtils.putLongValue(action.bb, mm.getHeapMemoryUsage().getMax());
                ImaPluginUtils.putLongValue(action.bb, mm.getHeapMemoryUsage().getUsed());
                ImaPluginUtils.putIntValue(action.bb, getGCRate());
                ImaPluginUtils.putIntValue(action.bb, getProcessCPU());
                action.send(channel);
            }
            
            break;

        case OnClose: /* h0=connect h1=seqnum h2=rc, h3=reason */
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                /* Call the plugin listener and then reply with the close */
                connect.listener.onClose(((Number) hdr[2]).intValue(), (String) hdr[3]);
                connect.close(((Number) hdr[2]).intValue(), (String) hdr[3]);
            } else {
                if (ImaPluginMain.trace.isTraceable(2)) {
                    ImaPluginMain.trace.trace("OnClose: connection not found: " + connectid);
                }
                /* Send a reply anyway */
                ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Close, 256, 4);
                ImaPluginUtils.putIntValue(action.bb, connectid);
                ImaPluginUtils.putIntValue(action.bb, 0); /* seqnum */
                ImaPluginUtils.putIntValue(action.bb, -1); /* rc */
                ImaPluginUtils.putNullValue(action.bb);
                action.send(channel);
            }

            break;

        case UpdateProperties: 
        	if (props!=null) {
        		String pluginname = ImaPluginImpl.getStringProperty(props, "Name");
        		ImaPluginImpl iplugin = findPluginByName(pluginname);
        		iplugin.updateConfig(props);
        	}
            break;
            
        case OnLivenessCheck:
            connectid = ((Number) hdr[0]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                connect.onLivenessCheck();
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnLivenessCheck: connection not found: " + connectid);
                }
            }
            break;
            
        case OnConnected: /* h0=connect h1=seqnum h2=rc, h3=reason */
            connectid = ((Number) hdr[0]).intValue();
            seqnum = ((Number) hdr[1]).intValue();
            connect = ImaConnectionImpl.connects.get(connectid);
            if (connect != null) {
                int xrc = ((Number) hdr[2]).intValue();
                if (xrc == 0) 
                    connect.state = ImaConnection.State_Open;
                connect.listener.onConnected(xrc, (String)hdr[3]);
                if (xrc != 0)
                    connect.close(xrc, (String)hdr[3]);
            } else {
                if (ImaPluginMain.trace.isTraceable(1)) {
                    ImaPluginMain.trace.trace("OnConnected: " + connectid);
                }
            }
            break;
        }
    }
    
    /*
     * Get the current garbage collection count
     */
    static public long getCurrentGC() {
        int total = 0;
        for (GarbageCollectorMXBean gc : ManagementFactory.getGarbageCollectorMXBeans()) {
            long count = gc.getCollectionCount();
            if (count >= 0) {
                total += count;
            }
        }  
        return total;
    }
    
    /*
     * Variables to keep previous time and count
     */
    static long baseStatTime = System.currentTimeMillis();
    static long lastStatTime = baseStatTime;
    static long baseGcCount = getCurrentGC();
    static long lastGcCount = baseGcCount;
    
    /*
     * Calculate the current garbage collection rate.
     * If the base stat time was at least 5 seconds ago, use it as the base, otherwise
     * retain the old base.
     */
    static public int  getGCRate() {
        long now = System.currentTimeMillis();
        long nowCount = getCurrentGC();
        
        if (now-lastStatTime > 5000) {
            baseStatTime = lastStatTime;
            baseGcCount = lastGcCount;
        }
        int ret = (int)(((nowCount - baseGcCount) * 60000) / (now-baseStatTime));
        lastStatTime = now;
        lastGcCount = nowCount;
        return ret;
    }
    
    static long baseCpuTime  = System.currentTimeMillis();
    static long lastCpuTime  = baseCpuTime;
    static long baseCpuValue = 0;
    static long lastCpuValue = 0;
    
    /*
     * Get the process CPU.  
     */
    static public int  getProcessCPU() {
        ThreadMXBean tm = ManagementFactory.getThreadMXBean();
        long [] threads = tm.getAllThreadIds();
        long cpuns = 0;
        for (long threadid: threads) {
            long ns = tm.getThreadCpuTime(threadid);
            if (ns > 0)
                cpuns += ns;
        }   
        long now = System.currentTimeMillis();
        
        if (now-lastCpuTime > 900) {
            baseCpuTime = lastCpuTime;
            baseCpuValue = lastCpuValue;
        }
        long delta = (cpuns - baseCpuValue) / (now-baseStatTime);
        lastCpuTime = now;
        lastCpuValue = cpuns;
        if (delta < 0)
            delta = 0;

        return (int)(delta / 1000000);
    }
    

    /*
     * Action name for diagnostics
     */
    static public String getActionName(int action) {
        switch (action) {
        case Initialize             : return "Initialize";
        case StartMessaging         : return "StartMessaging";
        case Terminate              : return "Terminate";
        case DefinePlugin           : return "DefinePlugin";
        case Endpoint               : return "Endpoint";
        case Authenticate           : return "Authenticate";
        case GetStats               : return "GetStats";
        case InitConnection         : return "InitConnection";
        case OnConnection           : return "OnConnection";
        case OnClose                : return "OnClose";
        case OnComplete             : return "OnComplete";
        case OnConnected            : return "OnConnected";
        case OnLivenessCheck        : return "ConLivenessCheck";
        case OnHttpData             : return "OnHttpData";
        case SuspendDelivery        : return "SuspendDelivery";
        case OnData                 : return "OnData";
        case OnMessage              : return "OnMessage";
        case Stats                  : return "Stats";
        case Reply                  : return "Reply";
        case Log                    : return "Log";
        case SendData               : return "SendData";
        case Accept                 : return "Accept";
        case Identify               : return "Identify";
        case Subscribe              : return "Subscribe";
        case CloseSub               : return "CloseSub";
        case DestroySub             : return "DestroySub";
        case Send                   : return "Send";
        case Close                  : return "Close";
        case AsyncReply             : return "AsyncReply";
        case SetKeepAlive           : return "SetKeepAlive";
        case Acknowledge            : return "Acknowledge";
        case DeleteRetain           : return "DeleteRetain";
        case SendHttp               : return "SendHttp";
        case ResumeDelivery         : return "ResumeDelivery";
        case CreateTransaction      : return "CreateTransaction";
        case CommitTransaction      : return "CommitTransaction";
        case RollbackTransaction    : return "RollbackTransaction";
        }
        return "Unknown";
    }

    /*
     * Find a plug-in when the protocol name is known
     */
    static ImaPluginImpl findPlugin(String protocol) {
        if (protocol.length() > 0 && protocol.charAt(0)=='/') {
            for (int i = 0; i < plugin_count; i++) {
                if (plugins[i].alias!=null && plugins[i].alias.equals(protocol))
                    return plugins[i];
            }
        } else {
	        for (int i = 0; i < plugin_count; i++) {
	            if (plugins[i].isWebSocket(protocol))
	                return plugins[i];
	        }
        }
        return null;
    }
    

    /*
     * Find a plug-in when the name is known
     */
    static ImaPluginImpl findPluginByName(String name) {
    	if (name != null) {
	        for (int i = 0; i < plugin_count; i++) {
	            if (plugins[i].getName().equals(name))
	                return plugins[i];
	        }
    	}
        return null;
    }

    /*
     * Find the plug-in from data
     */
    static int findPlugin(ImaConnectionImpl connect, byte[] body) {
        boolean rejected = true;
        for (int i = 0; i < plugin_count; i++) {
            if (plugins[i].isInitialByte(body[0])) {
                int more = plugins[i].plugin.onProtocolCheck((ImaConnection) connect, body);
                if (more < 0)
                    continue;
                rejected = false;
                if (more == 0)
                    return i;
            }
        }
        if (rejected)
            return -1;
        return -2;
    }
}
