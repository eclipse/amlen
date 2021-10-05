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

#include <stdint.h>

int  parseuri(char * uri, char * * scheme, char * * authority, char * pathsep, char * * path, char * * query, char * * fragment) {
    char * pos;
    char   sep = 0;

    /*
     * Return nulls for all fields
     */
    if (scheme)
        *scheme = NULL;
    if (authority)
        *authority = NULL;
    if (pathsep)
        *pathsep = 0;
    if (path)
        *path = NULL;
    if (query)
        *query = NULL;
    if (fragment)
        *fragment = NULL;

    /*
     * Check for invalid chars
     */
    pos = uri;
    while (*pos) {
        if (((uint8_t)*pos) <= ' ')
            return 1;
        pos++;
    }

    /*
     * Parse the scheme
     */
    pos = uri+strcspn(uri, "/?#:");
    if (*pos == ':') {
        if (scheme)
            *scheme = uri;
        *pos = 0;

        /* URI with authority */
        if (pos[1]=='/' && pos[2]=='/') {
            uri = pos+3;
            if (authority)
                *authority = uri;
            pos = uri + strcspn(uri, "/?#");
            sep = *pos;
            *pos = 0;
            uri = pos+1;
        } else {
            /* scheme but no authority */
            uri = pos+1;
            sep = *uri;
            if (sep=='/' || sep=='?'  || sep=='#')
                uri++;
        }
    } else {
        sep = ' ';
    }

    /* Parse the path */
    if (sep && sep != '?' && sep != '#') {
        if (pathsep && sep=='/')
            *pathsep = '/';
        pos = uri+strcspn(uri, "?#");
        sep = *pos;
        *pos = 0;
        if (path)
            *path = uri;
        uri = sep ? pos+1 : pos;
    }

    /* Parse the query */
    if (sep && sep == '?') {
        if (query)
            *query = uri;
        pos = uri+strcspn(uri, "#");
        sep = *pos;
        *pos = 0;
        uri = sep ? pos+1 : pos;
    }

    /* Parse the fragment */
    if (sep) {
        if (fragment)
            *fragment = uri;
    }
    return 0;
}
