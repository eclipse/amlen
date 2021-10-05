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


/*
 */
#include <ismutil.h>
#include <ismjson.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <pxtransport.h>
#include <log.h>
#include <throttle.h>
#include <pxrouting.h>
#include <sys/stat.h>
#ifndef NO_PXACT
#include <pxactivity.h>
#endif


static int notify_fd = 0;
static pthread_mutex_t notify_lock;
static pthread_mutex_t rulelock = PTHREAD_MUTEX_INITIALIZER;

#ifndef NO_PXACT
static int g_shutdown = 0;
static pthread_mutex_t pxactivity_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

extern int ism_log_setLogTableCleanUpInterval(int interval);
extern int ism_log_setSummarizeLogsInterval(int interval);
extern int ism_log_setSummarizeLogsEnable(int enabled);
extern int ism_log_startLogTableCleanUpTimerTask(void);
extern int ism_log_setSummarizeLogObjectTTL(int ttl);
extern int ism_proxy_setGWCleanupTime(int time);
extern int ism_proxy_setGWMaxActiveDevices(int maxDevices) ;
extern int ism_proxy_setTenantAlias(const char * str);
extern void ism_iotrest_setGwHTTPInboundEnable(int enabled);
extern void ism_iotrest_setHTTPOutboundEnable(int enabled);
extern tenant_fairuse_t * ism_proxy_makeFairUse(const char * name, const char * value);
extern int ism_route_setMsgRoutingEnabled(int value);
extern int ism_route_setMsgRoutingSysPropsEnabled(int value);
extern int ism_mhub_setMessageHubEnabled(int enabled);
extern int ism_mhub_setUseMHUBKafkaConnection(int enabled) ;
extern int ism_mqtt_setDiscardMsgsEnabled(int enabled);
extern int ism_mqtt_setDiscardMsgsSoftLimit(int limit);
extern int ism_mqtt_setDiscardMsgsHardLimit(int limit);
extern int ism_mqtt_setDiscardMsgsLogInterval(int interval);
extern int ism_mqtt_setSendGWPingREQ(int enabled);
extern int ism_transport_stopMessaging(void);
extern int ism_mhub_setMessageHubACK(int acks);
extern int ism_mhub_setMessageHubBatchSize(int batchSize);
extern int ism_mhub_setMessageHubBatchTimeMillis(int batchmillis);
extern int ism_mhub_setMessageHubBatchSizeBytes(int batchbytes);
extern bool ism_mqtt_isValidWillPolicy(int willPolicy) ;
extern int ism_mqtt_setWillValidation(int willValidation);
void ism_protocol_ensureBuffer(concat_alloc_t * buf, int len);
void ism_common_setDisableCRL(int disable);
void ism_common_setDisableCRLCallback(int (*)(ima_transport_info_t *));


int ism_tenant_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
int ism_proxy_config_mhub(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
int ism_transport_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
int ism_proxy_process_config(const char * name, const char * dname, int complex, int checkonly, int keepgoing);
int ism_proxy_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
int ism_proxy_setNoLog(const char * nolog);
int ism_proxy_makeClientClass(ism_json_parse_t * parseobj, int where, const char * name);
int ism_proxy_makeTopicRule(ism_json_parse_t * parseobj, int where, const char * name);
int ism_proxy_makeAuthentication(ism_json_parse_t * parseobj, int where, const char * name);
int ism_proxy_SetNoLog(const char * nolog);
int ism_transport_startMessaging(void);
int ism_monitor_setMeteringInterval(int interval);
ism_topicrule_t * ism_proxy_getTopicRule(const char * name);
int ism_monitor_enableDeviceStatusUpdate(int enable);
void ism_mqtt_setMQTTv5(int val);
int ism_kafka_setBroker(ism_json_entry_t * ents);
int ism_kafka_setMeteringTopic(const char * topic);
int ism_kafka_setUseKafkaMetering(int useKafka);
int ism_proxy_setSessionExpire(uint32_t time);
int ism_kafka_setKafkaAPIVersion(int api_version) ;
int ism_kafka_setIMMessagingTopic(const char * topic);
int ism_kafka_setUseKafkaIMMessaging(int useKafkaIMMessaging);
void ism_kafka_setIMMessagingSelector(const char * in_selector);
void ism_kafka_setKafkaIMConnDetailStatsEnabled(int enabled);
int ism_kafka_setKafkaIMProduceAck(int acks);
int ism_kafka_setKafkaIMBatchSize(int batchsize);
int ism_kafka_setKafkaIMBatchSizeBytes(int batchsizebytes);
int ism_kafka_setKafkaIMRecoverBatchSize(int batchsize);
int ism_kafka_setKafkaIMServerShutdownOnErrorTime(int time_in_secs);
int ism_mhub_parseBindings(ism_json_parse_t * parseobj, int where, const char * name);
int ism_mhub_parseCredentials(ism_json_parse_t * parseobj, int where, const char * name);
extern void ism_proxy_setDynamicTime(int millis);
extern void ism_proxy_setHashTime(int millis);
int ism_mhub_parseKafkaConnection(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing);
int ism_mhub_getKafkaConList(const char * match, ism_tenant_t * tenant, ism_json_t * jobj, const char * name);
int ism_transport_getEndpointList(const char * match, ism_json_t * jobj, int json, const char * name);

int ism_proxy_getAuthMask(const char * name);
const char * ism_json_getJsonValue(ism_json_entry_t * ent);

#ifndef NO_PXACT
static void  replaceString(const char * * oldval, const char * val);
#endif

#ifdef HAS_BRIDGE
int ism_bridge_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing);
int (* ism_bridge_swidtags_f)(void) = NULL;
#endif

ism_time_t g_dynamic_time = 0;    /* Time of last processing of dynamic config */
ism_time_t g_hash_time = 0;
int  g_dynamic_loglevel = 0;
int  g_dynamic_conloglevel = 0;
int  g_dynamic_tracelevel = 0;
int  g_dynamic_tracedata = 0;
int  g_dynamic_shutwait = 0;
int  g_dynamic_shutdelay = 0;
int  g_dynamic_license = 0;
#ifndef HAS_BRIDGE
int g_need_dyn_write = 0;
#endif

extern ism_routing_config_t *       g_routeconfig;
extern ism_routing_route_t * g_internalKafkaRoute;
extern const char * g_truststore;


typedef struct ism_notify_list_t {
    struct ism_notify_list_t * next;
    const char * path;
    const char * mask;
    int   handle;
    uint8_t  kind;
    uint8_t  resv[3];
} ism_notify_list_t;

#define PXNOTIFY_KIND_PROXY   0
#define PXNOTIFY_KIND_DYNAMIC 1
#define PXNOTIFY_KIND_TRUST   2

static ism_notify_list_t * notify_list = NULL;

ism_clientclass_t * ismClientClasses = NULL;
ism_topicrule_t * ismTopicRules = NULL;

/*
 * Aux Logger Settings ISM enum list
 */
ism_enumList enum_auxloggerp_settings [] = {
    { "Setting",    3,                  },
    { "Min",        AuxLogSetting_Min,      },
    { "Normal",     AuxLogSetting_Normal,   },
    { "Max",        AuxLogSetting_Max,      },

};

ism_enumList enum_licenses [] = {
    { "Setting",        4,                     },
    { "None",           LICENSE_None,          },
    { "Developers",     LICENSE_Developers,    },
    { "Non-Production", LICENSE_NonProduction, },
    { "Production",     LICENSE_Production,    },
};

typedef struct authmask_t {
    struct authmask_t * next;
    const  char *       name;
    uint32_t            mask;
} authmask_t;

authmask_t * authmask_head = NULL;
authmask_t * authmask_tail = NULL;

int g_licensedUsage = LICENSE_None;

#ifndef NO_PROXY
#include "topicrule.c"
#endif

#ifndef NO_PXACT

int pxactEnabled = 0;

void ism_proxy_setClientActivityMonitoring(int enable) {
	pxactEnabled = enable;
}

#endif


/*
 * Change the setting of an aux logger.
 * We assume that config has already done the
 */
void ism_proxy_setAuxLog(ism_domain_t * domain, int logger, ism_prop_t * props, const char * logname) {
    int auxsetting;
    const char * setting = ism_common_getStringProperty(props, logname);
    if (!setting)
        setting = "Normal";
    auxsetting = ism_common_enumValue(enum_auxloggerp_settings, setting);
    if (auxsetting != INVALID_ENUM) {
        domain->trace.logLevel[logger] = auxsetting;
        domain->selected.logLevel[logger] = auxsetting == AuxLogSetting_Max ? auxsetting : auxsetting+1;
        TRACE(6, "Set the log level: Domain=%s Log=%s Level=%s\n", domain->name, logname, setting);
    } else {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", logname, setting);
    }
}

/*
 * Add a notify
 * This is called with notify_lock held.
 */
int addNotify(const char * path, const char * mask, int kind) {
    ism_notify_list_t * notel = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,1),1, sizeof(ism_notify_list_t));
    notel->path = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_config,1000),path);
    notel->mask = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_config,1000),mask);
    notel->handle = inotify_add_watch(notify_fd, path, IN_MODIFY | IN_CREATE | IN_MOVED_TO | IN_MOVE_SELF | IN_MOVED_FROM | IN_DELETE);
    if (notel->handle < 0) {
        ism_common_free(ism_memory_proxy_config,notel);
        TRACE(2, "Unable to add notify for: %s\n", path);
        return -1;
    }
    notel->next = notify_list;
    notel->kind = kind;
    notify_list = notel;
    return 0;
}


/*
 * Find the notify entry
 */
ism_notify_list_t * findNotify(int handle) {
    ism_notify_list_t * notel = notify_list;
    while (notel) {
        if (notel->handle == handle)
            return notel;
        notel = notel->next;
    }
    return NULL;
}

/*
 * Add a notify for a single file.
 *
 * Since we want to know about cases where the file is moved in and out of the directory
 * we actually watch the directory and not the file.
 */
int ism_proxy_addNotifyDynamic(const char * name) {
    char * realname;
    char xbuf[4096];
    if (!name || !*name)
        return 1;
    realname = realpath(name, xbuf);
    if (realname) {
        char * pos = strrchr(realname, '/');
        *pos++ = 0;
        pthread_mutex_lock(&notify_lock);
        addNotify(*xbuf ? xbuf : "/", pos, PXNOTIFY_KIND_DYNAMIC);
        pthread_mutex_unlock(&notify_lock);
    } else {
        return 1;
    }
    return 0;
}


/*
 * Notify for the truststore change
 */
int ism_proxy_addNotifyTrust(const char * name) {
    char * realname;
    char xbuf[4096];
    if (!name || !*name)
        return 1;
    realname = realpath(name, xbuf);
    if (realname) {
        pthread_mutex_lock(&notify_lock);
        addNotify(realname, "*", PXNOTIFY_KIND_TRUST);
        pthread_mutex_unlock(&notify_lock);
    }
    return 0;
}

#ifndef NO_PROXY
/*
 * Set up a new config path.
 * The config path is a path (which can be relative to the current directory) and a file name mask.
 * The filename mask (final entry in path) can contain a wildcard, but the upper levels cannot.
 */
