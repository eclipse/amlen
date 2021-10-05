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

/**
 * @brief JSON Array Query interface for MessageSight SNMP Agent
 * @date July
 *
 * This provides the utilities of element query in JSON Array for MessageSight SNMP Agent.
 *
 */

#include <stdio.h>
#include <string.h>

#include <ismutil.h>
#include <ismjson.h>

#include "imaSnmpJson.h"

static int ima_snmp_jsonArray_get(ism_json_parse_t *pobj, int entnum, const char *name)
{
    int maxent;

    if (entnum < 0 || entnum >= pobj->ent_count ) {
        return -1;
    }
    /* Allow the entry to be directly sent */
    if ((uintptr_t)name < pobj->ent_count) {
        return (int)(uintptr_t)name;
    }

    maxent = pobj->ent_count;
    entnum++;
    while (entnum <= maxent) {
        ism_json_entry_t * ent = pobj->ent+entnum;
        if ((ent->name) && !strcmp(name, ent->name)) {
           // TRACE(9,"entnum %d \n",entnum);
            return entnum;
        }
        entnum++;
    }
    return -1;
}

int ima_snmp_jsonArray_getInt(ism_json_parse_t * pobj, const char * name, int deflt,
                 int* ent_num)
{
    ism_json_entry_t * ent;
    int entnum = *ent_num;
    int    val;
    char * eos;

    entnum = ima_snmp_jsonArray_get(pobj, entnum, name);
    TRACE(9,"jsonArray get entnum %d \n",entnum);
    *ent_num = entnum;
    if (entnum<0)
        return deflt;
    ent = pobj->ent+entnum;
    switch (ent->objtype) {
    case JSON_Integer: return ent->count;
    case JSON_True:    return 1;
    case JSON_False:   return 0;
    default:           return deflt;
    case JSON_String:
    case JSON_Number:
        val = (int)strtod(ent->value, &eos);
        while (*eos==' ' || *eos=='\t')
            eos++;
        if (*eos)
            return deflt;
        return val;
    }

}

const char *ima_snmp_jsonArray_getString(ism_json_parse_t * pobj, const char * name,
                 int* ent_num)
{
    ism_json_entry_t * ent;
    int entnum = *ent_num;
    entnum = ima_snmp_jsonArray_get(pobj, entnum, name);
    *ent_num = entnum;
    if (entnum<0)
        return NULL;
    ent = pobj->ent+entnum;
    switch (ent->objtype) {
    case JSON_True:    return "true";
    case JSON_False:   return "false";
    case JSON_Null:    return "null";
    default:           return NULL;
    case JSON_Integer:
    case JSON_String:
    case JSON_Number:
        return ent->value;
    }
}
/*
void testJsonStr(ism_json_parse_t * pobj)
{
	int i=0;
	TRACE(2,"pobj->entcount=%d \n",pobj->ent_count);

	for(;i<pobj->ent_count;i++)
	{
		TRACE(2,"pobj[%d]: \n",i);
		TRACE(2,"pobj->ent[i].objtype=%d\n",pobj->ent[i].objtype);
		TRACE(2,"pobj->ent[i].count=:%d\n",pobj->ent[i].count);
		if(pobj->ent[i].name)
			TRACE(2,"pobj->ent[i].name=:%s \n",pobj->ent[i].name);
		else
			TRACE(2,"pobj->ent[i].name=:NULL \n");
		if(pobj->ent[i].value)
			TRACE(2,"pobj->ent[i].value=%s \n\n",pobj->ent[i].value);
		else
			TRACE(2,"pobj->ent[i].value=NULL \n\n");
	}
}
*/
