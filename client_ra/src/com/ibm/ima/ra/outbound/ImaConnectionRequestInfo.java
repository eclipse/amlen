/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.ra.outbound;

import javax.resource.spi.ConnectionRequestInfo;

final class ImaConnectionRequestInfo implements ConnectionRequestInfo { 

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final String username;
    private final String password;
    private final int hashcode;
    /**
	 * 
	 */
	ImaConnectionRequestInfo(String user, String pwd) {
        int hc1 = (user != null) ? user.hashCode() : 0 ;
        int hc2 = (pwd != null) ? pwd.hashCode() : 0 ;
        hashcode =  (31*hc1+hc2);
	    username = user;
	    password = pwd;
	}
	  /**
	   * Get a username
	   * 
	   * @return username
	   */
	  String getUsername() {
	    return username;
	  }

	  /**
	   * Get a password
	   * 
	   * @return password
	   */
	  String getPassword() {
	    return password;
	  }

	  /**
	   * Compare two ConnectionRequestInfo objects for equality.
	   * 
	   * See 6.5.1.2 in the JCA 1.5 spec
	   */
	  public boolean equals(Object o) {
	     if (o instanceof ImaConnectionRequestInfo) {
            ImaConnectionRequestInfo cri = (ImaConnectionRequestInfo) o;
            if(hashcode == cri.hashcode){
                /* Compare user names */
                if(username == null){
                    if(cri.username != null)
                        return false;
                } else {
                    if(!username.equals(cri.username))
                        return false;
                }
                /* User names are the same - compare passwords */
                if(password == null){
                    if(cri.password == null)
                        return true;
                    return false;
                } 
                if(password.equals(cri.password)) {
                        return true;
                }
            }
        } 
	    return false; 
	  }
	  
	  /**
	   * Override hashCode()
	   * 
	   * See 6.5.1.2 in the JCA 1.5 spec
	   */
	  public int hashCode() {
	      return hashcode;
	  }
	  
	  public String toString(){
	      StringBuffer sb = new StringBuffer(128);
	      sb.append(username).append(":********");
	      return sb.toString();
	  }
}
