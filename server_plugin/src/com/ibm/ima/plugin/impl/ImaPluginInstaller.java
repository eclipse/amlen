/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.plugin.impl;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.FileVisitOption;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.StandardCopyOption;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;

import com.ibm.ima.plugin.util.ImaJson;

public class ImaPluginInstaller {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    static final Path USERFILES_DIR;
    static final String CONFIG_DIR;

    static {
        Map<String, String> env = System.getenv();
        if (env.containsKey("IMA_CONFIG_DIR")) {
            CONFIG_DIR = env.get("IMA_CONFIG_DIR");
        } else {
            CONFIG_DIR = "IMA_SVR_INSTALL_PATH/config/";
        }

		if (env.containsKey("IMA_USERFILES_DIR")) {
            USERFILES_DIR = Paths.get(env.get("IMA_USERFILES_DIR"));
        } else {
            USERFILES_DIR =  Paths.get("IMA_SVR_DATA_PATH/userfiles/");
        }
    }

    static final Path STAGING_INSTALL_DIR = Paths.get(CONFIG_DIR + "plugin/staging/install/");
    static final Path STAGING_UNINSTALL_DIR = Paths.get(CONFIG_DIR + "plugin/staging/uninstall/");
    static final Path PLUGINS_DIR = Paths.get(CONFIG_DIR + "plugin/plugins");
	static final String PLUGIN_CONFIG_NAME = "plugin.json";
	static final String PLUGIN_PROPS_FILE_NAME = "pluginproperties.json";
	static final int CAPABILITY_USETOPIC     = 0x01;
    static final int CAPABILITY_USESHARED    = 0x02;
    static final int CAPABILITY_USEQUEUE     = 0x04;
    static final int CAPABILITY_USEBROWSE    = 0x08;
	
    static final Path PLUGIN_TMP_FOLDER = Paths.get("/tmp/plugin");
    static final ImaPluginTraceImpl trace = ImaTrace.init("stderr", 9, false);
	static final HashSet<String> allowedStringProps = new HashSet<String>(32);
	static final HashSet<String> allowedIntProps = new HashSet<String>(32);
	static final HashSet<String> allowedBooleanProps = new HashSet<String>(32);
	static final HashSet<String> allowedArrayProps = new HashSet<String>(32);
	static final HashSet<String> allowedObjectProps = new HashSet<String>(32);
	static {
		allowedStringProps.add("Name");
		allowedStringProps.add("Class");
		allowedStringProps.add("Method");
		allowedStringProps.add("Protocol");
		allowedStringProps.add("Author");
		allowedStringProps.add("Version");
		allowedStringProps.add("Copyright");
		allowedStringProps.add("Build");
		allowedStringProps.add("Description");
		allowedStringProps.add("License");
		allowedStringProps.add("Title");
		allowedStringProps.add("Alias");
		allowedIntProps.add("Modification");
		allowedBooleanProps.add("UseQueue");
		allowedBooleanProps.add("UseTopic");
		allowedBooleanProps.add("UseBrowse");
		allowedBooleanProps.add("UseShared");
		allowedObjectProps.add("Properties");
		allowedArrayProps.add("Classpath");
		allowedArrayProps.add("WebSocket");
		allowedArrayProps.add("HttpHeader");
		allowedArrayProps.add("InitialByte");
	}
	
	private static Map<String, Object> parseConfigProperties(ImaJson jsConfig, int startIndex, int count) {
		Map<String, Object> result = new HashMap<String, Object>(count);
		for(int index = 0; index < count; index++) {
			ImaJson.Entry entry = jsConfig.getEntry(index+startIndex);
			switch(entry.objtype) {
			case JsonString:
				result.put(entry.name, entry.value);
				break;
			case JsonInteger:
				result.put(entry.name, new Integer(entry.count));
				break;
			case JsonNumber:
				result.put(entry.name, new Double(entry.value));
				break;
			case JsonTrue:
				result.put(entry.name, Boolean.TRUE);
				break;
			case JsonFalse:
				result.put(entry.name, Boolean.FALSE);
				break;
			case JsonNull:
				result.put(entry.name, null);
				break;
			case JsonObject:
			case JsonArray:
				/* Ignored for now */
				index += entry.count;
				break;
			}
		}
		return result;
	}
	