int newConfigPath(const char * cpath) {
    char   cwd[1024];
    char   fname [PATH_MAX];
    char * path;
    char * pp;
    char * mask;
    int    cwdlen;
    struct dirent * dents;
    DIR *  dir;

    /*
     * If the path is relative make it absolute but in any case make a copy of it.
     */
    if (*cpath != '/' && getcwd(cwd,sizeof cwd) != NULL) {
        cwdlen = strlen(cwd);
        path = alloca(cwdlen + strlen(cpath) + 2);
        strcpy(path, cwd);
        if (cwd[cwdlen-1] != '/') {
            path[cwdlen++] = '/';
        }
        strcpy(path+cwdlen, cpath);
    } else {
        path = alloca(strlen(cpath)+1);
        strcpy(path, cpath);
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
            TRACE(2, "ConfigPath entry is not valid as path contains an asterisk: %s\n", cpath);
            return 2;
        }
    } else {
        TRACE(2, "ConfigPath entry is not valid: %s\n", cpath);
        return 1;
    }


    dir = opendir(path);
    if (!dir) {
        TRACE(2, "ConfigPath entry does not exist or is not a directory: %s\n", cpath);
        return 3;
    }

    /* Lock */
    pthread_mutex_lock(&notify_lock);

    /*
     * Add this directory to the notify list
     */
    addNotify(path, mask, PXNOTIFY_KIND_PROXY);

    /*
     * Process the files in this directory now
     */
    dents = readdir(dir);
    while (dents) {
        if (dents->d_type != DT_DIR) {
            if (ism_common_match(dents->d_name, mask)) {
                snprintf(fname, sizeof fname, "%s/%s", path, dents->d_name);
                TRACE(4, "Config file: %s\n", fname);
                ism_proxy_process_config(fname, NULL, 2, 0, 1);
            }
        }
        dents = readdir(dir);
    }

    /* Unlock */
    pthread_mutex_unlock(&notify_lock);

    closedir(dir);
    return 0;
}


/*
 * Setup a config path
 */
int ism_proxy_addConfigPath(ism_json_parse_t * parseobj, int where) {
    int  rc = 0;
    ism_json_entry_t * ent;
    int endloc;

    if (!parseobj || where > parseobj->ent_count) {
        TRACE(2, "ConfigPath config JSON not correct\n");
        return 1;
    }
    ent = parseobj->ent + where;
    if (!ent->name || strcmp(ent->name, "ConfigPath") || ent->objtype != JSON_Array) {
        /* TODO: log if bad type */
        TRACE(2, "ConfigPath config JSON invoked for config which is not valid\n");
        return 2;
    }
    endloc = where + ent->count;
    where++;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if (ent->objtype == JSON_String) {
            int xrc = newConfigPath(ent->value);
            if (!rc)
                rc = xrc;
        } else {
            TRACE(2, "ConfigPath entry not a string\n");
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    return rc;
}
#endif

/*
 * Mask name for trace
 */
const char * maskname(uint32_t mask) {
    if (mask & IN_CREATE)
        return "create";
    if (mask & IN_MODIFY)
        return "modify";
    if (mask & IN_MOVED_TO)
        return "moveto";
    if (mask & IN_MOVED_FROM)
        return "movefrom";
    if (mask & IN_MOVE_SELF)
        return "move";
    if (mask & IN_DELETE)
        return "delete";
    return "none";
}


/*
 * Get the licensed usage
 */
int ism_proxy_getLicensedUsage(void) {
    return g_licensedUsage;
}

/*
 * Update the dynamic config if the last change was not ours
 */
static int updateDynamicConfig(ism_timer_t timer, ism_time_t now, void * userdata) {
    int  rc;
    char * fname = (char *)userdata;
    struct stat fstat;
    uint64_t ftime = 0;

    if (timer)
        ism_common_cancelTimer(timer);
    if (!stat(fname, &fstat)) {
        ftime = fstat.st_mtim.tv_sec;
        ftime = (ftime*1000000000L) + fstat.st_mtim.tv_nsec;
    }

    // printf("updated config file: %s  now=%0.9f file=%0.9f dyn=%0.9f\n",
    //       fname, (now * 1e-9), (ftime * 1e-9), (g_dynamic_time * 1e-9));
    if (ftime > g_dynamic_time) {
        ism_proxy_setDynamicTime(2000);    /* Ignore our own change */
        rc = ism_proxy_process_config(fname, NULL, 0x01, 0, 0);
        if (rc) {
            if (rc >= ISMRC_Error) {
                int  lastrc = rc;
                char * xbuf = alloca(4096);
                ism_common_formatLastError(xbuf, 4096);
                LOG(ERROR, Server, 946, "%s%s%u", "Error processing the modified dynamic config file: Name={0} RC={2} Error={1}",
                        fname, xbuf, lastrc);
            }
        } else {
            LOG(NOTICE, Server, 973,"%s", "The configuration is updated: {0}", fname);
        }
    }
    ism_proxy_setDynamicTime(0);   /* Set the process time of the dynamic config file */
    ism_common_free(ism_memory_proxy_config,userdata);
    return 0;
}

/*
 * Set the dynamic file update time.
 * When we are about to write the file, we set the time into the future so
 * we do not process our own change.
 */
void ism_proxy_setDynamicTime(int millis) {
    g_dynamic_time = ism_common_currentTimeNanos() + (millis * 1000000L);
}

/*
 * Timer job to update the trust store
 */
static int updateTruststore(ism_timer_t timer, ism_time_t now, void * userdata) {
    if (timer)
        ism_common_cancelTimer(timer);
    ism_common_hashTrustDirectory(g_truststore, 1, 0);
    ism_proxy_setHashTime(0);
    return 0;
}

/*
 * Set the hash update time
 * When we are about to write the file, we set the time into the future so
 * we do not process our own change.
 */
void ism_proxy_setHashTime(int millis) {
    g_hash_time = ism_common_currentTimeNanos() + (millis * 1000000L);
}


/*
 * Check for Bedrock changes to locale or timezone.
 * The notify_lock is used to stop us from getting notifies during the original config handling.
 */
void * ism_proxy_notify_thread(void * param, void * context, int value) {
    int   len;
    int   pos;
    char buf [8192];
    char fname[PATH_MAX];
    ism_notify_list_t * notel;

    notify_fd = inotify_init1(IN_CLOEXEC);
    for (;;) {
        len = read(notify_fd, buf, sizeof buf);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        pthread_mutex_lock(&notify_lock);
        pos = 0;
        while (pos < len) {
            struct inotify_event * event = (struct inotify_event *)(buf+pos);
            notel = findNotify(event->wd);
            if (notel) {
                TRACE(6, "inotify wd=%s mask=%s name=%s\n", notel->path, maskname(event->mask), event->name);
                if (event->len) {
                    if (ism_common_match(event->name, notel->mask)) {
                        // printf("File %s: %s/%s\n", maskname(event->mask), notel->path, event->name);
                        if (event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) {
                            snprintf(fname, sizeof fname, "%s/%s", notel->path, event->name);
                            if (notel->kind == PXNOTIFY_KIND_PROXY) {
                                TRACE(4, "Update config file: %s\n", fname);
                                ism_proxy_process_config(fname, NULL, 2, 0, 1);
                            }
                            if (notel->kind == PXNOTIFY_KIND_DYNAMIC) {
                                char * userdata = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_config,2),strlen(fname)+1);
                                strcpy(userdata, fname);
                                ism_common_setTimerOnce(ISM_TIMER_LOW, updateDynamicConfig, userdata, 100000000);   /* 100 ms */
                            }
                        }
                        if (notel->kind == PXNOTIFY_KIND_TRUST) {
                            if (g_hash_time < ism_common_currentTimeNanos()) {
                                ism_proxy_setHashTime(10000);
                                ism_common_setTimerOnce(ISM_TIMER_LOW, updateTruststore, NULL, 500000000);   /* 500 ms */
                            }
                        }
                    }
                }
            } else {
                TRACE(4, "Unknown inotify\n");
            }

            pos += sizeof(*event) + event->len;
        }
        pthread_mutex_unlock(&notify_lock);
    }
    return NULL;
}

/*
 * Callback from ssl.c to check for disableCRL
 */
int checkDisableCRL(ima_transport_info_t * transporti) {
    ism_transport_t * transport = (ism_transport_t *)transporti;
    ism_tenant_t * tenant = NULL;
    if (!transport || !transport->tenant) {
        if (transport->sniName) {
            ism_tenant_lock();
            tenant = ism_tenant_getTenant(transport->sniName);
            ism_tenant_unlock();
        }
    }
    return tenant ? (tenant->disableCRL==1) : 0;
}

/*
 * Init proxy notify
 */
void ism_proxy_notify_init(void) {
    ism_threadh_t thandle;
    pthread_mutex_init(&notify_lock, 0);
    ism_common_startThread(&thandle, ism_proxy_notify_thread, NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0,
            "pxnotify", "The change notification thread");
    ism_common_setDisableCRLCallback(checkDisableCRL);
}

#ifndef NO_PXACT
int ism_proxy_activity_start_thread(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	int rc = 0;

	if (g_shutdown) {
		return 0;
	}
	pthread_mutex_lock(&pxactivity_lock);
	if (!ism_pxactivity_is_started()) {
		rc = ism_pxactivity_start();
		TRACE(5, "ism_proxy_activity_start_thread: ism_pxactivity_start() RC = %d\n", rc);
		if (rc) {
			TRACE(5, "ism_proxy_activity_start_thread: ism_pxactivity_is_messaging_started()=%d ism_pxactivity_get_state()=%d\n",
					ism_pxactivity_is_messaging_started(), ism_pxactivity_get_state());
			if (!ism_pxactivity_is_messaging_started() && ism_pxactivity_get_state() == PXACT_STATE_TYPE_CONFIG) {
				g_shutdown = 1;
			}
		}
	} else {
		TRACE(5, "ism_proxy_activity_start_thread: already started.\n");
	}
	pthread_mutex_unlock(&pxactivity_lock);
	ism_common_cancelTimer(key);
	if (g_shutdown) {
		TRACE(1, "Terminate proxy RC = %d\n", rc);
		ism_transport_stopMessaging();
	    pid_t pid = getpid();
	    kill(pid, SIGTERM);
	}
	return rc;
}

void ism_proxy_activity_start(void) {
	ism_common_setTimerOnce(ISM_TIMER_LOW, ism_proxy_activity_start_thread, NULL, 1000);
}
#endif


/*
 * Known global properties for case independent fixup
 */
static const char * known_props [] = {
    "Affinity.",
    "ConfigPath",
    "ConnectionLogConcise",
    "ConnectionLogFile",
    "ConnectionLogIgnore",
    "ConnectionLogLevel",
    "FileLimit",
    "IODelay"
    "IOThread",
    "KeyStore",
    "LogFile",
    "LogLevel",
    "LogUnitTest",
    "MeteringInverval",
    "MqttProxyProtocol",
    "PeerPort",
    "StartMessaging",
    "TcpMaxCon",
    "TcpMaxListen",
    "TcpRecvSize",
    "TcpSendSize",
    "TraceFile",
    "TraceLevel",
    "TraceMax",
    "TraceMessageData",
    "TraceOpions",
    "TrustStore",
    "UserIsClientID",
    NULL,
};

/*
 * Convert to canonical name inplace
 */
static int canonicalName(char * name) {
    int  len;
    int  rc = 1;
    char * pos;
    char * pos2;
    const char * * prop = known_props;

    if (!name)
        return rc;

    pos = strchr(name, '.');
    if (pos) {
        pos2 = strchr(pos+1, '.');
        if (pos2)
            pos = pos2;
        len = (int)(pos-name)+1;
    } else {
        len = (int)strlen(name);
    }
    while (*prop) {
        if (!strncasecmp(*prop, name, len)) {
            memcpy(name, *prop, len);
            rc = 0;
            break;
        }
        prop++;
    }
    return rc;
}

/*
 * Start messaging
 */
static int startMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_transport_startMessaging();
    ism_common_cancelTimer(timer);
    return 0;
}


#ifndef NO_PXACT

static int init_called=0;
static ismPXActivity_ConfigActivityDB_t actDB[1];

