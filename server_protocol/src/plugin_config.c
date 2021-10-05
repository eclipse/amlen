/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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


#include <sys/stat.h>
#include "admin.h"


/*
 * Configuration for plugin.
 * This file is included from within plugin.c and should not be compiled separately
 */
static int ism_plugin_process_propertiesfile(const char * name, ism_plugin_t * plugin) ;
static int ism_plugin_process_config(const char * name, ism_plugin_t **result_plugin) ;
static ism_prop_t * ism_plugin_config_props(ism_json_parse_t * parseobj, int entloc);

#define FREE(x)       if (x != NULL) { ism_common_free(ism_memory_protocol_misc,(char *)x); x = NULL; }

static void freePlugin(ism_plugin_t * plugin)
{
    int count = 0;
    if (plugin != NULL) {
        FREE(plugin->alias);
        FREE(plugin->author);
        FREE(plugin->build);
        FREE(plugin->class);
        FREE(plugin->copyright);
        FREE(plugin->description);
        FREE(plugin->license);
        FREE(plugin->method);
        FREE(plugin->name);
        FREE(plugin->protocol);
        FREE(plugin->title);
        FREE(plugin->version);

        for(count=0; count< plugin->classpath_count; count++){
            FREE(plugin->classpath[count]);
        }
        for(count=0; count< plugin->httpheader_count; count++){
            FREE(plugin->httpheader[count]);
        }
        for(count=0; count< plugin->websocket_count; count++){
                    FREE(plugin->websocket[count]);
        }
        pthread_spin_destroy(&plugin->lock);
        ism_common_free(ism_memory_protocol_misc,plugin);
    }
}



/*
 * remove all plugins from list.
 */
void ism_plugin_removeAllPlugins(void) {
	ism_plugin_t * oldplugin=NULL;
	ism_plugin_t * plugin;
	plugin = plugins;
    plugins=NULL;
    while (plugin) {
		oldplugin = plugin;
		plugin = plugin->next;
		/*Remove from capabilities list in Admin also*/
		ism_admin_updateProtocolCapabilities(oldplugin->name, -1);
		freePlugin(oldplugin);
    }
}

/*
 * Replace a string
 */
static void replaceString(const char * * oldval, const char * val) {
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (!strcmp(freeval, val))
            return;
        *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),val);
        ism_common_free(ism_memory_protocol_misc,freeval);
    } else {
        *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),val);
    }
}

/*
 * Return a string from a JSON entry even for things without a value.
 */
static const char * getJsonValue(ism_json_entry_t * ent) {
    if (!ent)
        return "";
    switch (ent->objtype) {
    case JSON_True:    return "true";
    case JSON_False:   return "false";
    case JSON_Null:    return "null";
    case JSON_Object:  return "object";
    case JSON_Array:   return "array";
    case JSON_Integer:
    case JSON_String:
    case JSON_Number:
        return ent->value;
    }
    return "";
}

/*
 * Read and process plugin config files.
 * Each file is a JSON document.
 */