    private static Map<String, Object> parseConfig(Path configPath, boolean mustExist) {
        Path configFilePath = configPath.resolve("plugin.json");
        if (!Files.exists(configFilePath)) {
            if (mustExist)
                System.err.println("Configuration file does not exist: " + configFilePath);
			return null;			
		}
		ImaJson jsConfig  = new ImaJson();
		jsConfig.setAllowComments(true);
		try {
			int capability = 0;
            jsConfig.parse(configFilePath.toFile());
			Map<String, Object> config = new HashMap<String, Object>();
			for(int index = 1; index < jsConfig.getEntryCount(); index++) {
				ImaJson.Entry entry = jsConfig.getEntry(index);
				boolean found = false;
				switch(entry.objtype) {
					case JsonString:
						if(allowedStringProps.contains(entry.name)) {
							config.put(entry.name, jsConfig.getValue(index));
							found = true;
						}
					break;
					case JsonInteger:
						if(allowedIntProps.contains(entry.name)) {
							config.put(entry.name, jsConfig.getValue(index));
							found = true;
						}
					break;
					case JsonTrue:
						if("UseQueue".equals(entry.name)) {
							capability |= CAPABILITY_USEQUEUE;
							found = true;
							break;
						}
						if("UseTopic".equals(entry.name)) {
							capability |= CAPABILITY_USETOPIC;
							found = true;
							break;
						}
						if("UseBrowse".equals(entry.name)) {
							capability |= CAPABILITY_USEBROWSE;
							found = true;
							break;
						}
						if("UseShared".equals(entry.name)) {
							capability |= CAPABILITY_USESHARED;
							found = true;
							break;
						}
					break;
					case JsonFalse:
						if(allowedBooleanProps.contains(entry.name)) {
							found = true;
						}
						break;
					case JsonArray:
						if(allowedArrayProps.contains(entry.name)) {
							if(!"InitialByte".equals(entry.name)) {
								for(int j = 0; j < entry.count; j++) {
									ImaJson.Entry arrayEntry = jsConfig.getEntry(index+j+1);
									if(arrayEntry.objtype != ImaJson.JObject.JsonString) {
										System.err.println("The plug-in configuration property is not valid: Property=" + entry.name);
										return null;
									}
									String key = entry.name + '.' + j;
									config.put(key, arrayEntry.value);
								}
							} else {
								if(entry.count > 255) {
									System.err.println("The plug-in \"InitialByte\" configuration property is not valid: Too many array entries");
									return null;
								}
								for(int j = 0; j < entry.count; j++) {
									ImaJson.Entry arrayEntry = jsConfig.getEntry(index+j+1);
									Integer value;
									if(arrayEntry.objtype == ImaJson.JObject.JsonString) {
										value = new Integer(arrayEntry.value.getBytes()[0]);
									} else {
										if(arrayEntry.objtype == ImaJson.JObject.JsonInteger) {
											value = new Integer((arrayEntry.count & 0xff));
										} else {
											System.err.println("The plug-in configuration property is not valid: Property=" + entry.name);
											return null;
										}
									}
									String key = entry.name + '.' + j;
									config.put(key, value);
								}
							}
							found = true;
						}
						index += entry.count;
						break;
					case JsonObject:
						if(allowedObjectProps.contains(entry.name)) {
							Map<String, Object> props = parseConfigProperties(jsConfig, index+1, entry.count);
							config.put(entry.name, props);
							found = true;
						}
						index += entry.count;
						break;
					default:							
						break;
				}
				if(!found)
					System.err.println("The plug-in configuration property is unknown or not valid: Property=" + entry.name);
			}
			config.put("Capabilities", new Integer(capability));
			config.put("ProtocolMask", new Long(0x100000));
            configFilePath = configPath.resolve("pluginproperties.json");
            if (Files.exists(configFilePath)) {
				ImaJson jsProps  = new ImaJson();
				jsProps.setAllowComments(true);
				try {
                    jsProps.parse(configFilePath.toFile());
					if(jsProps.getEntryCount() > 1) {
						Map<String, Object> props = parseConfigProperties(jsProps, 1, (jsProps.getEntryCount() - 1) );
						config.put("Properties", props);						
					}
				} catch (Exception ex) {
					System.err.println("The parsing of plug-in configuration properties has failed.");
					ex.printStackTrace();
					return null;
				}
			}
			return config;
		} catch (Exception ex) {
			System.err.println("The parsing of plug-in configuration has failed.");
			ex.printStackTrace();
			return null;
		}
	}
	
