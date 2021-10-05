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
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;

import com.ibm.websphere.jaxrs20.multipart.IMultipartBody;
import com.ibm.websphere.jaxrs20.multipart.IAttachment;

import com.ibm.ima.resources.AbstractFileUploader;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.VALIDATION_RESULT;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

/**
 * BaseUploader class that supports generic file operations on the applicance
 */

public class BaseUploader extends AbstractFileUploader {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = BaseUploader.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    public final static String TEMP_DIR = "/tmp";

    public BaseUploader(HttpServletRequest currentRequest) {
        super(currentRequest);
    }

    /**
     * Uploads a file to TMP.
     */
    public String uploadFilesToTemp(UriInfo uriInfo, IMultipartBody mpMessage, Locale locale) {
        logger.traceEntry(CLAS, "uploadFilesToTemp");

        String result = "";

        List<String> files = new ArrayList<String>();

        File uploadedFile = null;

        List <IAttachment> attachments = mpMessage.getAllAttachments();

        try {
            for (IAttachment attachment : attachments) {
                uploadedFile = super.uploadFileFromPart(attachment, BaseUploader.TEMP_DIR);
                if (uploadedFile != null) {
                    files.add("tmp:" + uploadedFile.getName());
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
     * Moves a file from the temp directory to a specified directory.
     * 
     * @param filename The file to move.
     * @param destDirectory The destination directory to place the file.
     * @param destFilename The filename for the file in the new location.
     * @param deleteTempFile Flag to indicate whether to delete the file in the temp directory.
     */
    public static void moveFileFromTemp(String filename, String destDirectory, String destFilename, boolean deleteTempFile) {
        logger.traceEntry(CLAS, "moveFileFromTemp");

        moveFile(TEMP_DIR, filename, destDirectory, destFilename, deleteTempFile);
        logger.traceExit(CLAS, "moveFileFromTemp", filename);
    }

    /**
     * Moves a file from the specified directory to a specified destination directory.
     * 
     * @param filename The file to move.
     * @param destDirectory The destination directory to place the file.
     * @param destFilename The filename for the file in the new location.
     * @param deleteTempFile Flag to indicate whether to delete the file in the temp directory.
     * @return the destination file
     */
	public static File moveFile(String directory, String filename, String destDirectory,
			String destFilename, boolean deleteSource) {
		
		File sourceFile = Utils.safeFileCreate(directory, filename, true);
        File destinationFile = Utils.safeFileCreate(destDirectory, destFilename, true);

        if (sourceFile == null || !sourceFile.exists() || destinationFile == null) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "moveFile",
                    VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(), new Object[] { "filename" });
            throw new IsmRuntimeException(Status.BAD_REQUEST,
                    VALIDATION_RESULT.REFERENCED_OBJECT_NOT_FOUND.getMessageID(), new Object[] { "filename" });
        }

        FileInputStream in = null;
        try {
            in = new FileInputStream(sourceFile);
            writeToFile(in, destinationFile);
            in.close();
            if (deleteSource) {
                sourceFile.delete();
            }
        } catch (IOException e) {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e1) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "moveFile", "CWLNA5065", e1);
                }
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5065", null);
        }
        
        return destinationFile;
	}

    /**
     * Verifies the specified file is in the temp directory, ready for upload.
     * 
     * @param name file to check
     * @return true if it's in the temporary directory, false otherwise
     */
    public final static boolean checkFileExistsInTempDirectory(String name) {
        return checkFileExistsDirectory(TEMP_DIR, name);
    }

    /**
     * Verifies the specified file is in the specified directory, ready for upload.
     * 
     * @param name file to check
     * @return true if it's in the temporary directory, false otherwise
     */
    public final static boolean checkFileExistsDirectory(String directory, String name) {
        File file = Utils.safeFileCreate(directory, name, true);
        if (file != null && file.exists() && file.isFile()) {
            return true;
        }

        return false;
    }

    /**
     * Delete the specified file from the provided directory
     * 
     * @param directory
     * @param filename
     */
    public static boolean deleteFileFromDirectory(String directory, String filename) {
        File file = Utils.safeFileCreate(directory, filename, true);
        if (file != null && file.exists()) {
            return file.delete();
        }
        return false;
    }

    /**
     * Create a backup of the specified file
     * @param sourceDirectory
     * @param sourceFilename
     * @return
     */
	public static File backupFile(String sourceDirectory, String sourceFilename) {
        File source = Utils.safeFileCreate(sourceDirectory, sourceFilename, true);
        File destination = null;
        if (source != null && source.exists()) {
        	String destinationFilename = Utils.getUniqueName(TEMP_DIR, sourceFilename);
        	destination = moveFile(sourceDirectory, sourceFilename, TEMP_DIR, destinationFilename, false);        	
        }
		return destination;
	}

	/**
	 * Roll back the specified file
	 * @param backupFile
	 * @param originalDirectory
	 * @param originalFilename
	 */
	public static void rollbackFile(File backupFile, String originalDirectory, String originalFilename) {
		if (backupFile == null || !backupFile.exists() || !backupFile.isFile() || originalDirectory == null || originalFilename == null) {
			return;
		}
		
		moveFile(backupFile.getParent(), backupFile.getName(), originalDirectory, originalFilename, true);
	}

}