static int configPlugin(void) {
    char   cwd[1024];
    char   fname [PATH_MAX];
    char * path;
    char * pp;
    char * mask;
    int    cwdlen=0;
    struct dirent * dents;
    DIR *  dir;
    int pluginCount=0;

    const char * pipath = ism_common_getStringConfig("PluginPath");

    if (pipath != NULL) {
        TRACE(4, "Configure plug-in: path=%s\n", pipath);
		/*
		 * If the path is relative make it absolute but in any case make a copy of it.
		 */
		if (*pipath != '/' && getcwd(cwd, sizeof cwd) != NULL ) {
			cwdlen = strlen(cwd);
			path = alloca(cwdlen + strlen(pipath) + 2);
			strcpy(path, cwd);
			if (cwd[cwdlen-1] != '/') {
				path[cwdlen++] = '/';
			}
			strcpy(path+cwdlen, pipath);
		} else {
			path = alloca(strlen(pipath)+1);
			strcpy(path, pipath);
		}

		/*
		 * Separate the path an mask
		 */
		pp = path + strlen(path) - 1;
		while (pp >= path && *pp != '/')
			pp--;
		if (pp > path) {
			*pp = 0;
			mask = pp+1;
			if (strchr(path, '*')) {
				TRACE(2, "PluginPath entry is not valid as path contains an asterisk: %s\n", pipath);
				return ISMRC_PluginConfigNotValid;
			}
		} else {
			TRACE(2, "PluginPath entry is not valid: %s\n", pipath);
			return ISMRC_PluginConfigNotValid;
		}

		dir = opendir(path);
		if (!dir) {
			TRACE(2, "PluginPath entry does not exist or is not a directory: %s\n", pipath);
			return ISMRC_PluginConfigNotValid;
		}

		/*
		 * Process the files in this directory now
		 */
		dents = readdir(dir);
		while (dents) {
		    ism_plugin_t *plugin = NULL;
			if (dents->d_type != DT_DIR) {
				if (ism_common_match(dents->d_name, mask)) {
					snprintf(fname, sizeof fname, "%s/%s", path, dents->d_name);
					TRACE(4, "Plugin config file: %s\n", fname);
					ism_plugin_process_config(fname, &plugin);
					if(plugin) {
					    plugin->next = plugins;
					    plugins = plugin;
					    pluginCount++;
					}
				}
			}
			dents = readdir(dir);
		}

		closedir(dir);

    } else {
    	char filepath[1024];
    	char configFilePath[1024];
    	char propsFilePath[1024];
    	struct stat st;
    	/*Use default Config Plugin path */
    	dir = opendir(PLUGINS_DIR);
		if (!dir) {
			TRACE(6, "PluginPath entry does not exist or is not a directory: %s\n", PLUGINS_DIR);
			return ISMRC_PluginConfigNotValid;
		}
		while ((dents = readdir(dir)) != NULL) {
			if (dents->d_name[0] != '.') {
				memset(filepath, '\0', sizeof(filepath));
				snprintf(filepath, sizeof(filepath), "%s%s", PLUGINS_DIR, dents->d_name);
				lstat(filepath, &st);
				if (S_ISDIR(st.st_mode)){
					memset(configFilePath, '\0', sizeof(configFilePath));
					snprintf(configFilePath, sizeof(configFilePath), "%s%s/plugin.json", PLUGINS_DIR, dents->d_name);
					/* Check if configuration file exists */
					FILE * configFile = fopen(configFilePath, "rb");
					if (!configFile) {
						continue;
					}
					fclose(configFile);
					ism_plugin_t *plugin = NULL;
					ism_plugin_process_config(configFilePath, &plugin);

					if(plugin!=NULL){
						memset(propsFilePath, '\0', sizeof(propsFilePath));
						snprintf(propsFilePath, sizeof(propsFilePath), "%s%s/pluginproperties.json", PLUGINS_DIR, dents->d_name);
						ism_plugin_process_propertiesfile(propsFilePath, plugin);
					}
					plugin->next = plugins;
					plugins = plugin;
					pluginCount++;
				}
			}
		}
		closedir(dir);

    }

    return 0;
}


/*
 * Process a plugin properties file
 */
