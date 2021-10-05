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
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "adminInternal.h"
#include <config.h>
#include "configInternal.h"


#undef TRACE_COMP
#define TRACE_COMP Admin

extern ismHashMap * ismSecProtocolMap;
extern ism_config_callback_t ism_config_getCallback(int type);

typedef struct ism_config_plugin_t {
    struct ism_admin_plugin_t * next;
    const char * name;
} ism_config_plugin_t;

static int ism_config_removeDirectory(char *path);
static char *  ism_config_getPluginConfig(char * name);
static char *  ism_config_getPluginPropertiesFile(char * name);
extern int32_t ism_admin_setReturnCodeAndStringJSON(concat_alloc_t *output_buffer, int rc, const char * returnString) ;

typedef void (*protocolTermPlugin_f)(void);
static protocolTermPlugin_f protocolTermPlugin = NULL;

typedef void (*protocolRestartPlugin_f)(void);
static protocolRestartPlugin_f protocolRestartPlugin = NULL;

typedef int (*protocolPluginStatus_f)(void);

/**
 * Remove all files under a directory
 * @param path the directory path
 * @return zero for success. Otherwise, it is an error.
 */
static int ism_config_removeFiles(char *path) {
	struct dirent *d;
	DIR *dir;
	char filepath[FILENAME_MAX+1];
	int rc = 0;
	struct stat st;

    dir = opendir(path);
    if (dir == NULL)
    	return -1;

    while((d = readdir(dir)) != NULL) {
        //fprintf(stderr, "%s\n",d->d_name);
    	if(d->d_name[0] != '.') {
			memset(filepath, '\0', sizeof(filepath));
			sprintf(filepath, "%s/%s", path, d->d_name);
			lstat(filepath, &st);
			if(S_ISDIR(st.st_mode)){
				rc = ism_config_removeDirectory(filepath);
			}else{
				rc = unlink(filepath);
			}
			if(rc!=0) break;
    	}
    }
    closedir(dir);
    return rc;
}
/**
 * Remove a directory and all the files under it.
 * @param the directory path
 * @return zero for success. Otherwise, it is an error.
 */
static int ism_config_removeDirectory(char *path)
{
	int rc = 0;
	rc = ism_config_removeFiles(path);
	if(rc ==0){
		rc = rmdir(path);
	}
	return rc;
}

#if 0
/**
 * Copy a directory into another directory.
 * @param sourceDirectory	source directory
 * @param destDirectory		destination directory
 * @return zero if success. Otherwise, it is an error.
 */
static int ism_config_copyDirectory(const char * sourceDirectory, const char * destDirectory)
{

	pid_t pid = fork();
	if (pid < 0) {
	    TRACE(1, "Could not fork process to restart processes\n");
	    return 1;
	}
	if (pid == 0){
	    execl("/bin/cp", "cp", "-a", sourceDirectory, destDirectory , NULL);
        int urc = errno;
        TRACE(1, "Unable to run cp: errno=%d error=%s\n", urc, strerror(urc));
	    _exit(1);
	}
	int st;
	waitpid(pid, &st, 0);
	int result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

	return result;

}

/**
 * Install the plug-ins under the staging install directory.
 *
 * @return zero if success. Otherwise, it is an error.
 */
int ism_config_installPlugin(int * changeCount)
{
	/*
	 * Move any directories under STAGING_INSTALL_DIR into PLUGIN_DIR
	 */
	struct dirent *d;
	struct stat st;
	DIR *dir;
	char filepath[FILENAME_MAX+1];
	int rc = 0;


	dir = opendir(STAGING_INSTALL_DIR);
	if (dir == NULL)
		return -1;

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] != '.') {
			memset(filepath, '\0', sizeof(filepath));
			snprintf(filepath, sizeof(filepath), "%s%s", STAGING_INSTALL_DIR, d->d_name);
			lstat(filepath, &st);
			if(S_ISDIR(st.st_mode)){


				/*Copy the plugins from staging to actuall plugin directory*/
				rc = ism_config_copyDirectory(filepath, PLUGINS_DIR);
				if(rc!=ISMRC_OK){
					LOG(ERROR, Admin, 6142, "%-s%d", "Failed to install the plug-in. Name={0}. RC={1}", d->d_name, rc);
				}

				/*Call Protocol Config Callback to apply the updates.*/

				if(rc==0){
					/*Remove the plugin directory from staging area*/
					rc = ism_config_removeDirectory(filepath);
					if(rc!=0){
						LOG(ERROR, Admin, 6139, "%-s%d", "Failed to remove the plug-in from the staging directory. Name={0}. RC={1}", d->d_name, rc);
					}
				}

				if(changeCount!=NULL) (*changeCount)++;
			}
		}
	}
	closedir(dir);

	return rc;

}

