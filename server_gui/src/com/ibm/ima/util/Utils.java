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
/**
 */
package com.ibm.ima.util;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.lang.management.ManagementFactory;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.channels.FileLock;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.channels.OverlappingFileLockException;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.ResourceBundle;

import javax.management.JMX;
import javax.management.MBeanServer;
import javax.management.MalformedObjectNameException;
import javax.management.ObjectName;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.ws.rs.core.Response.Status;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.NodeList;

import com.ibm.ima.ISMWebUIProperties;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.websphere.application.ApplicationMBean;

/**
 *
 */
public class Utils {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public final static int OUT_MESSAGE = 0; // array index for process out stream
    public final static int ERR_MESSAGE = 1; // array index for process err stream
    public final static int FREE = 0;
    public final static int BUSY = 1;
    public final static int GET_LIBERTY_PROPERTY = 0;
    public final static int SET_LIBERTY_PROPERTY = 1;
    public final static String NAME = "name";
    public final static String VALUE = "value";
    public final static String PROPRETIES_NOT_AVAILABLE = "notAvailable";
    public final static String PROPERTY_NOT_FOUND = "notFound";
    public final static String SUCCESS = "success";

    public final static String KEYSTORE_LOCATION = "keystore_location";
    public final static String KEYSTORE_TYPE = "keystore_type";
    public final static String KEYSTORE_PASSWORD = "keystore_password";
    public final static String KEYSTORE_EXP = "keystore_expirationDate";
    public final static String DEFAULT_PASSWORD = "ixmwebui5725F96";
    public final static String CERT_NAME = "user_cert_name";
    public final static String KEY_NAME = "user_key_name";
    
    public final static String NOT_FOUND_RC = "113";
    public final static String ASYNC_COMPLETION = "10";
    public final static String OBJECT_ALREADY_EXISTS_RC = "335";
    public final static String PENDING_DELETE = "370";
    public static final String OAUTH_NOT_FOUND = "460";
    
    public final static String CCI_CONFIGURED_FILE_PATH = ISMWebUIProperties.instance().getDataDir() + "/.cci_configured";

    private final static String LIBERTY_CONFIG = ISMWebUIProperties.instance().getLibertyConfigDirectory();
    private Map <String, Long> ethernetInterfaceAccessMap;
// Comment out default false settings for isVirtual & isDocker when not building with Bedrock
//    private boolean isVirtual = false;
    private boolean isCCI = false;
//    private boolean isDocker = false;
    private boolean isVirtual = true;
    private boolean isDocker = true;
    private final static int MAX_LOCK_MILLIS = 60000;  // don't allow the lock to be held more than a minute

    private final static String CLAS = Utils.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    private static Utils instance =null;

    private static String BASE_PATH = "";
    
    private String defaultAdminServerAddress = "127.0.0.1";
    private String defaultAdminServerPort = "9089";
       
    static {
        try {
            BASE_PATH = (new File("a")).getCanonicalPath();
            BASE_PATH = BASE_PATH.substring(0, BASE_PATH.lastIndexOf(File.separator)+1);
            logger.trace(CLAS, "static", "BASE_PATH is set to " + BASE_PATH);
        } catch (IOException e) { 
            logger.log(LogLevel.LOG_ERR, CLAS, "static", "CWLNA5000", e);
        }        
    }


    public static synchronized Utils getUtils() {
        if (instance == null) {
            instance = new Utils();
        }
        return instance;
    }

    private Utils() {
    	getMessagingServerInfo();
    }

