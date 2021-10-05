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

#define TRACE_COMP Admin

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <jansson.h>

#include "admin.h"
#include "config.h"

#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/*
 * Lists TrustedCertficate or ClientCertificate in truststore
 */

XAPI json_t * ism_config_listTruststoreCertificate(char *object, char *profileName, char *certName, int *count) {
    char fpath[1024];
    int profLen = 0;
    int type = 0;
    json_t *array = json_array();

    *count = 0;

    char *truststorePath = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"TrustedCertificateDir");

    DIR * truststoreDir = opendir(truststorePath);

    if (truststoreDir == NULL) {
        TRACE(3, "Could not open truststore directory. errno=%d\n", errno);
        return array;
    }

    if ( !strcmp(object, "ClientCertificate") ) {
        type = 1;
    }

    /* Scan thru truststore */
    struct dirent tentry;
    struct dirent *result;
    int error = 0;

    for (;;) {
        error = readdir_r (truststoreDir, &tentry, &result);
        if ( error != 0 ) {
            TRACE(3, "Could not read dir entries of truststore directory. err=%d\n", error);
            break;
        }

        if ( result == NULL ) break;

        if ( type == 0 ) {
            if (!strcmp(result->d_name, ".") || !strcmp(result->d_name, "..") || (strstr(result->d_name, "_allowedClientCerts") != NULL) ||
                    (strstr(result->d_name, "_capath") != NULL) || (strstr(result->d_name, "_cafile.pem") != NULL) ||
					(strstr(result->d_name, "_crl") != NULL)) {
                continue;
            }
        } else {
            if ( strstr(result->d_name, "_allowedClientCerts") == NULL ) {
                continue;
            }
        }


        /* check for profile name if profileName is not NULL */
        if ( profileName ) {
            if ( type == 0 ) {
                if ( strcmp(profileName, result->d_name)) {
                    continue;
                }
            } else {
                profLen = strlen(profileName);
                char *tmpstr = result->d_name;
                if (memcmp(tmpstr, profileName, profLen) != 0) continue;
                if ( strlen(tmpstr) <= profLen + 2 ) continue;
                char *sep = tmpstr + profLen;
                if ( strcmp(sep, "_allowedClientCerts")) continue;
            }
        }

        sprintf(fpath, "%s/%s", truststorePath, result->d_name);
        DIR *profDir = opendir(fpath);
        if (profDir == NULL || errno == ENOTDIR ) {
            continue;
        }

        struct dirent centry;
        struct dirent *cresult;
        int error2 = 0;
        for (;;) {

            error2 = readdir_r(profDir, &centry, &cresult);
            if ( error2 != 0 ) {
                TRACE(3, "Could not read dir entries of truststore directory. err=%d\n", error);
                break;
            }

            if ( cresult == NULL ) break;

            if (!strcmp(cresult->d_name, ".") || !strcmp(cresult->d_name, ".."))
                continue;

            /* Match certName if certName is not NULL */
            if ( certName ) {
                if ( strcmp(cresult->d_name, certName)) continue;
            }

            json_t *entry = json_object();

            if ( type == 0 ) {
                json_object_set_new(entry, "SecurityProfileName", json_string(result->d_name));
                json_object_set_new(entry, "TrustedCertificate", json_string(cresult->d_name));
            } else {
                char *pptr = result->d_name;
                char *aptr = strstr(pptr, "_allowedClientCerts");
                if ( aptr ) *aptr = 0;
                json_object_set_new(entry, "SecurityProfileName", json_string(pptr));
                json_object_set_new(entry, "CertificateName", json_string(cresult->d_name));
            }
            json_array_append(array, entry);
            *count += 1;
        }

        closedir(profDir);

    }
    closedir(truststoreDir);

    if ( profileName && certName ) {
        TRACE(5, "ism_config_listTruststoreCertificate() returned data of %d certificates. Object:%s SecurityProfile:%s Cert:%s\n", *count,
                object?object:"", profileName?profileName:"", certName?certName:"");
    } else {
        TRACE(9, "ism_config_listTruststoreCertificate() returned data of %d certificates. Object:%s SecurityProfile:%s Cert:%s\n", *count,
                object?object:"", profileName?profileName:"", certName?certName:"");
    }

    return array;
}