/*
 * Parse ActivityDB Config from JSON Object
 */
 static int parseActivityDB(ism_json_parse_t * parseobj, int where) {
    int endloc;

    int  awhere;
    int  endarray;
    int  savewhere;
    int  rc = 0;
    int  hasAddress = actDB->pMongoEndpoints != NULL;
    int  hasURI = 0;
    ism_json_entry_t * ent;
    ism_json_entry_t * aent;
    char xbuf[1024];
    char jxbuf[1024];
    concat_alloc_t jbuf = {jxbuf, sizeof jxbuf};

    if (!parseobj || where > parseobj->ent_count)
        return 1;

    ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if (!strcmpi(ent->name, "Address")) {
            if (ent->objtype != JSON_Array) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Address", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } else {
                hasAddress = 1;
                awhere = where+1;
                endarray = awhere + ent->count;
                while (awhere < endarray) {
                    aent = parseobj->ent + awhere;
                    if (aent->objtype != JSON_String) {
                        /* TODO: check for URI auth */
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Address", ism_json_getJsonValue(ent));
                        rc = ISMRC_BadPropertyValue;
                    }
                    awhere++;
                }
            }
        } else if (!strcmpi(ent->name, "Type")) {
            if (ent->objtype != JSON_String || strcmp(ent->value,"MongoDB")) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Type", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "UseTLS")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False &&
                (ent->objtype != JSON_Integer || ent->count != 2)) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "UseTLS", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "URI")) {
            hasURI = 1;
            if (ent->objtype != JSON_String ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "URI", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } 
        } else if (!strcmpi(ent->name, "URIOptions")) {
            if (ent->objtype != JSON_String ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "URI", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "TrustStore")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } 
        } else if (!strcmpi(ent->name, "User")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } 
        } else if (!strcmpi(ent->name, "Password")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "AuthDB")) {
                    if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "AuthDB", ism_json_getJsonValue(ent));
                        rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "Name")) {
            if (ent->objtype != JSON_String ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Name", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "ClientState")) {
            if (ent->objtype != JSON_String ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ClientState", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "ProxyState")) {
            if (ent->objtype != JSON_String ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ProxyState", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "MemoryLimitPct")) {
            if (ent->objtype != JSON_Integer && ent->objtype != JSON_Number ) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ProxyState", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }            
        } else if (!strcmpi(ent->name, "WConcern") ||
                   !strcmpi(ent->name, "WJournal") ||
                   !strcmpi(ent->name, "RConcern") ||
                   !strcmpi(ent->name, "Rpref") ||
                   !strcmpi(ent->name, "WorkerThreads") ||
                   !strcmpi(ent->name, "BanksPerThread") ||
                   !strcmpi(ent->name, "WriteSize") ||
                   !strcmpi(ent->name, "hbTimeoutMS") ||
                   !strcmpi(ent->name, "hbIntervalMS") ||
                   !strcmpi(ent->name, "dbRateLimitGlobalEnabled") ||
                   !strcmpi(ent->name, "dbIOPSRateLimit") ||
				   !strcmpi(ent->name, "MonitoringPolicyAppClientClass") ||
				   !strcmpi(ent->name, "MonitoringPolicyAppScaleClientClass") ||
				   !strcmpi(ent->name, "MonitoringPolicyConnectorClientClass") ||
				   !strcmpi(ent->name, "MonitoringPolicyDeviceClientClass") ||
				   !strcmpi(ent->name, "MonitoringPolicyGatewayClientClass") ||
				   !strcmpi(ent->name, "ActivityShutdownWait")) {
            if (ent->objtype != JSON_Integer) {
            	ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "OtherInteger", ism_json_getJsonValue(ent));
            	rc = ISMRC_BadPropertyValue;
            }
        } else {
            ism_common_setErrorData(ISMRC_BadPropertyName, "%s", ent->name);
            rc = ISMRC_BadPropertyName;
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    if (hasAddress && hasURI) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "URI", ism_json_getJsonValue(ent));
        rc = ISMRC_BadPropertyValue;
    }

    if (rc == 0) {
    	if ( !init_called ) {
    		init_called++;
    		ism_pxactivity_ConfigActivityDB_Init(actDB);
    	}
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmpi(ent->name, "Address")) {
                if (ent->count) {
                    //replaceString(&actDB->pMongoURI, NULL); //PXACT_REM
                    awhere = where+1;
                    endarray = awhere + ent->count;
                    int entnum = 0;
                    ism_json_putBytes(&jbuf, "[");
                    while (awhere < endarray) {
                        aent = parseobj->ent + awhere;
                        if (entnum>0)
                            ism_json_putBytes(&jbuf, ",");
                        ism_json_putString(&jbuf, aent->value);
                        awhere++;
                        entnum++;
                    }
                    ism_common_allocBufferCopyLen(&jbuf, "]" , 2);
                    replaceString(&actDB->pMongoEndpoints, jbuf.buf);
                    awhere++;
                    entnum++;
                }
            } else if (!strcmpi(ent->name, "Type")) {
                replaceString(&actDB->pDBType, ent->value);
            } else if (!strcmpi(ent->name, "UseTLS")) {
                if (ent->objtype == JSON_Integer)
                    actDB->useTLS = ent->count;
                else
                    actDB->useTLS = ent->objtype == JSON_True;
            } else if (!strcmpi(ent->name, "URIOptions")) {
                replaceString(&actDB->pMongoOptions, ent->value);
            } else if (!strcmpi(ent->name, "TrustStore")) {
                replaceString(&actDB->pTrustStore, ent->value);
            } else if (!strcmpi(ent->name, "User")) {
            	replaceString(&actDB->pMongoUser, ent->value);
            } else if (!strcmpi(ent->name, "Password")) {
            	replaceString(&actDB->pMongoPassword, ent->value);
            } else if (!strcmpi(ent->name, "AuthDB")) {
                replaceString(&actDB->pMongoAuthDB, ent->value);
            } else if (!strcmpi(ent->name, "Name")) {
            	replaceString(&actDB->pMongoDBName, ent->value);
            } else if (!strcmpi(ent->name, "ClientState")) {
            	replaceString(&actDB->pMongoClientStateColl, ent->value);
            } else if (!strcmpi(ent->name, "ProxyState")) {
            	replaceString(&actDB->pMongoProxyStateColl, ent->value);
            } else if (!strcmpi(ent->name, "Wconcern")) {
            	actDB->mongoWconcern = ent->count;
            } else if (!strcmpi(ent->name, "Wjournal")) {
            	actDB->mongoWjournal = ent->count;
            } else if (!strcmpi(ent->name, "Rconcern")) {
            	actDB->mongoRconcern = ent->count;
            } else if (!strcmpi(ent->name, "Rpref")) {
            	actDB->mongoRpref = ent->count;
            } else if (!strcmpi(ent->name, "WorkerThreads")) {
            	actDB->numWorkerThreads = ent->count;
            } else if (!strcmpi(ent->name, "BanksPerThread")) {
            	actDB->numBanksPerWorkerThread = ent->count;
            } else if (!strcmpi(ent->name, "WriteSize")) {
            	actDB->bulkWriteSize = ent->count;
            } else if (!strcmpi(ent->name, "hbTimeoutMS")) {
            	actDB->hbTimeoutMS = ent->count;
            } else if (!strcmpi(ent->name, "hbIntervalMS")) {
            	actDB->hbIntervalMS = ent->count;
            } else if (!strcmpi(ent->name, "dbIOPSRateLimit")) {
            	actDB->dbIOPSRateLimit = ent->count;
            } else if (!strcmpi(ent->name, "dbRateLimitGlobalEnabled")) {
            	actDB->dbRateLimitGlobalEnabled = ent->count;
            } else if (!strcmpi(ent->name, "ActivityShutdownWait")) {
                actDB->terminationTimeoutMS = ent->count;
            } else if (!strcmpi(ent->name, "MemoryLimitPct")) {
            	actDB->memoryLimitPercentOfTotal = strtof(ent->value,NULL);
            } else if (!strcmpi(ent->name, "MonitoringPolicyAppClientClass")) {
                actDB->monitoringPolicyAppClientClass = ent->count;
            } else if (!strcmpi(ent->name, "MonitoringPolicyAppScaleClientClass")) {
                actDB->monitoringPolicyAppScaleClientClass = ent->count;
            } else if (!strcmpi(ent->name, "MonitoringPolicyConnectorClientClass")) {
                actDB->monitoringPolicyConnectorClientClass = ent->count;
            } else if (!strcmpi(ent->name, "MonitoringPolicyDeviceClientClass")) {
                actDB->monitoringPolicyDeviceClientClass = ent->count;
            } else if (!strcmpi(ent->name, "MonitoringPolicyGatewayClientClass")) {
                actDB->monitoringPolicyGatewayClientClass = ent->count;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }

        /*
         * Set defaulted and computed values
         */
        if (actDB->pDBType == NULL)
            replaceString(&actDB->pDBType, "MongoDB");

        ism_common_freeAllocBuffer(&jbuf);

        /*
         * Update the activity monitoring if the server location is filled in
         */
        //if (actDB->pMongoEndpoints || actDB->pMongoURI) {
        if (actDB->pMongoEndpoints) {
            rc = ism_pxactivity_ConfigActivityDB_Set(actDB);
            if (rc) {
                if (rc != ISMRC_NullPointer) {
                    TRACE(1," ism_pxactivity_ConfigActivityDB_Set failed; rc=%u\n",rc);
                } else {
                    rc = 0;
                }
            }
	    }
    } else {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        LOG(ERROR, Server, 950, "%s-%u", "ActivityDB configuration error: Error={0} Code={1}",
                            xbuf, ism_common_getLastError());
    }

    return rc;
}
 
 
 /*
  * Parse the ActivityMonitoring object.
  * This is allowed to be within an inner object named actDB, or at this object
  */
int ism_proxy_activity_json(ism_json_parse_t * parseobj, int where) {
	int   rc = 0;
	ism_json_entry_t * ent;

	if (!pxactEnabled) {
		TRACE(5,"ActivityMonitoringEnable is disabled in static config.\n");
		return 0;
	}

	TRACE(5,"Proxy Activity: Parsing ActivityDB Configuration.\n");

	if (!parseobj || where > parseobj->ent_count) {
		TRACE(2, "Proxy config JSON not correct\n");
		return 1;
	}

	ent = parseobj->ent+where;

	ent++;
	if (ent->name && !strcmp(ent->name, "actDB")) {
	    rc = parseActivityDB(parseobj, where+1);
	} else {
	    rc = parseActivityDB(parseobj, where);
	}
	return rc;

}
#endif

void ism_bridge_changeLicense(int old, int new);

/*
 * Set licensed usage
 */
int ism_bridge_setLicensedUsage(const char * lictype) {
    ism_field_t var;

    int usage = ism_common_enumValue(enum_licenses, lictype);
    if (usage == INVALID_ENUM) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "LicensedUsage", lictype);
        return ISMRC_BadPropertyValue;
    }
    var.type = VT_String;
    var.val.s = (char *)ism_common_enumName(enum_licenses, usage);
    ism_common_setProperty(ism_common_getConfigProperties(), "LicensedUsage", &var);
    if (usage != g_licensedUsage) {
#ifdef HAS_BRIDGE
        int old = g_licensedUsage;
        g_licensedUsage = usage;
        ism_bridge_changeLicense(old, usage);

        /* TODO update ITLM files */
        if (ism_bridge_swidtags_f)
            ism_bridge_swidtags_f();
#else
        g_licensedUsage = usage;
#endif
    }
    return 0;
}

/*
 * Process a complex JSON config DOM
 */
