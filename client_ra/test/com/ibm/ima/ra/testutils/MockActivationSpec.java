// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ima.ra.testutils;

import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.InvalidPropertyException;
import javax.resource.spi.ResourceAdapter;

public class MockActivationSpec implements ActivationSpec {

    @Override
    public ResourceAdapter getResourceAdapter() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void setResourceAdapter(ResourceAdapter arg0)
            throws ResourceException {
        // TODO Auto-generated method stub

    }

    @Override
    public void validate() throws InvalidPropertyException {
        // TODO Auto-generated method stub

    }

}
