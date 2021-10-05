/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima;
import java.util.UUID;
import com.ibm.ima.util.Utils;
import com.ibm.ima.plugin.util.ImaSsoCrypt;

public class ISMLDAPProperties {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	private static String pwval = null;

	public static String getURL() {
		return ISMWebUIProperties.instance().getOnBoardLDAPURL();
	}

	public static String getAuthenticationType() {
		return ISMWebUIProperties.instance().getOnBoardLDAPAuthType();
	}

	public static String getAuthenticationPrincipalID() {
		return ISMWebUIProperties.instance().getOnBoardLDAPPrincipalID();
	}

	public static String getAuthenticationPassword() {
		String pw = _getpw();
		if (pw != null && pw != "") {
			return pw;
		}
		return ISMWebUIProperties.instance().getOnBoardLDAPPassword();
	}

	public static String getContextForWebUiUsers() {
		return ISMWebUIProperties.instance().getOnBoardLDAPWebUiUsersContext();
	}

	public static String getContextForWebUiGroups() {
		return ISMWebUIProperties.instance().getOnBoardLDAPWebUiGroupsContext();
	}

	public static String getContextForMessagingUsers() {
		return ISMWebUIProperties.instance().getOnBoardLDAPMessagingUsersContext();
	}

	public static String getContextForMessagingGroups() {
		return ISMWebUIProperties.instance().getOnBoardLDAPMessagingGroupsContext();
	}
	
	private static String _getpw() {
		String pw = null;
		if (pwval != null) {
			try {
				byte[] barrpw = javax.xml.bind.DatatypeConverter.parseBase64Binary(pwval);
				String encpw = new String(barrpw, "UTF-8");
				String tmpp = ImaSsoCrypt.decrypt(encpw);
				int len = tmpp.length();
				pw = tmpp.substring(5,len-5);
			} catch (Exception ex) {
				// ex.printStackTrace();
				// Ignore exception.  Null password will trigger an error later.
			}
		} else {
			try {
				String encpw = Utils.getUtils().getLibertyProperty("keystore_imakey");
				if (encpw != null && encpw != "") {
					if (encpw.indexOf("{IMA}") == 0) {
						pwval = encpw.substring(5);
						byte[] barrpw = javax.xml.bind.DatatypeConverter.parseBase64Binary(pwval);
						encpw = new String(barrpw, "UTF-8");
						String tmpp = ImaSsoCrypt.decrypt(encpw);
						int len = tmpp.length();
						pw = tmpp.substring(5,len-5);
					} else {
						byte[] barrpw = javax.xml.bind.DatatypeConverter.parseBase64Binary(encpw);
						pw = new String(barrpw, "UTF-8");
						String uuid = UUID.randomUUID().toString();
						
						
						// Write the encrypted value to the properties file
						String tmpp = uuid.substring(0,5) + pw + uuid.substring(uuid.length()-5,uuid.length());
						encpw = ImaSsoCrypt.encrypt(tmpp);
						barrpw = encpw.getBytes("UTF-8");
						tmpp = javax.xml.bind.DatatypeConverter.printBase64Binary(barrpw);
						Utils.getUtils().setLibertyProperty("keystore_imakey", "{IMA}"+tmpp);
					}
				}
			} catch (Exception ex) {
				// ex.printStackTrace();
				// Ignore exception.  Null password value will trigger an error later.
			}
		}
		return pw;
	}
}