int ism_proxy_complexConfig(ism_json_parse_t * parseobj, int complex, int checkonly, int keepgoing) {
    int  rc = 0;
    int  xrc;
    ism_field_t var = {0};
    int entloc = 1;
    int needlog = 1;
#ifndef HAS_BRIDGE
#ifndef NO_PXACT
    xUNUSED int pxact = 0;
#endif
#endif
    while ((rc == 0 || keepgoing) && entloc < parseobj->ent_count) {
        ism_json_entry_t * ent = parseobj->ent+entloc;
        switch(ent->objtype) {
        /* Ignore simple objects */
        case JSON_String:  /* JSON string, value is UTF-8  */
#ifdef HAS_BRIDGE
            /* In bridge, allow TraceLevel and LogLevel to be in dynamic config */
            if (complex == 1) {
                if (!strcmp(ent->name, "LicensedUsage")) {
                    rc = ism_bridge_setLicensedUsage(ent->value);
                    if (rc)
                        g_dynamic_license = 1;
                } else if (!strcmp(ent->name, "TraceLevel")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_common_setTraceLevelX(ism_defaultTrace, ent->value);
                    g_dynamic_tracelevel = 1;
                } else if (!strcmp(ent->name, "LogLevel")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_proxy_setAuxLog(&ism_defaultDomain, LOGGER_SysLog, ism_common_getConfigProperties(), "LogLevel");
                    g_dynamic_loglevel = 1;
                } else {
                    rc = ISMRC_NotFound;
                }
            }
#endif
            /* Process runtime changes */
            if (complex == 2) {
#ifndef HAS_BRIDGE
                canonicalName((char *)ent->name);
#endif
                if (!strcmp(ent->name, "TraceLevel")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_common_setTraceLevelX(ism_defaultTrace, ent->value);
#ifdef HAS_BRIDGE
                    g_dynamic_tracelevel = 2;
                } else if (!strcmp(ent->name, "LicensedUsage")) {
                    rc = ism_bridge_setLicensedUsage(ent->value);
                    if (rc == 0)
                        g_dynamic_license = 1;
#endif
#ifndef NO_PROXY
                } else if (!strcmp(ent->name, "ConnectionLogLevel")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_proxy_setAuxLog(&ism_defaultDomain, LOGGER_Connection, ism_common_getConfigProperties(), "ConnectionLogLevel");
#endif
                } else if (!strcmp(ent->name, "LogLevel")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_proxy_setAuxLog(&ism_defaultDomain, LOGGER_SysLog, ism_common_getConfigProperties(), "LogLevel");
#ifdef HAS_BRIDGE
                    g_dynamic_loglevel = 2;
#endif
                } else if (!strcmp(ent->name, "TraceSelected")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_common_setTraceLevelX(&ism_defaultDomain.selected, ent->value);
                } else if (!strcmp(ent->name, "TraceConnection")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_common_setTraceConnection(ent->value);
#ifndef NO_PROXY
                } else if (!strcmp(ent->name, "ConnectionLogIgnore")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_proxy_setNoLog(ent->value);
                } else if (!strcmp(ent->name, "KafkaMeteringTopic")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_kafka_setMeteringTopic(ent->value);
                } else if (!strcmp(ent->name, "KafkaIMMessagingTopic")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_kafka_setIMMessagingTopic(ent->value);
                } else if (!strcmp(ent->name, "KafkaIMMessagingSelector")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_kafka_setIMMessagingSelector(ent->value);
                } else if (!strcmp(ent->name, "TenantAlias")) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_proxy_setTenantAlias(ent->value);
                } else if (strlen(ent->name) >= 14 && !memcmp(ent->name, "FairUsePolicy.", 14)) {
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_proxy_makeFairUse(ent->name+14, ent->value);
#endif
                } else {
                    rc = ISMRC_BadPropertyName;
                    ism_common_setErrorData(rc, "%s", ent->name);
                }
            }
            entloc++;
            break;
        case JSON_Integer: /* Number with no decimal point             */
            if (complex == 0) {
#ifndef NO_PXACT
                if (!strcmpi(ent->name, PXACT_CFG_ENABLE_ACTIVITY_TRACKING)) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), PXACT_CFG_ENABLE_ACTIVITY_TRACKING, &var);
                }
#endif
            }

            /* Process runtime changes */
            if (complex == 2) {
                if (!strcmpi(ent->name, "TraceMessageData")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    ism_common_setTraceMsgData(ent->count);
                } else if (!strcmpi(ent->name, "StartMessaging")) {
                    if (ent->count > 0) {
                        ism_common_setTimerOnce(ISM_TIMER_HIGH, startMessagingTimer, NULL, ((uint64_t)ent->count) * 1000000);
                    }
#ifndef NO_PROXY
                } else if (!strcmpi(ent->name, "MQTTv5")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MQTTv5", &var);
                    ism_mqtt_setMQTTv5(ent->count);
                } else if (!strcmpi(ent->name, "MeteringInterval")) {
                    ism_common_setProperty(ism_common_getConfigProperties(), "MeteringIntervalSec", NULL);
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MeteringInterval", &var);
                    ism_monitor_setMeteringInterval(ent->count * 60);
                } else if (!strcmpi(ent->name, "MeteringIntervalSec")) {
                    ism_common_setProperty(ism_common_getConfigProperties(), "MeteringInterval", NULL);
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MeteringIntervalSec", &var);
                    ism_monitor_setMeteringInterval(ent->count);
                } else if (!strcmpi(ent->name, "DelayTableCleanUpInterval")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "DelayTableCleanUpInterval", &var);
                    if(ism_throttle_isEnabled())
                    	ism_throttle_startDelayTableCleanUpTimerTask();
                } else if (!strcmpi(ent->name, "ThrottleObjectTTL")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "ThrottleObjectTTL", &var);
                    if(ism_throttle_isEnabled())
                    	ism_throttle_setObjectTTL(ent->count);
                } else if (!strcmpi(ent->name, "Throttle.Frequency") || !strcmpi(ent->name, "ThrottleFrequency")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(),"Throttle.Frequency", &var);
                    ism_throttle_setFrequency(ent->count);
                } else if (!strcmpi(ent->name, "Throttle.Enabled") || !strcmpi(ent->name, "ThrottleEnabled")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "Throttle.Enabled", &var);
                    ism_throttle_setEnabled(ent->count);
                } else if (!strcmpi(ent->name, "LogTableCleanUpInterval")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "LogTableCleanUpInterval", &var);
                    ism_log_setLogTableCleanUpInterval(ent->count);
                    ism_log_startLogTableCleanUpTimerTask();
                } else if (!strcmpi(ent->name, "SummarizeLogObjectTTL")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "SummarizeLogObjectTTL", &var);
                    ism_log_setSummarizeLogObjectTTL(ent->count);
                } else if (!strcmpi(ent->name, "SummarizeLogs.Interval")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "SummarizeLogs.Interval", &var);
                    ism_log_setSummarizeLogsInterval(ent->count);
                } else if (!strcmpi(ent->name, "SummarizeLogs.Enabled")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "SummarizeLogs.Enabled", &var);
                    ism_log_setSummarizeLogsEnable(ent->count);
                } else if (!strcmpi(ent->name, "DeviceStatusUpdateEnabled")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "DeviceStatusUpdateEnabled", &var);
                    ism_monitor_enableDeviceStatusUpdate(ent->count);
                } else if (!strcmpi(ent->name, "DisableCRL")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "DisableCRL", &var);
                    ism_common_setDisableCRL(ent->count);
                }  else if (!strcmpi(ent->name, "UseKafkaMetering")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "UseKafkaMetering", &var);
                    ism_kafka_setUseKafkaMetering(ent->count);
                } else if (!strcmpi(ent->name, "UseKafkaIMMessaging")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "UseKafkaIMMessaging", &var);
                    ism_kafka_setUseKafkaIMMessaging(ent->count);
                } else if (!strcmpi(ent->name, "KafkaIMConnDetailStatsEnabled")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "KafkaIMConnDetailStatsEnabled", &var);
                    ism_kafka_setKafkaIMConnDetailStatsEnabled(ent->count);
                } else if (!strcmpi(ent->name, "KafkaIMProduceAck")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "KafkaIMProduceAck", &var);
                    ism_kafka_setKafkaIMProduceAck(ent->count);
                } else if (!strcmpi(ent->name, "KafkaIMMessagingBatchSize")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "KafkaIMMessagingBatchSize", &var);
                    ism_kafka_setKafkaIMBatchSize(ent->count);
                } else if (!strcmpi(ent->name, "KafkaIMMessagingBatchSizeBytes")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "KafkaIMMessagingBatchSizeBytes", &var);
                    ism_kafka_setKafkaIMBatchSizeBytes(ent->count);
                } else if (!strcmpi(ent->name, "KafkaIMMessagingRecoveryBatchSize")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "KafkaIMMessagingRecoveryBatchSize", &var);
                    ism_kafka_setKafkaIMRecoverBatchSize(ent->count);
                }  else if (!strcmpi(ent->name, "KafkaIMServerShutdownOnErrorTime")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "KafkaIMServerShutdownOnErrorTime", &var);
                    ism_kafka_setKafkaIMServerShutdownOnErrorTime(ent->count);
#endif
#ifndef NO_PXACT
                } else if (!strcmpi(ent->name, PXACT_CFG_ENABLE_ACTIVITY_TRACKING)) {
            			pxact = 1;
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), PXACT_CFG_ENABLE_ACTIVITY_TRACKING, &var);
#endif
                } else if (!strcmpi(ent->name, "ShutdownWait")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "ShutdownWait", &var);
                } else if (!strcmpi(ent->name, "ShutdownDelay")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "ShutdownDelay", &var);
