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

import java.io.File;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaPluginConfigValidator;
import com.ibm.ima.plugin.ImaPluginListener;

/*
 * A instance of this class is the proxy for a plugin and is passed to the Plugin at init time.
 */
public class ImaPluginImpl implements ImaPlugin {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private   String    name;
    private   String    protocol;
    private   String    classname;
    private   String [] classpath;
    private   String [] websocket;
              String [] httpheader;
    private   byte []   initial_byte;
    private   int       initial_byte_count;
    private   int       capability;
           	  String 	author;
           	  String 	version;
           	  int 		modification;
           	  String	copyright;
           	  String	build;
              String	description;
           	  String	license;
           	  String	title;
           	  String	alias;
    
    private   ImaPluginTraceImpl trace;
    private   Map<String,Object> props;
              ImaPluginListener plugin;
              ImaChannel        channel;
    private final boolean		isValidator;
    private final URLClassLoader loader;
    static    int     vconn = 0x7f000000; 

    static final String CONFIG_DIR;

    static {
        Map<String, String> env = System.getenv();
        if (env.containsKey("IMA_CONFIG_DIR")) {
            CONFIG_DIR = env.get("IMA_CONFIG_DIR");
        } else {
            //Install Path set during build based on server_build/paths.properties
            CONFIG_DIR = "IMA_SVR_INSTALL_PATH/config/";
        }
    }
    
    static String PLUGINS_DIR = CONFIG_DIR + "plugin/plugins/";
    static HashMap<String,ImaPluginImpl> plugins = new HashMap<String,ImaPluginImpl>();
    
    @SuppressWarnings("unchecked")
    /*
     * Constructor from map
     */
    ImaPluginImpl(Map<String,Object> map, ImaChannel channel) throws ClassNotFoundException, NoSuchMethodException, 
            SecurityException, InstantiationException, IllegalAccessException, 
            IllegalArgumentException, InvocationTargetException, MalformedURLException  {
        final String pluginFolder;
        this.channel = channel;
        this.name = getStringProperty(map, "Name");
        if (this.name == null)
            throw new IllegalArgumentException("Plugin definition does not include name");
        if(channel == null) {
        	isValidator = true;
            this.trace = ImaPluginInstaller.trace;
            pluginFolder = getStringProperty(map, "ValidateConfigFolder") + File.separator;
            map.remove("ValidateConfigFolder");
        } else {
            pluginFolder = PLUGINS_DIR + name + File.separator;
            this.trace = ImaPluginMain.trace;
        	isValidator = false;
        }
        this.protocol   = getStringProperty(map, "Protocol");
        this.classname  = getStringProperty(map, "Class");
        this.capability = getIntProperty(map, "Capabilities", 0);
        Object obj = map.get("Properties");
        if (obj instanceof Map)
            this.props      = (Map<String,Object>)obj;
        this.classpath 		= getStringArray(map, "Classpath");
        this.websocket  	= getStringArray(map, "WebSocket");
        this.httpheader  	= getStringArray(map, "HttpHeader");
        this.author  		= getStringProperty(map, "Author");
        this.version  		= getStringProperty(map, "Version");
        this.copyright  	= getStringProperty(map, "Copyright");
        this.modification  	= getIntProperty(map, "Modification", 1);
        this.build  		= getStringProperty(map, "Build");
        this.description  	= getStringProperty(map, "Description");
        this.license  		= getStringProperty(map, "License");
        this.title  		= getStringProperty(map, "Title");
        this.alias  		= getStringProperty(map, "Alias");
        
        String  initialByte = getStringProperty(map, "InitialByte");
        if ("All".equals(initialByte)) {
            initial_byte_count = 256;
        } else if ("List".equals(initialByte)) {
            int  i;
            this.initial_byte = new byte[256];
            for (i=0; ; i++) {
                obj = map.get("InitialByte." + i);
                if (obj == null || !(obj instanceof Number))
                    break;    
                this.initial_byte[((Number)obj).intValue()&0xff] = 1;
                initial_byte_count++;
            }
        }
        URL urls[] = new URL[this.classpath.length];
        File pluginFile= null;
        File abPluginFile = null;
        int count=0;
        for (String path: this.classpath) {
        	pluginFile =  new File( path);
        	if (!pluginFile.isAbsolute()) {
                String abpath = pluginFolder + path;
        		abPluginFile = new File(abpath);
        	}else{
        		abPluginFile = pluginFile;
        	}
        	urls[count++] = abPluginFile.toURI().toURL();
        }
        loader = URLClassLoader.newInstance(urls, getClass().getClassLoader());
        if (trace.isTraceable(5)) {
            trace.trace("Create plug-in: "+this);
        }        

        @SuppressWarnings("rawtypes")
        Class cls = Class.forName(this.classname, true, loader);
        if (!ImaPluginListener.class.isAssignableFrom(cls)) {
        	throw new IllegalArgumentException("The class is not an ImaPluginListener");
        }
        Constructor<ImaPluginListener> init = cls.getConstructor(new Class[0]);
        if(init == null) {
        	throw new IllegalArgumentException("The class does not have an empty constructor");
        }
        plugin = init.newInstance();
        if(!isValidator) {
            try {
                plugins.put(name, this);
            	plugin.initialize(this);
            } catch (Throwable th) {
            	plugins.remove(name);
            	throw new RuntimeException("Initialization failed for plugin: " + name,th);
            }
        } else {
        	if (plugin instanceof ImaPluginConfigValidator) {
				((ImaPluginConfigValidator) plugin).validate(this);					
			}
        }
    }

