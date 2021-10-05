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

package com.ibm.ima.resources.data.utils;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Iterator;

import com.ibm.ima.ISMWebUIProperties;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.ObjectMapper;

/*
 * This class provides methods for reading/writing User preferences to
 * a flat file as JSON.
 */
public class PreferencesManager {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = PreferencesManager.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    private static final String USER_KEY = "user";
    private static final String GLOBAL_KEY = "global";

    private static PreferencesManager singleton;
    private static Object SYNC_OBJECT = new Object();  // object used to synchronize access to preferences


    private String userPrefsPath;        // path and filename of the user preferences json file
    private File userPrefsFile;          // the user preferences json file
    private ObjectNode userPreferences;  // in memory representation of user preferences

    /**
     * Get an instance of UserPreferences
     * @return
     */
    public static synchronized PreferencesManager getInstance() {
        if (singleton == null) {
            singleton = new PreferencesManager();
        }
        return singleton;
    }

    /**
     * Get the preferences for the specified user.
     * @param userId  the user id to get preferences for. If null, only global preferences are returned
     * @return the preferences for the user, including global preferences
     */
    public JsonNode getPreferences(String userId) {

        ObjectMapper objectMapper = new ObjectMapper();
    	ObjectNode obj = objectMapper.createObjectNode();

        JsonNode global = null;
        JsonNode user = null;

        // enter a synchronized block to get the current preferences
        synchronized (SYNC_OBJECT) {
            global = userPreferences.get(GLOBAL_KEY);
            if (userId != null) {
                user = userPreferences.get(userId);
            }
        }

        if (global != null) {
            obj.put(GLOBAL_KEY, global);
        }
        if (user != null) {
            obj.put(USER_KEY, user);
        }

        return obj;
    }

    /**
     * Sets the preferences for the specified user. Preferences
     * are added to any existing preferences already set for this user.
     * If global preferences are included, they are updated for all users.
     * @param userId if null, only global preferences are updated
     * @param preferences if null, preferences for the specified user are removed
     */
    public void setPreferences(String userId, JsonNode preferences) {
        if (preferences == null) {
            if (userId != null) {
                synchronized (SYNC_OBJECT) {
                    userPreferences.remove(userId);
                }
            }
        } else {

            Object global = preferences.get(GLOBAL_KEY);
            Object user = preferences.get(USER_KEY);

            if (global == null && user == null) {
                logger.trace(CLAS, "setPreferences", "Found no preferences to set");
                return;
            }

            JsonNode jGlobal = null;
            JsonNode jUser = null;

            if (global instanceof JsonNode) {
                jGlobal = (JsonNode)global;
            } else if (global != null){
                logger.trace(CLAS, "setPreferences", "global settings object is not a JsonNode");
            }

            if (user instanceof JsonNode) {
                jUser = (JsonNode)user;
            } else if (user != null){
                logger.trace(CLAS, "setPreferences", "user settings object is not a JsonNode");
            }

            synchronized (SYNC_OBJECT) {
                // update global preferences
                if (jGlobal != null && !jGlobal.isEmpty()) {
                    ObjectNode currentGlobal = (ObjectNode)userPreferences.get(GLOBAL_KEY);
                    if (currentGlobal == null || currentGlobal.isEmpty()) {
                        userPreferences.put(GLOBAL_KEY, jGlobal);
                    } else {
                        Iterator<String> it = jGlobal.fieldNames();
                        while(it.hasNext()){
                            String key = it.next();
                            JsonNode value = jGlobal.get(key);
                            currentGlobal.put(key, value);
                        }
                    }
                }
                // update user preferences
                if (userId != null && jUser != null && !jUser.isEmpty()) {
                    ObjectNode currentUser = (ObjectNode)userPreferences.get(userId);
                    if (currentUser == null || currentUser.isEmpty()) {
                        userPreferences.put(userId, jUser);
                    } else {
                        Iterator<String> it = jUser.fieldNames();
                        while(it.hasNext()){
                            String key = it.next();
                            JsonNode value = jUser.get(key);
                            currentUser.put(key, value);
                        }
                    }
                }
            } // end synchronized block

            try {
                saveUserPreferences();
            } catch (IOException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "setPreferences", "CWLNA5000", e);

            }
        }
    }

    private PreferencesManager() {
        userPrefsPath = ISMWebUIProperties.instance().getDataDir() + "userPreferences.json";
        userPrefsFile = new File(userPrefsPath);

        if (!userPrefsFile.exists()) {
            ObjectMapper objectMapper = new ObjectMapper();
            userPreferences = objectMapper.createObjectNode();
        } else {
            try {
                loadUserPreferences();
            } catch (IOException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "ctor", "CWLNA5000", e);
                ObjectMapper objectMapper = new ObjectMapper();
                userPreferences = objectMapper.createObjectNode();
            }
        }
    }

    private void loadUserPreferences() throws IOException {
        FileReader reader = null;
        try {
        	ObjectMapper objectMapper = new ObjectMapper();
            reader = new FileReader(userPrefsFile);
            userPreferences = objectMapper.readValue(reader, ObjectNode.class);
            reader.close();
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
    }

    private synchronized void saveUserPreferences() throws IOException {
        FileWriter writer = null;
        try {
            writer = new FileWriter(userPrefsFile, false);
            ObjectMapper objectMapper = new ObjectMapper();
            objectMapper.writeValue(writer, userPreferences);
        } finally {
            if (writer != null) {
                writer.close();
            }
        }


    }



}