/**
 * Install the plug-ins under the staging install directory.
 *
 * @return zero if success. Otherwise, it is an error.
 */
int ism_config_updatePluginProps(const char * pluginName)
{
	/*
	 * Move any directories under STAGING_INSTALL_DIR into PLUGIN_DIR
	 */
	struct dirent *d;
	struct dirent *d2;
	struct stat st;
	DIR *dir, *dir2;
	char filepath[FILENAME_MAX+1];
	int rc = 0;
	int pendingChanges=0;


	dir = opendir(STAGING_INSTALL_DIR);
	if (dir == NULL)
		return -1;

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] != '.' && !strcmp(d->d_name, pluginName)) {
			memset(filepath, '\0', sizeof(filepath));
			snprintf(filepath, sizeof(filepath), "%s%s", STAGING_INSTALL_DIR, d->d_name);
			lstat(filepath, &st);
			if(S_ISDIR(st.st_mode)){

				/*Determine if there are any other changes. If yes, skip. Wait for restart of plugin server*/
				dir2 = opendir(filepath);
				while((d2 = readdir(dir2)) != NULL) {
					if(d2->d_name[0] != '.' && strcmp(d2->d_name,"pluginproperties.json")) {
						pendingChanges++;
					}
				}
				closedir(dir2);

				if(pendingChanges==0){
					/*Copy the plugins from staging to actuall plugin directory*/
					rc = ism_config_copyDirectory(filepath, PLUGINS_DIR);
					if(rc!=ISMRC_OK){
						LOG(ERROR, Admin, 6138, "%-s%d", "Failed to apply updates to the plug-in. Name={0}. RC={1}", d->d_name, rc);
					}

					/*Call Protocol Config Callback to apply the updates.*/

					if(rc==0){
						/*Remove the plugin directory from staging area*/
						rc = ism_config_removeDirectory(filepath);
						if(rc!=0){
							LOG(ERROR, Admin, 6139, "%-s%d", "Failed to remove the plug-in from the staging directory. Name={0}. RC={1}", d->d_name, rc);
						}

						TRACE(7, "Call Protocol to apply Plugin updates. Name: %s.\n", pluginName);
						ism_config_callback_t callback = ism_config_getCallback(ISM_CONFIG_COMP_PROTOCOL);
						if ( callback ) {
							rc = callback("Plugin", (char *)pluginName, NULL, ISM_CONFIG_CHANGE_PROPS);
							if ( rc != ISMRC_OK )
								TRACE(5, "Unable to send update plugin to protocol. rc=%d\n", rc);
						}
					}
				}

			}
		}
	}
	closedir(dir);

	return rc;

}

/**
 * UnInstall the plug-ins under the staging uninstall directory.
 *
 * @return zero if success. Otherwise, it is an error.
 */
