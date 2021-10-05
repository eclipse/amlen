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
package com.ibm.ims.svt.clients;

public class RetainedMap {
	
	private String key = null;
	private boolean retained = false;
	
	
	public RetainedMap(String key, boolean retained)
	{
		this.key = key;
		this.retained = retained;
	}




	public String getKey() {
		return key;
	}


	public void setKey(String key) {
		this.key = key;
	}


	public boolean isRetained() {
		return this.retained;
	}


	public void setRetained(boolean retained) {
		this.retained = retained;
	}




	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((key == null) ? 0 : key.hashCode());
		result = prime * result + (retained ? 1231 : 1237);
		return result;
	}




	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		RetainedMap other = (RetainedMap) obj;
		if (key == null) {
			if (other.key != null)
				return false;
		} else if (!key.equals(other.key))
			return false;
		if (retained != other.retained)
			return false;
		return true;
	}

}