static int ism_plugin_process_propertiesfile(const char * name, ism_plugin_t * plugin) {
    FILE * f;
    int  len;
    char * buf;
    int    bread;
    int    rc;

    if (name == NULL)
        return 0;

    /*
     * Open the file and read it into memory
     */
    f = fopen(name, "rb");
    if (!f) {
        LOG(WARN, Server, 915, "%s", "The properties file is not found: {0}", name);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,7),len+2);
    if (!buf) {
        printf("Unable to allocate memory for config file: %s\n", name);
        fclose(f);
        return 2;
    }
    rewind(f);

    /*
     * Read the file
     */
    bread = fread(buf, 1, len, f);
    buf[bread] = 0;
    if (bread != len) {
        LOG(WARN, Server, 916, "%s", "The properties file cannot be read: {0}", name);
        ism_common_free(ism_memory_protocol_misc,buf);
        fclose(f);
        return 3;
    }
    fclose(f);

    /*
     * Parse the JSON file
     */
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[500];
    parseobj.ent_alloc = 500;
    parseobj.ent = ents;
    parseobj.source = (char *)buf;
    parseobj.src_len = len;
    parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
    rc = ism_json_parse(&parseobj);
    if (rc) {
        LOG(WARN, Server, 917, "%s%d%d", "The properties file is not valid JSON encoding: File={0} Error={1} Line={2}",
                 name, rc, parseobj.line);
    }


    /*
     * Process the JSON DOM
     */
    int entloc = 1;
    while (entloc < parseobj.ent_count) {
        ism_json_entry_t * ent = parseobj.ent+entloc;
        int found = 0;
        switch(ent->objtype) {
        /* Ignore simple objects */
        case JSON_String:  /* JSON string, value is UTF-8  */
        case JSON_Integer: /* Number with no decimal point             */
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
        case JSON_Number:  /* Number which is too big or has a decimal */
        case JSON_Array:
        case JSON_Null:
            entloc++;
            break;

        case JSON_Object:  /* JSON object, count is number of entries  */
            if (ent->name) {
                if (!strcmp(ent->name, "Properties")) {
                    ism_prop_t * oldProps;
                    ism_prop_t * newProps = ism_plugin_config_props(&parseobj, entloc);;
                    pthread_spin_lock(&plugin->lock);
                    oldProps = plugin->props;
                    plugin->props = newProps;
                    pthread_spin_unlock(&plugin->lock);
                    if(oldProps)
                        ism_common_freeProperties(oldProps);
                    found = 1;
                }
            }
            entloc += ent->count + 1;
            break;
        }
        if (!found) {
            LOG(WARN, Server, 2401, "%-s%-s%-s%u", "The plug-in configuration property is unknown or not valid: Property={0} Value={1} File={2} Line={3}.",
                    ent->name ? ent->name : "", getJsonValue(ent), name, ent->line);
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ent->name, getJsonValue(ent));
        }
    }


    /* Clean up */
    if (parseobj.free_ent)
        ism_common_free(ism_memory_utils_parser,parseobj.ent);
    ism_common_free(ism_memory_protocol_misc,buf);
    return 0;
}

/*
 * Process a plugin config file
 */