    static boolean validate(Path configPath, boolean createPluginInstance) {
        Map<String, Object> config = parseConfig(configPath, true);
        if (config == null)
            return false;
        return validate(configPath, config, createPluginInstance);
    }

    static boolean validate(Path configPath, Map<String, Object> config, boolean createPluginInstance) {
        int i = 0, j = 0;
        if (!config.containsKey("Name")) {
            System.err.println("A required plug-in property (Name) is not set");
            return false;
        }
        String name = (String) config.get("Name");
        if ((name == null) || (name.isEmpty())) {
            System.err.println("Plugin name property is empty");
            return false;
        }
		do {
			String classPath = (String) config.get("Classpath." + i);
			if(classPath == null)
				break;
			i++;
			String jarPath;
			if(classPath.startsWith(File.separator)) {
				jarPath = classPath;
			} else {
				jarPath = configPath + File.separator + classPath;
			}
			File file = new File(jarPath);
			if(file.exists()) {
				j++;
			} else {
				System.err.println("A specifiled Classpath entry does not exists: " + classPath 
						+ " (" + file.getAbsolutePath() + ')');
			}
		} while(true);
		if(i == 0) {
            System.err.println("A required plug-in property (Classpath) is not set");
			return false; 
		}
		if(j < i)
			return false;
		
		if(!config.containsKey("Class")) {
            System.err.println("A required plug-in property (Class) is not set");
			return false; 
		}
		if(!config.containsKey("Protocol")) {
            System.err.println("A required plug-in property (Protocol) is not set");
			return false; 
		}
        if (createPluginInstance) {
            try {
            	config.put("ValidateConfigFolder", configPath.toString());
                ImaPluginImpl plugin = new ImaPluginImpl(config, null);
                System.err.println("Plugin instance creation validated successfully for plugin " + plugin);
            } catch (Throwable th) {
                System.err.println("Plugin instance creation has failed.");
                th.printStackTrace();
                return false;
            }
		}
		return true;
	}
	
	public static void main(String[] args) {
		if((args.length == 0) || "test".equals(args[0])) {
			/* Validate that java process can be started successfully */
			System.err.println("Java plugin process started successfully.");
			System.exit(0);
		}
        int rc = 255;
        String action = args[0]; 
		try {
            Files.createDirectories(PLUGIN_TMP_FOLDER);
            String dir = System.getenv("PLUGINS_DIR");
            if(dir != null)
            	ImaPluginImpl.PLUGINS_DIR = dir;
            do {
                if(action.equals("Install")) {
                	boolean allowOverwrite = true;
                    String pluginName = null;
                    Path zipFile = null;
                    Path propsFile = null;
                	for(int i = 1; i < args.length; i++) {
                		if(args[i].startsWith("Zip=")) {
                			String pluginZip = args[i].substring("Zip=".length(), args[i].length());
                			if(pluginZip.length() > 0) {
                				zipFile = USERFILES_DIR.resolve(pluginZip);
	                	        if (!Files.exists(zipFile)) {
	                	        	zipFile = null;
	                	        }
                			}
                			continue;
                		}
                		if(args[i].startsWith("allowOverwrite=")) {
                			allowOverwrite = Boolean.parseBoolean(args[i].substring("allowOverwrite=".length(), args[i].length()));
                			continue;
                		}
                		if(args[i].startsWith("propertiesFile=")) {
                        	String propertiesFile = args[i].substring("propertiesFile=".length(), args[i].length());
                        	if(propertiesFile.length() > 0) {
                        		propsFile = USERFILES_DIR.resolve(propertiesFile);
	                            if (!Files.exists(propsFile)) {
	                            	propsFile = null;
	                            }
                        	}

                			continue;
                		}
                        if(args[i].startsWith("Name=")) {
                            pluginName = args[i].substring("Name=".length(), args[i].length());
                            continue;
                        }
                		System.err.println("Unknown parameter for plugin install action: " + args[i]);
                	}
                	if(installPlugin(pluginName, zipFile, propsFile, allowOverwrite)) {
                		rc = 0;
    					if (zipFile != null) {
    						try {
    							Files.delete(zipFile);
    						} catch (Exception e) {
    							// Nothing to do
    						}
    					}
    					if (propsFile != null) {
    						try {
    							Files.delete(propsFile);
    						} catch (Exception e) {
    							// Nothing to do
    						}
    					}
                	}
                	break;
                }
                if(action.equals("Remove")) {
                	String pluginName = null;
                	for(int i = 1; i < args.length; i++) {
                		if(args[i].startsWith("Name=")) {
                			pluginName = args[i].substring("Name=".length(), args[i].length());
                			continue;
                		}
                		System.err.println("Unknown parameter for plugin remove action: " + args[i]);
                	}
                	if(uninstallPlugin(pluginName)) {
                		rc = 0;
                	}
                	break;
                }
                               
    			System.err.println("Unknown action: " + action);
            } while (false);
		} catch (Throwable th) {
			System.err.println("Action " + action + " failed.");
			th.printStackTrace();
		}
        Remover rm = new Remover(PLUGIN_TMP_FOLDER);
        try {
            rm.execute();
        } catch (IOException e) {
        }
        System.exit(rc);
	}

