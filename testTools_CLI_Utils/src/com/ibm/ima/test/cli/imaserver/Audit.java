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
package com.ibm.ima.test.cli.imaserver;

import java.io.PrintStream;
import java.sql.Timestamp;
import java.util.Date;

public class Audit {
	
	private Date date = new Date();
	private PrintStream writer = null;
	private boolean doTrace = false;
	
	/**
	 * This class is not used in the automation but can be used to turn
	 * on trace in the local workspace to debug the test code. If you want to use 
	 * this then set the system properties when running the code.
	 * @param fileName
	 */
	public Audit(String fileName)
	{
		doTrace = new Boolean(System.getProperty("MyTrace")).booleanValue();
		if(doTrace)
		{
			try 
			{
				if (fileName.equals("stdout")) 
				{
					writer = new PrintStream(System.out, true, "UTF-8");
				} 
				else if (fileName.equals("stderr")) 
				{
					writer = new PrintStream(System.err, true, "UTF-8");
				} else 
				{
					writer = new PrintStream(fileName, "UTF-8");
				}
			} catch (Exception e) {
				throw new RuntimeException(e);
			}
		}
	}
	
	/**
	 * This class is not used in the automation but can be used to turn
	 * on trace in the local workspace to debug the test code. If you want to use 
	 * this then set the system properties when running the code.
	 * @param fileName
	 */
	public Audit(String fileName, boolean traceOn)
	{
		doTrace = traceOn;
		if(doTrace)
		{
			try 
			{
				if (fileName.equals("stdout")) 
				{
					writer = new PrintStream(System.out, true, "UTF-8");
				} 
				else if (fileName.equals("stderr")) 
				{
					writer = new PrintStream(System.err, true, "UTF-8");
				} else 
				{
					writer = new PrintStream(fileName, "UTF-8");
				}
			} catch (Exception e) {
				throw new RuntimeException(e);
			}
		}
	}
	
	public Audit()
	{
		String fileName = System.getProperty("MyTraceFile");
		doTrace = new Boolean(System.getProperty("MyTrace")).booleanValue();
		if(doTrace)
		{
			try 
			{
				if (fileName.equals("stdout")) 
				{
					writer = new PrintStream(System.out, true, "UTF-8");
				} 
				else if (fileName.equals("stderr")) 
				{
					writer = new PrintStream(System.err, true, "UTF-8");
				} else 
				{
					writer = new PrintStream(fileName, "UTF-8");
				}
			} catch (Exception e) {
				throw new RuntimeException(e);
			}
		}
	}
	
	public void traceAutomation()
	{
		try 
		{
			doTrace = true;
			writer = new PrintStream(System.out, true, "UTF-8");
			
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}
	
	
	public synchronized void trace(String message)
	{
		if(message != null)
		{
			date.setTime(System.currentTimeMillis());
			writer.println(new Timestamp(date.getTime()) + " " + message);
			writer.flush();
		}
	}
	
	public boolean traceOn()
	{
		return doTrace;
	}

}