int ism_config_uninstallPlugin(int * changeCount)
{
	struct dirent *d;
	struct stat st;
	DIR *dir;
	char filepath[FILENAME_MAX+1];
	char pfilepath[FILENAME_MAX+1];
	int rc=0;


	/*Check the staging uninstall directory to see if any plugin need to
	 * be removed.
	 * */
	dir = opendir(STAGING_UNINSTALL_DIR);
	if (dir == NULL)
		return -1;

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] != '.') {
			memset(filepath, '\0', sizeof(filepath));
			snprintf(filepath, sizeof(filepath), "%s%s", STAGING_UNINSTALL_DIR, d->d_name);
			lstat(filepath, &st);
			if(S_ISDIR(st.st_mode)){
				memset(pfilepath, '\0', sizeof(pfilepath));
				snprintf(pfilepath, sizeof(pfilepath), "%s%s", PLUGINS_DIR, d->d_name);

				/*TODO: Call Protocol Config Callback to remove the plugin*/

				/*Remove the plugin from the actual plugin directory*/
				rc = ism_config_removeDirectory(pfilepath);
				if(rc!=0){
					LOG(ERROR, Admin, 6140, "%-s%d", "Failed to delete the plug-in. Name={0}. RC={1}", d->d_name, rc);

				}


				/*Remove the plugin directory from staging uninstall area*/
				if(rc==0){
					rc = ism_config_removeDirectory(filepath);
					if(rc!=0){
						LOG(ERROR, Admin, 6141, "%-s%d", "Failed to delete the plug-in from the staging directory. Name={0}. RC={1}", d->d_name, rc);
					}
				}
				if(changeCount!=NULL) (*changeCount)++;


			}
		}
	}
	closedir(dir);
	return 0;

}

int ism_config_applyPluginUpdates(void)
{
	int changeCount=0;
	/*Apply Updates*/
	ism_config_installPlugin(&changeCount);

	ism_config_uninstallPlugin(&changeCount);

	return changeCount;
}
#endif

/**
 * Destroy the Plugin object
 * @param param 	the plugin object.
 */
static void  destroyPluginObject(void * param)
{
	if(param!=NULL){
		ism_config_plugin_t *plugin = (ism_config_plugin_t *) param;
		if(plugin->name){
			ism_common_free(ism_memory_admin_misc,(void *)plugin->name);
			plugin->name=NULL;
		}
		ism_common_free(ism_memory_admin_misc,(void *)plugin);
		plugin=NULL;
	}
}



int ism_admin_mapItemComparator(const void *data1, const void *data2){
	ismHashMapEntry * i = (ismHashMapEntry *) data1;
	ismHashMapEntry * j = (ismHashMapEntry *) data2;

	int len = (i->key_len<=j->key_len? i->key_len: j->key_len);
	if(memcmp(i->key, j->key, len) < 0) return -1;
	if(memcmp(i->key, j->key, len) > 0) return 1;

	return 0;
}

int ism_admin_pluginItemComparator(const void *data1, const void *data2){
	ism_config_plugin_t  * i = (ism_config_plugin_t *) data1;
	ism_config_plugin_t  * j = (ism_config_plugin_t *) data2;

	int ilen = strlen(i->name);
	int jlen = strlen(j->name);
	int len = (ilen<=jlen? ilen: jlen);
	if(memcmp(i->name, j->name, len) < 0) return -1;
	if(memcmp(i->name, j->name, len) > 0) return 1;
	return 0;
}

/**
 * Get the list of Plugin objects
 * @param pluginList 	The list object which will contain plugins.
 * 						Assume that the list is initialized.
 * @return the number of the plugins. If -1, then there is an error.
 */
static int ism_config_getPlugins(ism_common_list *pluginList)
{
	struct dirent *d;
	DIR *dir;
	int count=0;

	dir = opendir(PLUGINS_DIR);
	if (dir == NULL)
		return -1;

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] != '.' && strcmp(d->d_name, "staging")) {
			ism_config_plugin_t * plugin = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,517),1, sizeof(ism_config_plugin_t));
			/*Set name*/
			plugin->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),d->d_name);

			ism_common_list_insert_ordered(pluginList, (void *) plugin, ism_admin_pluginItemComparator);
			count++;
		}
	}
	closedir(dir);
	return count;

}


/**
 * Get Protocol Plug-in Info
 * @param json		 		json object
 * @param output_buffer		the output_buffer
 * @return ISMRC_OK if success. Otherwise, it is an error.
 */