    /**
     * Get the output of the Process
     * 
     * @param process the process to get the output from
     * @param output an array of length 2 to return the input stream and output stream in. The indices are defined by
     *            {@link #OUT_MESSAGE} and {@link #ERR_MESSAGE}
     * @return rc the return code from the process
     */
    public final int getOutput(Process process, String[] output) throws Exception {
        Object SYNC_OBJ = new Object();
        String sOutMsg = "";
        String sErrMsg = "";
        BufferedInputStream inError = null;
        BufferedInputStream inOutput = null;
        int rc = -1;

        try {
            inError = new BufferedInputStream(process.getErrorStream());
            inOutput = new BufferedInputStream(process.getInputStream());
            
            boolean bStop = false;
            while (!bStop) {

                // read the error stream
                if (inError != null) {
                    int bytesAvailable = inError.available();
                    if (bytesAvailable > 0) {
                        byte[] byteArray = new byte[bytesAvailable];
                        int bytesRead = inError.read(byteArray, 0, bytesAvailable);
                        if (bytesRead > 0) {
                            sErrMsg += new String(byteArray, 0, bytesRead);
                        }
                    }
                }

                // read the output stream
                if (inOutput != null) {
                    int bytesAvailable = inOutput.available();
                    if (bytesAvailable > 0) {
                        byte[] byteArray = new byte[bytesAvailable];
                        int bytesRead = inOutput.read(byteArray, 0, bytesAvailable);
                        if (bytesRead > 0) {
                            sOutMsg += new String(byteArray, 0, bytesRead);
                        }
                    }
                }

                // look for the exit value. if the process
                // has not terminated, then this call will
                // throw an exception.
                try {
                    rc = process.exitValue();
                    bStop = true;
                } catch (Exception e) {

                    // pause for 1 sec, then
                    // try again
                    synchronized (SYNC_OBJ) {
                        SYNC_OBJ.wait(100);
                    }
                }
            }

            if (output != null && output.length > 1) {
                output[OUT_MESSAGE] = sOutMsg;
                output[ERR_MESSAGE] = sErrMsg;
            }
        } finally {
            if (inError != null) inError.close();
            if (inOutput != null) inOutput.close();
            OutputStream outStream = process.getOutputStream();
            if (outStream != null) {
            	outStream.close();
            }
        }
        return rc;
    }

    public int getEthernetInterfaceAccess(String name) {
        int  state = FREE;
        if (ethernetInterfaceAccessMap.containsKey(name)) {
            long locktime = ethernetInterfaceAccessMap.get(name);
            if (locktime > 0) {
                if (locktime + MAX_LOCK_MILLIS < System.currentTimeMillis()) {
                    // it's been in busy state too long, something is wrong. Free it
                    setEthernetInterfaceAccess(name, FREE);
                } else {
                    System.err.println("lock is " + locktime);
                    state = BUSY;
                }
            }
        }
        return state;
    }

    public void setEthernetInterfaceAccess(String name, int state) {
        if (state == FREE) {
            ethernetInterfaceAccessMap.put(name, (long)FREE);
        } else {
            // put the current time in so we can prevent the lock from being held forever
            ethernetInterfaceAccessMap.put(name, System.currentTimeMillis());
        }
    }