static int ism_plugin_process_config(const char * name, ism_plugin_t **result_plugin) {
    FILE * f;
    int  len;
    char * buf;
    int    bread;
    int    rc;
    int    endarray;
    int    awhere;
    ism_json_entry_t * aent;
    ism_plugin_t * plugin;

    if (name == NULL)
        return 0;



    /*
     * Open the file and read it into memory
     */
    f = fopen(name, "rb");
    if (!f) {
        LOG(WARN, Server, 915, "%s", "The configuration file is not found: {0}", name);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,11),len+2);
    if (!buf) {
        printf("Unable to allocate memory for config file: %s\n", name);
        fclose(f);
        return 2;
    }
    rewind(f);

    /*
     * Read the file
     */
    bread = fread(buf, 1, len, f);
    buf[bread] = 0;
    if (bread != len) {
        LOG(WARN, Server, 916, "%s", "The configuration file cannot be read: {0}", name);
        ism_common_free(ism_memory_protocol_misc,buf);
        fclose(f);
        return 3;
    }
    fclose(f);

    /*
     * Parse the JSON file
     */
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[500];
    parseobj.ent_alloc = 500;
    parseobj.ent = ents;
    parseobj.source = (char *)buf;
    parseobj.src_len = len;
    parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
    rc = ism_json_parse(&parseobj);
    if (rc) {
        LOG(WARN, Server, 917, "%s%d%d", "The configuration file is not valid JSON encoding: File={0} Error={1} Line={2}",
                 name, rc, parseobj.line);
    }

    /*
     * Default name from config file name
     */

    plugin = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,13),1, sizeof(ism_plugin_t));
    pthread_spin_init(&plugin->lock, 0);
    plugin->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),name);
    char * cp = (char *)plugin->name + strlen(plugin->name) - 1;
    while (cp > plugin->name) {
        if (*cp == '.') {
            *cp = 0;
            break;
        }
        cp--;
    }


    /*
     * Process the JSON DOM
     */
    int entloc = 1;
    while (entloc < parseobj.ent_count) {
        ism_json_entry_t * ent = parseobj.ent+entloc;
        int found = 0;
        switch(ent->objtype) {
        /* Ignore simple objects */
        case JSON_String:  /* JSON string, value is UTF-8  */
            if (!strcmpi(ent->name, "Name")) {
                replaceString(&plugin->name, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Class")) {
                replaceString(&plugin->class, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Method")) {
                replaceString(&plugin->method, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Protocol")) {
                replaceString(&plugin->protocol, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Author")) {
                replaceString(&plugin->author, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Version")) {
                replaceString(&plugin->version, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Copyright")) {
                replaceString(&plugin->copyright, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Build")) {
                replaceString(&plugin->build, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Description")) {
                replaceString(&plugin->description, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "License")) {
                replaceString(&plugin->license, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Title")) {
                replaceString(&plugin->title, ent->value);
                found = 1;
            } else if (!strcmpi(ent->name, "Alias")) {
                replaceString(&plugin->alias, ent->value);
                found = 1;
            }
            entloc++;
            break;
        case JSON_Integer: /* Number with no decimal point             */
        	if (!strcmpi(ent->name, "Modification")) {
				if(ent->value!=NULL  && *ent->value!='\0'){
					plugin->modification = atoi(ent->value);
				}else{
					plugin->modification=1;
				}
				found = 1;
			}
			entloc++;
			break;
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
            if (!strcmpi(ent->name, "UseQueue")) {
                plugin->usequeue = ent->objtype == JSON_True;
                found = 1;
            } else if (!strcmpi(ent->name, "UseTopic")) {
                plugin->usetopic = ent->objtype == JSON_True;
                found = 1;
            } else if (!strcmpi(ent->name, "UseBrowse")) {
                plugin->usebrowse = ent->objtype == JSON_True;
                found = 1;
            } else if (!strcmpi(ent->name, "UseShared")) {
                plugin->useshared = ent->objtype == JSON_True;
                found = 1;
            } else if (!strcmpi(ent->name, "SimpleConsumer")) {
                plugin->simple_consumer = ent->objtype == JSON_True;
                found = 1;
            }
            entloc++;
            break;

        case JSON_Number:  /* Number which is too big or has a decimal */

        case JSON_Null:
            entloc++;
            break;

        case JSON_Object:  /* JSON object, count is number of entries  */
            if (ent->name) {
                if (!strcmp(ent->name, "Properties")) {
                    plugin->props = ism_plugin_config_props(&parseobj, entloc);
                    found = 1;
                }
            }
            entloc += ent->count + 1;
            break;
        case JSON_Array:   /* JSON array, count is number of entries   */
            if (!strcmp(ent->name, "Classpath")) {
                if (ent->count <= 32) {
                    awhere = entloc+1;
                    endarray = awhere + ent->count;
                    while (awhere < endarray) {
                        aent = parseobj.ent + awhere;
                        if (aent->objtype != JSON_String)
                            break;
                        plugin->classpath[plugin->classpath_count++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),aent->value);
                        awhere++;
                    }
                    if (awhere < endarray)
                        break;
                }
                found = 1;
            } else if (!strcmp(ent->name, "WebSocket")) {
                if (ent->count <= 8) {
                    awhere = entloc+1;
                    endarray = awhere + ent->count;
                    while (awhere < endarray) {
                        aent = parseobj.ent + awhere;
                        if (aent->objtype != JSON_String)
                            break;
                        plugin->websocket[plugin->websocket_count++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),aent->value);
                        awhere++;
                    }
                    if (awhere < endarray)
                        break;
                }
                found = 1;
            }  else if (!strcmp(ent->name, "HttpHeader")) {
                if (ent->count <= 16) {
                    awhere = entloc+1;
                    endarray = awhere + ent->count;
                    while (awhere < endarray) {
                        aent = parseobj.ent + awhere;
                        if (aent->objtype != JSON_String)
                            break;
                        plugin->httpheader[plugin->httpheader_count++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),aent->value);
                        awhere++;
                    }
                    if (awhere < endarray)
                        break;
                }
                found = 1;
            }else if (!strcmp(ent->name, "InitialByte")) {
                awhere = entloc+1;
                endarray = awhere + ent->count;
                while (awhere < endarray) {
                    aent = parseobj.ent + awhere;
                    int whichb;
                    if (aent->objtype == JSON_String) {
                        whichb = aent->value[0];
                    } else if (aent->objtype == JSON_Integer) {
                        if (aent->count < 0 || aent->count > 255)
                            break;
                        whichb = aent->count;
                    } else {
                        break;
                    }
                    plugin->initial_byte[whichb&0xff] = 1;
                    awhere++;
                }
                plugin->initial_byte_count = ent->count;
                found = 1;
            }
            entloc += ent->count + 1;
            break;
        }
        if (!found) {
            LOG(WARN, Server, 2401, "%-s%-s%-s%u", "The plug-in configuration property is unknown or not valid: Property={0} Value={1} File={2} Line={3}.",
                    ent->name ? ent->name : "", getJsonValue(ent), name, ent->line);
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ent->name, getJsonValue(ent));
        }
    }

    /*
     * Check for required fields and give error
     */
    if (plugin->classpath_count < 1) {
        rc = ISMRC_NotFound;
        LOG(WARN, Server, 2402, "%-s%-s", "A required plug-in property is not set. Property: {0} Plug-in: {1}.",
                "Classpath", plugin->name);
    }
    if (!plugin->class) {
        rc = ISMRC_NotFound;
        LOG(WARN, Server, 2402, "%-s%-s", "A required plug-in property is not set. Property: {0} Plug-in: {1}.",
                "Class", plugin->name);
    }
    if (!plugin->protocol) {
        rc = ISMRC_NotFound;
        LOG(WARN, Server, 2402, "%-s%-s", "A required plug-in property is not set. Property: {0} Plug-in: {1}.",
                "Protocol", plugin->name);
    }

    /*
     * Link in the plugin
     */
    if (rc == 0) {
        plugin->protomask = ism_transport_pluginMask(plugin->protocol, 1);
        TRACE(4, "plugin config complete: %s\n", plugin->name);

        int capability = 0;
        if (plugin->usetopic)
            capability |= ISM_PROTO_CAPABILITY_USETOPIC;
        if (plugin->useshared)
            capability |= ISM_PROTO_CAPABILITY_USESHARED;
        if (plugin->usequeue)
            capability |= ISM_PROTO_CAPABILITY_USEQUEUE;
        if (plugin->usebrowse)
            capability |= ISM_PROTO_CAPABILITY_USEBROWSE;
        if (plugin->simple_consumer)
            capability |= 0x100;
        ism_admin_updateProtocolCapabilities(plugin->protocol, capability);
    } else {
        LOG(ERROR, Server, 2403, "%-s", "The plug-in is not installed. Plug-in: {0}.", plugin->name);
        FREE(plugin);
    }
    if(result_plugin!=NULL) *result_plugin = plugin;

    /* Clean up */
    if (parseobj.free_ent)
        ism_common_free(ism_memory_utils_parser,parseobj.ent);
    ism_common_free(ism_memory_protocol_misc,buf);
    return 0;
}

/*
 * Put all items at this level into a properties object
 */
static ism_prop_t * ism_plugin_config_props(ism_json_parse_t * parseobj, int entloc) {
    ism_prop_t * props = ism_common_newProperties(20);
    ism_json_entry_t * ent = parseobj->ent + entloc;
    ism_field_t var = {0};
    int endloc = entloc + ent->count;

    for (++entloc; entloc <= endloc; entloc++) {
        ent = parseobj->ent + entloc;
        switch(ent->objtype) {
        case JSON_String:  /* JSON string, value is UTF-8              */
            var.type = VT_String;
            var.val.s = (char *)ent->value;
            ism_common_setProperty(props, ent->name, &var);
            entloc++;
            break;
        case JSON_Integer: /* Number with no decimal point             */
            var.type = VT_Integer;
            var.val.i = ent->count;
            ism_common_setProperty(props, ent->name, &var);
            entloc++;
            break;
        case JSON_Number:  /* Number which is too big or has a decimal */
            var.type = VT_Double;
            var.val.d = strtod(ent->value, NULL);
            ism_common_setProperty(props, ent->name, &var);
            entloc++;
            break;
        case JSON_Object:  /* JSON object, count is number of entries  */
        case JSON_Array:   /* JSON array, count is number of entries   */
            /* Just ignore complex objects for now */
            entloc += ent->count + 1;
            break;
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
            var.type = VT_Boolean;
            var.val.i = ent->objtype == JSON_True;
            ism_common_setProperty(props, ent->name, &var);
            entloc++;
            break;
        case JSON_Null:
            ism_common_setProperty(props, ent->name, NULL);
            entloc++;
        }
    }
    return props;
}
