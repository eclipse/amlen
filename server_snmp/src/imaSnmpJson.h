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
 * Note: this header file provides API definitions of JSON array query
 */

#ifndef _IMASNMPJSON_H_
#define _IMASNMPJSON_H_


/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

   int ima_snmp_jsonArray_getInt(ism_json_parse_t * pobj, const char * name, int deflt, int* ent_num);
   const char *ima_snmp_jsonArray_getString(ism_json_parse_t * pobj, const char * name, int* ent_num);
   //void testJsonStr(ism_json_parse_t * pobj);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPJSON_H_ */

