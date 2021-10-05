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
package com.ibm.ism.tools.msg.tms;

import org.w3c.dom.Document;

import com.ibm.ism.tools.msg.ISMLogEntryObject.LOG_ENTRY_COMPONENT_TYPE;

public class TMSCompDoc {
	

	
	private String tmsfilepath;
	
	private Document doc;
	
	private LOG_ENTRY_COMPONENT_TYPE _type;
	
	private boolean isSaved=true;
	
	public TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE type, String tmsfilepath_in, Document indoc){
		tmsfilepath=tmsfilepath_in;
		doc = indoc;
		set_type(type);
	}

	public String getTmsfilepath() {
		return tmsfilepath;
	}

	public void setTmsfilepath(String tmsfilepath) {
		this.tmsfilepath = tmsfilepath;
	}

	public Document getDoc() {
		return doc;
	}

	public void setDoc(Document doc) {
		this.doc = doc;
	}

	public boolean isSaved() {
		return isSaved;
	}

	public void setSaved(boolean isSaved) {
		this.isSaved = isSaved;
	}

	public void set_type(LOG_ENTRY_COMPONENT_TYPE _type) {
		this._type = _type;
	}

	public LOG_ENTRY_COMPONENT_TYPE get_type() {
		return _type;
	}
	
	
	
	
	

}
