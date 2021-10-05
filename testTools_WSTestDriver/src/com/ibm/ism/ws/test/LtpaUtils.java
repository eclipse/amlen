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
package com.ibm.ism.ws.test;

import java.security.Key;
import java.security.MessageDigest;
import java.security.spec.KeySpec;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.StringTokenizer;

import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESedeKeySpec;

import org.apache.commons.codec.binary.Base64;

public class LtpaUtils {

    public static void main(String[] args)
    {
        // If the key contains \= please replace it with =
        String ltpa3DESKey = "H4HSaGbJ1SA+cxjq/t8h3+yrYaIlBt/Fiaq38OZOd18\\="; // you can get this from your Websphere configuration
        String ltpaPassword = "ima4test"; // you can also get this from your Websphere cofiguration
        
        String tokenCipher = "";
        
        try{
            String option = args[0];
            if(option.compareToIgnoreCase("-d") == 0)
            {
                tokenCipher = args[1];
            }
            else
            {
                throw new Exception("Unhandled parameter.");
            }
        }
        catch(Exception e)
        {
            System.out.println("Usage: java LtpaUtils -d <encryptedLtpaToken> - Takes an encrypted base64 ASCII token and decrypts it");
            return;
        }
                
        try{
            byte[] secretKey = getSecretKey(ltpa3DESKey, ltpaPassword);
            String ltpaPlaintext = new String(decryptLtpaToken(tokenCipher, secretKey));
            displayTokenData(ltpaPlaintext);
        }
        catch(Exception e)
        {
            System.out.println("Caught inner: " + e);
        }
    }
    
    public static void displayTokenData(String token)
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
        System.out.println("\n\nFull token string : " + token);
    }
    
    public static byte[] getSecretKey(String shared3DES, String password) throws Exception
    {
        MessageDigest md = MessageDigest.getInstance("SHA");
        md.update(password.getBytes());
        byte[] hash3DES = new byte[24];
        System.arraycopy(md.digest(), 0, hash3DES, 0, 20);
        Arrays.fill(hash3DES, 20, 24, (byte) 0);
        // decrypt the real key and return it
        return decrypt(Base64.decodeBase64(shared3DES), hash3DES);
    }
    
    public static byte[] decryptLtpaToken(String encryptedLtpaToken, byte[] key) throws Exception 
    {
        final byte[] ltpaByteArray = Base64.decodeBase64 (encryptedLtpaToken);
        return decrypt(ltpaByteArray, key);
    }

    public static byte[] decrypt(byte[] ciphertext, byte[] key) throws Exception {
        final Cipher cipher = Cipher.getInstance("DESede/ECB/PKCS5Padding");
        final KeySpec keySpec = new DESedeKeySpec(key);
        final Key secretKey = SecretKeyFactory.getInstance("TripleDES").generateSecret(keySpec);

        cipher.init(Cipher.DECRYPT_MODE, secretKey);
        return cipher.doFinal(ciphertext);
    }
}
