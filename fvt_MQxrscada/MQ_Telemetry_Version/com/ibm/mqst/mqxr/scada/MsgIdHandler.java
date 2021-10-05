package com.ibm.mqst.mqxr.scada;
/*
 * Copyright (c) 2002-2021 Contributors to the Eclipse Foundation
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
 * @version 1.0
 */
public class MsgIdHandler
{

    int currentId;

    // Constructor - set id counter
    public MsgIdHandler()
    {

        currentId = 1;
    }

    // Get a new id based on counter
    public MsgIdentifier getId() throws ScadaException
    {

        MsgIdentifier result = new MsgIdentifier(currentId);
        currentId++;
        if (currentId > 65535)
        {
            currentId = 1;
        }
        return result;
    }

    // Get an ID with a certain value
    public MsgIdentifier getId(int id) throws ScadaException
    {

        MsgIdentifier result = new MsgIdentifier(id);
        return result;

    }

}
