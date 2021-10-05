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
package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

public class CheckIntVariableAction extends Action {
    private final String                _structureID;
    private final Integer               _expectedValue;
    private final String                _comparator;

    CheckIntVariableAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _comparator = _actionParams.getProperty("comparator", "==");
        String expectedValue = _actionParams.getProperty("expectedValue");
        if (expectedValue == null) {
            throw new RuntimeException("expectedValue is not defined for "
                    + this.toString());
        }
        _expectedValue = Integer.valueOf(expectedValue);
    }

    @Override
    protected boolean run() {
        Integer i = (Integer)_dataRepository.getVar(_structureID);
        if (i == null) {
            throw new RuntimeException(_structureID+" is not in the repository");
        }
        
        if ("==".equals(_comparator)) {
            if (!(i.equals(_expectedValue))) {
                throw new RuntimeException("Expected '"+_structureID+"' to be "+_expectedValue+" but was "+i);
            }
        } else if ("!=".equals(_comparator)) {
            if ((i.equals(_expectedValue))) {
                throw new RuntimeException("Expected '"+_structureID+"' to be other than "+i);
            }
        } else {
            throw new RuntimeException("Unknown comparator '"+_comparator+"'");
        }
        // TODO Auto-generated method stub
        return true;
    }

}