	static boolean uninstallPlugin(String pluginName) {
		if((pluginName == null) || (pluginName.length() == 0)) {
			System.err.println("Plugin name was not specified for uninstall action");
			return false;
		}
		try{
			boolean found = false;
			Path pluginPath = PLUGINS_DIR.resolve(pluginName);
			if(Files.exists(pluginPath)) {
				Path stagingPath = STAGING_UNINSTALL_DIR.resolve(pluginName);
				if(!Files.exists(stagingPath)) {
					Files.createDirectory(stagingPath);
				}
				found = true;
			}
			pluginPath = STAGING_INSTALL_DIR.resolve(pluginName);
			if(Files.exists(pluginPath)) {
				Remover rm = new Remover(pluginPath);
				rm.execute();
                found = true;
			}			
			if(found) {
				System.err.println("Plugin " + pluginName + " was uninstalled successfully");
			} else {
				System.err.println("Plugin " + pluginName + " is not installed");
			}
			return found;
			
		} catch (Throwable th) {
			System.err.println("Uninstall failed for plugin: " + pluginName);
			th.printStackTrace();
		}
		return false;
	}
	
    static boolean installPlugin(String pluginName, Path zipFile, Path propsFile, boolean allowOverwrite) {
    	if(zipFile == null){
    	    if(propsFile == null) { 
    	        System.err.println("Neither new plugin zip nor new properties file exist. Keep existing configuration.");
    	        return true;
    	    } 
    	    return updatePluginProperites(pluginName, propsFile, allowOverwrite);
    	}
        if((pluginName == null) || (pluginName.length() == 0)) {
            System.err.println("Plugin name was not specified for install action");
            return false;
        }
        Path tmpPath = PLUGIN_TMP_FOLDER.resolve("plugin." + System.currentTimeMillis());
        try {
        	validateZipFile(zipFile);
            unzipPluginFile(zipFile.toFile(), tmpPath);
            if (propsFile != null) {
                // Overwrite configuration properties
                Files.copy(propsFile, tmpPath.resolve(PLUGIN_PROPS_FILE_NAME));
            }
            // Parse config
            Map<String, Object> config = parseConfig(tmpPath, true);
            if (config == null) {
                return false;
            }
            if(config.containsKey("Name")) {
                if(!pluginName.equals(config.get("Name"))) {
                    System.err.println("Requested plug-in name is different from the configured name.");
                    return false;
                }
            } else {
                config.put("Name", pluginName);
            }
            // Check for conflicts with existing plugins
            ConfigValidator cv = new ConfigValidator(config, allowOverwrite);
            cv.execute();
            if (cv.getResult()) {
                System.err.println("Configuration conflict with existing plug-in detected.");
                return false;
            }
            // Validate config
            if (!validate(tmpPath, config, true)) {
                System.err.println("Plugin validation failed.");
                return false;
            }
            Path pluginFolder = STAGING_INSTALL_DIR.resolve(pluginName);
            if (Files.exists(pluginFolder)) {
                Remover rm = new Remover(pluginFolder);
                rm.execute();
            }
            Copier cp = new Copier(tmpPath, pluginFolder, true);
            cp.execute();
            System.err.println("Plug-in was installed successfully: " + pluginName);
            return true;
        } catch (Throwable th) {
            String error = "Failed to install plugin using : " + zipFile.toFile().getName();
            if (propsFile != null)
                error += " and " + propsFile.toFile().getName();
            System.err.println(error);
            th.printStackTrace();
        }
        return false;
    }