    /*
     * Return a virtual connection ID.
     * The virtual connection ID is only used until the real connection ID is assigned by
     * the server.
     */
    static synchronized int virtualConnectionID() {
        if (vconn == 0x7fffffff)
            vconn = 0x7f000000;
        return vconn++;
    }
    
    
    /*
     * Get a plugin by name
     */
    public static ImaPluginImpl getPlugin(String name) {
        return plugins.get(name);
    }

    
    /*
     * Utility to return a string property
     */
    static String getStringProperty(Map<String,Object>map, String name) {
        Object obj = map.get(name);
        if (obj == null)
            return null;
        if (obj instanceof String)
            return (String)obj;
        return ""+obj;
    }
    
    /*
     * Utility to return an int property
     */
    static int getIntProperty(Map<String,Object>map, String name, int defval) {
        Object obj = map.get(name);
        if (obj == null)
            return defval;
        if (obj instanceof Number)
            return ((Number)obj).intValue();
        return defval;
    }
    
    /*
     * Get an array of properties
     */
    static String [] getStringArray(Map<String,Object>map, String name) {
        int count = 0;
        int i;
        for (i=0; ; i++) {
            Object obj = map.get(name + '.' + i);
            if (obj == null || !(obj instanceof String))
                break;
            count++;
        }
        
        String [] ret = new String[count];
        for (i=0; i<count; i++) {
            ret[i] = (String)map.get(name + '.' + i);
        }
        return ret;
    }
    
    
    /*
     * @see com.ibm.ima.plugin.ImaPlugin#createConnection(java.lang.String)
     */
    public ImaConnection createConnection(String protocol, String endpoint) {
    	if(isValidator) {
            throw new RuntimeException("This call is not allowed from validation code");    		
    	}
        ImaEndpointImpl endp = ImaEndpointImpl.getEndpoint(endpoint, false);
        if (endp == null) {
            throw new RuntimeException("The endpoint is not known");
        }
        ImaConnectionImpl connect = new ImaConnectionImpl(this, virtualConnectionID(), null, 0, endp, ImaConnectionImpl.F_Internal);
        
        connect.state = ImaConnection.State_Handshake;
        connect.setProtocol(protocol);
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.NewConnection, 512, 4);
        action.setObject(this);
        ImaPluginUtils.putIntValue(action.bb, connect.connectID);         /* h0 = connectID   */
        action.bb = ImaPluginUtils.putStringValue(action.bb, protocol);   /* h1 = protocol    */
        action.bb = ImaPluginUtils.putStringValue(action.bb, endpoint);   /* h2 = endpoint    */
        action.bb = ImaPluginUtils.putStringValue(action.bb, this.protocol);  /* h3 = protocol family */
        action.send(channel);
        return connect;
    }
    

    /*
     * @see com.ibm.ima.plugin.ImaPlugin#createMessage(com.ibm.ima.plugint.ImaMessageType)
     */
    public ImaMessage createMessage(ImaMessageType mtype) {
        return new ImaMessageImpl(mtype);
    }

    /*
     * @see com.ibm.ima.plugin.ImaPlugin#getName()
     */
    public String getName() {
        return name;
    }

    /*
     * @see com.ibm.ima.plugin.ImaPlugin#getProtocolFamily()
     */
    public String getProtocolFamily() {
        return protocol;
    }
    
    /*
     * @see com.ibm.plugin.ext.ImaPlugin#getConfig()
     */
    public Map<String, Object> getConfig() {
        return props;
    }
   
    /*
     * Package private get classpath
     */
    String [] getClasspath() {
        return classpath;
    }
    
    /*
     * Package private get websocket
     */
    String [] getWebsocket() {
        return websocket;
    }
    
    /*
     * Package private get classname
     */
    String getClassName() {
        return classname;
    }

    
    /*
     * Package private get capabilities
     */
    int getCapabilities() {
        return capability;
    }
    
    
    
    /*
     * @see com.ibm.ima.plugin.ImaPlugin#getAuthor()
     */
	public String getAuthor() {
		return author;
	}


    /*
     * @see com.ibm.ima.plugin.ImaPlugin#getVersion()
     */
	public String getVersion() {
		return version;
	}


	/*
	 * Package private get modification
	 */
	int getModification() {
		return modification;
	}


	/*
	 * @see com.ibm.ima.plugin.ImaPlugin#getCopyright()
	 */
	public String getCopyright() {
		return copyright;
	}


	/*
	 * @see com.ibm.ima.plugin.ImaPlugin#getBuild()
	 */
	public String getBuild() {
		return build;
	}


	/*
	 * @see com.ibm.ima.plugin.ImaPlugin#getDescription()
	 */
	public String getDescription() {
		return description;
	}


    /*
     * @see com.ibm.ima.plugin.ImaPlugin#getLicense()
     */
	public String getLicense() {
		return license;
	}

    /*
     * @see com.ibm.ima.plugin.ImaPlugin#getTitle()
     */
	public String getTitle() {
		return title;
	}


	/*
     * Return true if the byte is one of our initial bytes
     */
    boolean isInitialByte(byte b) {
        if (initial_byte_count >= 256)
            return true;
        if (initial_byte_count == 0 || initial_byte == null)
            return false;
        return initial_byte[b&0xff] != 0; 
        
    }
    
    /*
     * Check if this websockets protocol is acceptable for this plugin
     */
    boolean isWebSocket(String s) {
        int  i;
        if (websocket == null || s == null)
            return false;
        for (i=0; i<websocket.length; i++) {
            if (s.equalsIgnoreCase(websocket[i]))
                return true;
                
        }
        return false;
    }
    
    /*
     * Send a log item to the MessageSight logs.
     * 
     * @see com.ibm.ima.plugin.ImaPlugin#log(java.lang.String, int, java.lang.String, java.lang.String, java.lang.Object[])
     */
    public void log(String msgid, int level, String category, String msgformat, Object ... repl) {
    	if(isValidator) {
            throw new RuntimeException("This call is not allowed from validation code");    		
    	}
    	Exception e = new Exception();
        StackTraceElement [] elem = e.getStackTrace();
        String filen = "java";
        int    lineno = 0;
        
        if (elem.length > 1) {
            filen = elem[1].getFileName();
            if (filen == null) {
                filen = "java";
            } else {
                int pos = filen.length();
                while (pos > 0 && filen.charAt(pos-1) != '/' && filen.charAt(pos-1) != '\\') 
                    pos--;
                filen = filen.substring(pos);
            }    
            lineno = elem[1].getLineNumber();
        }    
        trace(4, "log " + (msgid != null ? msgid : "*") + " " + filen + ':' + lineno + " " +
                (category != null ? category : "*") + " " + msgformat); 
        
        /* Convert the repl data to a JSON string */
        HashMap<String,Object> map = null;
        if (repl.length > 0) {
            map = new HashMap<String,Object>(repl.length);
            for (int i=0; i<repl.length; i++) {
                map.put(""+i, repl[i]);    
            }
        }
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.Log, 1024, 6);
        action.setObject(this);
        action.bb = ImaPluginUtils.putStringValue(action.bb, msgid);     /* h0 = msgid     */
        action.bb = ImaPluginUtils.putIntValue(action.bb, level);        /* h1 = level     */
        action.bb = ImaPluginUtils.putStringValue(action.bb, category);  /* h2 = category  */
        action.bb = ImaPluginUtils.putStringValue(action.bb, filen);     /* h3 = filen     */
        action.bb = ImaPluginUtils.putIntValue(action.bb, lineno);       /* h4 = lineno    */ 
        action.bb = ImaPluginUtils.putStringValue(action.bb, msgformat); /* h5 = msgformat */
        action.bb = ImaPluginUtils.putMapValue(action.bb, map);     /* p = repl       */
        action.send(channel);
    }
   
    /**
     * Check if trace is allowed at the specified level.
     * @param level the trace level
     * @return true if the level is allowed to trace
     */
    public boolean isTraceable(int level) {
        return trace.isTraceable(level);
    }
    
    /**
     * Trace with a level specified.
     * @param level the level (1-9)
     * @param message the message
     */
    public void trace(int level, String message) {
        if (level <= trace.traceLevel)
            trace.trace(message);
    }
    
    /**
     * Trace unconditionally.
     * @param message - the message
     */
    public void trace(String message) {
        trace.trace(message);
    }
    
    /*
     * Put an exception stack trace to the trace file.
     * @param ex   The exception
     * @return the exception if it is a JMSException, null otherwise
     */
    public void traceException(Throwable ex) {
        trace.traceException(ex);
    }
    
    /**
     * Put an exception stack trace to the trace file at a specified level.
     * @param level The level
     * @param ex   The exception
     * @return the exception if it is a JMSException, null otherwise
     */
    public void traceException(int level, Throwable ex) {
        if (level <= trace.traceLevel) 
            trace.traceException(ex);
    }
    
   
    /*
     * @see com.ibm.plugin.ext.ImaPlugin#getConfig()
     */
    public void updateConfig( Map<String, Object> imap) {
    	 Object obj = imap.get("Properties");
         if (obj instanceof Map){
             @SuppressWarnings("unchecked")
             Map<String, Object> iprops = (Map<String,Object>) obj; 
             
             Set<String> pendingRemoveKeySet = new HashSet<String>();
             for( Entry<String, Object> entry  : this.props.entrySet() ) {
            	 String okey = entry.getKey();
            	 if(!iprops.containsKey(okey)){
            		 pendingRemoveKeySet.add(okey);
            	 }
             }
             
             for( Entry<String, Object> entry  : iprops.entrySet() ) {
            	 String okey = entry.getKey();
            	 Object value = entry.getValue();
            	 Object oldvalue = this.props.put(okey, value);
            	 
            	 Object subkey=null;
            	 Object subvalue=null;
            	 
            	 if (value instanceof Map && oldvalue!=null) {
            		 @SuppressWarnings("unchecked")
                     Map<Object, Object> oldvaluemap = (Map<Object, Object>) oldvalue;
            		 @SuppressWarnings("unchecked")
                     Map<Object, Object> valuemap = (Map<Object, Object>) value;
            		
            		 Set<Object> pendingValueRemoveKeySet = new HashSet<Object>();
                     for (Entry<Object, Object> subentry  : oldvaluemap.entrySet()) {
                    	 subkey = subentry.getKey();
                    	 if(!valuemap.containsKey(subkey)){
                    		 pendingValueRemoveKeySet.add(subkey);
                    	 }
                     }
                     
                     for (Entry<Object, Object> subentry  : valuemap.entrySet()) {
                    	 subkey = subentry.getKey();
                    	 subvalue = subentry.getValue();
                    	 this.plugin.onConfigUpdate(okey, subkey, subvalue);
                     }
                     
                     /* Remove from props */
                     for (Object removeKey : pendingValueRemoveKeySet) {
                    	 this.plugin.onConfigUpdate(okey, removeKey, null);
                     }
                     
            	 } else {
            		 this.plugin.onConfigUpdate(okey, null, value);
            	 }
            	 
            	 
             }
             
             /*Remove from props*/
             for (String removeKey : pendingRemoveKeySet) {
            	 props.remove(removeKey);
            	 this.plugin.onConfigUpdate(removeKey, null, null);
             }
         }
         
    }
    
    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
    	StringBuffer sb = new StringBuffer();
    	sb.append("ImaPluginImpl ");
    	sb.append("Name=\""+ name + "\" ");
    	if (title != null)
    	    sb.append("Title=\""+ title + "\" ");
    	if (version != null) 
    	    sb.append("Version=\""+ version + "\" ");
    	if (build != null) {
    		sb.append("Build=\""+ build + "\" ");
    		sb.append("Modification=\""+ modification + "\" ");
    	}
    	sb.append("Protocol=\""+ protocol + "\" ");
        if (alias != null) 
            sb.append("Alias=\""+ alias + "\" ");
        sb.append("Classpath=" + loader.getURLs());
    	return sb.toString();
    }
}