#ifndef NO_PROXY
                }  else if (!strcmpi(ent->name, "MaxSessionExpiryInterval")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MaxSessionExpiryInterval", &var);
                    ism_proxy_setSessionExpire(ent->count);
                }  else if (!strcmpi(ent->name, "gwCleanupTime")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "gwCleanupTime", &var);
                    ism_proxy_setGWCleanupTime(ent->count);
                } else if (!strcmpi(ent->name, "gwMaxActiveDevices")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "gwMaxActiveDevices", &var);
                    ism_proxy_setGWMaxActiveDevices(ent->count);
                } else if (!strcmpi(ent->name, "gwHTTPInboundEnabled")) {
                	/* No longer exists */
                } else if (!strcmpi(ent->name, "HTTPOutboundEnabled")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "HTTPOutboundEnabled", &var);
                    ism_iotrest_setHTTPOutboundEnable(ent->count);
                } else if (!strcmpi(ent->name, "KafkaAPIVersion")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "KafkaAPIVersion", &var);
					ism_kafka_setKafkaAPIVersion(ent->count);
                } else if (!strcmpi(ent->name, "ProxyMsgRoutingEnabled")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "ProxyMsgRoutingEnabled", &var);
					ism_route_setMsgRoutingEnabled(ent->count);
                } else if (!strcmpi(ent->name, "ProxyMsgRoutingSysPropsEnabled")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "ProxyMsgRoutingSysPropsEnabled", &var);
					ism_route_setMsgRoutingSysPropsEnabled(ent->count);
                } else if (!strcmpi(ent->name, "MessageHubEnabled")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "MessageHubEnabled", &var);
					ism_mhub_setMessageHubEnabled(ent->count);
                } else if (!strcmpi(ent->name, "UseMHUBKafkaConnection")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "UseMHUBKafkaConnection", &var);
					ism_mhub_setUseMHUBKafkaConnection(ent->count);
                } else if (!strcmpi(ent->name, "MessageHubACKs")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MessageHubACKs", &var);
                    ism_mhub_setMessageHubACK(ent->count);
                } else if (!strcmpi(ent->name, "MessageHubBatchSize")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MessageHubBatchSize", &var);
                    ism_mhub_setMessageHubBatchSize(ent->count);
                } else if (!strcmpi(ent->name, "MessageHubBatchTimeMillis")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MessageHubBatchTimeMillis", &var);
                    ism_mhub_setMessageHubBatchTimeMillis(ent->count);
                } else if (!strcmpi(ent->name, "MessageHubBatchSizeBytes")) {
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), "MessageHubBatchSizeBytes", &var);
                    ism_mhub_setMessageHubBatchSizeBytes(ent->count);
                } else if (!strcmpi(ent->name, "DiscardMsgsEnabled")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "DiscardMsgsEnabled", &var);
					ism_mqtt_setDiscardMsgsEnabled(ent->count);
                } else if (!strcmpi(ent->name, "DiscardMsgsLogInterval")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "DiscardMsgsLogInterval", &var);
					ism_mqtt_setDiscardMsgsLogInterval(ent->count);
                } else if (!strcmpi(ent->name, "DiscardMsgsSoftLimit")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "DiscardMsgsSoftLimit", &var);
					ism_mqtt_setDiscardMsgsSoftLimit(ent->count);
                } else if (!strcmpi(ent->name, "DiscardMsgsHardLimit")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "DiscardMsgsHardLimit", &var);
					ism_mqtt_setDiscardMsgsHardLimit(ent->count);
                } else if (!strcmpi(ent->name, "SendGWPingREQ")) {
					var.type = VT_Integer;
					var.val.i = ent->count;
					ism_common_setProperty(ism_common_getConfigProperties(), "SendGWPingREQ", &var);
					ism_mqtt_setSendGWPingREQ(ent->count);
                } else if (!strcmpi(ent->name, "WillTopicValidationPolicy")) {
                    if (ism_mqtt_isValidWillPolicy(ent->count)) {
                        var.type = VT_Integer;
                        var.val.i = ent->count;
                        ism_common_setProperty(ism_common_getConfigProperties(), "WillTopicValidationPolicy", &var);
                        ism_mqtt_setWillValidation(ent->count);
                    }
#endif
                } else {
                    rc = ISMRC_NotFound;
                }
            }
            entloc++;
            break;
        case JSON_Number:  /* Number which is too big or has a decimal */
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
        case JSON_Null:
            if (complex == 2) {
                rc = ISMRC_NotFound;
            }
            entloc++;
            break;
        case JSON_Object:  /* JSON object, count is number of entries  */
            if (ent->name) {
                if (!strcmp(ent->name, "Endpoint")) {
                    xrc = ism_transport_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0) {
                        rc = xrc;
                        needlog = 0;
                    } else {
                        needlog = 1;
                    }
                } else if (!strcmp(ent->name, "User")) {
                    xrc = ism_tenant_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0) {
                        rc = xrc;
                        needlog = 0;
                    } else {
                        needlog = 1;
                    }
#ifndef NO_PROXY
                } else if (!strcmp(ent->name, "Server")) {
                    xrc = ism_tenant_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0)
                        rc = xrc;
                } else if (!strcmp(ent->name, "Tenant")) {
                    /* Handle this later */
                } else if (!strcmp(ent->name, "ClientClass")) {
                    xrc = ism_proxy_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0)
                        rc = xrc;
                } else if (!strcmp(ent->name, "TopicRule")) {
                    xrc = ism_proxy_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0)
                        rc = xrc;
                } else if (!strcmp(ent->name, "Authentication")) {
                    xrc = ism_proxy_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0)
                        rc = xrc;
				} else if (!strcmp(ent->name, "MessageHubBindings")) {
                     /* handle after Tenant Processing */
                } else if (!strcmp(ent->name, "MessageHubCredentials")) {
                    /* handle after Tenant Processing */
                } else if (!strcmp(ent->name, "KafkaConnection")) {
                    /* Handle after Tenant Processing */

#endif
#ifdef HAS_BRIDGE
                } else if (!strcmp(ent->name, "Forwarder") || !strcmp(ent->name, "Connection")) {
                    xrc = ism_bridge_config_json(parseobj, entloc, checkonly, keepgoing);
                    if (rc == 0) {
                        rc = xrc;
                        needlog = 0;
                    } else {
                        needlog = 1;
                    }
#endif
#ifndef NO_PROXY
                } else if (!strcmp(ent->name, "RouteConfig")) {
                    if(g_routeconfig==NULL){
                	    g_routeconfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,3),1, sizeof(ism_routing_config_t));
                		ism_route_config_init(g_routeconfig);
                		g_routeconfig->enabled=1;
                	}
                    xrc = ism_proxy_route_json(parseobj, entloc, g_routeconfig);
                    if (rc == 0){
                        rc = xrc;
                        //iterate thru Rules. If no Route, Assign Default Route
                        if(g_routeconfig && g_routeconfig->inited){
                            int i=0;
                            ismHashMapEntry ** array;
                            ism_routing_rule_t * routeRule;
                            pthread_rwlock_wrlock(&g_routeconfig->lock);
                            array = ism_common_getHashMapEntriesArray(g_routeconfig->routerulemap);
                            while(array[i] != ((void*)-1)){
                                routeRule = (ism_routing_rule_t *)array[i]->value;
                                if(routeRule->routes){
                                int numRoute = ism_common_getArrayNumElements(routeRule->routes);

                                if(numRoute==0){
                                    if(g_internalKafkaRoute!=NULL){
                                        TRACE(5, "Assign default route to rule: rulename=%s routeName=%s\n",routeRule->name,  g_internalKafkaRoute->name);
                                        ism_common_addToArray(routeRule->routes, g_internalKafkaRoute);
                                    }else{
                                        TRACE(5, "Assign default route to rule. global route is not set yet: rulename=%s\n",routeRule->name);
                                    }
                                }

                                }
                                i++;
                            }
                            pthread_rwlock_unlock(&g_routeconfig->lock);
                            ism_common_freeHashMapEntriesArray(array);
                        }
                    }
#endif
#ifndef NO_PXACT
                } else if (!strcmp(ent->name, "ActivityMonitoring") ||
                        !strcmp(ent->name, "ActivityDB")) {   /* TEMP: Allow legacy name */
                    xrc = ism_proxy_activity_json(parseobj, entloc);
                    pxact = 1;
                    if (rc == 0)
                        rc = xrc;
#endif
                }  else {
                    rc = ISMRC_BadPropertyName;
                    ism_common_setErrorData(rc, "%s", ent->name);
                }
            } else {
                if (complex == 2)
                    rc = ISMRC_BadPropertyName;
                    ism_common_setErrorData(rc, "%s", ent->name);
            }
            entloc += ent->count + 1;
            break;
        case JSON_Array:   /* JSON array, count is number of entries   */
            if (complex > 0) {
#ifndef NO_PROXY
                if (!strcmp(ent->name, "ConfigPath")) {
                } else if (!strcmp(ent->name, "KafkaBroker")) {
                    rc = ism_kafka_setBroker(ent);
                } else
#endif
                {
                    rc = ISMRC_BadPropertyName;
                    ism_common_setErrorData(rc, "%s", ent->name);
                }
            }
            entloc += ent->count + 1;
            break;
        }
    }

    /*
     * One more pass and process tenant config.
     * This is done to allow the Server config to be in the same file but
     * after the Tenant config.
     */
    if (rc == 0 || keepgoing) {
        /* Process tenant */
        entloc = 1;
        while (entloc < parseobj->ent_count) {
            ism_json_entry_t * ent = parseobj->ent+entloc;
            switch(ent->objtype) {
            default:
                entloc++;
                break;
            case JSON_Object:  /* JSON object, count is number of entries  */
                if (ent->name) {
                    if (!strcmp(ent->name, "Tenant")) {
                        xrc = ism_tenant_config_json(parseobj, entloc, checkonly, keepgoing);
                        if (rc == 0)
                            rc = xrc;
                    } else if (!strcmp(ent->name, "MessageHubBindings")) {
                        xrc = ism_proxy_config_mhub(parseobj, entloc, checkonly, keepgoing);
                        if (rc == 0)
                            rc = xrc;
                    } else if (!strcmp(ent->name, "MessageHubCredentials")) {
                        xrc = ism_proxy_config_mhub(parseobj, entloc, checkonly, keepgoing);
                        if (rc == 0)
                            rc = xrc;
                    } else if (!strcmp(ent->name, "KafkaConnection")) {
                        xrc = ism_proxy_config_mhub(parseobj, entloc, checkonly, keepgoing);
                        if (rc == 0)
                            rc = xrc;
                    }
                }
                entloc += ent->count + 1;
                break;
            case JSON_Array:   /* JSON array, count is number of entries   */
                entloc += ent->count + 1;
                break;
            }
        }
    }

#ifndef NO_PXACT
    if (pxact && (rc == 0 || keepgoing)) {
		int pxactState = ism_pxactivity_get_state();
		int enabled = ism_common_getIntConfig(PXACT_CFG_ENABLE_ACTIVITY_TRACKING, 0);
		TRACE(5, "ism_pxactivity_get_state=%u PXACT_CFG_ENABLE_ACTIVITY_TRACKING=%u\n", pxactState, enabled);
		if (enabled) {
			switch (pxactState) {
			case PXACT_STATE_TYPE_CONFIG:
				ism_proxy_activity_start();
				break;
			case PXACT_STATE_TYPE_STOP:
				ism_proxy_activity_start();
				break;
			case PXACT_STATE_TYPE_ERROR:
				rc = ism_pxactivity_stop();
				TRACE(5, "ism_pxactivity_stop; rc=%d (after being in error state)\n", rc);
				if (!rc) {
					ism_proxy_activity_start();
				}
				break;
			default:
				//Do nothing
				TRACE(4, "ism_pxactivity_get_state=%u is not handled.\n", pxactState);
				break;
			}
		} else {
			if (pxactState == PXACT_STATE_TYPE_START || pxactState == PXACT_STATE_TYPE_ERROR) {
				rc = ism_pxactivity_stop();
				TRACE(5," ism_pxactivity_stop; rc=%u\n", rc);
			} else {
				TRACE(4, "ism_pxactivity_get_state=%u is not handled.\n", pxactState);
			}
		}
    }
#endif
    if (rc && needlog) {
        char * errbuf = alloca(4096);
        ism_common_formatLastError(errbuf, 4096);
        LOG(ERROR, Server, 948, "%s%u", "Error processing a config update: RC={1} Error={0}", errbuf, rc);
    }
    return rc;
}

#ifndef NO_PXACT
/*
 * Do a strcmp allowing for NULLs
 */
static int my_strcmp(const char * str1, const char * str2) {
    if (str1 == NULL || str2 == NULL) {
        if (str1 == NULL && str2 == NULL)
            return 0;
        return str1 ? 1 : -1;
    }
    return strcmp(str1, str2);
}


/*
 * Replace a string
 */
static void  replaceString(const char * * oldval, const char * val) {
    if (!my_strcmp(*oldval, val))
        return;
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (val && !strcmp(freeval, val))
            return;
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
        ism_common_free(ism_memory_proxy_utils,freeval);
    } else {
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
    }
}
#endif


/*
 * Process a config file
 */
