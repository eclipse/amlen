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
package com.ibm.ism.mqbridge.test;

import java.io.FileOutputStream;
import java.io.PrintStream;
import java.text.SimpleDateFormat;
import java.util.Date;

public class LoggingUtilities {
	private boolean verboseLogging= false;
	private PrintStream log = System.out;
	private String logFile;
		
	public String timeNow() {
		Date now = new Date(System.currentTimeMillis());
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
		return sdf.format(now);
	}
	
	public void printString(String stringToPrint) {
		log.println(timeNow() + " " + stringToPrint);
	}
		
	public void verboseLoggingPrintString(String stringToPrint) {
		if (verboseLogging) {
			printString(stringToPrint);
		}
	}	
	
	public String getLogFile() {
		return logFile;
	}

	public void setLogFile(String logFile) {
		this.logFile = logFile;
	}

	public PrintStream getLog() {
		return log;
	}

	public void setLog(PrintStream log) {		
		this.log = log;
	}

	public void setLog(String filename) {
		if (filename != null && !filename.equalsIgnoreCase("")) {
			try {
				FileOutputStream fos = new FileOutputStream(filename);
				log = new PrintStream(fos);
				if (verboseLogging)
					log.println("Log sent to " + filename);

			} catch (Throwable e) {
				log.println("Log entries sent to log instead of " + filename);
			}
		}
	}
	
	public boolean isVerboseLogging() {
		return verboseLogging;
	}

	public void setVerboseLogging(boolean verboseLogging) {
		this.verboseLogging = verboseLogging;
	}
	
	public void flushFile() {
		log.flush();
	}
	
	public void closeFile() {
		log.close();
	}
}