int ism_admin_getPluginsInfo(ism_json_parse_t *json, concat_alloc_t *output_buffer)
{
	int listOpt = 0;
	int dataFound=0;
	int arrayOpenBracketSet = 0;

	char xbuf[8192];
	concat_alloc_t configPropsFileBuf = {0};
	configPropsFileBuf.buf = xbuf;
	configPropsFileBuf.len = sizeof xbuf;

	char *name = (char *)ism_json_getString(json, "Name");
	char *retList = (char *)ism_json_getString(json, "CLIListOption");

	 if ( retList && !strcmpi(retList, "True"))
	        listOpt = 1;

	ism_common_list pluginsList;
	ism_common_list_init(&pluginsList, 0, destroyPluginObject);

	int count = ism_config_getPlugins(&pluginsList);


	if (count > 0) {
		 ism_common_listIterator iter;
		 ism_common_list_iter_init(&iter, &pluginsList);



		char buf[4096];
		/*Set Protocol Data*/
		int i=0;
		while (ism_common_list_iter_hasNext(&iter)) {

			ism_common_list_node * node = ism_common_list_iter_next(&iter);
			ism_config_plugin_t * plugin =(ism_config_plugin_t *)  node->data;

			const char * key = plugin->name;
			int key_len = strlen(key);

			if(listOpt==0){
				if(name!=NULL){
					if(memcmp(name, key, key_len)){
						i++;
						continue;
					}
				}

				char * config = ism_config_getPluginConfig(name);

				if(config!=NULL){

					if(arrayOpenBracketSet==0){
						ism_common_allocBufferCopyLen(output_buffer, "[",1);
						arrayOpenBracketSet =1;
					}

					if(dataFound>0)
						ism_common_allocBufferCopyLen(output_buffer, ",",1);

					ism_common_allocBufferCopyLen(output_buffer, "{",1);

					sprintf(buf, "\"Name\":");
					ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));

					ism_common_allocBufferCopyLen(&configPropsFileBuf, "\n",1);
					ism_common_allocBufferCopyLen(&configPropsFileBuf, config,strlen(config));

					char * propertiesFile =   ism_config_getPluginPropertiesFile(name);
					if(propertiesFile!=NULL){
						ism_common_allocBufferCopyLen(&configPropsFileBuf, propertiesFile,strlen(propertiesFile));
						ism_common_free(ism_memory_admin_misc,propertiesFile);
					}

					char * tmpbuf = alloca(configPropsFileBuf.used+1);
					memcpy(tmpbuf,configPropsFileBuf.buf,configPropsFileBuf.used);
					tmpbuf[configPropsFileBuf.used]='\0';

					ism_json_putString(output_buffer, tmpbuf);

					ism_common_allocBufferCopyLen(output_buffer, "}",1);

					ism_common_free(ism_memory_admin_misc,config);

					dataFound++;

					if(name!=NULL){
						break;
					}
				}

			}else if(listOpt==1){
				if(arrayOpenBracketSet==0){
					ism_common_allocBufferCopyLen(output_buffer, "[",1);
					arrayOpenBracketSet =1;
				}
				if(i>0)
					ism_common_allocBufferCopyLen(output_buffer, ",",1);

				ism_common_allocBufferCopyLen(output_buffer, "{",1);

				sprintf(buf, "\"Name\":\"");
				ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));

				memcpy(buf,key,key_len);
				buf[key_len]='\0';

				ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
				ism_common_allocBufferCopyLen(output_buffer, "\"}",2);

				dataFound++;

			}

			i++;


		}

		ism_common_list_iter_destroy(&iter);

		if(dataFound>0) ism_common_allocBufferCopyLen(output_buffer, "]",1);
	}

	if(dataFound==0){
		/*Set No Data */
		ism_common_setError(ISMRC_NotFound);
		return ISMRC_NotFound;
	}
	ism_common_list_destroy(&pluginsList);


	return ISMRC_OK;


}


/**
 * Get the list of Plugin objects
 * @param pluginList 	The list object which will contain plugins.
 * 						Assume that the list is initialized.
 * @return the number of the plugins. If -1, then there is an error.
 */
