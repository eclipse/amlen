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
package com.ibm.ima.resources.security;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;

import com.ibm.websphere.jaxrs20.multipart.IMultipartBody;
import com.ibm.websphere.jaxrs20.multipart.IAttachment;

import com.ibm.ima.ISMWebUIProperties;
import com.ibm.ima.resources.AbstractFileUploader;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.VALIDATION_RESULT;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

/**
 * CertificateUtilities
 */

public class CertificatesFileUploader extends AbstractFileUploader {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = CertificatesFileUploader.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server)
    protected final static String KEYSTORE_DIR = "/certificates/keystore";
    
    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server)
    protected final static String LDAP_TRUSTSTORE_DIR = "/certificates/LDAP";
    
    private final static String TEMP_DIR = "/tmp";

    private static final String OPENSSL_COMMAND = "/usr/bin/openssl";
    private static final String RSA_ARG = "rsa";
    private static final String X509_ARG = "x509";
    private static final String PKCS12_ARG = "pkcs12";
    private static final String INFO_ARG = "-info";
    private static final String EXPORT_ARG = "-export";
    private static final String NODES_ARG = "-nodes";
    private static final String ENDDATE_ARG = "-enddate";
    private static final String NOOUT_ARG = "-noout";
    private static final String CHECK_ARG = "-check";
    private static final String PASSOUT_ARG = "-passout";
    private static final String PASS_PREFIX = "pass:";
    private static final String PASSIN_ARG = "-passin";
    private static final String OUT_ARG = "-out";
    private static final String INKEY_ARG = "-inkey";
    private static final String IN_ARG = "-in";

    private static final String PARSED_PEM_SUFFIX = ".parsed.pem";

    //Relative to com.ibm.ima.webUIInstallDirectory
    private final static String CHECK_KEY_MATCHES_CERT_COMMAND = "/bin/matchkeycert.sh";
    // {0} certificate, {1} key, return code of 0 is pass

    //Relative to com.ibm.ima.wlpInstallDirectory 
    private static final String LIBERTY_SECURITY_UTIL = "/bin/securityUtility";
    private static final String ENCODE = "encode";
    
    private static final int NOT_AFTER_FIRST_DAY_INDEX = 13;
    private static final String NOT_AFTER_FORMAT_STRING_1 = "'notAfter='MMM  d kk:mm:ss yyyy zzz";
    private static final String NOT_AFTER_FORMAT_STRING_2 = "'notAfter='MMM dd kk:mm:ss yyyy zzz";

    private final static String PKCS12 = "PKCS12";
    private final static String USER_KEY_STORE = "key.p12";
    
    public final static int CERTIFICATE_RESOURCE_FAILED = 1;
    public final static int CERTIFICATE_RESOURCE_FAILED_BAD_PASSWORD = 4;

    public CertificatesFileUploader(HttpServletRequest currentRequest) {
        super(currentRequest);
    }

    /**
     * Retrieves a list of all Certificates currently uploaded to the ISMServer.
     * 
     * @param keystoreDir store to list certificates for
     * 
     * @return A list of all Certificate file names.
     */
    public final static ArrayList<String> getCertificates(String keystoreDir) {
        logger.traceEntry(CLAS, "getCertificates");

        ArrayList<String> result = new ArrayList<String>();
        if (keystoreDir == null) {
            logger.trace(CLAS, "getCertificates", "No keystoreDir specified");
            logger.log(LogLevel.LOG_ERR, CLAS, "getCertificates", "CWLNA5011");            
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5011", null);
        }

        try {
            File keystore = new File(keystoreDir);
            if (!keystore.exists()) {
                logger.log(LogLevel.LOG_ERR, CLAS, "getCertificates", "CWLNA5010");
            } else {
                File[] files = keystore.listFiles();
                if (files != null) {
                    for (File f : files) {
                        result.add(f.getName());
                    }
                }
            }
        } catch (Throwable t) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "getCertificates", "CWLNA5009", t);
        }

        logger.traceExit(CLAS, "getCertificates", result);

        return result;
    }

    /**
     * Uploads a Certificate file to TMP. Doesn't do anything to decrypt or anything
     * 
     * @param uriInfo UriInfo from request
     * @param mpMessage multi-part message from request
     * @param locale user's locale from request
     * @param directoryName optional directory to upload files to, default is TEMP_DIR
     * @param fileList optional arraylist to store a list of filenames that were uploaded.
     */
    public String uploadFiles(UriInfo uriInfo, IMultipartBody mpMessage, Locale locale, String directoryName,
            ArrayList<String> fileNames) {
        logger.traceEntry(CLAS, "uploadFilesToTemp");

        String result = "";

        if (directoryName == null) {
            directoryName = CertificatesFileUploader.TEMP_DIR;
        }
        ArrayList<String> files = new ArrayList<String>();

        File uploadedFile = null;

        List <IAttachment> attachments = mpMessage.getAllAttachments();

        try {
            for (IAttachment attachment : attachments) {
                uploadedFile = super.uploadFileFromPart(attachment, directoryName);
                if (uploadedFile != null) {
                    files.add("tmp:" + uploadedFile.getName());
                    if (fileNames != null) {
                        fileNames.add(uploadedFile.getName());
                    }
                }
            }

        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Throwable t) {            
            logger.log(LogLevel.LOG_ERR, CLAS, "uploadFile", "CWLNA5065", t);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, t, "CWLNA5065", null);
        }

        result = formatResponse(files);

        logger.traceExit(CLAS, "uploadFilesToTemp", result);
        return result;
    }

    /**
     * Method to process Liberty profile key/cert pair and replace the existing keystore
     * 
     * @param cert LibertyCertificate object with names and references
     */
    public void processLibertyCertificate(LibertyCertificate cert) {
        // check if we're dealing with separate key/cert or .p12 file
        logger.traceEntry(CLAS, "processLibertyCertificate", null);

        File uploadedCert = Utils.safeFileCreate(CertificatesFileUploader.TEMP_DIR, cert.getCertificate(), true);
        File uploadedKey = Utils.safeFileCreate(CertificatesFileUploader.TEMP_DIR, cert.getKey(), true);

        if (uploadedCert == null || uploadedKey == null) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "processLibertyCertificate", "CWLNA5065", new Object[] { cert });
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", new Object[] { cert });
        }

        if (!isPkcs12(uploadedCert)) {
            try {
                verifyPem(uploadedCert);
            } catch (IsmRuntimeException e) {
                // the certificate is not a PEM, we should keep checking
                logger.log(LogLevel.LOG_WARNING, CLAS, "addLibertyCertificate", "CWLNA5073", new Object[] { cert });
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5073", new Object[] { cert });
            }
        }

        String pass = null;
        String keyPass = null;
        String certPass = null;
        if ((cert.getCertificatePassword() != null) && (!cert.getCertificatePassword().equals(""))) {
            certPass = cert.getCertificatePassword();
            // now we need to encode it but skipping this step for now
        }

        if ((cert.getKeyPassword() != null) && (!cert.getKeyPassword().equals(""))) {
            keyPass = cert.getKeyPassword();
            // now we need to encode it but skipping this step for now
        }

        if (certPass != null) {
            pass = certPass;
        } else if (keyPass != null) {
            pass = keyPass;
        } else {
            pass = getEncodedPassword();
        }
        
        if (!uploadedCert.exists()) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "processLibertyCertificate", "CWLNA5065", new Object[] { cert });
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", new Object[] { cert });
        } else if (cert.getCertificate().equals(cert.getKey())) {
            // we have the same file for key and cert, checking if it's .p12
            if (isPkcs12(uploadedCert) && verifyPkcs12(uploadedCert, certPass)) {
                // looks like we're in luck, just need to copy and update Liberty properties
                File keystore = uploadedCert;
                if (!pass.equals(certPass)) {
                    // we need to recreate the pksc12 if no password was provided
                    parsePkcs12File(uploadedCert, certPass);
                    keystore = consolidateCertKey(getParsedFile(uploadedCert), null, certPass, pass);
                }
                String newPath = copyCertToLiberty(keystore);
                if (newPath != null) {
                    String currentPath = updateLibertyPropertiesKeystore(newPath, PKCS12, pass);
                    // now we should record the name in properties.xml to display in the UI
                    updateLibertyPropertiesCertKey(cert.getCertificate(), cert.getKey());
                    // we need to unencrypt to get the expiration date
                    Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.KEYSTORE_EXP,
                            String.valueOf(unencrypt(uploadedCert, pass, false)));
                    // clean files up from temp
                    if (uploadedCert.exists()) {
                        uploadedCert.delete();
                    }
                    File pemFile = getParsedFile(uploadedCert);
                    if (pemFile.exists()) {
                        pemFile.delete();
                    }
                    if (keystore.exists()) {
                        keystore.delete();
                    }
                    if (currentPath != null && !currentPath.endsWith(".jks")) {
                        File f = new File(currentPath);
                        if (f.exists()) {
                            f.delete();
                        }
                    }
                    return;
                } else {
                    logger.log(LogLevel.LOG_ERR, CLAS, "processLibertyCertificate", "CWLNA5065", new Object[] { cert });
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", new Object[] { cert });
                }
            } else {
                // handle combined cert and password in PEM file
                String password = certPass != null && certPass.trim().length() > 0 ? certPass : keyPass;
                if (password != null && password.trim().length() > 0) {
                    uploadedKey = splitCertAndKey(uploadedCert, password);
                    // now we can proceed as usual, it will be like separate cert and key pem files with no passwords
                }
                // don't need passwords any more
                certPass = null;
                keyPass = null;
            }
        }

        long expDate = -1;
        if (certPass != null && certPass.trim().length() > 0) {
            expDate = unencrypt(uploadedCert, certPass, false);
        } else {
            expDate = getCertificateExpiry(uploadedCert);
        }
        if (keyPass != null && keyPass.trim().length() > 0) {
            unencrypt(uploadedKey, keyPass, true);
        } else {
            checkKey(uploadedKey);
        }
        checkKeyMatchesCert(uploadedCert, uploadedKey);

        if (expDate > -1) {
            if ((isPkcs12(uploadedCert)) && (isPkcs12(uploadedKey))) {
                // both cert and key are .p12 throw an error
                logger.log(LogLevel.LOG_ERR, CLAS, "processLibertyCertificate", "CWLNA5067", new Object[] { cert });
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5067", new Object[] { cert });
            } else if (uploadedKey.exists()) {
                // we have separate key and certificate so we need to convert them to .p12
                File keyStore = consolidateCertKey(uploadedCert, uploadedKey, keyPass, pass);
                String newPath = copyCertToLiberty(keyStore);
                String currentPath = null;
                if (newPath != null) {
                    currentPath = updateLibertyPropertiesKeystore(newPath, PKCS12, pass);
                    // now we should record the name in properties.xml to display in the UI
                    updateLibertyPropertiesCertKey(cert.getCertificate(), cert.getKey());
                } else {
                    logger.log(LogLevel.LOG_ERR, CLAS, "processLibertyCertificate", "CWLNA5065", new Object[] { cert });
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", new Object[] { cert });
                }
                // clean up files from temp
                if (uploadedCert.exists()) {
                    uploadedCert.delete();
                }
                if (uploadedKey.exists()) {
                    uploadedKey.delete();
                }
                if (keyStore.exists()) {
                    keyStore.delete();
                }
                if (currentPath != null && !currentPath.endsWith(".jks")) {
                    File f = new File(currentPath);
                    if (f.exists()) {
                        f.delete();
                    }
                }
            } else {
                // the certificate and key are not the same file but only one exists
                logger.log(LogLevel.LOG_ERR, CLAS, "processLibertyCertificate", "CWLNA5065", new Object[] { cert });
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", new Object[] { cert });
            }

            Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.KEYSTORE_EXP,
                    String.valueOf(expDate));
        }
        logger.traceExit(CLAS, "processLibertyCertificate", null);
    }

    /**
     * Helper method to consolidate the key/certificate pair using openssl
     * 
     * @param uploadedCert
     *            Certificate to use
     * @param uploadedKey
     *            Key to use
     * @param passIn
     *            the input password (may be null)
     * @param passOut
     *            the output password (should not be null)
     * @return outputFile Returns the file object of the newly create .p12 file or null if there was an error
     */
    private File consolidateCertKey(File uploadedCert, File uploadedKey, String passIn, String passOut) {
        logger.traceEntry(CLAS, "consolidateCertKey", new Object[] { uploadedCert, uploadedKey });

        String[] output = new String[2];
        String outputFilePath = TEMP_DIR + "/" + USER_KEY_STORE;
        File outputFile = null;
        int rc = -1;


        try {
            String certificateFilename = uploadedCert.getCanonicalPath();
            String keyFilename = "";
            if (passIn == null) {
                passIn = "";
            }

            // /usr/bin/openssl pkcs12 -export -in {0} -inkey {1} -out {2} -passin pass:{3} -passout pass:{4}
            Runtime runtime = Runtime.getRuntime();
            Process process = null;
            if (uploadedKey != null) {
                keyFilename = uploadedKey.getCanonicalPath();
                String[] command = {OPENSSL_COMMAND, PKCS12_ARG, EXPORT_ARG, IN_ARG, certificateFilename, INKEY_ARG, keyFilename,
                        OUT_ARG, outputFilePath, PASSIN_ARG, PASS_PREFIX+passIn, PASSOUT_ARG, PASS_PREFIX+passOut};
                
                process = runtime.exec(command);
            } else {
                String[] command = {OPENSSL_COMMAND, PKCS12_ARG, EXPORT_ARG, IN_ARG, certificateFilename,
                        OUT_ARG, outputFilePath, PASSIN_ARG, PASS_PREFIX+passIn, PASSOUT_ARG, PASS_PREFIX+passOut};

                process = runtime.exec(command);                
            }
           
            rc = Utils.getUtils().getOutput(process, output);
            logger.trace(CLAS, "consolidateCertKey", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
            if (rc != 0) {
                Object[] params =
                        new Object[] { certificateFilename, keyFilename, output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "consolidateCertKey", "CWLNA5065", params);
            }
            // check if the file was actually created
            outputFile = new File(outputFilePath);
            if (outputFile.exists()) {
                return outputFile;
            }
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "consolidateCertKey", "CWLNA5011", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
        logger.traceExit(CLAS, "consolidateCertKey");
        return null;
    }

    private String getEncodedPassword() {
        String pass = Utils.DEFAULT_PASSWORD;
        // now we need to encode the password here, but for now just return
        return pass;
    }

    /**
     * Helper method to update Liberty profile properties to use the new keystore
     * 
     * @param newPath Path as a string to the new keystore
     * @param type Type of the new keystore
     * @param pass Password for the new keystore
     * @return current path to the keystore (before update)
     */
    private String updateLibertyPropertiesKeystore(String newPath, String type, String pass) {
        logger.traceEntry(CLAS, "updateLibertyPropertiesKeystore", new Object[] {newPath, type, "*****"});
        String currentPath = Utils.getUtils().libertyPropertyAccessRequest(Utils.GET_LIBERTY_PROPERTY,Utils.KEYSTORE_LOCATION, null);
        
        Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.KEYSTORE_LOCATION, newPath);
        Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.KEYSTORE_TYPE, type);
        Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.KEYSTORE_PASSWORD, encodePassword(pass));
        
        logger.traceExit(CLAS, "updateLibertyPropertiesKeystore", currentPath);
        return currentPath;
    }
    
    private String encodePassword(String password) {
        String encodedPassword = password;
        String[] output = new String[2];
        int rc = -1;
        
        try {
            // <wlp path>/bin/securityUtility encode password
            String[] command = {ISMWebUIProperties.instance().getWlpInstallDirectory() + LIBERTY_SECURITY_UTIL, ENCODE, password};

            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc == 0) {
                String s = output[Utils.OUT_MESSAGE];
                if (s.startsWith("{xor}")) {
                    if (s.endsWith("\n")) {
                        s = s.substring(0, s.length()-1);
                    }
                    // only store the encodedPassword if everything goes as expected!
                    encodedPassword = s;
                } else {
                    logger.trace(CLAS, "encodePassword", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                            + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
                }
            } 
        } catch (Exception e) {
            logger.trace(CLAS, "encodePassword", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
            logger.log(LogLevel.LOG_ERR, CLAS, "encodePassword", "CWLNA5011", e);
        }
        
        return encodedPassword;
    }

    /**
     * Helper method to update Liberty profile properties to use store cert/key names
     * 
     * @param certName Name of the certificate
     * @param keyName Name of the key
     */
    private void updateLibertyPropertiesCertKey(String certName, String keyName) {
        Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.CERT_NAME, certName);
        Utils.getUtils().libertyPropertyAccessRequest(Utils.SET_LIBERTY_PROPERTY, Utils.KEY_NAME, keyName);
    }

    /**
     * Helper method to copy key/cert pair to Liberty profile security resources folder
     * 
     * @param uploadedFile File to copy
     * @return newPath String path to the copied file
     */
    private String copyCertToLiberty(File uploadedFile) {
        logger.traceEntry(CLAS, "copyCertToLiberty");
        if (uploadedFile == null) {
            return null;
        }       
        
        String destFilename = Utils.getUniqueName(ISMWebUIProperties.instance().getLibertyConfigDirectory() + "/resources/security/", uploadedFile.getName());
        logger.trace(CLAS, "copyCertToLiberty", "We should be copying cert file " + uploadedFile.getName() + " to directory: " + ISMWebUIProperties.instance().getLibertyConfigDirectory() + ".");
       
        File dest = Utils.safeFileCreate(ISMWebUIProperties.instance().getLibertyConfigDirectory() + "/resources/security/", destFilename, true);
        if (dest == null) {
            logger.trace(CLAS, "copyCertToLiberty", "could not create safe file. This can happen if the WebUI data directory is not located at the usual location");
            return null;
        }
        String newPath = null;
        logger.trace(CLAS, "copyCertToLiberty", "Attempting to copy " + uploadedFile.getName()
                + " to the keystore folder");
        InputStream in = null;
        try {
            in = new FileInputStream(uploadedFile);
            writeToFile(in, dest);
            newPath = dest.getCanonicalPath();
        } catch (IOException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "copyCertToLiberty", VALIDATION_RESULT.UNEXPECTED_ERROR.getMessageID(),
                    new Object[] { uploadedFile });
            throw new IsmRuntimeException(Status.BAD_REQUEST, VALIDATION_RESULT.UNEXPECTED_ERROR.getMessageID(),
                    new Object[] { uploadedFile });
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "writeToFile", "CWLNA5007", new Object[] { in }, e);
                }
            }
        }
        logger.traceExit(CLAS, "copyCertToLiberty");
        return newPath;
    }

    /**
     * @param profile
     * @param certificatePassword
     * @param keyPassword
     * @param strict If false, skips certificates/keys that were not downloaded if they already exist in the keystore
     * @return
     */
    public void processCertificatesAndPasswords(CertificateProfile profile, String certificatePassword,
            String keyPassword, boolean strict) {

        File certificate = Utils.safeFileCreate(CertificatesFileUploader.TEMP_DIR, profile.getCertFilename(), true);
        File key = Utils.safeFileCreate(CertificatesFileUploader.TEMP_DIR, profile.getKeyFilename(), true);
        
        if (certificate == null || key == null) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "processCertificatesAndPasswords", "CWLNA5065");
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", null);
        }

        File keystoreCertFile = Utils.safeFileCreate(ISMWebUIProperties.instance().getServerInstallDirectory()+KEYSTORE_DIR, 
                                                     certificate.getName(), true);
        File keystoreKeyFile = Utils.safeFileCreate(ISMWebUIProperties.instance().getServerInstallDirectory()+KEYSTORE_DIR,
                                                    key.getName(), true);

        if (keystoreCertFile == null || keystoreKeyFile == null) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "processCertificatesAndPasswords", "CWLNA5065");
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", null);
        }
        
        // there is a special case where the certificate and key are the same file, they are not pkcs12, and
        // a password is provided. In this case, the file contains both the certificate and key and the normal
        // unencrypt will result in the key be extracted and the certificate being lost.  To handle this
        // case, break in to two and put it back at the end
        boolean certAndKeyPem = false;
        if (certificate.getName().equals(key.getName()) && !isPkcs12(certificate)) {
            String password = certificatePassword != null && certificatePassword.trim().length() > 0 ? certificatePassword : keyPassword;
            if (password != null && password.trim().length() > 0) {
                certAndKeyPem = true;
                key = splitCertAndKey(certificate, password);
                // now we can proceed as usual, it will be like separate cert and key pem files with no passwords
            }
            // don't need passwords any more
            certificatePassword = null;
            keyPassword = null;
        }
        
        try {
            boolean newCertificate = false;
            boolean newKey = false;
            long certificateExpiry = -1;

            if (!certificate.exists()) {
                File certInKeystore = Utils.safeFileCreate(
                               ISMWebUIProperties.instance().getServerInstallDirectory() + CertificatesFileUploader.KEYSTORE_DIR,
                               profile.getCertFilename(), true);
                if (strict || certInKeystore == null || !certInKeystore.exists()) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "uploadFiles",
                            VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(),
                            new Object[] { "certificate" });
                    throw new IsmRuntimeException(Status.BAD_REQUEST,
                            VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(),
                            new Object[] { "certificate" });
                } else {
                    certificate = certInKeystore;
                }
            } else {
                newCertificate = true;
                // verify we can remove any passphrases from them
                if (certificatePassword != null && certificatePassword.trim().length() > 0) {
                    certificateExpiry = unencrypt(certificate, certificatePassword, false);
                }
            }
            // verify we can get the end dates
            if (certificateExpiry < 0) {
                profile.setCertificateExpiry(getCertificateExpiry(certificate));
            } else profile.setCertificateExpiry(certificateExpiry);

            if (!key.exists()) {
                File keyInKeystore = Utils.safeFileCreate(
                            ISMWebUIProperties.instance().getServerInstallDirectory() + CertificatesFileUploader.KEYSTORE_DIR,
                            profile.getKeyFilename(), true);
                if (strict || keyInKeystore == null ||  !keyInKeystore.exists()) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "uploadFiles",
                            VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(), new Object[] { "key" });
                    throw new IsmRuntimeException(Status.BAD_REQUEST,
                            VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(), new Object[] { "key" });
                } else {
                    key = keyInKeystore;
                }
            } else {
                newKey = true;
                if (keyPassword != null && keyPassword.trim().length() > 0) {
                    unencrypt(key, keyPassword, true);
                } else {
                    // verify we don't need a password
                    checkKey(key);
                }
            }
            