    static void validateZipFile(Path pluginZip) throws IOException {
    	try {
    	 ZipFile zipfile = new ZipFile(pluginZip.toFile()); 
    	 zipfile.close();
    	} catch (IOException ex) {
    		System.err.println("Plugin zip file is corrupted");
    		throw ex;
    	}
    }
    
    static void unzipPluginFile(File pluginZip, Path pluginFolder) throws IOException {
        byte[] data = new byte[8192];
        
        if (!Files.exists(pluginFolder)) {
            Files.createDirectories(pluginFolder);
        }
        ZipInputStream zis = new ZipInputStream(new FileInputStream(pluginZip));
        for(ZipEntry entry = zis.getNextEntry(); entry != null; zis.closeEntry(), entry = zis.getNextEntry()) {
            Path path = pluginFolder.resolve(entry.getName());
            if (!entry.isDirectory()) {
                BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(path.toFile()));
                for(int n = zis.read(data); n != -1; n = zis.read(data)) {
                    bos.write(data, 0, n);
                }
                bos.close();            
            } else {
                Files.createDirectories(path);
            }
        }
        zis.close();
    }

    static boolean updatePluginProperites(String pluginName, Path propsFile, boolean allowOverwrite) {
        boolean exists = false;
    	if((pluginName == null) || (pluginName.length() == 0)) {
    		System.err.println("Plugin name was not specified for update properties action");
    		return false;
    	}
        Path tmpPath = PLUGIN_TMP_FOLDER.resolve(pluginName);
        //Copy installed plug-in if exists 
		Path pluginDir = PLUGINS_DIR.resolve(pluginName);
		if(Files.exists(pluginDir)) {
			exists = true;
			Copier cp =  new Copier(pluginDir, tmpPath, true);
            // Copy all plug-in files to tmp folder to allow validation
            try {
                cp.execute();
            } catch (IOException e) {
                System.err.println("Failed to copy plugin directory into temporary area.");
                e.printStackTrace();
                return false;
            }
        }

		//Copy plugin files from staging if exist 
    	Path stagingDir = STAGING_INSTALL_DIR.resolve(pluginName);
    	if(Files.exists(stagingDir)) {
    		exists = true;
    		Copier cp = new Copier(stagingDir, tmpPath, true);
            // Copy all plug-in files to tmp folder to allow validation
            try {
                cp.execute();
            } catch (IOException e) {
                System.err.println("Failed to copy plugin(staging) directory in temporary area.");
                e.printStackTrace();
                return false;
            }
        }
        if (!exists) {
            System.err.println("Plugin " + pluginName + " does not exist. Update is not allowed for a non-existing object.");
            return false;
    	}
        
        Path dstFile = tmpPath.resolve(PLUGIN_PROPS_FILE_NAME);
        try {
        	if(allowOverwrite)
        		Files.copy(propsFile, dstFile, StandardCopyOption.REPLACE_EXISTING);
        	else
        		Files.copy(propsFile, dstFile);
        } catch (IOException e) {
            System.err.println("Failed to copy plug-in properties file to temporary location.");
            e.printStackTrace();
            return false;
        }
        if (validate(tmpPath, true)) {
            dstFile = stagingDir.resolve(PLUGIN_PROPS_FILE_NAME);            
            try {
            	if(!Files.exists(stagingDir)) {
            		Files.createDirectory(stagingDir);
            	}
                Files.copy(propsFile, dstFile, StandardCopyOption.REPLACE_EXISTING);
            } catch (IOException e) {
                System.err.println("Failed to copy plug-in properties file to staging folder.");
                e.printStackTrace();
                return false;
            }
            System.err.println("Configuration properties were updated successfully for plugin: " + pluginName);
            return true;
        }
        System.err.println("Configuration properties were not updated for plugin: " + pluginName);
        return false;
    }
    

    private static final class Copier extends SimpleFileVisitor<Path> {
        private final Path source;
        private final Path destination;
        private final boolean replaceExisting;

        Copier(Path src, Path dst, boolean overwrite) {
            source = src;
            destination = dst;
            replaceExisting = overwrite;
        }

        void execute() throws IOException {
            Files.walkFileTree(source, EnumSet.noneOf(FileVisitOption.class), 128, this);
        }

        @Override
        public FileVisitResult preVisitDirectory(Path srcFolder, BasicFileAttributes attrs) throws IOException {
            if (!Files.isSymbolicLink(srcFolder)) {
                Path targetFolder = destination.resolve(source.relativize(srcFolder));
                if (replaceExisting)
                    Files.copy(srcFolder, targetFolder, StandardCopyOption.REPLACE_EXISTING);
                else
                    Files.copy(srcFolder, targetFolder);
            }
            return FileVisitResult.CONTINUE;
        }

        @Override
        public FileVisitResult visitFile(Path srcFile, BasicFileAttributes attrs) throws IOException {
            if (!Files.isSymbolicLink(srcFile)) {
                Path targetFile = destination.resolve(source.relativize(srcFile));
                if (replaceExisting)
                    Files.copy(srcFile, targetFile, StandardCopyOption.REPLACE_EXISTING);
                else
                    Files.copy(srcFile, targetFile);

            }
            return FileVisitResult.CONTINUE;
        }

    }

    private static final class Remover extends SimpleFileVisitor<Path> {
        private final Path folder;

        Remover(Path dir) {
            super();
            folder = dir;
        }

        void execute() throws IOException {
            Files.walkFileTree(folder, EnumSet.noneOf(FileVisitOption.class), 128, this);
        }

        @Override
        public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
            Files.delete(file);
            return FileVisitResult.CONTINUE;
        }

        @Override
        public FileVisitResult postVisitDirectory(Path folder, IOException exc) throws IOException {
            if (exc == null) {
                Files.delete(folder);
                return FileVisitResult.CONTINUE;
            }
            throw exc;
        }

    }

    private static final class ConfigValidator extends SimpleFileVisitor<Path> {
        private final Map<String, Object> newConfig;
        private final String name;
        private final String protocol;
        private final String[] webSockets;
        private final boolean allowOverwrite;
        private boolean result = false;

        ConfigValidator(Map<String, Object> config, boolean overwrite) {
            super();
            newConfig = config;
            allowOverwrite = overwrite;
            name = (String) newConfig.get("Name");
            protocol = (String) newConfig.get("Protocol");
            LinkedList<String> list = new LinkedList<String>();
            int i = 0;
            while (true) {
                String ws = (String) config.get("WebSocket." + i);
                if (ws != null)
                    list.addLast(ws);
                else
                    break;
                i++;
            }
            String[] array = new String[list.size()];
            webSockets = list.toArray(array);
        }

        void execute() throws IOException {
            Files.walkFileTree(PLUGINS_DIR, EnumSet.noneOf(FileVisitOption.class), 1, this);
            Files.walkFileTree(STAGING_INSTALL_DIR, EnumSet.noneOf(FileVisitOption.class), 1, this);
        }

        boolean getResult() {
            return result;
        }

        @Override
        public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
            if (Files.isDirectory(file)) {
                Map<String, Object> existingConfig = parseConfig(file, false);
                if ((existingConfig != null) && (checkForConflicts(existingConfig, allowOverwrite))) {
                    result = true;
                    return FileVisitResult.TERMINATE;
                }
            }
            return FileVisitResult.CONTINUE;
        }

        boolean checkForConflicts(Map<String, Object> existingConfig, boolean allowOverwrite) {
            String existingName;
            existingName = (String) existingConfig.get("Name");
            if (existingName.compareToIgnoreCase(name) == 0) {
                if (!allowOverwrite) {
                    System.err.println("Plugin already exists: " + existingName);
                    return true;
                }
                String existingProtocol = (String) existingConfig.get("Protocol");
                if (existingProtocol.compareToIgnoreCase(protocol) != 0) {
                    System.err.println("Protocol change is not allowed for existing plug-in: Name=" + existingName
                            + " CurrentProtocol=" + existingProtocol + " NewProtocol=" + protocol);
                    return true;
                }
                return false;
            }
            for (int i = 0; i < webSockets.length; i++) {
                int j = 0;
                while (true) {
                    String ws = (String) existingConfig.get("WebSocket." + j);
                    if (ws == null)
                        break;
                    if (ws.compareToIgnoreCase(webSockets[i]) == 0) {
                        System.err.println("The value of WebSocket is not unique across plug-ins: WebSocket=" + ws
                                + " ExistingPlugin=" + existingName + " NewPlugin=" + name);
                        return true;
                    }
                    j++;
                }
            }
            return false;
        }

    }
}
