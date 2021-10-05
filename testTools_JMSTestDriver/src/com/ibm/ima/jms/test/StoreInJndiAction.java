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
 */
package com.ibm.ima.jms.test;

import java.util.LinkedList;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TestUtils.Property;

public class StoreInJndiAction extends Action {    

    private final LinkedList<Property> _props = new LinkedList<Property>();
    private final TrWriter             _trWriter;
    private final String               _ldapPrefix;
    
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public StoreInJndiAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _trWriter = trWriter;
        _ldapPrefix = getJndiNamePrefix();

        TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
            public void process(Element element) {
                final String jndiName;
                String structureId = element.getAttribute("structure_id");
                if (structureId == null)
                    throw new RuntimeException(
                            "structure_id is not specified for AdminObject. "
                                    + TestUtils.xmlToString(element, true));
                String name = element.getAttribute("name");
                if (name == null)
                    throw new RuntimeException(
                            "Name is not specified for AdminObject"
                                    + " ."
                                    + TestUtils.xmlToString(element, true));
                if (_ldapPrefix == null || _ldapPrefix.length()<1)
                    jndiName = name;
                else
                    jndiName = _ldapPrefix+name;

                Property p = new Property(jndiName,structureId,"STRING");
                _props.addLast(p);
            }
        };
        TestUtils.walkXmlElements(config, "AdminObject", processor);
    }
    
    /* (non-Javadoc)
     * @see com.ibm.mds.rm.test.Action#run()
     */
    @Override    
    protected final boolean run() {
        for (TestUtils.Property p : _props) {
            String jndiName = p.getName();
            String objRef = (String)p.getValue(_dataRepository,_trWriter);
            Object jndiObject = _dataRepository.getVar(objRef);
            if (jndiObject == null)
            {
                System.out.println("No data repository object found for "+jndiName);
                return false;
            }
            try
            {
                jndiBind(jndiName, jndiObject, _trWriter);
            } catch (JmsTestException e) {
                _trWriter.writeException(e);
                return false;
            }
        }
        return true;
    }


}
