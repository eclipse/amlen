/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
 * 
 */
package com.ibm.ima.jms.test;

import java.util.HashSet;
import java.util.LinkedList;

import javax.jms.JMSException;

import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaIllegalArgumentException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.test.TestUtils.Property;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 */
public class FillIsmPropsAction extends ApiCallAction {
    private final String               _structureID;
    private final LinkedList<Property> _props = new LinkedList<Property>();
    private final boolean              _validateAll;
    private final boolean              _validateNoWarn;
    private final int                  _ipVer;
    private final HashSet<String>      _invalidProps = new HashSet<String>();
    private final boolean              _remove;
    private final boolean              _clear;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public FillIsmPropsAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _validateAll = Boolean.parseBoolean(_actionParams.getProperty("validateAll","false"));
        _validateNoWarn = Boolean.parseBoolean(_actionParams.getProperty("validateNoWarn","true"));
        _ipVer = Integer.parseInt(_actionParams.getProperty("ipVer","1"));
        _remove = Boolean.parseBoolean(_actionParams.getProperty("remove", "false"));
        _clear = Boolean.parseBoolean(_actionParams.getProperty("clear", "false"));
        if(_ipVer > 2) {
            throw new RuntimeException("ISM JMS supports only IPv4 or IPv6.  It does not support combinations.  Only IP_VER values of 1 or 2 are valid for test purposes.");
        }
        TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
            public void process(Element element) {
                String name = element.getAttribute("name");
                if (name == null)
                    throw new RuntimeException(
                            "Name is not specified for ImaProperty. "
                                    + TestUtils.xmlToString(element, true));
                if (_remove) {
                    Property p = new Property(name, null, "STRING");
                    _props.addLast(p);
                    return;
                }
                String value = element.getAttribute("value");
                if (value == null)
                    throw new RuntimeException(
                            "Value is not specified for ImaProperty " + name
                                    + " ."
                                    + TestUtils.xmlToString(element, true));
                Property p = new Property(name,value,getConfigAttribute(element, "type", "STRING"));
                if(element.hasAttribute("validator")){
                    String validator = element.getAttribute("validator");
                    if (validator != null) {    
                            p.setValidator(validator);
                    }
                }
                if(element.hasAttribute("valid")) {
                    boolean invalid = Boolean.parseBoolean(element.getAttribute("valid"));
                    if(!invalid){
                        _invalidProps.add(name);
                    }
                }
                _props.addLast(p);
            }
        };
        TestUtils.walkXmlElements(config, "ImaProperty", processor);
    }
    
    
    private final String  parseErrorString(String err){
        int start = err.indexOf("property ");
        String tmpStr = null;
        if (start != -1){
            tmpStr = err.substring(start+"property ".length());
        } else if ((start = err.lastIndexOf(':')) != -1) {
            return err.substring(start+2);
        } else {
            return err;
        }
        /* There are too many possible substrings that could come after the property name
         * now that it isn't surrounded in quotes...
         * " because"
         * " is of"
         * " was set"
         * " due to"
         * 
         * soooo ugly :(
         */
        int end = tmpStr.indexOf(" because");
        if (end != -1) {
        	return tmpStr.substring(0,end);
        } else if ((end = tmpStr.indexOf(" is of")) != -1) {
            return tmpStr.substring(0,end);
        } else if ((end = tmpStr.indexOf(" was set")) != -1) {
        	return tmpStr.substring(0,end);
        } else if ((end = tmpStr.indexOf(" due to")) != -1) {
        	return tmpStr.substring(0,end);
        } else {
        	return err;
        }
    }
    @SuppressWarnings("unchecked")
    private final boolean validateProps(ImaProperties ismProps){
        if(!_validateAll)
            return true;
        boolean retval = true;
        // TODO: Update this to more fully use the exception objects
        // that are now returned.  For now, just update enough so 
        // that the test driver will still build and run correctly.
        ImaJmsException badpropex[] = ismProps.validate(_validateNoWarn);
        String badprops[] = null;
        if(badpropex != null && badpropex.length > 0){
            badprops = new String[badpropex.length];
            for(int i = 0; i < badpropex.length; i++)
                badprops[i] = badpropex[i].toString();
        }
        if(badprops != null) {
            HashSet<String> badpropsSet = new HashSet<String>();
            HashSet<String>         invalidProps = (HashSet<String>) _invalidProps.clone();
            StringBuffer propErrList = new StringBuffer();
            for(int i=0; i< badprops.length; i++) {
                badprops[i] = parseErrorString(badprops[i]);
                if(i > 0)
                    propErrList.append(", ");
                propErrList.append(badprops[i]);
                badpropsSet.add(badprops[i]);
            }
                    
            if(_invalidProps.isEmpty()){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Action {0}: {1} invalid properties were found: {2}",
                        _id,badprops.length, propErrList);
                return false;
            }
            
            for (int i = 0; i < badprops.length; i++) {
                if(invalidProps.remove(badprops[i])) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD2045",
                            "Action {0}: Property \"{1}\" correctly identified as invalid",_id, badprops[i]);
                    continue;
                }
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2047",
                        "Action {0}: Property {1} is not valid",_id, badprops[i]);
                
                retval = false;
            }
            if(!invalidProps.isEmpty()){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2046",
                        "Action {0}: Failed to identify the following properties as invalid:",_id, 
                        invalidProps.toString());
                retval = false;
                
            }

        }
        else if(!_invalidProps.isEmpty()){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2044",
                    "Action {0}: There are no invalid properties. Expected: [{1}]",
                    _id,_invalidProps.toString());
            
            retval = false;
            
        }
        
        return retval;
    }
    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.mds.rm.test.Action#run()
     */
    protected final boolean invokeApi() throws JMSException {
        ImaProperties ismProps = (ImaProperties) _dataRepository
                .getVar(_structureID);
        if (ismProps == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Failed to locate the ImaProperties object ({0}).",
                    _structureID);
            return false;
        }
        // If IP_VER was set to 2 for automation test runs, then explicitly set IPv6 to true before 
        // setting the remaining properties.  If the test case has explicitly set a value for IPv6,
        // allow that value to override the IP_VER setting.
        // quickly.
        if(_ipVer == 2){
            ismProps.put("IPv6", true);
        }
        
        if(_remove) {
            for (TestUtils.Property p: _props) {
                String name = p.getName();
                ismProps.remove(name);
            }
            return validateProps(ismProps);
        }
        
        if(_clear) {
            ismProps.clear();
            return validateProps(ismProps);
        }
        
        for (TestUtils.Property p : _props) {
            String name = p.getName();
            Object value = p.getValue(_dataRepository,_trWriter);
            if (value == null)
                return false;
            String validator = p.getValidator();
            if (validator != null) {
            	try {
            		// Catch the runtime exception so that we can test the exceptions
            		// that we get here with invalid validators
            		ismProps.addValidProperty(name, validator);
            	} catch (ImaIllegalArgumentException re) {
            		throw new ImaJmsExceptionImpl(re.getErrorCode(), re.getMessage());
            	}
            }
            ismProps.put(name, value);
        }
        
        return validateProps(ismProps);
    }

}