static char *  ism_config_getPluginConfig(char * name)
{
	struct dirent *d;
	DIR *dir;
	int count=0;
	char pluginJSONpath[FILENAME_MAX+1];
	char *configBuffer=NULL;
	int rc1 = 0;

	dir = opendir(PLUGINS_DIR);
	if (dir == NULL)
		return NULL;

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] != '.' && strcmp(d->d_name, "staging") && !strcmp(d->d_name, name)) {

			sprintf(pluginJSONpath,"%s/%s/%s",PLUGINS_DIR, d->d_name, DEFAULT_PLUGIN_CONFIG_NAME);
			FILE* configfile = fopen(pluginJSONpath, "r");
			if (!configfile)
			{
				TRACE(6, "Failed to read the plugin JSON configuration file.\n");
				return NULL;
			}

			/*Read in the JSOn and get the Name for the directory*/
			fseek(configfile, 0, SEEK_END);
			long fsize = ftell(configfile);
			fseek(configfile, 0, SEEK_SET);

			configBuffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,520),fsize + 1);
			if(!configBuffer){
				return NULL;
			}
			if (( rc1 = fread(configBuffer, fsize, 1, configfile)) != 0) {
			    TRACE(4, "fread returned error: %d\n", rc1);
			}
			fclose(configfile);
			configBuffer[fsize] = 0;
			count++;
		}
	}

	closedir(dir);
	return configBuffer;

}

/**
 * Get Plugin Properties file
 */
static char *  ism_config_getPluginPropertiesFile(char * name)
{
	struct dirent *d;
	DIR *dir;
	int count=0;
	char pluginJSONpath[FILENAME_MAX+1];
	char *configBuffer=NULL;
	int rc1 = 0;

	dir = opendir(PLUGINS_DIR);
	if (dir == NULL)
		return NULL;

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] != '.' && strcmp(d->d_name, "staging") && !strcmp(d->d_name, name)) {

			sprintf(pluginJSONpath,"%s/%s/%s",PLUGINS_DIR, d->d_name, DEFAULT_PLUGIN_PROPS_FILE_NAME);
			FILE* configfile = fopen(pluginJSONpath, "r");
			if (!configfile)
			{
				TRACE(6, "Failed to read the plugin JSON configuration file.\n");
				return NULL;
			}

			/*Read in the JSOn and get the Name for the directory*/
			fseek(configfile, 0, SEEK_END);
			long fsize = ftell(configfile);
			fseek(configfile, 0, SEEK_SET);

			configBuffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,521),fsize + 1);
			if(!configBuffer){
				return NULL;
			}
			if (( rc1 = fread(configBuffer, fsize, 1, configfile)) != 0) {
			    TRACE(4, "fread returned error: %d\n", rc1);
			}
			fclose(configfile);
			configBuffer[fsize] = 0;
			count++;
		}
	}

	closedir(dir);
	return configBuffer;

}


/**
 * Run command of Plugin Process
 *
 * The current valid commands:
 *
 * status
 * stop
 * start
 */
int ism_admin_runPluginProcessCommand(const char * command)
{

	pid_t pid = fork();
	if (pid < 0){
		TRACE(1, "Could not fork process to restart processes\n");
		return 1;
	}
	if (pid == 0){
		execl(IMA_SVR_INSTALL_PATH "/bin/pluginServerUtils.sh", IMA_SVR_INSTALL_PATH "/bin/pluginServerUtils.sh", command, NULL);
        int urc = errno;
        TRACE(1, "Unable to run pluginServerUtils.sh: errno=%d error=%s\n", urc, strerror(urc));
		_exit(1);
	}
	int st;
	waitpid(pid, &st, 0);
	int result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
	return result;
}

/**
 * Stop Plug-in Server Process
 */
XAPI int ism_admin_stopPluginProcess(void)
{

	return ism_admin_runPluginProcessCommand("stop");
}


/**
 * RESTAPI Get Protocol Info
 * @param json		 		json object
 * @param output_buffer		the output_buffer
 * @return ISMRC_OK if success. Otherwise, it is an error.
 */