int ism_proxy_process_config(const char * fname, const char * dname, int complex, int checkonly, int keepgoing) {
    FILE * f;
    int  len;
    char * buf = NULL;
    int    bread;
    int    printerr;
    int    rc = 0;
    int    xrc;
    ism_field_t var = {0};

    if (fname == NULL)
        return 0;

    printerr = complex >> 4;
    complex &= 0x0F;
    if (complex == 0)
        printerr = 1;

    /*
     * Replace last segment of the name with the override name
     */
    if (dname) {
        char * cp;
        char * namebuf = alloca(strlen(fname) + strlen(dname) + 2);
        strcpy(namebuf, fname);
        cp = namebuf+strlen(fname) - 1;
        while (cp > namebuf && *cp != '/')
            cp--;
        if (*cp == '/')
            cp++;
        strcpy(cp, dname);
        fname= namebuf;
    }

    /*
     * Open the file and read it into memory
     */
    f = fopen(fname, "rb");
    if (!f) {
        if (complex) {
            LOG(ERROR, Server, 908, "%s", "The configuration file is not found: {0}", fname);
        } else {
            printf("The config file is not found: %s\n", fname);
        }
        return ISMRC_NotFound;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_config,4),len+2);
    if (!buf) {
        printf("Unable to allocate memory for config file: %s\n", fname);
        fclose(f);
        return ISMRC_AllocateError;
    }
    rewind(f);

    /*
     * Read the file
     */
    bread = fread(buf, 1, len, f);
    buf[bread] = 0;
    if (bread != len) {
        if (complex) {
            LOG(ERROR, Server, 916, "%s", "The configuration file cannot be read: {0}", fname);
        }
        if (printerr) {
            printf("The configuration file cannot be read: %s\n", fname);
        }
        ism_common_free(ism_memory_proxy_config,buf);
        fclose(f);
        return ISMRC_ObjectNotValid;
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
        if (complex) {
            LOG(ERROR, Server, 917, "%s%d%d", "The configuration {0} is not valid JSON at line {2}.",
                    fname, rc, parseobj.line);
        }
        if (printerr) {
            printf("The configuration file is not valid JSON: File=%s Error=%d Line=%d\n", fname, rc, parseobj.line);
        }
    } else {

        /*
         * Process the JSON DOM
         */
        if (!complex) {
            int entloc = 1;
            while (entloc < parseobj.ent_count) {
                ism_json_entry_t * ent = parseobj.ent+entloc;
                const char * name = ent->name;
                canonicalName((char *)ent->name);
                switch(ent->objtype) {
                case JSON_String:  /* JSON string, value is UTF-8              */
                    var.type = VT_String;
                    var.val.s = (char *)ent->value;
                    ism_common_setProperty(ism_common_getConfigProperties(), name, &var);
#ifndef NO_PROXY
                    /* Temporary until we are on the GA MessageSight */
                    if (!strcmp(name, "TrustStore")) {
                        ism_common_setProperty(ism_common_getConfigProperties(), "TrustedCertificateDir", &var);
                    }
#endif
                    entloc++;
                    break;
                case JSON_Integer: /* Number with no decimal point             */
                    var.type = VT_Integer;
                    var.val.i = ent->count;
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    entloc++;
                    break;
                case JSON_Number:  /* Number which is too big or has a decimal */
                    var.type = VT_Double;
                    var.val.d = strtod(ent->value, NULL);
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
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
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, &var);
                    entloc++;
                    break;
                case JSON_Null:
                    ism_common_setProperty(ism_common_getConfigProperties(), ent->name, NULL);
                    entloc++;
                }
            }
        }

        /*
         * Do a complex pass at the config
         */
        if (complex) {
            /* Process most complex items  */
            xrc = ism_proxy_complexConfig(&parseobj, complex, checkonly, keepgoing);
            if (rc == 0)
                rc = xrc;
            if (rc != 0 && !keepgoing) {

            }

            /* Process ConfigPath */
            int entloc = 1;
            while (entloc < parseobj.ent_count) {
                ism_json_entry_t * ent = parseobj.ent+entloc;
                switch(ent->objtype) {
                default:
                    entloc++;
                    break;
                case JSON_Object:  /* JSON object, count is number of entries  */
                    entloc += ent->count + 1;
                    break;
                case JSON_Array:   /* JSON array, count is number of entries   */
#ifndef NO_PROXY
                    if (!strcmp(ent->name, "ConfigPath")) {
                        xrc = ism_proxy_addConfigPath(&parseobj, entloc);
                        if (rc == 0)
                            rc = xrc;
                    }
#endif
                    entloc += ent->count + 1;
                    break;
                }
            }
        }
    }

    /* Clean up */
    if (parseobj.free_ent)
        ism_common_free(ism_memory_utils_parser,parseobj.ent);
    ism_common_free(ism_memory_proxy_config,buf);
    return rc;
}

/*
 * Configure a tenant
 */
