// Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.tools.msg.logger;

import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

public class LogManager {

	private static Logger logger = Logger.getLogger("LLMMsgLogger");

	private static FileHandler fh;
	private static boolean isInitLogger=false;
	
	

	static public Logger getLogger() {
		if(!isInitLogger) initLogger();
		return logger;
	}
	
	private static void initLogger(){
		try {
			logger.setUseParentHandlers(false);
			fh = new FileHandler("llmmsgtool.log", false);
			logger.addHandler(fh);
			logger.setLevel(Level.ALL);
			SimpleFormatter formatter = new SimpleFormatter();
			fh.setFormatter(formatter);
			isInitLogger=true;
		} catch (Exception exception) {
			exception.printStackTrace();
		}
	}

}