    /**
     * Helper method to make several attempts to access Liberty profile properties file
     * and call the get or set method. This should be used when there is a programmatic need
     * to make changes to properties file.
     * @param action Specify the request tipe using Utils.GET_LIBERTY_PROPERTY or Utils.SET_LIBERTY_PROPERTY
     * @param property Property to get or set
     * @param value Value to set for the property. Null if sending a get request
     * @throws IsmRuntimeException IsmRuntimeException gets thrown if the property is not found or the file is not available
     * @throws Exception generic exception gets thrown if an unexpected error occured
     */
    public String libertyPropertyAccessRequest(int action, String property, String value) {
        String result = null;
        try {
            int retry = 0;
            while (retry < 3) {
                if (action == GET_LIBERTY_PROPERTY) {
                    result = this.getLibertyProperty(property);
                } else if (action == SET_LIBERTY_PROPERTY) {
                    result = this.setLibertyProperty(property, value);
                } else {
                    logger.log(LogLevel.LOG_ERR, CLAS, "libertyPropertyAccessRequest", "CWLNA5000");
                    throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5000", new Object[] {property});
                }

                if (result.equals(PROPERTY_NOT_FOUND)) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "libertyPropertyAccessRequest", "CWLNA5069", new Object[] {property});
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5069", new Object[] {property});
                } else if (result.equals(PROPRETIES_NOT_AVAILABLE)) {
                    retry++;
                    Thread.sleep(500);
                } else {
                    break;
                }
            }
            if (retry == 3) {
                logger.log(LogLevel.LOG_ERR, CLAS, "libertyPropertyAccessRequest", "CWLNA5068");
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5068", new Object[] {property});
            } 
        } catch (IsmRuntimeException isme) {
            throw isme;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "libertyPropertyAccessRequest", "CWLNA5000", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5000", new Object[] {property});
        }
        return result;
    }
    /**
     * Get the Liberty profile property value from properties.xml
     * 
     * @param property Property name to lookup
     * @return value Value of the requested property. Will return null if the property could not be located
     * @throws Exception
     */
    public synchronized String getLibertyProperty(String property) throws Exception {
        FileLock myLock = null;
        RandomAccessFile props = null;
        try {
            File libertyProperties = new File(LIBERTY_CONFIG+"/properties.xml");
            props = new RandomAccessFile(libertyProperties.getAbsolutePath(),"rw");
            myLock = props.getChannel().tryLock();        
            if (myLock != null) {
                DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
                DocumentBuilder builder = factory.newDocumentBuilder();
                Document document = builder.parse(libertyProperties);
    
                NodeList nl = document.getFirstChild().getChildNodes();
                NamedNodeMap attributes = null;
    
                for (int i=0; i<nl.getLength();i++) {
                    attributes = nl.item(i).getAttributes();
                    if ((attributes!=null)&&(attributes.getNamedItem(NAME).getNodeValue().equals(property))) {
                        return attributes.getNamedItem(VALUE).getNodeValue();
                    }
                }
                return PROPERTY_NOT_FOUND;
            }
            return PROPRETIES_NOT_AVAILABLE;
        } catch (OverlappingFileLockException ex) {
            return PROPRETIES_NOT_AVAILABLE;
        } catch (Exception e) {
            throw e;
        } finally {
            if (myLock != null) {
                myLock.release();
            }
            if (props != null) {
                props.close();
            }
        }
    }

    /**
     * Set the Liberty profile property value stored in properties.xml
     * 
     * @param property Property name to set
     * @return value Value to assign to the property.
     * @throws Exception
     */
    public synchronized String setLibertyProperty(String property, String value) throws Exception {
        FileLock myLock = null;
        RandomAccessFile props = null;
        try {
            File libertyProperties = new File(LIBERTY_CONFIG+"/properties.xml");
            props = new RandomAccessFile(libertyProperties.getAbsolutePath(),"rw");
            myLock = props.getChannel().tryLock();
            if (myLock != null) {
                DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
                DocumentBuilder builder = factory.newDocumentBuilder();
                Document document = builder.parse(libertyProperties);
    
                NodeList nl = document.getFirstChild().getChildNodes();
                NamedNodeMap attributes = null;
    
                boolean noPropertyMatch = true;
                int numNodes = nl.getLength();
                for (int i=0; i < numNodes; i++) {
                    attributes = nl.item(i).getAttributes();
                    if ((attributes!=null)&&(attributes.getNamedItem(NAME).getNodeValue().equals(property))) {
                        attributes.getNamedItem(VALUE).setNodeValue(value);
                        TransformerFactory tFactory = TransformerFactory.newInstance();
                        Transformer transformer = tFactory.newTransformer();
                        DOMSource s = new DOMSource(document);
                        StreamResult r = new StreamResult(libertyProperties);
                        transformer.transform(s, r);
                        noPropertyMatch = false;
                        break;
                    }
                }                
                if (!noPropertyMatch) {
                    return SUCCESS;
                } else {
                    return PROPERTY_NOT_FOUND;
                }
            }
            return PROPRETIES_NOT_AVAILABLE;
        } catch (OverlappingFileLockException ex) {
            return PROPRETIES_NOT_AVAILABLE;
        } catch (Exception e) {
            throw e;
        } finally {
            if (myLock != null) {
                myLock.release();
            }
            if (props != null) {
                props.close();
            }            
        }
    }
    
    
    /**
     * Validates the filename and ensures the canonicalPath of the created file
     * starts with the directory name to help guard against pathTraversal
     * vulnerability (http://cwe.mitre.org/top25/index.html#CWE-22)
     * @param directory  The directory for the file, should be a known safe directory
     * @param filename  The filename for the file
     * @param enforceFile If true, the filename must not contain any path fragment
     * @return the File or null if not safe or impossible to create
     */
    public static final File safeFileCreate(String directory, String filename, boolean enforceFile) {       
        logger.traceEntry(CLAS, "safeFileCreate");
        if (directory == null || directory.trim().length() == 0 || filename == null || filename.trim().length() == 0) {
            return null;
        }
        
        if (enforceFile) {
            try {
                String canonicalPath = (new File(filename)).getCanonicalPath();
                String expectedPath = BASE_PATH + filename;
                logger.trace(CLAS, "safeFileCreate", "BASE path: " + BASE_PATH);
                logger.trace(CLAS, "safeFileCreate", "Canonical path: " + canonicalPath);
                logger.trace(CLAS, "safeFileCreate", "Expected path: " + expectedPath);
                logger.trace(CLAS, "safeFileCreate", "Index of file separator: " + filename.indexOf(File.separator));
                if (!canonicalPath.equals(expectedPath) || filename.indexOf(File.separator) > -1) {
                    logger.trace(CLAS, "safeFileCreate", "Canonical path was unexpected: " + canonicalPath);
                    return null;
                }
            } catch (IOException e) {
                logger.trace(CLAS, "safeFileCreate", "Something went wrong checking file: " + filename);
                logger.log(LogLevel.LOG_ERR, CLAS, "checkFilename", "CWLNA5000", e);
                return null;
            }
        }
        File file = new File(directory, filename);
        Path filepath = file.toPath();
        try {
            // String canonicalPath = file.getCanonicalPath();
            String canonicalPath = filepath.toAbsolutePath().normalize().toString();
            logger.trace(CLAS, "safeFileCreate", "Canonical path: " + canonicalPath);
            logger.trace(CLAS, "safeFileCreate", "Directory: " + directory);
            if (canonicalPath.startsWith(directory)) {
                return file;
            }
        } catch (SecurityException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "safeFileCreate", "CWLNA5000", e);
        } finally {
            // logger.trace(CLAS, "safeFileCreate", "Canonical path: " + canonicalPath);
            logger.trace(CLAS, "safeFileCreate", "Directory: " + directory);
            logger.trace(CLAS, "safeFileCreate", "Normalized path did not match directory.");
        }

        logger.traceExit(CLAS, "safeFileCreate");
        return null;    
    }
    
    public static String getUniqueName(String directory, String filename) {
        logger.traceEntry(CLAS, "getUniqueName", new Object[] {directory, filename});
        
        int extIndex = filename.lastIndexOf('.');
        String name = filename;
        String ext = "";
        if (extIndex > 0) {
            name = filename.substring(0, extIndex);
            ext = filename.substring(extIndex);
        }
        
        File f = new File(directory, name+ext);
        for  (int i = 0; f.exists() && i < 1000; i++) {
            f = new File(directory, name+"_"+i+ext);
        }
        logger.traceExit(CLAS, "getUniqueName", f.getName());

        return f.getName();
    }


    /**
     * Get the correct error message for errors related to ISMServer
     * 
     * @param returnCode returnCode as a string
     * @return errorMessage the error message code to use in the exception
     */
    public String getIsmErrorMessage(String returnCode) {
        return getIsmErrorMessage(Integer.parseInt(returnCode));        
    }
    
    /**
     * Get the correct error message for errors related to ISMServer
     * 
     * @param returnCode returnCode as an int
     * @return errorMessage the error message code to use in the exception
     */
    public String getIsmErrorMessage(int rc) {
        String errorMessage = "CWLNA5005";

        switch (rc) {

        case 18:
        	errorMessage = "CWLNA5117";
        	break;
        case 113:
            errorMessage = "CWLNA5017";
            break;
        case 114:
            errorMessage = "CWLNA5017";
            break;
        case 130:
            errorMessage = "CWLNA5102";
            break; 
        case 190:
            errorMessage = "CWLNA5116";
            break; 
        case 191:
            errorMessage = "CWLNA5113";
            break; 
        case 192:
            errorMessage = "CWLNA5114";
            break;
        case 198:
            errorMessage = "CWLNA5123";
            break;          
        case 199:
            errorMessage = "CWLNA5124";
            break;                      
        case 209:
            errorMessage = "CWLNA5092";
            break;
        case 213:
            errorMessage = "CWLNA5142";
            break;            
        case 215:
            errorMessage = "CWLNA5101";
            break;            
        case 300:
            errorMessage = "CWLNA5700";
            break;
        case 301:
            errorMessage = "CWLNA5701";
            break;
        case 302:
            errorMessage = "CWLNA5702";
            break;
        case 303:
            errorMessage = "CWLNA5703";
            break;
        case 306:
            errorMessage = "CWLNA5078";
            break;
        case 307:
            errorMessage = "CWLNA5082";
            break;
        case 353:
            errorMessage = "CWLNA5152";
            break;        	
        case 357:
            errorMessage = "CWLNA5120";
            break;
        case 363:
            errorMessage = "CWLNA5132";
            break;
        case 370:
        	errorMessage = "CWLNA5144";
        	break;
        case 440:
            errorMessage = "CWLNA5055";
            break;
        case 441:
            errorMessage = "CWLNA5058";
            break;
        case 446:
            errorMessage = "CWLNA5074";
            break;
        case 447:
            errorMessage = "CWLNA5075";
            break;
        case 448:
            errorMessage = "CWLNA5076";
            break;
        case 449:
            errorMessage = "CWLNA5090";
            break;
        case 450:
            errorMessage = "CWLNA5093";
            break;
    	case 452:
    		errorMessage = "CWLNA5115";
    		break;
        case 454:
            errorMessage = "CWLNA5137";
            break;    		
    	case 456:
    	    errorMessage = "CWLNA5121";
    	    break;
        case 459:
            errorMessage = "CWLNA5138";
            break;          
        case 460:
            errorMessage = "CWLNA5017";
            break;          
        }

        return errorMessage;
    }
    
    /**
     * Restart the ISMWebUI
     * @param delay number of milliseconds to sleep before restarting. May be 0.
     */
    public static final synchronized void restartApplication(int delay) {
        MBeanServer mbs = ManagementFactory.getPlatformMBeanServer();
        ApplicationMBean  webUIappMBean = null;
        try {
            ObjectName appMBean = new ObjectName("WebSphere:service=com.ibm.websphere.application.ApplicationMBean,name=ISMWebUI");
            if (mbs.isRegistered(appMBean)) {
                webUIappMBean = JMX.newMBeanProxy(mbs, appMBean, ApplicationMBean.class);
                logger.trace(CLAS, "restartApplication", "restarting Web UI in " + delay + " milliseconds");
                if (delay > 0) {
                    try {
                        // ignore FindBugs complaint, we don't want any other thread to obtain the intrinsic lock
                        Thread.sleep(delay); 
                    } catch(InterruptedException e) {
                        // no worries, just keep going
                    }
                }
                webUIappMBean.restart();
            } else {
                logger.log(LogLevel.LOG_ERR, CLAS, "restartApplication", "No ApplicationMBean registered");
            }
        } catch (MalformedObjectNameException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "restartApplication", "CWLNA5000", e);
        } 
     }
    
   /**
    * Check if this is a virtual appliance
    * 
    * @return true if virtual, false otherwise

    */
    public boolean isVirtualAppliance() {
	   return isVirtual;
	}
    
    /**
     * Obtain information about whether or not this server is
     * a virtual appliance or not. Also detect if the instance
     * is CCI.
     * 
    * @throws IsmRuntimeException if there is an issue obtaining
    *                             the information via dpinfo
     */
    private void getMessagingServerInfo()  {
    	
        String adminServer = System.getenv("IMA_SERVER_HOST");
        if (adminServer != null && !adminServer.isEmpty()) {
            defaultAdminServerAddress = adminServer;
            String adminPort = System.getenv("IMA_SERVER_PORT");
            if (adminPort == null || adminPort.isEmpty()) {
            	adminPort = "9089";
            }
            defaultAdminServerPort = adminPort;
        }
        logger.trace(CLAS, "getMessagingServerInfo", "adminServer is " + adminServer);
    }
    
    /**
	 * @return the defaultAdminServerAddress
	 */
	public String getDefaultAdminServerAddress() {
		return defaultAdminServerAddress;
	}

	/**
	 * @return the defaultAdminServerPort
	 */
	public String getDefaultAdminServerPort() {
		return defaultAdminServerPort;
	}

	/**
     * Based on an list of acceptable languages provided by an 
     * HttpServletRequest obtain a String that represents the
     * best Locale match. This match will consist of a supported
     * language and the best/most preferred country or cultural 
     * preference. For example en_US, en_UK, en_RU, de_DE, etc..
     * 
     * @param request  The HttpServletRequest which we will obtain
     *                 the list of preferred languages
     *                 
     * @return         A string that is the best match of language
     *                 and country
     */
    public static String getLocaleString(HttpServletRequest request) {
    	
        logger.traceEntry(CLAS, "getLocaleString");

    	if (request == null) {
    		return "en";
    	}
    	
    	String country = null;
    	String lang = null;

    	Enumeration<Locale> localeEnum = request.getLocales();
    	
    	if (localeEnum == null) {
    		return "en";
    	}

    	while(localeEnum.hasMoreElements()) {
    		
    		Locale localeValue = localeEnum.nextElement();
    		
    		// track only the first country preferred
    		if ((country == null) &&  (localeValue.getCountry().length() > 0)) {
    			country = localeValue.getCountry();
    		}

    		lang = localeValue.getLanguage();
    		Locale actualLocale = ResourceBundle.getBundle("com.ibm.ima.msgcatalog.IsmUIStringsResourceBundle", localeValue).getLocale();
    		String bundleLanguageString = actualLocale.getLanguage();

    		/*
    		 * If the load of the ResourceBundle had to fall all the way to 
    		 * English the locale language will be empty string since we do
    		 * not actually have a bundle ending with _en 
    		 */
    		if (lang.equals(bundleLanguageString)) {
    			break;
    		}

    		// reset language to null
    		lang = null;

    	}

    	// no supported language found - use English
    	if (lang == null) {
    		lang = "en";
    	}

    	String actualLocaleString = null;

    	if (country != null) {
    		actualLocaleString = lang + "-" + country;
    	} else {
    		actualLocaleString = lang;
    	}

    	logger.traceExit(CLAS, "getLocaleString", actualLocaleString.toLowerCase());
    	return actualLocaleString.toLowerCase();
    }
    
    /**
     * Validates each address in the list. At the first invalid address, throws an exception
     * @param ipList the list of addresses to validate
     * @param stripBrackets if true, strips leading and trailing brackets from IPv6 addresses before validating
     */
    public static void validateIPAddresses(List<String> ipList, boolean stripBrackets) {
        for (String ip : ipList) {
            if (stripBrackets && ip.startsWith("[") && ip.endsWith("[") && ip.contains(":")) {
                ip = ip.substring(1, ip.lastIndexOf("]"));
            }
            try {
                InetAddress address = InetAddress.getByName(ip);
                address.getAddress();
                logger.trace(CLAS, "validate", "Validation is successful for ip " + ip);
            } catch (UnknownHostException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "validate", "CWLNA5098", e);
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5098", new Object[] { ip });
            } catch (Exception unknownException) {
                logger.log(LogLevel.LOG_ERR, CLAS, "validate", "CWLNA5005", unknownException);
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { ip });
            }
        }
    }
    
    /**
     * This utility is to assist the Login.jsp. It checks the request to see if it contains the WasReqURL
     * cookie. if the request contains the cookie, it makes sure the value is an html or jsp file rather
     * than a rest request or image. If it is not an html or jsp file, then it clears the cookie to prevent
     * Liberty from redirecting to an image or rest request after login. 
     * @param request
     * @param response
     */
    public static void checkWasRequestURL(HttpServletRequest request, HttpServletResponse response) {
    	 
    	try {
	    	if (request == null || response == null) {
	    		logger.trace(CLAS, "checkWasRequestURL", "request or response is null");
	    		return;
	    	}
	    	Cookie[] cookies = request.getCookies();
	    	if (cookies == null || cookies.length == 0) {
	    		logger.trace(CLAS, "checkWasRequestURL", "request has no cookies");
	    		return;
	    	}
	    	for (Cookie cookie : cookies) {
	    		if (cookie.getName().equals("WASReqURL")) {
	    			String lastRequest = cookie.getValue();
	    			if (lastRequest != null && !(lastRequest.contains(".html") || lastRequest.contains(".jsp"))) {
	    				// we don't want to redirect to anything that isn't an html or jsp page, so remove this cookie
	    				logger.trace(CLAS, "checkWasRequestURL", "clearing WASReqURL cookie: " + lastRequest);
	    				cookie.setValue(null);
	    				cookie.setMaxAge(0);
	    				response.addCookie(cookie);
	    			}
	    			break;
	    		}
	    	}
    	} catch (Throwable t) {
    		// we never want the login page request to fail, be super cautious and catch any errors...
    		logger.trace(CLAS, "checkWasRequestURL", t.getMessage());    		
    	}
    	
    }

    /**
     * Determine whether we are running in CCI environment
     * @return true if running in CCI, false otherwise
     */
    public boolean isCCIEnvironment() {
    	return isCCI;
    } 
    
    /**
     * Determine whether we are running in Docker container
     * @return true if running in Docker, false otherwise
     */
    public boolean isDockerContainer() {
    	return isDocker;
    } 
  
}