int ism_admin_getProtocolsInfo(ism_json_parse_t *json, concat_alloc_t *output_buffer)
{
	int listOpt = 0;
	int dataFound=0;
	int arrayOpenBracketSet = 0;
	ism_common_list protocolList;
	int count=0;

	char *name = (char *)ism_json_getString(json, "Name");
	char *retList = (char *)ism_json_getString(json, "CLIListOption");

	 if ( retList && !strcmpi(retList, "True"))
	        listOpt = 1;

	if (ism_common_getHashMapNumElements(ismSecProtocolMap)) {
		char buf[4096];

		ism_common_HashMapLock(ismSecProtocolMap);
		ismHashMapEntry **entries = ism_common_getHashMapEntriesArray(ismSecProtocolMap);

		/*Put in sorted list*/
		ism_common_list_init(&protocolList, 0, NULL);
		while(entries[count] != ((void*) -1)) {
			ism_common_list_insert_ordered(&protocolList, (void *) entries[count], ism_admin_mapItemComparator);
			count++;
		}
		if (count > 0) {
			 ism_common_listIterator iter;
			 ism_common_list_iter_init(&iter, &protocolList);

			while (ism_common_list_iter_hasNext(&iter)) {

				ism_common_list_node * node = ism_common_list_iter_next(&iter);
				ismHashMapEntry * entry =(ismHashMapEntry *)  node->data;
				int key_len = entry->key_len;
				void * key = entry->key;

				if(listOpt==0){
					if(name!=NULL){
						if(memcmp(name, key, key_len)){
							continue;
						}
					}

					if(arrayOpenBracketSet==0){
						ism_common_allocBufferCopyLen(output_buffer, "[",1);
						arrayOpenBracketSet =1;
					}

					if(dataFound>0)
							ism_common_allocBufferCopyLen(output_buffer, ",",1);

					ismSecProtocol * secprotocol = (ismSecProtocol *)entry->value;
					int capabilities = secprotocol->capabilities;

					ism_common_allocBufferCopyLen(output_buffer, "{",1);

					sprintf(buf, "\"Name\":\"");
					ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));

					memcpy(buf,key,key_len);
					buf[key_len]='\0';

					ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));


					sprintf(buf, "\", \"Capabilities\":%d", capabilities);
					ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));

					ism_common_allocBufferCopyLen(output_buffer, "}",1);
					dataFound++;

					if(name!=NULL){
						break;
					}

				}else if(listOpt==1){
					if(arrayOpenBracketSet==0){
						ism_common_allocBufferCopyLen(output_buffer, "[",1);
						arrayOpenBracketSet =1;
					}
					if(dataFound>0)
						ism_common_allocBufferCopyLen(output_buffer, ",",1);

					ism_common_allocBufferCopyLen(output_buffer, "{",1);

					sprintf(buf, "\"Name\":\"");
					ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));

					memcpy(buf,key,key_len);
					buf[key_len]='\0';

					ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
					ism_common_allocBufferCopyLen(output_buffer, "\"}",2);

					dataFound++;

				}

			}
			ism_common_list_iter_destroy(&iter);
		}

		ism_common_HashMapUnlock(ismSecProtocolMap);
		ism_common_freeHashMapEntriesArray(entries);
		ism_common_list_destroy(&protocolList);

		if(dataFound>0) ism_common_allocBufferCopyLen(output_buffer, "]",1);
	}

	if(dataFound==0){
		/*Set No Data */

		ism_common_setError(ISMRC_NotFound);
		return ISMRC_NotFound;
	}

	return ISMRC_OK;


}


/**
 * Process stop plugin request.
 */
XAPI int ism_admin_stopPlugin(void)
{
	protocolTermPlugin = (protocolTermPlugin_f)(uintptr_t)ism_common_getLongConfig("_ism_protocol_termPlugin_fnptr", 0L);
	protocolTermPlugin();
	return ISMRC_OK;
}

/**
 * Process stop plugin request.
 */
XAPI int ism_admin_startPlugin(void)
{
//	int changeCount=0;

	/*Apply Updates*/
//	changeCount = ism_config_applyPluginUpdates();

//    TRACE(7, "Call Protocol to apply Plugin updates. changeCount: %d.\n", changeCount);
    TRACE(7, "Call Protocol to start Plugin server.");
	
	protocolRestartPlugin = (protocolRestartPlugin_f)(uintptr_t)ism_common_getLongConfig("_ism_protocol_restartPlugin_fnptr", 0L);
	protocolRestartPlugin();
	return ISMRC_OK;
}

