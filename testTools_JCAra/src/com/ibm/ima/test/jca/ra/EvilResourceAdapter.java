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
package com.ibm.ima.test.jca.ra;


import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.Charset;
import java.security.AccessController;
import java.security.PrivilegedExceptionAction;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.ResourceAdapterInternalException;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAResource;

import com.ibm.ima.jms.test.jca.TestProps;
import com.ibm.ima.jms.test.jca.Utils;

public class EvilResourceAdapter implements ResourceAdapter {
    private static final SimpleDateFormat iso8601 = new SimpleDateFormat("[yyyy-MM-dd HH:mm:ss.SSSZ] ");

    static {
        iso8601.setTimeZone(TimeZone.getDefault());
    }
    
	private TestProps test;
	private String destination = Utils.WASPATH() + "/evilra.trace";
	private PrintWriter writer;
	private final Date date = new Date();
	
	public void log(String className, String msg) {
		if (test != null) {
			test.log("[" + className + "] " + msg);
		} else {
			System.out.println("[" + className + "] (null test)" + msg);
		}
		if (writer != null) {
		    date.setTime(System.currentTimeMillis());
            writer.println(iso8601.format(date) + className + ": " + msg);
            writer.flush();
		}
	}
	
	
	@Override
	public void start(BootstrapContext ctx) throws ResourceAdapterInternalException {
		log("EvilResourceAdapter","start(ctx)");
		log("EvilResourceAdapter","\tctx: " + ctx);
		
		OutputStreamWriter osw;
		final Charset ch = Charset.forName("UTF-8");

        try {
            /* Trace to a file */
            FileOutputStream fos; 
            fos = (FileOutputStream) AccessController.doPrivileged(new PrivilegedExceptionAction<Object>() {
                public Object run() throws FileNotFoundException {
                    return new FileOutputStream(destination, false);
                }
            });
            osw = new OutputStreamWriter(fos, ch);
            writer = new PrintWriter(osw);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
	}
	
	public TestProps getTest() {
		return test;
	}
	
	public void setTest(TestProps test) {
		log("EvilResourceAdapter","setTest()");
		this.test = test;
	}
	
	@Override
	public void stop() {
		log("EvilResourceAdapter","stop()");
	}
	
	@Override
	public void endpointActivation(MessageEndpointFactory mef, ActivationSpec as) throws ResourceException {
		log("EvilResourceAdapter","endpointActivation(mef, as)");
		log("EvilResourceAdapter","\tmef: " + mef);
		log("EvilResourceAdapter","\tas: " + as);
	}

	@Override
	public void endpointDeactivation(MessageEndpointFactory mef, ActivationSpec as) {
		log("EvilResourceAdapter","endpointDeactivation(mef, as)");
		log("EvilResourceAdapter","\tmef: " + mef);
		log("EvilResourceAdapter","\tas: " + as);
	}

	/**
	 * At recover time, get a list of all XA Resources which might need
	 * to be rolled back or committed.
	 */
	@Override
	public XAResource[] getXAResources(ActivationSpec[] aspecs) throws ResourceException {
		log("EvilResourceAdapter","getXAResources(aspecs)");
		for (ActivationSpec as : aspecs)
			log("EvilResourceAdapter","\t as: " + as);
		
		XAResource[] result = new XAResource[aspecs.length];
		
		for (int i=0; i<result.length; i++)
			result[i] = new EvilXAResource(test,this);
		return result;
	}
	
	@Override
	public boolean equals(Object o) {
		if (o == null || !(o instanceof EvilResourceAdapter))
			return false;
		return true;
	}
	
	@Override
	public int hashCode() {
		return super.hashCode();
	}
}