//            // verify the key matches the certificate
//            checkKeyMatchesCert(certificate, key);

            /*
             * verify it's not expired if (profile.getCertificateExpiry() <= System.currentTimeMillis()) { throw new
             * IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5020", new Object[] {profile.getCertFilename()}); }
             */

            FileInputStream in = null;
            try {
                if (certAndKeyPem) {
                    // time to put them back together
                    File mergedFile = mergeKeyAndCert(certificate, key);
                    in = new FileInputStream(mergedFile);
                    File keystore = new File(ISMWebUIProperties.instance().getServerInstallDirectory() + KEYSTORE_DIR);
                    if (!keystore.exists()) {
                        keystore.mkdirs();
                    }
                    writeToFile(in, keystoreCertFile);
                    in.close();
                    mergedFile.delete();
                    if (certificate.exists()) {                                                                    
                        certificate.delete();
                    }
                    if (key.exists()) {
                        key.delete();
                    }
                }
                else {
                    if (newCertificate) {
                        in = new FileInputStream(certificate);
                        File keystore = new File(
                                    ISMWebUIProperties.instance().getServerInstallDirectory() + KEYSTORE_DIR);
                        if (!keystore.exists()) {
                            keystore.mkdirs();
                        }
                        writeToFile(in, keystoreCertFile);
                        in.close();
                        certificate.delete();
                    }
                    if (newKey && key.exists()) {
                        in = new FileInputStream(key);
                        writeToFile(in, keystoreKeyFile);
                        in.close();
                        key.delete();
                    }
                }
            } catch (IOException e) {
                if (in != null) {
                    try {
                        in.close();
                    } catch (IOException e1) {
                        logger.log(LogLevel.LOG_WARNING, CLAS, "processCertificatesAndPasswords", "CWLNA5065", e1);
                    }
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5065", null);
            }
        } finally {
            File parsedPem = getParsedFile(certificate);
            if (parsedPem != null && parsedPem.exists()) {
                parsedPem.delete();
            }
            parsedPem = getParsedFile(key);
            if (parsedPem != null && parsedPem.exists()) {
                parsedPem.delete();
            }
        }
        logger.traceExit(CLAS, "uploadFiles", profile);
    }

    /**
     * Certificate and key are in a single passphrase protected PEM file
     * Split them and put the certificate in certifcate and return key
     * @param certificate the merged file
     * @param password the password
     * @return the key file (certificate keeps orginal filename
     */
    private File splitCertAndKey(File certificate, String password) {
       
        String keyFilename = certificate.getName()+PARSED_PEM_SUFFIX;
        File key = Utils.safeFileCreate(TEMP_DIR, keyFilename, true);
        int rc = 0;
        String[] output = new String[2];
        
        if (password == null) {
            password = "";
        }
        
        try {
            // extract the key: /usr/bin/openssl rsa -in {0} -out {1} -passin pass:{2};
            String[] command = {OPENSSL_COMMAND, RSA_ARG, IN_ARG, certificate.getCanonicalPath(), 
                    OUT_ARG, key.getCanonicalPath(), PASSIN_ARG, PASS_PREFIX+password};

            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                Object[] params = new Object[] { certificate.getName(), output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "splitCertAndKey", "CWLNA5018", params);
            }
            logger.trace(CLAS, "splitCertAndKey", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
            
            // extract the cert: /usr/bin/openssl x509 -in {0} -out {0} 
            String[] command2 = {OPENSSL_COMMAND, X509_ARG, IN_ARG, certificate.getCanonicalPath(), 
                    OUT_ARG, certificate.getCanonicalPath()};

            process = runtime.exec(command2);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                // try with the password
                String[] command3 = {OPENSSL_COMMAND, X509_ARG, IN_ARG, certificate.getCanonicalPath(), 
                        OUT_ARG, certificate.getCanonicalPath(), PASSIN_ARG, PASS_PREFIX+password};
                process = runtime.exec(command3);
                rc = Utils.getUtils().getOutput(process, output);
                if (rc != 0) {                
                    Object[] params = new Object[] { certificate.getName(), output[Utils.ERR_MESSAGE] };
                    logger.log(LogLevel.LOG_ERR, CLAS, "splitCertAndKey", "CWLNA5018", params);
                }
            }
            logger.trace(CLAS, "splitCertAndKey", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "splitCertAndKey", "CWLNA5011", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }      
        
        return key;
    }

    /**
     * Undo the splitCertAndKey
     * @param certificate the certificate with original filename
     * @param key the temporary key file
     * @return the merged file
     * @throws IOException 
     */
    private File mergeKeyAndCert(File certificate, File key) throws IOException {
        InputStream in = null;
        OutputStream out = null;
        try {
            in = new FileInputStream(certificate);
            out = new FileOutputStream(key, true);
            byte[] buf = new byte[1024];
            int len;
            while ((len = in.read(buf)) > 0){
                out.write(buf, 0, len);
            }
        } finally {
            if (in != null) {
                in.close();
            }
            if (out != null) {
                out.close();
            }
        }
        return key;
    }
    
    /**
     * @param certificateFilename PEM certificate filename to process, assumed in TEMP_DIR
     * @param keystoreDir keystore directory. If the directory doesn't exist, it is created
     */
    public void processCertificateWithNoKey(String certificateFilename, String keystoreDir, boolean deleteCert) {
        processCertificateWithNoKey(certificateFilename, certificateFilename, keystoreDir, deleteCert);
    }
    
    /**
     * @param certificateFilename PEM certificate filename to process, assumed in TEMP_DIR
     * @param targetFilename filename for the certificate when placed in the new directory
     * @param keystoreDir keystore directory. If the directory doesn't exist, it is created
     */  
    public void processCertificateWithNoKey(String certificateFilename, String targetFilename, String keystoreDir, boolean deleteCert) {
        logger.traceEntry(CLAS, "processCertificateWithNoKey");

        File certificate = Utils.safeFileCreate(CertificatesFileUploader.TEMP_DIR, certificateFilename, true);
        File keystoreCertificate = Utils.safeFileCreate(keystoreDir, targetFilename, true);
        if (certificate == null || keystoreCertificate == null) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "processCertificateWithNoKey", "CWLNA5065");
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5065", null);
        }

        if (keystoreDir == null) {
            logger.trace(CLAS, "processCertificateWithNoKey", "No keystoreDir specified");
            logger.log(LogLevel.LOG_ERR, CLAS, "processCertificateWithNoKey", "CWLNA5009");            
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5009", null);
        }

        try {
            if (!certificate.exists()) {
                logger.log(LogLevel.LOG_WARNING, CLAS, "processCertificateWithNoKey",
                        VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(), new Object[] { "certificate" });
                throw new IsmRuntimeException(Status.BAD_REQUEST,
                        VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(), new Object[] { "certificate" });
            }
            
            // validate the certificate is PEM format
            verifyPem(certificate);
            
            FileInputStream in = null;
            try {
                in = new FileInputStream(certificate);
                File keystore = new File(keystoreDir);
                if (!keystore.exists()) {
                    keystore.mkdirs();
                }
                writeToFile(in, keystoreCertificate);
                in.close();

                if (deleteCert) {
                    certificate.delete();
                }
            } catch (IOException e) {
                if (in != null) {
                    try {
                        in.close();
                    } catch (IOException e1) {
                        logger.log(LogLevel.LOG_WARNING, CLAS, "processCertificateWithNoKey", "CWLNA5065", e1);
                    }
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5065", null);
            }
        } finally {
            File parsedPem = getParsedFile(certificate);
            if (parsedPem != null && parsedPem.exists()) {
                parsedPem.delete();
            }
        }
        logger.traceExit(CLAS, "processCertificateWithNoKey", certificateFilename);
    }

    public static void verifyPem(File certificate) {
        logger.traceEntry(CLAS, "verifyPem");

        String[] output = new String[2];
        int rc = -1;

        try {
            // /usr/bin/openssl x509 -in {0} -noout
            String[] command = { OPENSSL_COMMAND, X509_ARG, IN_ARG, certificate.getCanonicalPath(), NOOUT_ARG };
            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                Object[] params = new Object[] { certificate.getName(), output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "verifyPem", "CWLNA5020", params);
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5080", new Object[] { certificate.getName() });
            }
            logger.trace(CLAS, "verifyPem", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "verifyPem", "CWLNA5011", e);

            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
        logger.traceExit(CLAS, "verifyPem");
    }

    /**
     * Check the specified certificate to validate it is Pkcs12 and the password
     * is correct
     * 
     * @param uploadedCert
     * @param certificatePassword
     * @return true if valid, false otherwise
     */
    private boolean verifyPkcs12(File certificate, String certificatePassword) {
        logger.traceEntry(CLAS, "verifyPkcs12");

        String[] output = new String[2];
        int rc = -1;

        if (certificatePassword == null) {
            certificatePassword = "";
        }

        try {
            // /usr/bin/openssl pkcs12 -info -in {0} -passin pass:{1} -noout
            String[] command = {OPENSSL_COMMAND, PKCS12_ARG, INFO_ARG, IN_ARG, certificate.getCanonicalPath(), 
                     PASSIN_ARG, PASS_PREFIX+certificatePassword, NOOUT_ARG};

            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            logger.trace(CLAS, "verifyPkcs12", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
            if (rc != 0) {
                Object[] params = new Object[] { certificate.getName(), output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "verifyPkcs12", "CWLNA5020", params);
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5020", params);
            } else {
                return true;
            }
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "verifyPkcs12", "CWLNA5011", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
    }

    private static File getParsedFile(File uploadedFile) {
        if (uploadedFile == null) {
            return null;
        }
        return Utils.safeFileCreate(TEMP_DIR, uploadedFile.getName() + PARSED_PEM_SUFFIX, true);
    }

    /**
     * Attempts to unencrypt the certificate using the specified password
     * 
     * @param uploadedFile
     * @param password
     * @param isKey
     * @return -1 unless we had to get the certificate expiry to verify. If we did, returns it.
     */
    private static long unencrypt(File uploadedFile, String password, boolean isKey) {
        logger.traceEntry(CLAS, "unencrypt");

        long certificateExpiry = -1;

        if (isPkcs12(uploadedFile)) {
            return unencryptPkcs12(uploadedFile, password);
        }

        String[] output = new String[2];
        int rc = -1;

        try {
            // /usr/bin/openssl rsa -in {0} -out {0} -passin pass:{1};
            String[] command = {OPENSSL_COMMAND, RSA_ARG, IN_ARG, uploadedFile.getCanonicalPath(), 
                    OUT_ARG, uploadedFile.getCanonicalPath(), PASSIN_ARG, PASS_PREFIX+password};

            // System.out.println("about to run command " + command);
            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                Object[] params = new Object[] { uploadedFile.getName(), output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "unencrypt", "CWLNA5018", params);

                // if the file is not encrypted, or the file is not a valid certificate or key, the error message
                // will start with "unable to load Private Key"

                // make sure the file was actually encrypted...
                if (!isKey) {
                    // if its a certificate, we can test this by checking the expiration date
                    certificateExpiry = getCertificateExpiry(uploadedFile);
                } else {
                    // otherwise, we need to run a check of the key
                    checkKey(uploadedFile);
                }
            }
            logger.trace(CLAS, "unencrypt", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);

        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {

            logger.log(LogLevel.LOG_ERR, CLAS, "unencrypt", "CWLNA5011", e);

            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
        logger.traceExit(CLAS, "unencrypt");

        return certificateExpiry;
    }

    private static String getSanitizedMessage(String message) {
        if (message == null) {
            return message;
        }
        // openSSL returns a text message followed by 0x0A, followed by stuff we don't
        // want the user to see
        int index = message.indexOf(0x0A);
        if (index > 0) {
            message = message.substring(0, index);
        }

        return message;

    }

    private static void checkKey(File uploadedFile) {
        logger.traceEntry(CLAS, "checkKey");

        if (isPkcs12(uploadedFile)) {
            return;
        }

        String[] output = new String[2];
        int rc = -1;

        try {
            // /usr/bin/openssl rsa -in {0} -passin pass: -check -noout
            String[] command = { OPENSSL_COMMAND, RSA_ARG, IN_ARG, uploadedFile.getCanonicalPath(), PASSIN_ARG, PASS_PREFIX, CHECK_ARG, NOOUT_ARG };
            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                Object[] params = new Object[] { uploadedFile.getName(), output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "checkKey", "CWLNA5020", params);
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5020", new Object[] { uploadedFile.getName(),
                        getSanitizedMessage(output[Utils.ERR_MESSAGE]) });
            }
            logger.trace(CLAS, "checkKey", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "checkKey", "CWLNA5011", e);

            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
        logger.traceExit(CLAS, "checkKey");

    }

    private static void checkKeyMatchesCert(File certificate, File key) {
        logger.traceEntry(CLAS, "checkKey");

        String[] output = new String[2];
        int rc = -1;

        try {

            if (certificate.getCanonicalPath().equals(key.getCanonicalPath()) && isPkcs12(certificate)) {
                return;
            }

            String[] command =  {ISMWebUIProperties.instance().getWebUIInstallDirectory() + CHECK_KEY_MATCHES_CERT_COMMAND, 
                                       certificate.getCanonicalPath(), key.getCanonicalPath()};
            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                logger.log(LogLevel.LOG_ERR, CLAS, "checkKeyMatchesCert", "CWLNA5077");
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5077", null);
            }
            logger.trace(CLAS, "checkKeyMatchesCert", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "checkKey", "CWLNA5011", e);

            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
        logger.traceExit(CLAS, "checkKeyMatchesCert");

    }

    private static long unencryptPkcs12(File uploadedFile, String password) {
        logger.traceEntry(CLAS, "unencryptPkcs12");

        long certificateExpiry = -1;

        String[] output = new String[2];
        int rc = -1;
        parsePkcs12File(uploadedFile, password);

        try {
            String original = uploadedFile.getCanonicalPath();
            File pemFile = getParsedFile(uploadedFile);
            String pem = pemFile.getCanonicalPath();

            // /usr/bin/openssl pkcs12 -export -in {0} -out {1} -passout pass:
            String[] command = {OPENSSL_COMMAND, PKCS12_ARG, EXPORT_ARG, IN_ARG, pem, OUT_ARG, original, PASSOUT_ARG, PASS_PREFIX};

            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            logger.trace(CLAS, "unencryptPkcs12", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
            Object[] params = new Object[] { uploadedFile.getName(), output[Utils.ERR_MESSAGE] };
            if (rc != 0) {
                logger.log(LogLevel.LOG_ERR, CLAS, "unencryptPkcs12", "CWLNA5018", params);
                // make sure the file was actually encrypted...
                // we can test this by checking the expiration date
            }
            certificateExpiry = getCertificateExpiry(uploadedFile);
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "unencryptPkcs12", "CWLNA5011", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
        logger.traceExit(CLAS, "unencryptPkcs12");
        return certificateExpiry;
    }

    private static boolean isPkcs12(File uploadedFile) {

        if (uploadedFile == null) {
            return false;
        }

        String name = uploadedFile.getName().toLowerCase();
        if (name.endsWith(".p12") || name.endsWith(".pfx")) {
            return true;
        }

        return false;
    }

    private static void parsePkcs12File(File pkcs12File, String password) {
        try {
            String original = pkcs12File.getCanonicalPath();
            File pemFile = getParsedFile(pkcs12File);
            String pem = pemFile.getCanonicalPath();
            
            if (password == null) {
                password = "";
            }

            // /usr/bin/openssl pkcs12 -in {0} -nodes -out {1} -passin pass:{2} -passout pass:
            String[] command = {OPENSSL_COMMAND, PKCS12_ARG, IN_ARG, original, NODES_ARG,
                    OUT_ARG, pem, PASSIN_ARG, PASS_PREFIX+password, PASSOUT_ARG, PASS_PREFIX};
            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            String[] output = new String[2];
            int rc = Utils.getUtils().getOutput(process, output);
            logger.trace(CLAS, "parsePkcs12File", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);
            if (rc != 0 && password.length() > 0) {
                // if it fails, try with a blank password. we may have already removed
                // the password in an earlier step.
                String[] command2 = {OPENSSL_COMMAND, PKCS12_ARG, IN_ARG, original, NODES_ARG,
                        OUT_ARG, pem, PASSIN_ARG, PASS_PREFIX, PASSOUT_ARG, PASS_PREFIX};

                process = runtime.exec(command2);
                output[0] = "";
                output[1] = "";
                rc = Utils.getUtils().getOutput(process, output);
                if (rc != 0) {
                    Object[] params = new Object[] { pkcs12File.getName(), output[Utils.ERR_MESSAGE] };
                    logger.log(LogLevel.LOG_ERR, CLAS, "parsePkcs12File", "CWLNA5018", params);
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5018", new Object[] { pkcs12File.getName(),
                            getSanitizedMessage(output[Utils.ERR_MESSAGE]) });
                }
            }
        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "parsePkcs12File", "CWLNA5011", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5011", null);
        }
    }

    /**
     * Verifies the specified certificate is in the temp directory, ready for upload.
     * 
     * @param name certificate to check
     * @return true if it's in the temporary directory, false otherwise
     */
    public final static boolean checkCertificateExistsInTempDirectory(String name) {
        File cert = Utils.safeFileCreate(TEMP_DIR, name, true);
        if (cert != null && cert.exists() && cert.isFile()) {
            return true;
        }

        return false;
    }

    protected static void setCertificateExpiry(CertificateProfile profile) {
        File cert = Utils.safeFileCreate(
                           ISMWebUIProperties.instance().getServerInstallDirectory() + KEYSTORE_DIR,
                           profile.getCertFilename(), true);   
        profile.setCertificateExpiry(getCertificateExpiry(cert));
    }

    /**
     * Gets the expiry date for the specified certificate name. The certificate must be in the keystore.
     * 
     * @param name The name of the certificate
     * @return The date the certificate expires or null if it could not be determined
     */
    protected static long getCertificateExpiry(File cert) {
        logger.traceEntry(CLAS, "getCertificateExpiry");
        if (cert == null) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getCertificateExpiry", "CWLNA5012", new Object[] { "name" });
            return 0;
        }

        Date expiryDate = null;

        // openSSL returns dates like this: "notAfter=Jun  8 18:25:20 2013 GMT" or "notAfter=Jun 18 18:25:20 2013 GMT"

        String[] output = new String[2];
        int rc = -1;

        try {
            String filename = cert.getCanonicalPath();

            if (isPkcs12(cert)) {
                File parsedFile = getParsedFile(cert);
                if (!parsedFile.exists()) {
                    parsePkcs12File(cert, "");
                }
                filename = parsedFile.getCanonicalPath();
            }

            // /usr/bin/openssl x509 -in {0} -noout -enddate
            String[] command = { OPENSSL_COMMAND, X509_ARG, IN_ARG, filename, NOOUT_ARG, ENDDATE_ARG};
            Runtime runtime = Runtime.getRuntime();
            Process process = runtime.exec(command);
            rc = Utils.getUtils().getOutput(process, output);
            if (rc != 0) {
                Object[] params = new Object[] { cert.getName(), output[Utils.ERR_MESSAGE] };
                logger.log(LogLevel.LOG_ERR, CLAS, "getCertificateExpiry", "CWLNA5019", params);
                logger.trace(CLAS, "getCertificateExpiry", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]);
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5019", new Object[] { cert.getName(),
                        getSanitizedMessage(output[Utils.ERR_MESSAGE]) });
            } else {
                String dateStr = output[Utils.OUT_MESSAGE];
                SimpleDateFormat format;
                if (dateStr.charAt(NOT_AFTER_FIRST_DAY_INDEX) == ' ') {
                    format = new SimpleDateFormat(NOT_AFTER_FORMAT_STRING_1, Locale.ENGLISH);
                } else {
                    format = new SimpleDateFormat(NOT_AFTER_FORMAT_STRING_2, Locale.ENGLISH);
                }

                logger.trace(CLAS, "getCertificateExpiry", "using format " + format.toPattern() + " for dateStr "
                        + dateStr + " with length " + dateStr.length());
                expiryDate = format.parse(dateStr);
            }
            logger.trace(CLAS, "getCertificateExpiry", "RC = " + rc + "; OUT_MESSAGE: " + output[Utils.OUT_MESSAGE]
                    + "; ERR_MESSAGE: " + output[Utils.ERR_MESSAGE]);

        } catch (IsmRuntimeException ire) {
            throw ire;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getCertificateExpiry", "CWLNA5011", e);
        }
        if (expiryDate == null) {
            return 0;
        }
        logger.traceExit(CLAS, "getCertificateExpiry");
        return expiryDate.getTime();
    }

    /**
     * Delete the specified certificate and key from the keystore
     * 
     * @param filename name of file to delete
     * @param keystoreDir keystore directory to delete it from
     * 
     */
    public static void deleteFileFromStore(String filename, String keystoreDir) {
        File file = Utils.safeFileCreate(keystoreDir, filename, true);
        if (file != null) {
            if (keystoreDir == null) {
                logger.trace(CLAS, "deleteFileFromStore", "No keystoreDir specified");
                logger.log(LogLevel.LOG_ERR, CLAS, "deleteFileFromStore", "CWLNA5011");            
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5011", null);
            }

            File parsedFile = getParsedFile(file);
            if (parsedFile != null && parsedFile.exists()) {
                parsedFile.delete();
            }
            if (file.exists()) {
                file.delete();
            }
        }
    }
}