XAPI int ism_admin_getPluginStatus(void)
{
    protocolPluginStatus_f protocolPluginStatus = (protocolPluginStatus_f)(uintptr_t)ism_common_getLongConfig("_ism_protocol_isPluginServerRunning_fnptr", 0L);
    if(protocolPluginStatus)
        return protocolPluginStatus();
    return 0;
}




int ism_admin_processPluginNotification(ism_json_parse_t *json, concat_alloc_t *output_buffer)
{
	int rc=ISMRC_OK;

	char *notificationType = (char *)ism_json_getString(json, "NotificationType");

	if(notificationType!=NULL && strcasecmp(notificationType,"stop")==0){
		rc = ism_admin_stopPlugin();
	}else if(notificationType!=NULL && strcasecmp(notificationType,"start")==0){
	    //TODO: Do we really need start and restart actions?
		rc = ism_admin_startPlugin();
	}else if(notificationType!=NULL && strcasecmp(notificationType,"restart")==0){
		rc = ism_admin_startPlugin();
	} else{
		rc= ISMRC_NotFound;
	}

	/*Set Notification Ack*/
	char errstr [512];
	ism_common_getErrorString(rc, errstr, sizeof(errstr));
	ism_admin_setReturnCodeAndStringJSON(output_buffer, rc, errstr);

	return rc;
}

/**
 * Get Protocol Info
 * @param http		ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_admin_restapi_getProtocolsInfo(char *pname, ism_http_t *http)
{
	int dataFound=0;
	int arrayOpenBracketSet = 0;
	ism_common_list protocolList;
	int count=0;

	if (ism_common_getHashMapNumElements(ismSecProtocolMap)) {
		char buf[4096];

		ism_common_HashMapLock(ismSecProtocolMap);
		ismHashMapEntry **entries = ism_common_getHashMapEntriesArray(ismSecProtocolMap);

		/*Put in sorted list*/
		ism_common_list_init(&protocolList, 0, NULL);
		while(entries[count] != ((void*) -1)) {
			ism_common_list_insert_ordered(&protocolList, (void *) entries[count], ism_admin_mapItemComparator);
			count++;
		}

		TRACE(9, "%s: found %d protocols.\n", __FUNCTION__, count);
		if (count > 0) {
			 ism_common_listIterator iter;
			 ism_common_list_iter_init(&iter, &protocolList);

			while (ism_common_list_iter_hasNext(&iter)) {

				ism_common_list_node * node = ism_common_list_iter_next(&iter);
				ismHashMapEntry * entry =(ismHashMapEntry *)  node->data;
				int key_len = entry->key_len;
				void * key = entry->key;

				if(pname!=NULL){
					if(memcmp(pname, key, key_len)){
						continue;
					}
				}

				memset(buf, '\0', sizeof(buf));
				memcpy(buf,key,key_len);
				buf[key_len]='\0';

				TRACE(9, "%s: key is %s.\n", __FUNCTION__, buf);
				if(arrayOpenBracketSet==0){
					ism_json_putBytes(&http->outbuf, "{\n  \"Version\":\"");
					ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
					ism_json_putBytes(&http->outbuf, "\",\n  \"Protocol\": {");
					arrayOpenBracketSet =1;
				}

				if(dataFound>0)
					ism_json_putBytes(&http->outbuf, ",");

				ismSecProtocol * secprotocol = (ismSecProtocol *)entry->value;
				int capabilities = secprotocol->capabilities;

				//create an entry
				ism_json_putBytes(&http->outbuf, "\n    \"");
				ism_json_putEscapeBytes(&http->outbuf, buf, (int) strlen(buf));

				ism_json_putBytes(&http->outbuf, "\": {\n      \"UseTopic\":");
				ism_json_putBytes(&http->outbuf, (capabilities & ISM_PROTO_CAPABILITY_USETOPIC)?"true":"false");
				ism_json_putBytes(&http->outbuf, ",\n      \"UseQueue\":");
				ism_json_putBytes(&http->outbuf, (capabilities & ISM_PROTO_CAPABILITY_USEQUEUE)?"true":"false");
				ism_json_putBytes(&http->outbuf, ",\n      \"UseShared\":");
				ism_json_putBytes(&http->outbuf, (capabilities & ISM_PROTO_CAPABILITY_USESHARED)?"true":"false");
				ism_json_putBytes(&http->outbuf, ",\n      \"UseBrowse\":");
				ism_json_putBytes(&http->outbuf, (capabilities & ISM_PROTO_CAPABILITY_USEBROWSE)?"true":"false");

				ism_json_putBytes(&http->outbuf, "\n    }");
				dataFound++;

				if(pname!=NULL){
					break;
				}
			}
			ism_common_list_iter_destroy(&iter);
			if (arrayOpenBracketSet == 1)
				ism_json_putBytes(&http->outbuf, "\n  }");
		}

		ism_common_HashMapUnlock(ismSecProtocolMap);
		ism_common_freeHashMapEntriesArray(entries);
		ism_common_list_destroy(&protocolList);

		ism_json_putBytes(&http->outbuf, "\n}\n");
	}

	if(dataFound==0){
		/*Set No Data */
        TRACE(3, "%s: no protocol data has been found\n", __FUNCTION__ );
		ism_common_setError(ISMRC_NotFound);
		return ISMRC_NotFound;
	}

	return ISMRC_OK;
}

