/*
 * Copyright (c) 2011-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.jms.test;

import java.util.LinkedList;
import java.util.ArrayList;

import java.net.URI;

import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.test.TestUtils.Property;
import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class CheckIsmPropsAction extends Action {
    private final String                _structureID;
    private final LinkedList<Property>  _props = new LinkedList<Property>();


    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CheckIsmPropsAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }

        TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
            public void process(Element element) {
                String name = element.getAttribute("name");
                if (name == null)
                    throw new RuntimeException(
                            "Name is not specified for ImaProperty. "
                                    + TestUtils.xmlToString(element, true));
                String value = element.getAttribute("value");
                if (value == null)
                    throw new RuntimeException(
                            "Value is not specified for ImaProperty " + name
                                    + " ."
                                    + TestUtils.xmlToString(element, true));
                String method = element.getAttribute("method");
                if (method == null || method.length()<1)
                    method = "get";
                if(!(method.equals("get") || method.equals("getString") || method.equals("getInt") || method.equals("getBoolean")))
                        throw new RuntimeException(
                                "Invalid method name, " + method
                                        + ", specified for property " + name);
                        
                CheckObject chkObj = new CheckObject(value,method);
                Property p = new Property(name,chkObj.toString(),"STRING");
                _props.addLast(p);
            }
        };
        TestUtils.walkXmlElements(config, "ChkImaProperty", processor);
    }
    
    
//    private final String  parseErrorString(String err){
//        int start;
//        if((start = err.indexOf("The property \"")) != -1){
//            String tmpStr = err.substring(start+"The property \"".length());
//            int end = tmpStr.indexOf('\"');
//            return tmpStr.substring(0,end);
//        }
//        if((start = err.lastIndexOf(':')) != -1){
//            return err.substring(start+2);
//        }
//        return err;
//    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.mds.rm.test.Action#run()
     */
    protected final boolean run() {
        ImaProperties ismProps = (ImaProperties) _dataRepository
                .getVar(_structureID);
        boolean retval = true;
        ArrayList<String> failures = new ArrayList<String>();
        if (ismProps == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Failed to locate the ImaProperties object ({0}).",
                    _structureID);
            return false;
        }

        for (TestUtils.Property p : _props) {
            String name = p.getName();
            String value = (String)p.getValue(_dataRepository,_trWriter);
            if (value == null)
                return false;
            CheckObject chkObj = new CheckObject(value);
            String method  = chkObj.getMethod();
            String expectedValue = chkObj.getValue();
            if(method.equals("get"))
            {
                Object propObj = ismProps.get(name);
                String propStr = null;
                
                if(propObj instanceof URI)
                    propStr = ((URI)propObj).toString();
                else
                    propStr = (String)propObj;
                
                if(propStr == null || !propStr.equals(expectedValue))
                {
                    failures.add(new String("get: Value for "+name+", "+propStr+", does not match expected value, "+expectedValue));
                    retval = false;
                }
            }
            else if(method.equals("getBoolean"))
            {
                Boolean propBool = Boolean.valueOf(ismProps.getString(name));
                Boolean expectedBoolValue = Boolean.valueOf(expectedValue);
                if(propBool == null || !propBool.equals(expectedBoolValue))
                {
                    failures.add(new String("getBool: value for " + name + ", " + propBool + ", does not match expected value, " + expectedBoolValue + " (" + expectedValue + ")"));
                    retval = false;
                }
            }
            else if(method.equals("getString"))
            {
                String propStr = ismProps.getString(name);
                if(propStr==null || !propStr.equals(expectedValue))
                {
                    failures.add(new String("getString: Value for "+name+", "+propStr+", does not match expected value, "+expectedValue));
                    retval = false;
                }
            }
            else if(method.equals("getInt"))
            {
                Integer propInt = Integer.valueOf(ismProps.getInt(name,-9999));
                Integer expectedIntValue = Integer.valueOf(expectedValue);
                if(propInt==null || !propInt.equals(expectedIntValue))
                {
                    failures.add(new String("getInt: Value for "+name+", "+propInt+", does not match expected value, "+expectedIntValue+" ("+expectedValue+")"));
                    retval = false;
                }
            }
            else
            {
                System.out.println(method+" is not a valid method name.");
                return false;
            }
        }
        
        if(retval == false)
        {
            //TODO: Convert these prints to log output
            System.out.println(failures.size()+" errors found");
            for(int i = 0; i< failures.size(); i++)
                System.out.println(failures.get(i));
        }

        return retval;
    }
    
    class CheckObject
    {
        String _value;
        String _method;
        
        CheckObject(String inValue, String inMethod)
        {
            _value = inValue;
            _method = inMethod;
        }
        CheckObject(String chkObjString)
        {
            String delimiter=":-:";
            String [] chkObjArray = chkObjString.split(delimiter);
            _value = chkObjArray[0].substring("value=".length());
            _method = chkObjArray[1].substring("method=".length());
        }
        String getValue()
        {
            return _value;
        }
        String getMethod()
        {
            return _method;
        }
        public String toString()
        {
            return new String("value="+_value+":-:method="+_method);
        }
    }

}