int ism_proxy_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing) {
    int   rc = 0;
    xUNUSED int   xrc;
    int   endloc;
    ism_json_entry_t * ent;
    int isClientClass;
    int isTopicRule;
    int isAuthentication;

    pthread_mutex_lock(&rulelock);

    if (!parseobj || where > parseobj->ent_count) {
        TRACE(2, "Proxy config JSON not correct\n");
        pthread_mutex_unlock(&rulelock);
        return 1;
    }
    ent = parseobj->ent+where;
    isClientClass = !strcmp(ent->name, "ClientClass");
    isTopicRule = !strcmp(ent->name, "TopicRule");
    isAuthentication = !strcmp(ent->name, "Authentication");
    if (!ent->name || (!isClientClass && !isTopicRule   && !isAuthentication) || ent->objtype != JSON_Object) {
        TRACE(2, "Proxy config JSON invoked for config which is not a client class, topic rule, or authentication\n");
        pthread_mutex_unlock(&rulelock);
        return 2;
    }
    endloc = where + ent->count;
    where++;
    ent++;
    while (where <= endloc) {
#ifndef NO_PROXY
        if (isClientClass) {
            xrc = ism_proxy_makeClientClass(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
        if (isTopicRule) {
            xrc = ism_proxy_makeTopicRule(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
        if (isAuthentication) {
            xrc = ism_proxy_makeAuthentication(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
#endif
        ent = parseobj->ent+where;
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    pthread_mutex_unlock(&rulelock);
    return rc;
}


/*
 * Configure a tenant
 */
int ism_proxy_config_mhub(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing) {
    int   rc = 0;
    int   endloc;
    ism_json_entry_t * ent = parseobj->ent+where;

#ifndef HAS_BRIDGE
    int   xrc;
    int isBindings = 0;
    int isCredentials = 0;
    int isConnection = 0;
    isBindings = !strcmp(ent->name, "MessageHubBindings");
    isCredentials = !strcmp(ent->name, "MessageHubCredentials");
    isConnection = !strcmp(ent->name, "KafkaConnection");
#endif

    endloc = where + ent->count;
    where++;
    ent++;
    while (where <= endloc) {
#ifndef HAS_BRIDGE
        if (isBindings) {
            xrc = ism_mhub_parseBindings(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
        if (isCredentials) {
            xrc = ism_mhub_parseCredentials(parseobj, where, NULL);
            if (rc == 0)
                rc = xrc;
        }
        if (isConnection) {
            xrc = ism_mhub_parseKafkaConnection(parseobj, where, NULL, checkonly, keepgoing);
            if (rc == 0)
                rc = xrc;
        }
#endif
        ent = parseobj->ent+where;
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    return rc;
}



/*
 * Simple lower case (ASCII7 only)
 */
xUNUSED static char * my_strlwr(char * cp) {
    char * buf = cp;
    if (!cp)
        return NULL;

    while (*cp) {
        if (*cp >= 'A' && *cp <= 'Z')
            *cp = (char)(*cp|0x20);
        cp++;
    }
    return buf;
}

#ifndef NO_PROXY
/*
 * Compile a proxy rule
 */
static ism_pxrule_t * compileRule(const char * rule, ism_pxrule_t * outbuf, int type, int * ret) {
    int          rc = 0;
    int          endtype = RULEEND_None;
    const char * rp;
    uint8_t * op;
    uint8_t * lenp;
    int docopy = 0;
    char * extra = NULL;

    if (!rule)
        return NULL;
    if (type == RULETYPE_Convert)
        endtype = 0;

    /* Make a copy of a topic rule */
    if (type == RULETYPE_Topic) {
        char * xrule = alloca(strlen(rule)+1);
        strcpy(xrule, rule);
        extra = strchr(xrule, ';');
        if (extra)
           *extra++ = 0;
        rule = xrule;
    }

    if (!outbuf) {
        docopy = 1;
        outbuf = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,5),1, strlen(rule)*2 + 2 + sizeof(ism_pxrule_t));
        outbuf->rule = (uint8_t *)(outbuf+1);
    }
    rp = rule;
    op = outbuf->rule;
    lenp = op++;

    /* Find the after count */
    if (*rp >= '0' && *rp <= '9' && rp[1] == ',') {
        outbuf->after = *rp-'0';
        rp += 2;
    }

    /* Process the rule */
    while (*rp) {
        char ch = *rp;
        if (ch == '\\') {
            if (rp[1]) {
                *op++ = *++rp;
            } else {
                rc = 2;
                break;
            }
        } else {
            if (ch == '+' || ch == '~' || ch == '{' || ch == '*') {
                if ((type==RULETYPE_Class) && (ch == '+' || ch == '~')) {
                    rc = 4;
                    break;
                }
                if ((type==RULETYPE_Convert) && ((ch == '+' || ch == '~') || ch == '*')) {
                    rc = 4;
                    break;
                }
                if (op > (lenp+1)) {
                    if (op-lenp > 250) {
                        rc = 3;
                        break;
                    }
                    *lenp = (uint8_t)(op-lenp-1);
                } else {
                    op = lenp;
                }
                if (ch == '*') {
                    endtype = RULEEND_Any;
                     if (rp[1]) {
                         rc = 5;
                    }
                    break;
                }
                if (ch == '+') {
                    if (type != RULETYPE_Topic) {
                        rc = 8;
                        break;
                    }
                    *op++ = RULEWILD;
                    *op++ = RULEWILD_Plus;
                }
                if (ch == '~') {
                    if (type != RULETYPE_Topic) {
                        rc = 8;
                        break;
                    }
                    *op++ = RULEWILD;
                    *op++ = RULEWILD_NoWild;
                }
                if (ch == '?') {
                    if (type != RULETYPE_Topic) {
                        rc = 8;
                        break;
                    }
                    *op++ = RULEWILD;
                    *op++ = RULEWILD_Any;
                }
                if (ch == '{') {
                    int whichvar = 0;
                    char token[64];
                    char * pos = strchr(++rp, '}');
                    if (pos && (pos-rp)<63) {
                        memcpy(token, rp, pos-rp);
                        token[pos-rp] = 0;
                        my_strlwr(token);
                        if (!strcmp(token, "org") || !strcmp(token, "tenant") )
                            whichvar = RULEVAR_Org;
                        else if (!strcmp(token, "type") || !strcmp(token, "inst"))
                            whichvar = RULEVAR_Type;
                        else if (!strcmp(token, "id") || !strcmp(token, "device"))
                            whichvar = RULEVAR_Id;
                        rp = pos;
                    } else {
                        rc = 6;
                        break;
                    }

                    *op++ = RULEVAR;
                    *op++ = whichvar;
                }
                lenp = op;
                op++;
            } else {
                *op++ = *rp;
            }
        }
        rp++;
    }

    /* Complete trailing const string */
    if (op > (lenp+1)) {
        if (op-lenp > 250) {
            rc = 3;
        }
        *lenp = (uint8_t)(op-lenp-1);
    } else {
        op = lenp;
    }

    /* Parse the extra items in the topic rule */
    while (extra) {
        char * token = ism_common_getToken(extra, " ;", " ;", &extra);
        if (token) {
            if (*token == '@') {
                int authmask = ism_proxy_getAuthMask(token+1);
                if (authmask == -1) {
                    rc = 9;
                }
                outbuf->auth = authmask;
            }
        }
    }

    /* Check for error */
    if (ret)
        *ret = rc;
    if (rc) {
        if (docopy)
            ism_common_free(ism_memory_proxy_config,outbuf);
        return NULL;
    }

    /* Put out end record */
    *op++ = (uint8_t)RULEEND;
    outbuf->endrule = (uint8_t)endtype;
    outbuf->ruletype = type;
    outbuf->rulelen = op-outbuf->rule;
    return outbuf;
}

/*
 * Dump a rule
 */
const char * ism_proxy_showRule(ism_pxrule_t * pxrule, char * buf, int len) {
    char * tbuf;
    char * tp;
    uint8_t * rp;
    if (!pxrule) {
        ism_common_strlcpy(buf, "null", len);
    } else {
        char * value;
        tbuf = alloca(64 + pxrule->rulelen*3);
        tp = tbuf;
        if (pxrule->class) {
            sprintf(tp, "class=%c ", pxrule->class);
            tp += strlen(tp);
        }
        if (pxrule->ruletype) {
            switch (pxrule->ruletype) {
            case RULETYPE_Topic:   value = "topic";     break;
            case RULETYPE_Class:   value = "class";     break;
            case RULETYPE_Convert: value = "convert";   break;
            default:               value = "unknown";
            }
            sprintf(tp, "type=%s ", value);
            tp += strlen(tp);
        }
        if (pxrule->endrule) {
            switch (pxrule->endrule) {
            case RULEEND_Any:   value = "any";     break;
            case RULEEND_None:  value = "none";    break;
            default:             value = "uknown";  break;
            }
            sprintf(tp, "end=%s ", value);
            tp += strlen(tp);
        }
        if (pxrule->after) {
            sprintf(tp, "after=%u ", pxrule->after);
            tp += strlen(tp);
        }
        if (pxrule->rulelen) {
            sprintf(tp, "rule=\"");
            tp += strlen(tp);
            rp =  pxrule->rule;
            while (*rp != RULEEND) {
                if (*rp < 0xf8) {
                    int xlen = *rp++;
                    while (xlen > 0) {
                        if (*rp == '"' || *rp == '{' || *rp == '~' || *rp == '+' || *rp == '*' ||
                                *tp == '?' || *rp == '\\')
                            *tp++ = '\\';
                        *tp++ = *rp++;
                        xlen--;
                    }
                } else if (*rp == RULEVAR) {
                    switch (*++rp) {
                    case RULEVAR_Org:  value = "org";      break;
                    case RULEVAR_Id:   value = "id";       break;
                    case RULEVAR_Type: value = "type";     break;
                    default:           value = "xxx";      break;
                    }
                    sprintf(tp, "{%s}", value);
                    tp += strlen(tp);
                    rp++;
                } else if (*rp == RULEWILD) {
                    if (*++rp == RULEWILD_Plus) {
                        *tp++ = '+';
                    } else if (*rp == RULEWILD_NoWild) {
                        *tp++ = '~';
                    } else {
                        *tp++ = '?';
                    }
                    rp++;
                }
            }
            *tp++ = '"';
        }
        *tp = 0;
        ism_common_strlcpy(buf, tbuf, len);
    }
    return buf;
}


/*
 * Check for a valid rule.
 * Compile on the stack and thruw the result away
 */
static int validRule(const char * rule, int type, int line) {
    int rc;
    ism_pxrule_t * outbuf;

    if (!rule)
        return 1;      /* NULL is valid */

    outbuf = alloca(strlen(rule)*2 + 2 + sizeof(ism_pxrule_t));
    memset(outbuf, 0, sizeof(ism_pxrule_t));
    outbuf->rule = (uint8_t *)(outbuf+1);

    compileRule(rule, outbuf, type, &rc);
    if (rc) {
        TRACE(3, "Proxy config rule not valid: rule=\"%s\" rc=%d line=%d\n", rule, rc, line);
    } else {
        // char xbuf[1000];
        // printf("compile rule: [%s] to [%s]\n", rule, ism_proxy_showRule(outbuf, xbuf, sizeof xbuf));
    }
    return rc == 0;
}

/*
 * Find the client class by name
 */
ism_clientclass_t * ism_proxy_getClientClass(const char * name) {
    static int default_init = 0;
    ism_clientclass_t * ret = ismClientClasses;
    if (!name) {
        default_init = 1;
        return NULL;
    }
    while (ret) {
        if (!strcmp(name, ret->name))
            break;
        ret = ret->next;
    }
    if (!default_init) {
        default_init = 1;
        ism_proxy_getTopicRule("__none");    /* Use code in getTopicRule */
        ret = ism_proxy_getClientClass(name);
    }
    return ret;
}


/*
 * Link this clientclass
 */
static void linkClientClass(ism_clientclass_t * rclientclass) {
    rclientclass->next = NULL;
    if (!ismClientClasses) {
        ismClientClasses = rclientclass;
    } else {
        ism_clientclass_t * clientclass = ismClientClasses;
        while (clientclass->next) {
            clientclass = clientclass->next;
        }
        clientclass->next = rclientclass;
    }
}

/*
 * Free up a client class
 */
static void freeClientClass(ism_clientclass_t * clientclass) {
    if (clientclass->deflt)
        ism_common_free(ism_memory_proxy_config,clientclass->deflt);
    /* TODO */
}

/*
 * Return the name of the topic rule
 */
const char * ism_proxy_getTopicRuleName(ism_topicrule_t * topicrule) {
    return topicrule ? topicrule->name : "";
}

/*
 * Return the subordinate topic rule based on client class
 */
ism_topicrule_t * ism_proxy_getTopicRuleClass(ism_tenant_t * tenant, uint8_t client_class) {
    ism_topicrule_t * topicrule = tenant->topicrule;
    if (!topicrule)
        return NULL;
    if (!topicrule->child)
        return topicrule;
    topicrule = topicrule->child;
    while (topicrule) {
        if (topicrule->clientclass == client_class)
            return topicrule;
        topicrule = topicrule->next;
    }
    return tenant->topicrule;
}


/*
 * Find the client class by name
 */
ism_topicrule_t * ism_proxy_getTopicRule(const char * name) {
    static int default_init = 0;
    ism_topicrule_t * ret = ismTopicRules;
    if (!name) {
        default_init = 1;
        return NULL;
    }
    while (ret) {
        if (!strcmpi(name, ret->name))
            break;
        ret = ret->next;
    }
    if (!default_init) {
        int  rc;
        int  len = strlen(default_topic_rules);
        default_init = 1;
        ism_json_parse_t parseobj = { 0 };
        ism_json_entry_t ents[150];
        parseobj.ent_alloc = 150;
        parseobj.ent = ents;
        parseobj.source = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_config,6),len);
        memcpy(parseobj.source, default_topic_rules, len);
        parseobj.src_len = len;
        parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
        rc = ism_json_parse(&parseobj);
        if (rc == 0) {
            rc = ism_proxy_complexConfig(&parseobj, 2, 0, 1);
            TRACE(3, "Add default topic rule config: rc=%d\n", rc);
        }
        ism_common_free(ism_memory_proxy_config,parseobj.source);
        return ism_proxy_getTopicRule(name);
    }
    return ret;
}


/*
 * Link this topicrule
 */
static void linkTopicRule(ism_topicrule_t * rtopicrule) {
    rtopicrule->next = NULL;
    if (!ismTopicRules) {
        ismTopicRules = rtopicrule;
    } else {
        ism_topicrule_t * topicrule = ismTopicRules;
        while (topicrule->next) {
            topicrule = topicrule->next;
        }
        topicrule->next = rtopicrule;
    }
}


/*
 * Free up a client class
 */
static void freeTopicRule(ism_topicrule_t * topicrule) {
    if (topicrule->convert)
        ism_common_free(ism_memory_proxy_config,topicrule->convert);
    /* TODO */
}


/*
 * Make a client class object from the configuration.
 * TODO: additional validation of the values
 */
int ism_proxy_makeClientClass(ism_json_parse_t * parseobj, int where, const char * name) {
    int endloc;
    char  xbuf[1024];
    int   rc = 0;
    ism_clientclass_t * clientclass;

    ism_proxy_getClientClass(NULL);
    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    clientclass = ism_proxy_getClientClass(name);
    if (clientclass) {
        TRACE(2, "A client class cannot be redefined\n");
        return 1;
    } else {
        if (ent->objtype == JSON_Object) {
            clientclass = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,7),1, sizeof(ism_clientclass_t));
            strcpy(clientclass->structID, "IoTClC");
            clientclass->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_config,1000),name);
            clientclass->namelen = (int)strlen(name);
        } else {
            TRACE(2, "Proxy object does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if ((ent->objtype != JSON_String && ent->objtype != JSON_Null) ||
                (strlen(ent->name) > 1 && strcmp(ent->name, "default")) ||
                !validRule(ent->value, RULETYPE_Class, ent->line)) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, "");
            rc = ISMRC_BadPropertyValue;
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmp(ent->name, "default")) {
                clientclass->deflt = compileRule(ent->value, NULL, RULETYPE_Class, NULL);
            } else {
                ism_pxrule_t * nextrule;
                nextrule = compileRule(ent->value, NULL, RULETYPE_Class, NULL);
                nextrule->class = *ent->name;
                nextrule->next = clientclass->classlist;
                clientclass->classlist = nextrule;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    }

    if (rc) {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        LOG(ERROR, Server, 928, "%s%-s%u", "ClientClass configuration error: ClientClass={0} Error={1} Code={2}",
                        clientclass->name, xbuf, ism_common_getLastError());
    }
    if (!rc) {
        linkClientClass(clientclass);
    } else {
        freeClientClass(clientclass);
    }
    return rc;
}

/*
 * Look up an authorization mask value
 */
int ism_proxy_getAuthMask(const char * name) {
    authmask_t * ap = authmask_head;
    while (ap) {
        if (!strcmp(ap->name, name)) {
            // printf("found auth: %s=%04x\n", name, ap->mask);
            return ap->mask;
        }
        ap = ap->next;
    }
    return -1;
}


/*
 * Make an authentication entry from the configuration.
 */
int ism_proxy_makeAuthentication(ism_json_parse_t * parseobj, int where, const char * name) {
    ism_json_entry_t * ent;
    authmask_t * ap;

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ent = parseobj->ent+where;

    if (ent->objtype != JSON_Integer) {
        return ISMRC_BadPropertyValue;
    }
    ap = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_config,8),sizeof(authmask_t) + strlen(ent->name) + 1);
    ap->next = NULL;
    ap->name = (char *)(ap+1);
    ap->mask = ent->count;
    strcpy((char *)ap->name, ent->name);
    if (!authmask_tail)
        authmask_head = ap;
    else
        authmask_tail->next = ap;
    authmask_tail = ap;
    return 0;
}

/*
 * Make a client class object from the configuration.
 * TODO: additional validation of the values
 */