/**
 * Returns 1 if protocol is valid for a capability code - Topic, Queue, Shared, Browse.
 * If capcode is -1, then returns 1 if Protocol is valid
 */
XAPI int ism_config_checkProtocolsInfo(char *pname, int capcode)
{
	ism_common_list protocolList;
	int count=0;
	int retval = 0;

	if ( !pname ) return 0;

	if (ism_common_getHashMapNumElements(ismSecProtocolMap)) {

		ism_common_HashMapLock(ismSecProtocolMap);
		ismHashMapEntry **entries = ism_common_getHashMapEntriesArray(ismSecProtocolMap);

		/*Put in sorted list*/
		ism_common_list_init(&protocolList, 0, NULL);
		while(entries[count] != ((void*) -1)) {
			ism_common_list_insert_ordered(&protocolList, (void *) entries[count], ism_admin_mapItemComparator);
			count++;
		}

		TRACE(9, "%s: Check protocol %s in number of protocols=%d\n", __FUNCTION__, pname, count);

		if (count > 0) {
			 ism_common_listIterator iter;
			 ism_common_list_iter_init(&iter, &protocolList);

			while (ism_common_list_iter_hasNext(&iter)) {

				ism_common_list_node * node = ism_common_list_iter_next(&iter);
				ismHashMapEntry * entry =(ismHashMapEntry *)  node->data;
				int key_len = entry->key_len;
				void * key = entry->key;

				if (pname!=NULL){
					if (memcmp(pname, key, key_len)){
						continue;
					}
				}

				if ( capcode == -1 ) return 1;

				ismSecProtocol * secprotocol = (ismSecProtocol *)entry->value;
				int capabilities = secprotocol->capabilities;

				switch(capcode) {
				case ISM_PROTO_CAPABILITY_USETOPIC:
					if ( capabilities & ISM_PROTO_CAPABILITY_USETOPIC ) retval = 1;
					break;

				case ISM_PROTO_CAPABILITY_USEQUEUE:
					if ( capabilities & ISM_PROTO_CAPABILITY_USEQUEUE ) retval = 1;
					break;

				case ISM_PROTO_CAPABILITY_USESHARED:
					if ( capabilities & ISM_PROTO_CAPABILITY_USESHARED ) retval = 1;
					break;

				case ISM_PROTO_CAPABILITY_USEBROWSE:
					if ( capabilities & ISM_PROTO_CAPABILITY_USEBROWSE ) retval = 1;
					break;

				default:
					retval = 0;
				}
			}
			ism_common_list_iter_destroy(&iter);
		}

		ism_common_HashMapUnlock(ismSecProtocolMap);
		ism_common_freeHashMapEntriesArray(entries);
		ism_common_list_destroy(&protocolList);
	}

	return retval;
}
