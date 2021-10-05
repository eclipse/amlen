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
package svt.util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLEncoder;
import java.util.Properties;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;

import org.eclipse.paho.client.mqttv3.internal.security.SSLSocketFactoryFactory;

import svt.scale.backend.ATBackend;

import com.ibm.json.java.JSONObject;

/**
 * Retrieve an OAuth 2 AccessToken from the MessageSightOAuth OAuth provider that we have configured in WAS. The
 * provider in WAS will have OAuth clients defined. clientId and clientSecret are used to specify one of the OAuth
 * clients. username and password are used to specify a WAS user.
 * 
 * Ex. I have OAuth client "imaclient" and make the token request as WAS user "admin".
 * 
 * Creating the HttpsUrlConnection also requires having the WAS certificate in our truststore...
 * 
 * TODO: There are other forms of OAuth authentication than "password".
 * 
 * 
 */
public class GetOAuthAccessToken {
    private final String charset = "UTF-8";
    private String accessToken;
    private String _uri;
    private String _clientId;
    private String _clientSecret;
    private String _username;
    private String _password;
    // private static String scope = "";
    // private static String state = "";
    private String tokenEndPoint;

    static {
        HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier() {
            public boolean verify(String hostname, SSLSession session) {
                // I don't care that our automation WAS servers have self-signed certificates.
                // Just let me in please :(
                return true;
            }
        });
    }

    // public GetOAuthAccessToken(MqttSample config) {
    //
    // }

    public String getOAuthToken(String clientId, ATBackend config) {
       return getOAuthToken(clientId, config.password, config.oauthProviderURI, null, null, null, null);
    }
    
   public String getOAuthToken(String clientId, String password, String oauthProviderURI, String trustStore, String trustStorePassword,
         String keyStore, String keyStorePassword) {

      _uri = oauthProviderURI;
      _clientId = clientId;
      _clientSecret = password;
      _username = "admin";
      _password = "admin";

      // _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "Request OAuth 2 AccessToken from WAS.");
      int attempt = 1;
      accessToken = null;
      while ((accessToken == null) && (attempt <= 20)) {
         try {
            // Get just the IP
            // _uri = EnvVars.replace(_uri);
            tokenEndPoint = "https://" + _uri + "/oauth2/endpoint/MessageSightOAuth/token";
            String grant_type = "password";// "client_credentials";
            // String query = String.format("grant_type=%s&scope=%s&client_id=%s&client_secret=%s",
            String query = String.format("grant_type=%s&client_id=%s&client_secret=%s&username=%s&password=%s",
                  URLEncoder.encode(grant_type, charset),
                  // URLEncoder.encode(scope, charset),
                  URLEncoder.encode(_clientId, charset), URLEncoder.encode(_clientSecret, charset), URLEncoder.encode(_username, charset),
                  URLEncoder.encode(_password, charset));
            // send Resource Request using (accessToken);
            sendRequestForAccessToken(query, trustStore, trustStorePassword, keyStore, keyStorePassword);
            // if (accessToken != null) {
            // _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "Retrieved Access Token: " + accessToken);
            // } else {
            // _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Failed to get Access Token");
            // return false;
            // }
         } catch (Exception exc) {
            // _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Exception thrown in GetOAuthAccessToken: "
            // + exc.getMessage());
            // exc.printStackTrace();
            try {
               long s = Math.min(5000, 2000 + Thread.currentThread().getId());
               System.out.println("sleeping "+s+" ms");
               Thread.sleep(s);
            } catch (InterruptedException e) {
            }
         }
         attempt++;
      }
      // _dataRepository.storeVar("OAUTH_ACCESS_TOKEN", accessToken);
      return accessToken;
   }

    private void sendRequestForAccessToken(String query, String trustStore, String trustStorePassword, String keyStore, String keyStorePassword) throws IOException {
        URL url = new URL(tokenEndPoint);
        HttpsURLConnection con = (HttpsURLConnection) url.openConnection();
        con.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=" + charset);
        con.setDoOutput(true);
        con.setRequestMethod("POST");
        
        Properties sslClientProps = getSSLClientProps(trustStore, trustStorePassword, keyStore, keyStorePassword);
        SSLSocketFactory sf = getSSLSocketFactory(sslClientProps);
        con.setSSLSocketFactory(sf);
        
        OutputStream output = null;
        try {
            output = con.getOutputStream();
            output.write(query.getBytes(charset));
            output.flush();
        } finally {
            if (output != null)
                try {
                    output.close();
                } catch (IOException logOrIgnore) {
                }
        }
        con.connect();
        // _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "response message is = " +
        // con.getResponseMessage());
        // read the output from the server
        BufferedReader reader = null;
        StringBuilder stringBuilder;
        reader = new BufferedReader(new InputStreamReader(con.getInputStream()));
        stringBuilder = new StringBuilder();
        String line = null;
        try {
            while ((line = reader.readLine()) != null) {
                stringBuilder.append(line + "\n");
            }
        } finally {
            if (reader != null)
                try {
                    reader.close();
                } catch (IOException logOrIgnore) {
                }
        }
        String tokenResponse = stringBuilder.toString();
        // _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "response is = " + tokenResponse);
        JSONObject json = JSONObject.parse(tokenResponse);
        if (json.containsKey("access_token")) {
            /* Pass the entire JSON response as password instead of just access_token */
            // accessToken = (String)json.get("access_token");
            accessToken = tokenResponse;
        }
    }
    
    

    private Properties getSSLClientProps() {
       String keyStore = System.getProperty("com.ibm.ssl.keyStore");
       if (keyStore == null)
          keyStore = System.getProperty("javax.net.ssl.keyStore");

       String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
       if (keyStorePassword == null)
          keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");

       String trustStore = System.getProperty("com.ibm.ssl.trustStore");
       if (trustStore == null)
          trustStore = System.getProperty("javax.net.ssl.trustStore");

       String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
       if (trustStorePassword == null)
          trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");
       
       return getSSLClientProps(trustStore, trustStorePassword, keyStore, keyStorePassword);

    }

    private Properties getSSLClientProps(String trustStore, String trustStorePassword, String keyStore, String keyStorePassword) {
       Properties sslClientProps = new Properties();
       sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1");
       sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");

       if (keyStore != null)
          sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
       if (keyStorePassword != null)
          sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
       if (trustStore != null)
          sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
       if (trustStorePassword != null)
          sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);

       return sslClientProps;

    }
    
    
    // NOTE:  SSLSocketFactoryFactory is an internal paho class
    // this method is an attempt to optimize loading of the truststore.
    // though I have not noticed a real improvement.
    private SSLSocketFactory getSSLSocketFactory(Properties sslClientProps) {
       SSLSocketFactoryFactory socketFactoryFactory = new SSLSocketFactoryFactory();
       socketFactoryFactory.initialize(sslClientProps, null);
       SSLSocketFactory sf = null;
       try {
          sf = socketFactoryFactory.createSocketFactory(null);
       } catch (Exception e) {
          // TODO Auto-generated catch block
          e.printStackTrace();
       }
       return sf;
    }

    
}
