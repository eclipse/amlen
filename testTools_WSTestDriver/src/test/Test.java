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
package test;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.StringTokenizer;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;


public class Test {
	static {
	    HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier() {
			
			public boolean verify(String hostname, SSLSession session) {
				// TODO Auto-generated method stub
				return true;
			}
		});
	        
	}

//	private static String 	_WebServer="10.10.10.10:9443";
//	private static	String	_userID="LTPAUser1";
//	private static	String	_password="ima4test";
//	private static boolean exit = true;
	public static void main(String [] args) {
		String version = " Java Version: " +
        	System.getProperty("java.version")+
        	" from "+System.getProperty("java.vendor");
		System.out.println(version);
		/*SSLSocketFactory factory;
		try {
			SSLContext context = SSLContext.getInstance("TLSv1.2");
			String keyStoreType = KeyStore.getDefaultType();
			String keyStoreName = "../common/ibm.jks";
			String keyMgrAlgo = KeyManagerFactory.getDefaultAlgorithm();
			KeyStore keyStore=KeyStore.getInstance(keyStoreType);
			keyStore.load(new FileInputStream(keyStoreName), null);
			KeyManagerFactory keyMgrFact = KeyManagerFactory.getInstance(keyMgrAlgo);
			keyMgrFact.init(keyStore, "password".toCharArray());
			KeyManager[] keyMgr=keyMgrFact.getKeyManagers();
			context.init(keyMgr, null, null);

			factory = context.getSocketFactory();
			//factory = SSLContext.getDefault().getSocketFactory();
			String[] suites = factory.getSupportedCipherSuites();
			for (int i=0; i<suites.length; i++) {
				System.out.println("CipherSuite["+i+"]='"+suites[i]+"'");
			}
			Socket socket = factory.createSocket("10.10.1.119", 21470);
			SSLSocket sslSocket = (SSLSocket)socket;
			String[] esuites = sslSocket.getEnabledCipherSuites();
			for (int i=0; i<esuites.length; i++) {
				System.out.println("EnabledCipherSuite["+i+"]='"+esuites[i]+"'");
			}
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (KeyManagementException e) {
			e.printStackTrace();
		} catch (KeyStoreException e) {
			e.printStackTrace();
		} catch (CertificateException e) {
			e.printStackTrace();
		} catch (UnrecoverableKeyException e) {
			e.printStackTrace();
		}
		Date date = new Date(1377879073399L);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd-HH:mm:ss z");
		System.out.println("Date(1377879073399L) is "+sdf.format(date));
		ClientConfig clientConfig = new ClientConfig();
		if (exit) return;
		// Use Basic Authentication
		BasicAuthSecurityHandler basicAuthSecHandler = new BasicAuthSecurityHandler(); 
		basicAuthSecHandler.setUserName(_userID); basicAuthSecHandler.setPassword(_password);
		clientConfig.handlers(basicAuthSecHandler);
		
		// Define resource
//		RestClient client = new RestClient(clientConfig);
//		Resource resource = client.resource("https://"+_WebServer+"/ltpawebsample/rest/ltpa/token");
*/
//		ClientResponse response = resource.contentType("application/json").accept("*/*").get();
/*		String LTPAToken = "AAfVju+FU+Ix9FBTOARxUT3+rFvzs9dC0d1x7vpERyaneesf4Y/gWnS0rBnbHnuyni1U5cXJaTHmRncEl5hb1bK2KTgrSk1fCeclu6Q1LIjnpDBoOIWlJo9gt2MNq4eDDipdG1I7tvJfM3P5wtnOv93iyX6AdVp/FOtGScFTYLOjYwot+1PVI2j5Svmc7SjdQzZj1G7ySh+J4zv3Akoe3MaOOEmTBwPaVSXDSk7SXcqxgAJ6h+Kx0hQAyUrBXy7C4ddZMhqjVUiw+EtlQVHtbWQhe5j2H+Vhcff44zJYKE4HAlVJnlfTAulNUnyyEh54zemVhfMupu5sdABY9AQMHIcg/JfIiwGKEkZrZlP13IM2cdhALyKRYQ==";
		System.out.println(runCommand("pwd"));
		String ltpa3DESKey = runCommand("grep com.ibm.websphere.ltpa.3DESKey /cygdrive/c/IBM/LTPA.key");
		System.out.println("ltpa3DESKey line '"+ltpa3DESKey+"'");
		ltpa3DESKey = ltpa3DESKey.substring(ltpa3DESKey.indexOf("=")+1);
		System.out.println("ltpa3DESKey '"+ltpa3DESKey+"'");
		ltpa3DESKey = "H4HSaGbJ1SA+cxjq/t8h3+yrYaIlBt/Fiaq38OZOd18\\=";
		
		try {
			byte[] secretKey = LtpaUtils.getSecretKey(ltpa3DESKey, _password);
			System.out.println("secretKey is '"+secretKey+"'");
			String ltpaPlaintext = new String(LtpaUtils.decryptLtpaToken(LTPAToken, secretKey));
			logTokenData(ltpaPlaintext);
		} catch (Exception e) {
			e.printStackTrace();
			return;
		}
		System.out.println("Got LTPA Token '"+LTPAToken+"'.");
*/
	}
	
    public static void logTokenData(String token)
    {
        System.out.println("\n");
        StringTokenizer st = new StringTokenizer(token, "%");
        String userInfo = st.nextToken();
        String expires = st.nextToken();
//        String signature = st.nextToken();
        
        Date d = new Date(Long.parseLong(expires));
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd-HH:mm:ss z");
        System.out.println("Token is for: " + userInfo);
        System.out.println("Token expires at: " + sdf.format(d));
        System.out.println("Full token string : " + token);
    }

/*
	private static String runCommand(String command) {
		String commandResult = new String();
		String commandError = new String();

		try {
			if (System.getProperty("os.name").toLowerCase().indexOf("win") >= 0) {
//				if(!_windbg)
//					command = "bash -c \"" + command +"\"";
			}
			System.out.println("command='"+command+"'");
			Process pr = Runtime.getRuntime().exec(command);
			pr.waitFor();

			BufferedReader buf;
			String line;
			buf = new BufferedReader(new InputStreamReader(pr.getErrorStream())); 
			while ((line = buf.readLine()) != null) {
				commandError += line + "\n";
			}
			System.out.println("commandError = '"+commandError+"'");

			buf = new BufferedReader(new InputStreamReader(pr.getInputStream()));

			while ((line = buf.readLine()) != null) {
				commandResult += line + "\n";
			}
			
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

		return commandResult;
	}
*/
}