int ism_proxy_makeTopicRule(ism_json_parse_t * parseobj, int where, const char * name) {
    int endloc;
    char  xbuf[1024];
    ism_topicrule_t * topicrule;
    ism_topicrule_t * base_topicrule;
    int   rc = 0;
    int   i;
    ism_json_entry_t * ent;
    ism_json_entry_t * subent;
    int rwhere;
    int rendloc;

    ism_proxy_getTopicRule(NULL);
    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    topicrule = ism_proxy_getTopicRule(name);
    if (topicrule) {
        TRACE(2, "A topic rule cannot be redefined\n");
        return 1;
    } else {
        if (ent->objtype == JSON_Object) {
            topicrule = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,9),1, sizeof(ism_topicrule_t));
            strcpy(topicrule->structID, "IoTClC");
            topicrule->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_config,1000),name);
            base_topicrule = topicrule;
        } else {
            TRACE(2, "Proxy object does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where < endloc) {
        ent = parseobj->ent + where;
        rwhere = where+1;
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
        if (ent->objtype != JSON_Object ||
                (strcmp(ent->name, "default") && strlen(ent->name)>1)) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
            rc = ISMRC_BadPropertyValue;
        } else {
            rendloc = rwhere + ent->count;
            while (rwhere < rendloc) {
                ent = parseobj->ent + rwhere;
                if (!strcmp(ent->name, "publish") || !strcmp(ent->name, "subscribe")) {
                    if (ent->objtype != JSON_Array) {
                        if (ent->objtype != JSON_Null) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                        }
                    } else {
                        for (i=0; i<ent->count; i++) {
                            subent = parseobj->ent+rwhere+i+1;
                            if (subent->objtype != JSON_String || !validRule(subent->value, RULETYPE_Topic, subent->line)) {
                                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", subent->name, ism_json_getJsonValue(subent));
                                rc = ISMRC_BadPropertyValue;
                            }
                        }
                    }
                } else if (!strcmp(ent->name, "convert")) {
                    if (ent->objtype != JSON_String || !validRule(ent->value, RULETYPE_Convert, ent->line)) {
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "convert", ism_json_getJsonValue(ent));
                        rc = ISMRC_BadPropertyValue;
                    }
                } else if (!strcmp(ent->name, "nohash")) {
                    if (ent->objtype != JSON_True && ent->objtype != JSON_False) {
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Enabled", ism_json_getJsonValue(ent));
                        rc = ISMRC_BadPropertyValue;
                    }
                } else {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, "");
                    rc = ISMRC_BadPropertyValue;
                }
                if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                    rwhere += ent->count + 1;
                else
                    rwhere++;
            }
        }
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            topicrule = base_topicrule;
            if (strcmp(ent->name, "default")) {
                topicrule = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,10),1, sizeof(ism_topicrule_t));
                strcpy(topicrule->structID, "IoTClC");
                topicrule->name = base_topicrule->name;
                topicrule->next = base_topicrule->child;
                base_topicrule->child = topicrule;
                topicrule->clientclass = (uint8_t)ent->name[0];
            }
            rwhere = where+1;
            where += ent->count + 1;
            rendloc = rwhere + ent->count;
            while (rwhere < rendloc) {
                ent = parseobj->ent + rwhere;
                if (!strcmp(ent->name, "publish")) {
                    if (ent->objtype == JSON_Array) {
                        subent = ent + 1;
                        topicrule->publish = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,11),ent->count, sizeof(ism_pxrule_t *));
                        for (i=0; i<ent->count; i++) {
                            topicrule->publish[i] = compileRule(subent->value, NULL, RULETYPE_Topic, NULL);
                            subent++;
                        }
                        topicrule->publish_count = ent->count;
                    }
                } else if (!strcmp(ent->name, "subscribe")) {
                    if (ent->objtype == JSON_Array) {
                        subent = ent + 1;
                        topicrule->subscribe = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_config,12),ent->count, sizeof(ism_pxrule_t *));
                        for (i=0; i<ent->count; i++) {
                            topicrule->subscribe[i] = compileRule(subent->value, NULL, RULETYPE_Topic, NULL);
                            subent++;
                        }
                        topicrule->subscribe_count = ent->count;
                    }
                } else if (!strcmp(ent->name, "convert")) {
                    int scount = 0;
                    const char * cp = ent->value;
                    while (*cp) {
                        if (*cp == '/')
                            scount++;
                        cp++;
                    }
                    topicrule->convert = compileRule(ent->value, NULL, RULETYPE_Convert, NULL);
                    topicrule->remove_count = scount;
                } else if (!strcmp(ent->name, "nohash")) {
                    topicrule->nohash = ent->objtype == JSON_True;
                }
                if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                    rwhere += ent->count + 1;
                else
                    rwhere++;
            }
        }
    }

    if (rc) {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        LOG(ERROR, Server, 929, "%s%-s%u", "TopicRule configuration error: TopicRule={0} Error={1} Code={2}",
                        topicrule->name, xbuf, ism_common_getLastError());
    }
    if (!rc) {
        linkTopicRule(base_topicrule);
    } else {
        freeTopicRule(base_topicrule);
    }
    return rc;
}
#endif

#ifndef HAS_BRIDGE
/*
 * Get the file contents into a buffer
 */
static int getFileContents(const char * fname, concat_alloc_t * buf) {
    int len;
    FILE * f;

    /* Open the file and get the length */
    f = fopen(fname, "rb");
    if (!f) {
        return 1;    /* File cannot be opened */
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    ism_protocol_ensureBuffer(buf, len+1);
    rewind(f);

    /* Read the file */
    buf->used = fread(buf->buf, 1, len, f);
    buf->buf[buf->used] = 0;   /* Null terminate */
    if (buf->used != len) {
        fclose(f);
        return 3;
    }
    buf->buf[buf->used] = 0;   /* Null terminate */
    fclose(f);
    return 0;
}

/*
 * Get the per tenant dynamic config
 *
 * This consists of the tenant object, tenant users, and tenant mhub entries
 */
int ism_proxy_getTenantDynamicConfig(ism_json_t * jobj, ism_tenant_t * tenant, int outer) {
    const char * name = NULL;
    ism_json_startObject(jobj, NULL);
    ism_json_startObject(jobj, "Tenant");
    name = tenant->name;
    ism_tenant_getTenantJson(tenant, jobj, name);
    ism_json_endObject(jobj);
    if (tenant->mhublist) {
        ism_json_startObject(jobj, "KafkaConnection");
        ism_mhub_getKafkaConList("*", tenant, jobj, name);
        ism_json_endObject(jobj);
    }
    ism_json_endObject(jobj);
    if (jobj->indent)
        ism_json_putBytes(jobj->buf, "\n");
    ism_json_putNull(jobj->buf);
    return 0;
}

/*
 * Get the dynamic config
 */
int ism_proxy_getDynamicConfig(ism_json_t * jobj) {
    const char * val;
    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "LicensedUsage", ism_common_enumName(enum_licenses, g_licensedUsage));
    if (g_dynamic_loglevel) {
        val = ism_common_getStringConfig("LogLevel");
        if (val)
            ism_json_putStringItem(jobj, "LogLevel", val);
    }
    if (g_dynamic_tracelevel) {
        val = ism_common_getStringConfig("TraceLevel");
        if (val)
            ism_json_putStringItem(jobj, "TraceLevel", val);
    }
    /*
     * TODO: ConnectionLogLevel, TraceMessageData, ShutdownWait, ShutdownDelay
     */
    ism_tenant_getServerList("*", jobj, 1, "Server");
    ism_transport_getEndpointList("*", jobj, 1, "Endpoint");
    ism_tenant_getUserList("*", NULL, jobj, 1, "User");
    ism_json_endObject(jobj);
    if (jobj->indent)
        ism_json_putBytes(jobj->buf, "\n");
    ism_json_putNull(jobj->buf);
    return 0;
}

/*
 * Update the config file.
 */
int ism_proxy_updateDynamicConfig(int doBackup) {
    int nobackup;
    int bwritten;
    FILE * configf;
    char xbuf[8192];
    char cbuf[8192];
    char tbuf[64];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    concat_alloc_t curbuf = {cbuf, sizeof cbuf};

    const char * dyncfg = ism_common_getStringConfig("DynamicConfig");
    if (!dyncfg)
        return 0;
    nobackup  = getFileContents(dyncfg, &curbuf);
    configf = fopen(dyncfg, "wb");
    if (!configf) {
        LOG(ERROR, Server, 971,"%s", "Configuration updates were not written; unable to open dynamic config file for update: {0}", dyncfg);
        return ISMRC_Error;
    }

    /* Get the new config into a buffer */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_formatTimestamp(ts, tbuf, sizeof(tbuf), 6, ISM_TFF_ISO8601_2);
    sprintf(buf.buf, "/* imaproxy dynamic config updated %s */\n", tbuf);
    buf.used = strlen(buf.buf);
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, &buf, 4, 0);
    ism_common_closeTimestamp(ts);
    ism_proxy_getDynamicConfig(jobj);

    /* Backup the config */
    if  (!nobackup && curbuf.used > 0) {
        FILE * backf;
        char * backup = alloca(strlen(dyncfg)+16);
        strcpy(backup, dyncfg);
        char * ext = strrchr(backup, '/');
        if (!ext)
            ext = backup;
        ext = strchr(backup, '.');
        if (ext) {
            ext = strchr(ext, '.');
            if (!ext)
                ext = backup + strlen(backup);
            strcpy(ext, ".bak");
        }
        backf = fopen(backup, "wb");
        if (backf) {
            fwrite(curbuf.buf, curbuf.used, 1, backf);
            fclose(backf);
        }
    }

    /* Update the config */
    bwritten = fwrite(buf.buf, 1, buf.used, configf);
    fclose(configf);
    if (bwritten != buf.used) {
        LOG(ERROR, Server, 972,"%s", "Configuration updates were not written; unable to write dynamic config file for update: {0}", dyncfg);
        return 1;
    }
    LOG(NOTICE, Server, 973,"%s", "The configuration is updated: {0}", dyncfg);
    return 0;
}

/*
 * Update the tenant config file.
 */
int ism_proxy_updateTenantDynamicConfig(ism_tenant_t * tenant, int doBackup) {
    int nobackup;
    int bwritten;
    FILE * configf;
    char xbuf[8192];
    char cbuf[8192];
    char tbuf[64];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    concat_alloc_t curbuf = {cbuf, sizeof cbuf};
    char * tenantcfg;

    const char * dyncfg = ism_common_getStringConfig("DynamicConfig");
    if (!dyncfg)
        return 0;

    tenantcfg = alloca(strlen(dyncfg) + 16 + strlen(tenant->name));
    strcpy(tenantcfg, dyncfg);
    char * base = strrchr(tenantcfg, '/');
    if (base)
        base++;
    else
        base = tenantcfg;
    strcpy(base, "proxy#");
    strcat(base, tenant->name);       /* TODO: check tenant name does not contain bad chars */

    nobackup  = getFileContents(tenantcfg, &curbuf);
    configf = fopen(tenantcfg, "wb");
    if (!configf) {
        LOG(ERROR, Server, 971,"%s", "Configuration updates were not written; unable to open dynamic config file for update: {0}", dyncfg);
        return ISMRC_Error;
    }

    /* Get the new config into a buffer */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_formatTimestamp(ts, tbuf, sizeof(tbuf), 6, ISM_TFF_ISO8601_2);
    sprintf(buf.buf, "/* proxy#%s dynamic config updated %s */\n", tenant->name, tbuf);
    buf.used = strlen(buf.buf);
    ism_common_closeTimestamp(ts);

    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, &buf, 4, 0);
    ism_proxy_getTenantDynamicConfig(jobj, tenant, 1);

    /* Backup the config */
    if  (!nobackup && curbuf.used > 0) {
        FILE * backf;
        char * backup = alloca(strlen(dyncfg)+16);
        strcpy(backup, dyncfg);
        char * ext = strrchr(backup, '/');
        if (!ext)
            ext = backup;
        ext = strchr(backup, '.');
        if (ext) {
            ext = strchr(ext, '.');
            if (!ext)
                ext = backup + strlen(backup);
            strcpy(ext, ".bak");
        }
        backf = fopen(backup, "wb");
        if (backf) {
            fwrite(curbuf.buf, curbuf.used, 1, backf);
            fclose(backf);
        }
    }

    /* Update the config */
    bwritten = fwrite(buf.buf, 1, buf.used, configf);
    fclose(configf);
    if (bwritten != buf.used) {
        LOG(ERROR, Server, 972, "%s", "Configuration updates were not written; unable to write dynamic config file for update: {0}", dyncfg);
        return 1;
    }
    LOG(NOTICE, Server, 973,"%s", "The configuration is updated: {0}", dyncfg);
    return 0;
}
#endif
