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
package ldap;

import java.util.Hashtable;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.naming.Context;
import javax.naming.directory.Attributes;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.mq.jms.MQQueueConnectionFactory;

/**
 * Purpose of this tool: Configure LDAP server on 9.3.179.108 for lab use. (only
 * needs to be run once)
 * 
 * The purpose of this file is to configure the LDAP server at 9.3.179.108 with
 * the objects that later will be queried by the SVTBridge. The intention is
 * that by using LDAP and JNDI that better portability could be achieved by a
 * customer for an enterprise application, and this functionality is meant to
 * illustrate that, and provide some demonstration of that.
 * 
 * Prerequisites to running this tool:
 * 
 * This tool needs to be run prior to starting the SVTBridge to make sure that
 * the LDAP Server is properly configured. Prior to running this tool, the LDAP
 * server must be started (slapd) and configured with jndiTest as per Caroline
 * M's notes which were attached to RTC task 12901 for future reference. Also,
 * prior to running this tool the ismserver at 10.10.3.22 must be started,
 * because it is used as verification that the tool ran successfully.
 * 
 * Here is an example of how this tool will look when run on the command line
 * when it is run successfully:
 * 
 * 
 * [root@mar022 developer]# java -cp
 * ./LdapConfigure.jar:./com.ibm.mqjms.jar:/niagara
 * /application/client_ship/lib/jms
 * .jar:/niagara/application/client_ship/lib/imaclientjms.jar ldap.LdapConfigure
 * objectClass: organizationalRole SUCCESS. Successfully configured LDAP on
 * ldap://9.3.179.108:389/o=jndiTest SUCCESS. Successfully used LDAP to connect
 * to ISM server on: 10.10.3.22 You may type CTRL-C now to end the configuration
 * and start using your LDAP configuration ^C[root@mar022 developer]#
 * 
 * Generally , this tool only needs to be run a single time, unless for some
 * reason the 9.3.179.108 server gets reconfigured or needs to be setup from
 * scratch again.
 * 
 */

public class LdapConfigure {

	public static void main(String[] args) {

		Hashtable<String, String> env = new Hashtable<String, String>();
		env.put(Context.INITIAL_CONTEXT_FACTORY,
				"com.sun.jndi.ldap.LdapCtxFactory");
		env.put(Context.PROVIDER_URL, "ldap://9.3.179.108:389/o=jndiTest");
		env.put(Context.SECURITY_AUTHENTICATION, "simple");
		env.put(Context.SECURITY_PRINCIPAL, "cn=Manager,o=jndiTest"); // specify
		// the
		// username
		env.put(Context.SECURITY_CREDENTIALS, "secret");

		try {
			DirContext ctx = new InitialDirContext(env);
			// Ask for all attributes of the object
			Attributes attrs = ctx.getAttributes("cn=Manager");

			try {
				ConnectionFactory storeISMconnectionFactory = ImaJmsFactory
						.createConnectionFactory();
				ctx.rebind("cn=ISM_CONNECTION_FACTORY",
						storeISMconnectionFactory);

				ConnectionFactory storeMQConnectionFactory = new MQQueueConnectionFactory();
				ctx
						.rebind("cn=MQ_CONNECTION_FACTORY",
								storeMQConnectionFactory);
			} catch (JMSException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (Exception e) {
				System.err.println("Exception occurred: " + e.getMessage());
			}

			// Find the surname attribute ("sn") and print it
			System.out
					.println("objectClass: " + attrs.get("objectClass").get());

			ConnectionFactory ISMconnectionFactory = (ConnectionFactory) ctx
					.lookup("cn=ISM_CONNECTION_FACTORY");
			String ismserver = "10.10.3.22";
			String ismport = "16102";
			int clientIdNum = 0 + (int) (Math.random() * ((1000000 - 0) + 1));
			String clientId = "id" + clientIdNum;
			if (ismserver != null)
				((ImaProperties) ISMconnectionFactory).put("Server", ismserver);
			if (ismport != null)
				((ImaProperties) ISMconnectionFactory).put("Port", ismport);

			Connection consumerConnISM = null;
			consumerConnISM = ISMconnectionFactory.createConnection();
			consumerConnISM.setClientID(clientId);

			System.out.println("SUCCESS. Successfully configured LDAP on "
					+ env.get(Context.PROVIDER_URL));
			System.out
					.println("SUCCESS. Successfully used LDAP to connect to ISM server on: "
							+ ismserver);
			System.out
					.println("You may type CTRL-C now to end the configuration and start using your LDAP configuration");
		} catch (Exception e) {
			System.err.println("Exception occurred:" + e);
		}
	}
}
