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

#define TRACE_COMP Security

#include "securityInternal.h"
#include <security.h>

extern ism_trclevel_t * ism_defaultTrace;
extern int criticalConfigError;
extern int doAuthorize;
extern int auditLogControl;

//extern void ism_admin_ldapHexEscape(char ** new, char * from, int len);
//extern int ism_admin_ldapHexExtraLen(char * str, int len);
extern ism_config_callback_t ism_config_getCallback(ism_ConfigComponentType_t comptype);
extern int ism_security_validateDestinationWithGroupID(const char *objectName, ismSecurity_t *sContext, int ngrp, const char *destination);
extern char * ism_security_context_getOAuthGroupDelimiter(ismSecurity_t *sContext);

extern char * tolowerDN(const char * dn);
extern void string_strip_leading(char * dn);

extern pthread_rwlock_t   policylock;
extern ismSecPolicies_t  *policies;

ism_config_t     * secConhandle = NULL;

int disconnClientNotificationThreadStarted = 0;


/*
 * Find the next token in a comma separated list and trim leading and trailing spaces
 */
static  char * nextToken(char * * tmpstr) {
    char * token;
    char * cp = *tmpstr;
    while (*cp == ' ' || *cp == ',') {
        cp++;
    }
    token = cp;
    while (*cp && *cp != ',') {
        cp++;
    }
    if (*cp == ',')
        *cp++ = 0;
    *tmpstr = cp--;
    while (cp >= token && *cp == ' ') {
        *cp-- = 0;
    }
    return *token ? token : NULL;
}


/* Add a security policy to the policy rules table */
static int ism_security_addPolicyRule(ismPolicyRule_t * rule, int type) {
    int i;

    /* Initialize rules table */
    if (policies->count == policies->nalloc) {
        ismPolicyRule_t ** tmp = NULL;
        int firstSlot = policies->nalloc;
        policies->nalloc = policies->nalloc == 0 ? 64 : policies->nalloc * 2;
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,66),policies->rules, sizeof(ismPolicyRule_t *) * policies->nalloc);
        if (tmp == NULL) {
            return ISMRC_AllocateError;
        }
        policies->rules = tmp;
        for (i = firstSlot; i < policies->nalloc; i++)
            policies->rules[i] = NULL;
        policies->slots = policies->count;
    }

    /* Add policy rule */
    if (policies->count == policies->slots) {
        policies->rules[policies->count] = rule;
        policies->id = policies->count;
        policies->count++;
        policies->slots++;
    } else {
        for (i = 0; i < policies->slots; i++) {
            if (!policies->rules[i]) {
                policies->rules[i] = rule;
                policies->id = i;
                policies->count++;
                break;
            }
        }
    }

    switch (type) {
    case ismSEC_POLICY_CONFIG:
        policies->noConfPols++;
        break;
    case ismSEC_POLICY_CONNECTION:
        policies->noConnPols++;
        break;
    case ismSEC_POLICY_TOPIC:
        policies->noTopicPols++;
        break;
    case ismSEC_POLICY_QUEUE:
        policies->noQueuePols++;
        break;
    case ismSEC_POLICY_SUBSCRIPTION:
        policies->noSubsPols++;
        break;
    default:
        break;
    }

    return ISMRC_OK;
}

/* Delete a security policy rule from table */
static void ism_security_freePolicy(ismPolicyRule_t *policy) {
    if ( !policy )
        return;

    pthread_spin_lock(&policy->lock);
    if ( policy->name )          ism_common_free(ism_memory_admin_misc,(void *)policy->name);
    if ( policy->UserID )        ism_common_free(ism_memory_admin_misc,(void *)policy->UserID);
    if ( policy->GroupID )       ism_common_free(ism_memory_admin_misc,(void *)policy->GroupID);
    if ( policy->GroupLDAPEx )   ism_common_free(ism_memory_utils_to_lower,(void *)policy->GroupLDAPEx);
    if ( policy->CommonNames )   ism_common_free(ism_memory_admin_misc,(void *)policy->CommonNames);
    if ( policy->ClientID )      ism_common_free(ism_memory_admin_misc,(void *)policy->ClientID);
    if ( policy->ClientAddress ) ism_common_free(ism_memory_admin_misc,(void *)policy->ClientAddress);
    if ( policy->Protocol )      ism_common_free(ism_memory_admin_misc,(void *)policy->Protocol);
    if ( policy->Destination )   ism_common_free(ism_memory_admin_misc,(void *)policy->Destination);
    if ( policy->context )       policy->context = NULL;
    pthread_spin_unlock(&policy->lock);
    pthread_spin_destroy(&policy->lock);
    ism_common_free(ism_memory_admin_misc,policy);
    policy = NULL;
}


/* process address pairs in policy */
static int ism_security_processClientAddress(ismPolicyRule_t *policy, const char *clientAddress) {
    int ipv4 = 0;
    int ipv6 = 0;
    int rc = ISMRC_OK;

    if ( !clientAddress )
        return rc;

    TRACE(5, "ClientAddress rule: %s\n", clientAddress);

    int len = strlen(clientAddress);
    char *tmpstr = (char *)alloca(len+1);
    memcpy(tmpstr, clientAddress, len);
    tmpstr[len] = 0;

    if ( *tmpstr == '*' ||
         !strcmp(tmpstr, "0.0.0.0") || !strcmp(tmpstr, "[::]") )
    {
        if(policy->ClientAddress!=NULL)
            ism_common_free(ism_memory_admin_misc,(void *)policy->ClientAddress);
        policy->ClientAddress = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000), "*");
        return rc;
    }


    char *p, *token, *nexttoken = NULL;
    for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL;
        token = strtok_r(NULL, ",", &nexttoken))
    {
        int isPair = 0;
        int type = 1;

        /* remove leading space */
        while ( *token == ' ' )
            token++;
        if ( *token == 0 )
            continue;

        /* remove trailing space */
        p = token + strlen(token) - 1;
        while ( p > token && *p == ' ' )
            p--;
        *(p+1) = 0;

        if ( strstr(token, "-"))
            isPair = 1;

        if ( strstr(token, ":"))
            type = 2;

        char *addstr = token;
        char *tmptok;
        char *IPlow = strtok_r(addstr, "-", &tmptok);
        char *IPhigh = strtok_r(NULL, "-", &tmptok);

        if ( !isPair )
            IPhigh = IPlow;

        if ( type == 1 ) {
            policy->low4[ipv4] = ntohl(inet_addr(IPlow));
            policy->high4[ipv4] = ntohl(inet_addr(IPhigh));
            TRACE(5, "Policy=%s | AddressPair=%s - %s | %u - %u\n",
                policy->name, IPlow, IPhigh, policy->low4[ipv4],
                policy->high4[ipv4]);
            ipv4 += 1;
        } else {
            int rc1=0, rc2=0;
            char buf[64];
            int i = 0;
            char *tmpstr2;

            TRACE(5, "Policy=%s | AddressPair=%s - %s\n",
                policy->name, IPlow, IPhigh );

            memset(buf, 0, 64);
            i = 0;
            tmpstr2 = IPlow;
            while(*tmpstr2) {
                if ( *tmpstr2 == '[' || *tmpstr2 == ']' ) {
                    tmpstr2++;
                    continue;
                }
                buf[i++] = *tmpstr2;
                tmpstr2++;
            }

            rc1 = inet_pton(AF_INET6, buf, &policy->low6[ipv6]);
            if ( rc1 != 1 ) {
                TRACE(2, "Policy=%s AddressPair: inet_pton failed:%s errno=%d\n", policy->name, IPlow, errno);
            }

            memset(buf, 0, 64);
            i = 0;
            tmpstr2 = IPhigh;
            while(*tmpstr2) {
                if ( *tmpstr2 == '[' || *tmpstr2 == ']' ) {
                    tmpstr2++;
                    continue;
                }
                buf[i++] = *tmpstr2;
                tmpstr2++;
            }

            rc2 = inet_pton(AF_INET6, buf, &policy->high6[ipv6]);
            if ( rc2 != 1 ) {
                TRACE(8, "Policy=%s AddressPair: inet_pton failed:%s errno=%d\n", policy->name, IPhigh, errno);
            }

            if (rc1 == 1 && rc2 == 1 ) {
                ipv6 += 1;
                TRACE(2, "Policy=%s | AddressPair=%s-%s\n", policy->name,
                    IPlow, IPhigh);
            }
        }
        if(ipv4 > MAX_ADDR_PAIRS || ipv6 > MAX_ADDR_PAIRS){
            rc= ISMRC_ClientAddrMaxError;
            break ;
        }
    }
    if(rc == ISMRC_OK){
        void * oldClientAddress = (void *)policy->ClientAddress;
        policy->ClientAddress = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000), clientAddress);
        policy->ipv4Pairs = ipv4;
        policy->ipv6Pairs = ipv6;
        if(oldClientAddress){
            ism_common_free(ism_memory_admin_misc,(void *)oldClientAddress);
        }
    }
    return rc;
}

/* process protocol family */
static void ism_security_processProtoFamily(ismPolicyRule_t *policy) {
   int i;
   int protocount=0;

        for (i=0; i<MAX_PROTO_FAM; i++){
                  policy->protofam[i] = 0;
                  policy->protofam_count=0;
        }

        if ( !policy->Protocol || (policy->Protocol && *policy->Protocol == '\0') ) {
                return;
        }

        int len = strlen(policy->Protocol);
        if(len==1 && *policy->Protocol=='*'){
                /*Protocol filter is all. So no need to set*/
                return;
        }

        char *tmpstr = (char *)alloca(len+1);
        memcpy(tmpstr, policy->Protocol, len);
        tmpstr[len] = 0;

        char *token, *nexttoken = NULL;
        for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token
                = strtok_r(NULL, ",", &nexttoken)) {
                int protoid =  ism_admin_getProtocolId(token);
                policy->protofam[protocount++] = protoid;
                TRACE(9, "Set Protocol family rule (%s). ID=%d\n",
                                policy->Protocol, protoid);

        }
        policy->protofam_count=protocount;
        TRACE(9, "Protocol family rule count=%d\n", policy->protofam_count);

    return;
}

/* check for matformed policy rules - Destination and ClientID */
static int ism_security_checkForMalformedPolicy(const char *match) {
    int rc = ISMRC_OK;
    if ( !match )
        return ISMRC_ArgNotValid;

    char trans[1024] = {0};
    ism_transport_t * tport = (ism_transport_t *)trans;
    rc = ism_common_matchWithVars(match, match, (ima_transport_info_t *)tport, NULL, 0, 0);
    if ( rc == MWVRC_MalformedVariable ) {
        return ISMRC_MalformedVariable;
    }
    if ( rc == MWVRC_UnknownVariable ) {
          return ISMRC_UnknownVariable;
    }

    return ISMRC_OK;
}

/* Find security policy by name and type */
static ismPolicyRule_t * ism_security_getPolicyByName(char *name, int type) {
    int i = 0;
    for (i = 0; i < policies->count; i++) {
        ismPolicyRule_t *policy = policies->rules[i];
        if (type != -1 && policy->type != type)
            continue;
        if (name && policy->name && strcmp(policy->name, name) == 0) {
            return policy;
        }
    }
    return NULL;
}

/* Update or add a policy in the list */
static int ism_security_addUpdatePolicyRule(ismPolicyRule_t * rule, int type) {
    int i;
    int rc = ISMRC_OK;
    if (!rule || !rule->name){
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "rule");
        return ISMRC_ArgNotValid;
    }

    TRACE(9, "Add or Update Policy=%s Type=%d\n", rule->name, type);

    /* if rule exist in the list - update else add policy in the list */
    pthread_rwlock_wrlock(&policylock);

    char *policyDestination = "Topic";
    if ( type == ismSEC_POLICY_MESSAGING )
        policyDestination = "Destination";
    else if ( type == ismSEC_POLICY_QUEUE )
        policyDestination = "Queue";
    else if ( type == ismSEC_POLICY_SUBSCRIPTION )
        policyDestination = "Subscription";

    int found = 0;
    for (i = 0; i < policies->count; i++) {
        int validate = 0;
        ismPolicyRule_t *policy = policies->rules[i];

        if (strcmp(rule->name, policy->name) == 0 && policy->type == type ) {

            TRACE(9, "Update Policy %s\n", rule->name);

            /* check type */
            if (rule->type == policy->type) {

                /* validate rules for variable substitution */
                if ( rule->Destination ) {
                    rc = ism_security_checkForMalformedPolicy(rule->Destination);
                    if ( rc != ISMRC_OK ) {
                        LOG(ERROR, Admin, 6127, "%-s%-s%s%-s%d", "The rule specified as the {0} is not valid. Policy name={1}. {2}={3}. Return code={4}.",
                            policyDestination, policy->name, policyDestination, rule->Destination, rc);
                        if (serverState == ISM_SERVER_INITIALIZING) {
                            criticalConfigError = rc;
                            ism_common_setError(rc);
                        }
                        pthread_rwlock_unlock(&policylock);
                        return rc;
                    }
                }
                if ( rule->ClientID ) {
                    rc = ism_security_checkForMalformedPolicy(rule->ClientID);
                    if ( rc != ISMRC_OK ) {
                        LOG(ERROR, Admin, 6127, "%-s%-s%s%-s%d", "The rule specified as the {0} is not valid. Policy name={1}. {2}={3}. Return code={4}.",
                            "ClientID", policy->name, "ClientID", rule->ClientID, rc);
                        if (serverState == ISM_SERVER_INITIALIZING) {
                            criticalConfigError = rc;
                            ism_common_setError(rc);
                        }
                        pthread_rwlock_unlock(&policylock);
                        return rc;
                    }
                }

                found = 1;
                /* update policy fields */
                if ( rule->UserID ) {
                    if ( policy->UserID ) ism_common_free(ism_memory_admin_misc,(void *)policy->UserID);
                    policy->UserID = rule->UserID;
                    if ( validate == 0 && strlen(policy->UserID) == 1 && *policy->UserID == '*') {
                        validate = 0;
                    } else {
                        validate = 1;
                    }
                }
                if ( rule->GroupID ) {
                    if ( policy->GroupID ) ism_common_free(ism_memory_admin_misc,(void *)policy->GroupID);
                    policy->GroupID = rule->GroupID;
                    if ( validate == 0 && strlen(policy->GroupID) == 1 && *policy->GroupID == '*') {
                        validate = 0;
                    } else {
                        validate = 1;
                    }

                }
                if ( rule->ClientID ) {
                    if ( policy->ClientID ) ism_common_free(ism_memory_admin_misc,(void *)policy->ClientID);
                    policy->ClientID = rule->ClientID;
                    if ( validate == 0 && strlen(policy->ClientID) == 1 && *policy->ClientID == '*') {
                        validate = 0;
                    } else {
                        validate = 1;
                    }
                }
                if ( rule->ClientAddress ) {
                    if ( strcmp(policy->ClientAddress, "0.0.0.0"))
                        validate = 1;
                    rc = ism_security_processClientAddress(policy, rule->ClientAddress);
                    if ( rc != ISMRC_OK ) {
                        if(serverState == ISM_SERVER_INITIALIZING){
                            LOG(ERROR, Admin, 6127, "%-s%-s%s%-s%d", "The rule specified as the {0} is not valid. Policy name={1}. {2}={3}. Return code={4}.",
                                "ClientAddress", policy->name, "ClientAddress", rule->ClientAddress, rc);
                            criticalConfigError = rc;
                            if(rc==ISMRC_ClientAddrMaxError){
                                ism_common_setErrorData(ISMRC_ClientAddrMaxError, "%d", MAX_ADDR_PAIRS);
                            }else{
                                ism_common_setError(rc);
                            }
                        }else{
                            TRACE(2, "The rule specified as the ClientAddress is not valid. Policy name=%s. Client=%s. Return code: %d.\n", policy->name, rule->ClientAddress, rc);
                        }
                        pthread_rwlock_unlock(&policylock);
                        return rc;
                    }
                }
                if ( rule->Protocol ) {
                    if ( policy->Protocol ) ism_common_free(ism_memory_admin_misc,(void *)policy->Protocol);
                    policy->Protocol = rule->Protocol;
                    if ( validate == 0 && strlen(policy->Protocol) == 1 && *policy->Protocol == '*') {
                        validate = 0;
                    } else {
                        validate = 1;
                    }
                    ism_security_processProtoFamily(policy);
                }
                if ( rule->CommonNames ) {
                    if ( policy->CommonNames ) ism_common_free(ism_memory_admin_misc,(void *)policy->CommonNames);
                    policy->CommonNames = rule->CommonNames;
                    if ( validate == 0 && strlen(policy->CommonNames) == 1 && *policy->CommonNames == '*') {
                        validate = 0;
                    } else {
                        validate = 1;
                    }
                }
                if ( rule->Destination ) {
                    if ( policy->Destination ) ism_common_free(ism_memory_admin_misc,(void *)policy->Destination);
                    policy->Destination = rule->Destination;
                    if ( validate == 0 && strlen(policy->Destination) == 1 && *policy->Destination == '*') {
                        validate = 0;
                    } else {
                        validate = 1;
                    }
                }

                /* updatePolicyRule will be called only be ism_security_createPolicyFromProps
                 * which for adding new policy, so the context will always be assigned
                 */
                policy->context = rule->context;

                policy->DisconnectedClientNotification = rule->DisconnectedClientNotification;
                policy->varID = rule->varID;
                policy->varCL = rule->varCL;
                policy->varCN = rule->varCN;
                policy->varGR = rule->varGR;
                policy->varIDinCL = rule->varIDinCL;
                policy->varCNinCL = rule->varCNinCL;
                int j = 0;
                for (j=0; j<ismSEC_AUTH_ACTION_LAST; j++)
                    policy->actions[j] = rule->actions[j];
                policy->validate = validate;
                TRACE(8, "Policy %s (validate=%d) is updated\n", policy->name, policy->validate);
                ism_common_free(ism_memory_admin_misc,rule);
            } else {
                /* Type mismatch - can not have two different types of Policies with same name */
                TRACE(2, "Type mismath. : name=%s oldtype=%d newtype=%d\n", policy->name, policy->type, rule->type);
                rc = ISMRC_InvalidPolicy;
            }
            break;
        }
    }
    if (found == 0) {
        if ( rule->ClientAddress ){
            rc = ism_security_processClientAddress(rule, rule->ClientAddress);

            if ( rc != ISMRC_OK ) {
                if(serverState == ISM_SERVER_INITIALIZING){
                    LOG(ERROR, Admin, 6127, "%-s%-s%s%-s%d", "The rule specified as the {0} is not valid. Policy name={1}. {2}={3}. Return code={4}.",
                        "ClientAddress", rule->name, "ClientAddress", rule->ClientAddress, rc);
                    criticalConfigError = rc;
                    if(rc==ISMRC_ClientAddrMaxError){
                        ism_common_setErrorData(ISMRC_ClientAddrMaxError, "%d", MAX_ADDR_PAIRS);
                    }else{
                        ism_common_setError(rc);
                    }
                }else{
                    TRACE(2, "The rule specified as the ClientAddress is not valid. Policy name=%s. Client=%s. Return code: %d.\n", rule->name, rule->ClientAddress, rc);
                }
                pthread_rwlock_unlock(&policylock);
                return rc;
            }

        }

        if (( rule->UserID && strlen(rule->UserID) == 1 && *rule->UserID == '*' ) &&
            ( rule->ClientID && strlen(rule->ClientID) == 1 && *rule->ClientID == '*' ) &&
            ( rule->CommonNames && strlen(rule->CommonNames) == 1 && *rule->CommonNames == '*' ) &&
            ( rule->Protocol && strlen(rule->Protocol) == 1 && *rule->Protocol == '*' ) &&
            ( rule->Destination && strlen(rule->Destination) == 1 && *rule->Destination == '*' ))
        {
            rule->validate = 0;
        } else {
            rule->validate = 1;
        }

        /* validate rules for variable substitution */
        if ( rule->Destination ) {
            rc = ism_security_checkForMalformedPolicy(rule->Destination);
            if ( rc != ISMRC_OK ) {
                LOG(ERROR,Admin,6127,"%-s%-s%s%-s%d","The rule specified as the {0} is not valid. Policy name={1}. {2}={3}. Return code={4}.",
                    policyDestination, rule->name, policyDestination, rule->Destination, rc);
                if (serverState == ISM_SERVER_INITIALIZING) {
                    criticalConfigError = rc;
                    ism_common_setError(rc);
                }
                pthread_rwlock_unlock(&policylock);
                return rc;
            }
        }
        if ( rule->ClientID ) {
            rc = ism_security_checkForMalformedPolicy(rule->ClientID);
            if ( rc != ISMRC_OK ) {
                LOG(ERROR, Admin, 6127, "%-s%-s%s%-s%d", "The rule specified as the {0} is not valid. Policy name={1}. {2}={3}. Return code={4}.",
                    "ClientID", rule->name, "ClientID", rule->ClientID, rc);
                if (serverState == ISM_SERVER_INITIALIZING) {
                    criticalConfigError = rc;
                    ism_common_setError(rc);
                }
                pthread_rwlock_unlock(&policylock);
                return rc;
            }
        }

        ism_security_processProtoFamily(rule);
        rc = ism_security_addPolicyRule(rule, type);
        if ( rc == ISMRC_OK )
            TRACE(8, "Policy %s (validate=%d) is added\n", rule->name, rule->validate);
    }

    pthread_rwlock_unlock(&policylock);

    return rc;
}

/* Creates ism_prop_t structure from JSON string */
XAPI ism_prop_t * ism_security_createPolicyPropsFromJson(const char *policyBuf, int *type) {
    int buflen = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[20];

    if (policyBuf == NULL || *policyBuf != '{') {
        TRACE(5, "Can not create policy props - Policy buffer is NULL\n");
        return NULL;
    }

    /* Parse policy buffer and create rule definition */
    char *tmpbuf;
    buflen = strlen(policyBuf);
    tmpbuf = (char *)alloca(buflen + 1);
    memcpy(tmpbuf, policyBuf, buflen);
    tmpbuf[buflen] = 0;

    parseobj.ent = ents;
    parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseobj.source = (char *) tmpbuf;
    parseobj.src_len = buflen;
    ism_json_parse(&parseobj);

    if (parseobj.rc) {
        TRACE(3, "Failed to parse policy buffer (rc=%d): %s\n", parseobj.rc, policyBuf);
        return NULL;
    }

    const char *item = ism_json_getString(&parseobj, "Item");
    const char *name = ism_json_getString(&parseobj, "Name");

    if ( !name || *name == '\0' || !item ) {
        TRACE(5, "Can not create policy - invalid item (%s) or name (%s)\n",
            item?item:"NULL", name?name:"NULL");
        return NULL;
    }

    /* check policy type */
    if (item && strcasecmp(item, "ConfigurationPolicy") == 0) {
        *type = ismSEC_POLICY_CONFIG;
    } else if (item && strcasecmp(item, "ConnectionPolicy") == 0) {
        *type = ismSEC_POLICY_CONNECTION;
    } else if (item && strcasecmp(item, "TopicPolicy") == 0) {
        *type = ismSEC_POLICY_TOPIC;
    } else if (item && strcasecmp(item, "QueuePolicy") == 0) {
        *type = ismSEC_POLICY_QUEUE;
    } else {
        *type = -1;
        TRACE(3, "Invalid Policy type: item:%s name:%s\n", item, name);
        return NULL;
    }

    const char *value;
    ism_field_t f;
    ism_prop_t *props = ism_common_newProperties(32);

    f.type = VT_String;
    f.val.s = (void *)item;
    ism_common_setProperty(props, "Item", &f);

    f.type = VT_String;
    f.val.s = (void *)name;
    ism_common_setProperty(props, "Name", &f);

    value = ism_json_getString(&parseobj, "Description");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "Description", &f);
    }

    value = ism_json_getString(&parseobj, "UserID");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "UserID", &f);
    }

    value = ism_json_getString(&parseobj, "ActionList");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "ActionList", &f);
    }

    value = ism_json_getString(&parseobj, "GroupID");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "GroupID", &f);
    }

    value = ism_json_getString(&parseobj, "ClientID");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "ClientID", &f);
    }

    value = ism_json_getString(&parseobj, "ClientAddress");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "ClientAddress", &f);
    }

    value = ism_json_getString(&parseobj, "Protocol");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "Protocol", &f);
    }

    value = ism_json_getString(&parseobj, "CommonNames");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "CommonNames", &f);
    }

    value = ism_json_getString(&parseobj, "Topic");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "Topic", &f);
    }

    value = ism_json_getString(&parseobj, "Queue");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "Queue", &f);
    }

    value = ism_json_getString(&parseobj, "Subscription");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "Subscription", &f);
    }

    value = ism_json_getString(&parseobj, "Destination");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "Destination", &f);
    }

    value = ism_json_getString(&parseobj, "DestinationType");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "DestinationType", &f);
    }

    value = ism_json_getString(&parseobj, "MaxMessages");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "MaxMessages", &f);
    }

    value = ism_json_getString(&parseobj, "MaxMessagesBehavior");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "MaxMessagesBehavior", &f);
    }

    value = ism_json_getString(&parseobj, "DisconnectedClientNotification");
    if (value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "DisconnectedClientNotification", &f);
    }

    value = ism_json_getString(&parseobj, "MaxMessageTimeToLive");
    if (value && *value != '\0') {
        long ttlval = atol(value);
        f.type = VT_Long;
        f.val.l = ttlval;
        ism_common_setProperty(props, "MaxMessageTimeToLive", &f);
    }

    value = ism_json_getString(&parseobj, "DefaultSelectionRule");
    if (value) {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(props, "DefaultSelectionRule", &f);
    }

    return props;
}


/* create props for one policy */
static ism_prop_t * ism_security_createOnePolicyProp(ism_prop_t *props, char *polname, char *propname, int pos) {
    char *value;
    ism_field_t f;
    int found = 0;
    ism_prop_t *newProps = ism_common_newProperties(32);

    int cfglen = 0;
    char *cfgname = NULL;
    char *polType = alloca(pos+1);
    snprintf(polType, pos, "%s", propname);

    /* Process following properties:
     * Name, Description, DestinationType, UserID, ClientID, Protocol, Topic, Destination, Queue, Subscription,
     * DisconnectedClientNotification, GroupId, ClientAddress, ClientName, CommonName, Protocol. AllowDurable,
     * AllowPersistentMessages, ExpectedMessageRate, MaxSessionExpiryInterval, ActionList
     *
     * Maximum length of the configuration item = 30
     * Note: If you add any configuration item with more than 30 character long, you will have to update cfglen
     * cfglen = length of polType + length of configuration item name + policy name + 8
     * 8 is for dots, NULL terminator etc.
     */
    cfglen = pos + strlen(polname) + 30 + 8;
    cfgname = alloca(cfglen);

    snprintf(cfgname, cfglen, "%s.Name.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Name", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.Description.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Description", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.UserID.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "UserID", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.GroupID.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "GroupID", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.ActionList.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "ActionList", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.ClientID.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "ClientID", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.ClientAddress.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "ClientAddress", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.Protocol.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Protocol", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.CommonNames.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "CommonNames", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.Topic.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Topic", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.Queue.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Queue", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.Subscription.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Subscription", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.Destination.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "Destination", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.DestinationType.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "DestinationType", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.MaxMessages.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "MaxMessages", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.MaxMessagesBehavior.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "MaxMessagesBehavior", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.DisconnectedClientNotification.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "DisconnectedClientNotification", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.MaxMessageTimeToLive.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        long ttlval = atol(value);
        f.type = VT_Long;
        f.val.l = ttlval;
        ism_common_setProperty(newProps, "MaxMessageTimeToLive", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.DefaultSelectionRule.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value ) {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "DefaultSelectionRule", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.AllowDurable.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "AllowDurable", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.AllowPersistentMessages.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "AllowPersistentMessages", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.ExpectedMessageRate.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "ExpectedMessageRate", &f);
        found = 1;
    }

    snprintf(cfgname, cfglen, "%s.MaxSessionExpiryInterval.%s", polType, polname);
    value = (char *) ism_common_getStringProperty(props, cfgname);
    if ( value && *value != '\0') {
        f.type = VT_String;
        f.val.s = (void *)value;
        ism_common_setProperty(newProps, "MaxSessionExpiryInterval", &f);
        found = 1;
    }

    if ( found == 0 ) {
        ism_common_freeProperties(newProps);
        return NULL;
    }

    return newProps;
}

/* Create security policy rule from policy properties */
XAPI int ism_security_createPolicyFromProps(ism_prop_t *polprops, int type, char *polname, char *propname, int pos ) {
    int rc = ISMRC_OK;
    ism_prop_t *props = polprops;
    char *UserID=NULL;
    char *gid = NULL;
    char *action = NULL;
    char *name = NULL;

    char *cid = NULL;
    char *cip = NULL;
    char *protocol = NULL;
    char *cnames = NULL;
    char *destination = NULL;
    char *notifyStr = NULL;
    int  allowDurable            = 1;
    int  allowPersistentMessages = 1;
    ExpectedMsgRate_t expMsgRate = EXPECTEDMSGRATE_DEFAULT;
    uint32_t maxSessionExpiry = 0;
    ism_prop_t *newProps = NULL;

    /* MaxMessages, MaxMessagesBehavior, MaxMessageTimeToLive will be stored in context by engine*/
    int needFreeProps=0;

    if (props == NULL) {
        TRACE(5, "Can not create policy - Policy buffer and props NULL\n");
        return ISMRC_InvalidPolicy;
    }


    if ( propname ) {
        newProps = ism_security_createOnePolicyProp(props, polname, propname, pos);
        if ( newProps == NULL ) {
            TRACE(5, "Could not find valid policy properties: %s\n", propname);
            return ISMRC_InvalidPolicy;
        }
        needFreeProps=1;
        props = newProps;
    }


    name = (char *) ism_common_getStringProperty(props, "Name");
    if (!name || (*name == '\0') ) {
        TRACE(5, "Can not create policy - invalid name\n");
        return ISMRC_InvalidPolicy;
    }

    if ( type < ismSEC_POLICY_CONFIG || type >= ismSEC_POLICY_LAST ) {
        TRACE(5, "Can not create policy %s - invalid type: %d\n", name, type);
        return ISMRC_InvalidPolicy;
    }

    ismPolicyRule_t *policy = NULL;
    int i;

    /* Get common configuration items */
    UserID = (char *) ism_common_getStringProperty(props, "UserID");
    action = (char *) ism_common_getStringProperty(props, "ActionList");
    gid = (char *) ism_common_getStringProperty(props, "GroupID");
    cid = (char *) ism_common_getStringProperty(props, "ClientID");
    cip = (char *) ism_common_getStringProperty(props, "ClientAddress");
    cnames = (char *) ism_common_getStringProperty(props, "CommonNames");
    protocol = (char *) ism_common_getStringProperty(props, "Protocol");

    if ( type != ismSEC_POLICY_CONNECTION ) {
        if (!action || *action == '\0') {
            TRACE(3, "Invalid policy. Action list is not specified.\n");
            return ISMRC_InvalidPolicy;
        }
    }

    switch (type) {

    case ismSEC_POLICY_CONFIG: {
        /* Process configuration policy */
        if (!UserID && !gid && !cip && !cnames) {
            TRACE(3, "Invalid configuration policy. Could not find any valid item to create rule.\n");
            rc = ISMRC_InvalidPolicy;
            break;
        }

        TRACE(9, "Add ConfigPolicy: %s\n", name);
        policy = (ismPolicyRule_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,86),1, sizeof(ismPolicyRule_t));
        pthread_spin_init(&policy->lock, 0);
        policy->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        policy->type = ismSEC_POLICY_CONFIG;
        break;
    }

    case ismSEC_POLICY_CONNECTION: {
         /* Process Connection policy */
        char *value = (char *) ism_common_getStringProperty(props, "AllowDurable");
        if (value && !strcmpi(value, "False")) {
            allowDurable = 0;
        }

        value = NULL;
        value = (char *) ism_common_getStringProperty(props, "AllowPersistentMessages");
        if (value && !strcmpi(value, "False")) {
            allowPersistentMessages = 0;
        }

        value = NULL;
        value = (char *) ism_common_getStringProperty(props, "ExpectedMessageRate");
        if (value && (*value != '\0')) {
            if (!strcmpi(value, "Low")) {
                expMsgRate = EXPECTEDMSGRATE_LOW;
            } else if (!strcmpi(value, "High")) {
                expMsgRate = EXPECTEDMSGRATE_HIGH;
            } else if (!strcmpi(value, "Max")) {
                expMsgRate = EXPECTEDMSGRATE_MAX;
            }
        }

        maxSessionExpiry = ism_common_getUintProperty(props, "MaxSessionExpiryInterval", maxSessionExpiry);
        if (maxSessionExpiry == 0) {
            maxSessionExpiry = UINT32_MAX;
        } else if (maxSessionExpiry == UINT32_MAX) {
            maxSessionExpiry--;
        }

        if (!UserID && !gid && !cid && !cip && !protocol && !cnames) {
            TRACE(3, "Invalid connection policy. Could not find any valid item to create rule.\n");
            rc = ISMRC_InvalidPolicy;
            break;
        }

        TRACE(9, "Add ConnectionPolicy: %s\n", name);
        policy = (ismPolicyRule_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,87),1, sizeof(ismPolicyRule_t));
        pthread_spin_init(&policy->lock, 0);
        policy->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        policy->type = ismSEC_POLICY_CONNECTION;
        policy->AllowDurable = allowDurable;
        policy->AllowPersistentMessages = allowPersistentMessages;
        policy->ExpMsgRate = expMsgRate;
        policy->MaxSessionExpiry = maxSessionExpiry;

        for (i=0; i<ismSEC_AUTH_ACTION_LAST; i++)
            policy->actions[i] = 0;

        break;
    }

    case ismSEC_POLICY_TOPIC: {
        /* Process Topic Policy */
        destination = (char *) ism_common_getStringProperty(props, "Topic");
        if (!destination || (destination && *destination == '\0')) {
            TRACE(3, "Invalid Topic policy. Topic is not specified.\n");
            rc = ISMRC_InvalidPolicy;
            break;
        }

        TRACE(9, "Add TopicPolicy: %s\n", name);
        policy = (ismPolicyRule_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,88),1, sizeof(ismPolicyRule_t));
        pthread_spin_init(&policy->lock, 0);
        policy->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        policy->type = ismSEC_POLICY_TOPIC;
        policy->Destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),destination);
        policy->destType = ismSEC_AUTH_TOPIC;

        /* Get DisconnectedClientNotification config */
        notifyStr = (char *) ism_common_getStringProperty(props, "DisconnectedClientNotification");
        int notify = 0;
        if ( notifyStr && *notifyStr != '\0') {
            if ( *notifyStr=='T' || *notifyStr=='t' ) {
                notify = 1;
            } else {
                notify = 0;
            }
        }
        policy->DisconnectedClientNotification = notify;

        /*Start the DisconnectedClientNotification Thread */
        if ( notify > 0 &&
             disconnClientNotificationThreadStarted == 0) {
            if ( disconnClientNotificationThread != NULL ){
                disconnClientNotificationThread();
                disconnClientNotificationThreadStarted = 1;
            }
        }

        break;
    }

    case ismSEC_POLICY_QUEUE: {
        /* Process Queue Policy */
        destination = (char *) ism_common_getStringProperty(props, "Queue");
        if (!destination || (destination && *destination == '\0')) {
            TRACE(3, "Invalid Queue policy. No Queue name is specified.\n");
            rc = ISMRC_InvalidPolicy;
            break;
        }

        TRACE(9, "Add QueuePolicy: %s\n", name);
        policy = (ismPolicyRule_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,89),1, sizeof(ismPolicyRule_t));
        pthread_spin_init(&policy->lock, 0);
        policy->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        policy->type = ismSEC_POLICY_QUEUE;
        policy->Destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),destination);
        policy->destType = ismSEC_AUTH_QUEUE;

        break;
    }

    case ismSEC_POLICY_SUBSCRIPTION: {
        /* Process Subscription Policy */
        destination = (char *) ism_common_getStringProperty(props, "Subscription");
        if (!destination || (destination && *destination == '\0')) {
            TRACE(3, "Invalid Subscription policy. Subscription name is not specified.\n");
            rc = ISMRC_InvalidPolicy;
            break;
        }

        TRACE(9, "Add SubscriptionPolicy: %s\n", name);
        policy = (ismPolicyRule_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,90),1, sizeof(ismPolicyRule_t));
        pthread_spin_init(&policy->lock, 0);
        policy->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        policy->type = ismSEC_POLICY_SUBSCRIPTION;
        policy->Destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),destination);
        policy->destType = ismSEC_AUTH_SUBSCRIPTION;

        break;
    }

    default:
        rc = ISMRC_InvalidPolicy;
        break;

    }

    /* Process filters and action List */
    if ( rc == ISMRC_OK ) {

        /* check if ID or ClientID has variable substitution.
         * For variable substitution case, do not allow authorization based on other items
         */
        if (destination && *destination != '\0' ) {
            if ( strstr(destination, "${UserID}") ) {
                policy->varID = 1;
            }
            if ( strstr(destination, "${ClientID}")) {
                policy->varCL = 1;
            }
            if ( strstr(destination, "${CommonName}")) {
                policy->varCN = 1;
            }

            char *p = destination;
            if ( strstr(destination, "${GroupID}")) {
                /* Look for GroupID instances */
                int n = 0;
                while ((p = strstr(p, "${GroupID}"))) {
                    p += 10;
                    n += 1;

                    if ( n > 1 ) {
                        TRACE(1, "Policy:%s more than one GroupID found: %d\n", policy->name, n);
                        rc = ISMRC_GroupIDLimitExceeded;
                        goto CREATEPOL_ENDFUNC;
                    }
                }
                policy->varGR = 1;
                TRACE(7, "Policy:%s Variable GroupID found: %d\n", policy->name, policy->varGR);
            }
        }

        if ( UserID && *UserID != '\0')
            policy->UserID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),UserID);

        if ( cid && *cid != '\0')
            policy->ClientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cid);

        if ( gid && *gid != '\0') {
            string_strip_leading(gid);
            policy->GroupID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),gid);
            if ( policy->GroupLDAPEx ) ism_common_free(ism_memory_utils_to_lower,(void *)policy->GroupLDAPEx);
            policy->GroupLDAPEx = tolowerDN(policy->GroupID);
            //dn_normalize(policy->GroupLDAPEx);


            /*int groupLen = strlen(policy->GroupLDAPEx);
            int groupExtraLen = ism_admin_ldapHexExtraLen((char *)policy->GroupLDAPEx, groupLen);
            int newsize=0;

            if ( groupExtraLen > 0 ) {
                newsize= groupExtraLen + groupLen+1;
                policy->GroupLDAPEx = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,92),newsize);
                ism_admin_ldapHexEscape(&policy->GroupLDAPEx, (char *)policy->GroupID, groupLen);
                policy->GroupLDAPEx[newsize-1]='\0';

            }*/
        }

        if ( cip && *cip != '\0')
            policy->ClientAddress = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000), cip);

        if ( protocol && *protocol != '\0') {
            if ( *protocol != '*' && strcasecmp(protocol, "all")) {
                char *tmpstr = protocol;
                while (*tmpstr) {
                    *tmpstr = tolower(*tmpstr);
                    tmpstr++;
                }
            }
            if ( *protocol == '*' || strcasecmp(protocol, "all") == 0 ) {
                policy->Protocol = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"*");
            } else {
                policy->Protocol = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),protocol);
            }
        }

        if ( cnames && *cnames != '\0')
            policy->CommonNames = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cnames);


        /* Process action list */
        if ( type != ismSEC_POLICY_CONNECTION ) {

            for (i=0; i<ismSEC_AUTH_ACTION_LAST; i++)
                policy->actions[i] = 0;

            int len = strlen(action);
            char *tmpstr = (char *)alloca(len+1);
            memcpy(tmpstr, action, len);
            tmpstr[len] = 0;

            TRACE(7, "ActionTypes: %s\n", action);
            char *token, *nexttoken = NULL;
            for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken)) {
                if ( !token || (token && *token == '\0')) {
                    TRACE(3, "Invalid Policy:%s NULL action\n", name);
                } else {
                    int actionType = ism_security_getActionType(token);
                    if ( actionType == ismSEC_AUTH_ACTION_LAST ) {
                        TRACE(3, "Invalid Policy:%s InvalidAction:%s\n", name, token);
                        rc = ISMRC_InvalidPolicy;
                        goto CREATEPOL_ENDFUNC;
                    } else {
                        policy->actions[actionType] = 1;
                    }
                }
            }
        }

        /* Update policy rule */
        if ( rc == ISMRC_OK )
            rc = ism_security_addUpdatePolicyRule(policy, type);
    }

CREATEPOL_ENDFUNC:

    if (needFreeProps==1 && props!=NULL){
        ism_common_freeProperties(props);
    }
    if ( policy && rc != ISMRC_OK ) {
        ism_security_freePolicy(policy);
    }

    return rc;
}


/* Dynamic update of policy data */
XAPI int ism_security_dynamicPolicyUpdate(char *propname, int offset, ism_prop_t *props, const char * name, ismPolicyRule_t *policy, int type) {
    const char *newValue;
    int rc = ISMRC_OK;
    int cfglen = 0;
    char *cfgname = NULL;
    char *polType = alloca(offset+1);
    snprintf(polType, offset, "%s", propname);

    /* Process following configuration items:
     * UserID, DestinationType, Topic, Destination, Queue, Subscription , DisconnectedClientNotification
     * GroupId, ClientAddress, ClientName, CommonName, Protocol. AllowDurable, AllowPersistentMessages
     * AllowPersistentMessages, ExpectedMessageRate, MaxSessionExpiryInterval, ActionList
     *
     * Maximum length of the configuration item = 30
     * Note: If you add any configuration item with more than 30 character long, you will have to update cfglen
     * cfglen = length of polType + length of configuration item name + policy name + 8
     * 8 is for dots, NULL terminator etc.
     */
    cfglen = offset + strlen(name) + 30 + 8;
    cfgname = alloca(cfglen);

    snprintf(cfgname, cfglen, "%s.UserID.%s", polType, name);
    newValue = ism_common_getStringProperty(props, cfgname);

    if ( policy->UserID ) {
        TRACE(7, "Update Policy %s | update UserID=%s\n", policy->name, newValue?newValue:"");
        if ( newValue && *newValue != '\0' ) {
            if (strcmp(policy->UserID, newValue)) {
                ism_common_free(ism_memory_admin_misc,(void *)policy->UserID);
                policy->UserID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
            }
        } else if ( newValue && *newValue == '\0' ) {
            ism_common_free(ism_memory_admin_misc,(void *)policy->UserID);
            policy->UserID = NULL;
        }
    } else  {
        if ( newValue && *newValue != '\0' ) {
            TRACE(7, "Update Policy %s | add UserID=%s\n", policy->name, newValue);
            policy->UserID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        }
    }

    /* Process Destination type */
    if ( type == ismSEC_POLICY_MESSAGING ) {
        // get destination type
        snprintf(cfgname, cfglen, "%s.DestinationType.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
        if ( newValue && *newValue != '\0' ) {
            if ( strcasecmp(newValue, "Topic") == 0 ) {
                policy->destType = ismSEC_AUTH_TOPIC;
            } else if ( strcasecmp(newValue, "Queue") == 0 ) {
                policy->destType = ismSEC_AUTH_QUEUE;
            } else if ( strcasecmp(newValue, "Subscription") == 0 ) {
                policy->destType = ismSEC_AUTH_SUBSCRIPTION;
            }
         }
    }

    if ( type == ismSEC_POLICY_TOPIC ) {
        policy->destType = ismSEC_AUTH_TOPIC;
    } else if ( type == ismSEC_POLICY_QUEUE ) {
        policy->destType = ismSEC_AUTH_QUEUE;
    } else if ( type == ismSEC_POLICY_SUBSCRIPTION ) {
        policy->destType = ismSEC_AUTH_SUBSCRIPTION;
    }

    /* Update Destination */
    snprintf(cfgname, cfglen, "%s.Topic.%s", polType, name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( !newValue || (newValue && *newValue == '\0' )) {
       	snprintf(cfgname, cfglen, "%s.Destination.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
    }
    if ( !newValue || (newValue && *newValue == '\0' )) {
        snprintf(cfgname, cfglen, "%s.Queue.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
    }
    if ( !newValue || (newValue && *newValue == '\0' )) {
      	snprintf(cfgname, cfglen, "%s.Subscription.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
    }

    if ( policy->Destination ) {
        if ( newValue && *newValue != '\0' ) {
            if (strcmp(policy->Destination, newValue)) {
                ism_common_free(ism_memory_admin_misc,(void *)policy->Destination);
                policy->Destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
            }
        } else if ( newValue && *newValue == '\0' ) {
            ism_common_free(ism_memory_admin_misc,(void *)policy->Destination);
            policy->Destination = NULL;
        }
    } else  {
        if ( newValue && *newValue != '\0' ) {
            policy->Destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        }
    }

    if ( policy->Destination ) {
        rc = ism_security_checkForMalformedPolicy(policy->Destination);
        if ( rc != ISMRC_OK ) {
            TRACE(2, "Invallid rule in Destination config item of policy: %s : %s : rc=%d\n", policy->name, policy->Destination, rc);
            return rc;
        }


        /* check if ID or ClientID has variable substitution. */
        policy->varID = 0;
        policy->varCL = 0;
        policy->varCN = 0;
        if (policy->Destination && (strstr(policy->Destination, "${UserID}"))) {
            policy->varID = 1;
        }
        if (policy->Destination && strstr(policy->Destination, "${ClientID}")) {
            policy->varCL = 1;
        }
        if (policy->Destination && strstr(policy->Destination, "${CommonName}")) {
            policy->varCN = 1;
        }

        policy->varGR = 0;
        char *p = (char *)policy->Destination;
        if ( policy->Destination && strstr(policy->Destination, "${GroupID}")) {
            /* Look for GroupID instances */
            int n = 0;
            while ((p = strstr(p, "${GroupID}"))) {
                p += 10;
                n += 1;
            }
            if ( n > 1 ) {
                return ISMRC_GroupIDLimitExceeded;
            }
            policy->varGR = n;
        }
    }

    /* DisconnectedClientNotification - for Messaging and Topic Policy */
    if ( type == ismSEC_POLICY_MESSAGING || type == ismSEC_POLICY_TOPIC ) {
        int curNotify = 0;
        snprintf(cfgname, cfglen, "%s.DisconnectedClientNotification.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
        int notify = 0;
        if ( newValue ) {
            if(*newValue == 'T' || *newValue=='t'){
                notify=1;
            }else{
                notify=0;
            }
            if ( curNotify != notify ) curNotify = notify;
        }
        policy->DisconnectedClientNotification = notify;

        /*Start the DisconnectedClientNotification Thread */
        if ( curNotify > 0 &&
             disconnClientNotificationThreadStarted == 0) {
             if ( disconnClientNotificationThread != NULL ){
                disconnClientNotificationThread();
                disconnClientNotificationThreadStarted = 1;
            }
        }
    }

    /* Update GroupID */
    snprintf(cfgname, cfglen, "%s.GroupID.%s", polType, name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( policy->GroupID ) {
        if ( newValue && *newValue != '\0' ) {
            if (strcmp(policy->GroupID, newValue)) {
                ism_common_free(ism_memory_admin_misc,(void *)policy->GroupID);
                policy->GroupID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);

                        }
        } else if ( newValue && *newValue == '\0' ) {
            ism_common_free(ism_memory_admin_misc,(void *)policy->GroupID);
            policy->GroupID = NULL;
        }
    } else  {
        if ( newValue && *newValue != '\0' ) {
            policy->GroupID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        }
    }

    if (policy->GroupID != NULL) {

        if (policy->GroupLDAPEx) ism_common_free(ism_memory_utils_to_lower,(void *)policy->GroupLDAPEx);

        policy->GroupLDAPEx = tolowerDN(policy->GroupID);

        /*int groupLen = strlen(policy->GroupID);
        int groupExtraLen = ism_admin_ldapHexExtraLen((char *)policy->GroupID, groupLen);
        int newsize=0;

        if (groupExtraLen > 0) {
            newsize= groupExtraLen + groupLen+1;
            policy->GroupLDAPEx = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,100),newsize);
            ism_admin_ldapHexEscape(&policy->GroupLDAPEx, (char *)policy->GroupID, groupLen);
            policy->GroupLDAPEx[newsize-1]='\0';

        } else {
            policy->GroupLDAPEx = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),policy->GroupID);
        }*/
    } else {
        if (policy->GroupLDAPEx != NULL) {
            ism_common_free(ism_memory_utils_to_lower,(void *)policy->GroupLDAPEx);
            policy->GroupLDAPEx = NULL;
        }
    }

    /* check if ID or ClientID has variable substitution. */
    snprintf(cfgname, cfglen, "%s.ClientAddress.%s", polType, name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( policy->ClientAddress ) {
        if ( newValue && *newValue != '\0' ) {
            if (strcmp(policy->ClientAddress, newValue)) {
                rc = ism_security_processClientAddress(policy, newValue);
                if ( rc != ISMRC_OK ) {
                   TRACE(2, "Invalid rule in ClientAddress config item of policy: %s : %s : rc=%d\n", policy->name, policy->ClientAddress, rc);
                   return rc;
                }
            }
        } else if ( newValue && *newValue == '\0' ) {
            ism_common_free(ism_memory_admin_misc,(void *)policy->ClientAddress);
            policy->ClientAddress = NULL;
            policy->ipv4Pairs = 0;
            policy->ipv6Pairs = 0;
        }
    } else  {
        if ( newValue && *newValue != '\0' ) {
            policy->ClientAddress = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000), newValue);
            rc = ism_security_processClientAddress(policy, newValue);
            if ( rc != ISMRC_OK ) {
               TRACE(2, "Invalid rule in ClientAddress config item of policy: %s : %s : rc=%d\n", policy->name, policy->ClientAddress, rc);
               return rc;
           }
        }
    }

    snprintf(cfgname, cfglen, "%s.CommonNames.%s", polType, name);
    newValue = ism_common_getStringProperty(props, cfgname);
    if ( policy->CommonNames ) {
        if ( newValue && *newValue != '\0' ) {
            if (strcmp(policy->CommonNames, newValue)) {
                ism_common_free(ism_memory_admin_misc,(void *)policy->CommonNames);
                policy->CommonNames = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
            }
        } else if ( newValue && *newValue == '\0' ) {
            ism_common_free(ism_memory_admin_misc,(void *)policy->CommonNames);
            policy->CommonNames = NULL;
        }
    } else  {
        if ( newValue && *newValue != '\0' ) {
            policy->CommonNames = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
        }
    }

    if ( type != ismSEC_POLICY_CONFIG ) {
        snprintf(cfgname, cfglen, "%s.Protocol.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
        if ( policy->Protocol ) {
            if ( newValue && *newValue != '\0' ) {
                if (strcmp(policy->Protocol, newValue)) {
                    ism_common_free(ism_memory_admin_misc,(void *)policy->Protocol);
                    if ( *newValue == '*' || strcasecmp(newValue, "all") == 0 ) {
                        policy->Protocol = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"*");
                    } else {
                        policy->Protocol = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
                    }
                }
            } else if ( newValue && *newValue == '\0' ) {
                ism_common_free(ism_memory_admin_misc,(void *)policy->Protocol);
                policy->Protocol = NULL;
            }
            ism_security_processProtoFamily(policy);
        } else  {
            if ( newValue && *newValue != '\0' ) {
                policy->Protocol = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
                ism_security_processProtoFamily(policy);
            }
        }

        snprintf(cfgname, cfglen, "%s.ClientID.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
        if ( policy->ClientID ) {
            if ( newValue && *newValue != '\0') {
                if (strcmp(policy->ClientID, newValue)) {
                    ism_common_free(ism_memory_admin_misc,(void *)policy->ClientID);
                    policy->ClientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
                }
            } else if ( newValue && *newValue == '\0' ) {
                ism_common_free(ism_memory_admin_misc,(void *)policy->ClientID);
                policy->ClientID = NULL;
            }
        } else  {
            if ( newValue && *newValue != '\0' ) {
                policy->ClientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newValue);
            }
        }

        if ( policy->ClientID ) {
            rc = ism_security_checkForMalformedPolicy(policy->ClientID);
            if ( rc != ISMRC_OK ) {
                TRACE(2, "Invallid rule in Destination config item of policy: %s : %s : rc=%d\n", policy->name, policy->ClientID, rc);
                return rc;
            }
        }

        /* check AllowDurable. */
        snprintf(cfgname, cfglen, "%s.AllowDurable.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);

        if (newValue) {
                if (!strcmpi(newValue, "False")) {
                    policy->AllowDurable = 0;
                }else if (!strcmpi(newValue, "True")) {
                    policy->AllowDurable = 1;
                }else {
                    TRACE(3, "updatePolicy: AllowDurable has invalid value:%s\n", newValue);
                }
        }

        /* check AllowPersistentMessages. */
        snprintf(cfgname, cfglen, "%s.AllowPersistentMessages.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
        if (newValue) {
            if (!strcmpi(newValue, "False")) {
                policy->AllowPersistentMessages = 0;
            } else if (!strcmpi(newValue, "True")) {
                policy->AllowPersistentMessages = 1;
            } else {
                TRACE(3, "updatePolicy: AllowPersistentMessages has invalid value:%s\n", newValue);
            }
       }

        /* check for ExpectedMessageRate */
        snprintf(cfgname, cfglen, "%s.ExpectedMessageRate.%s", polType, name);
        newValue = (char *) ism_common_getStringProperty(props, cfgname);
        if (newValue && (*newValue != '\0')) {
            /* TRACE(1, "NEWVALUE: '%s'\n", newValue); */
            if (!strcmpi(newValue, "Low")) {
                policy->ExpMsgRate = EXPECTEDMSGRATE_LOW;
            } else if (!strcmpi(newValue, "Default")) {
                policy->ExpMsgRate = EXPECTEDMSGRATE_DEFAULT;
            } else if (!strcmpi(newValue, "High")) {
                policy->ExpMsgRate = EXPECTEDMSGRATE_HIGH;
            } else if (!strcmpi(newValue, "Max")) {
                policy->ExpMsgRate = EXPECTEDMSGRATE_MAX;
            }
        }

        /* check for MaxSessionExpiryInterval */
        snprintf(cfgname, cfglen, "%s.MaxSessionExpiryInterval.%s", polType, name);
        newValue = (char *) ism_common_getStringProperty(props, cfgname);
        if (newValue && (*newValue != '\0')) {
            uint32_t maxSessionExpiry = ism_common_getUintProperty(props, cfgname, 0);
            if (maxSessionExpiry == 0) {
                maxSessionExpiry = UINT32_MAX;
            } else if (maxSessionExpiry == UINT32_MAX) {
                maxSessionExpiry--;
            }
            policy->MaxSessionExpiry = maxSessionExpiry;
        }
    }

    if ( type != ismSEC_POLICY_CONNECTION ) {
        /* check if current action list is specified */
        int actSpecified = 0;
        int i;
        for (i=0; i<ismSEC_AUTH_ACTION_LAST; i++) {
            if ( policy->actions[i] == 1 ) {
                actSpecified = 1;
                break;
            }
        }

        snprintf(cfgname, cfglen, "%s.ActionList.%s", polType, name);
        newValue = ism_common_getStringProperty(props, cfgname);
        if ( actSpecified == 0 && ( !newValue || (newValue && *newValue == '\0'))) {
            TRACE(3, "Action field is required for updating policy:%s\n", name);
            rc = ISMRC_InvalidPolicy;
            return rc;
        }

        if ( actSpecified == 1 && ( !newValue || (newValue && *newValue == '\0')))
            return rc;

        for (i=0; i<ismSEC_AUTH_ACTION_LAST; i++)
            policy->actions[i] = 0;

        TRACE(9, "Set ActionTypes=%s for Policy=%s PolicyType=%d\n", newValue, name, type);

        int nlen = strlen(newValue);
        char *tmpstr = (char *)alloca(nlen+1);
        memcpy(tmpstr, newValue, nlen);
        tmpstr[nlen] = 0;

        char *token, *nexttoken = NULL;
        for (token = strtok_r(tmpstr, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken)) {
            if ( !token || (token && *token == '\0')) {
                TRACE(3, "Invalid ConfigPolicy:%s NULL action\n", name);
            } else {
                int actionType = ism_security_getActionType(token);
                if ( actionType == ismSEC_AUTH_ACTION_LAST ) {
                    TRACE(3, "Invalid ConfigPolicy:%s InvalidAction:%s\n", name, token);
                    rc = ISMRC_InvalidPolicy;
                } else {
                    policy->actions[actionType] = 1;
                }
            }
        }
    }

    if ( rc == ISMRC_OK )
        policy->deleted = 0;
    return rc;
}


/* Update policies in the policy table */
XAPI int ism_security_policy_update(ism_prop_t *props, char *oldname, int flag) {
    int rc = ISMRC_OK;
    int needCallBack = 1;

    if (!props)
        return ISMRC_NullPointer;

    if (flag == ISM_CONFIG_CHANGE_DELETE) {
        needCallBack = ism_common_getBooleanProperty(props, "InvokeEngineCallBack", 1);
        TRACE(5, "Need to invoke engine callback for this policy update:%d\n", needCallBack);
    }

    /* loop thru the list and create security Policies */
    const char * propertyName = NULL;
    int i = 0;
    for (i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++) {
        const char *name = NULL;
        int pos = 0;
        int type = 0;
        int proplen = strlen(propertyName);
        char *contextPropName = NULL;
        int contextPropNameLen = 0;
        char *policyObject = "ConnectionPolicy";

        TRACE(9,"Process policy configuration item %s\n", propertyName);

        if (proplen >= 22 && !memcmp(propertyName, "ConnectionPolicy.Name.",22)) {
            name = propertyName + 22;
            pos = 17;
            type = ismSEC_POLICY_CONNECTION;
        } else if (proplen >= 17 && !memcmp(propertyName, "TopicPolicy.Name.", 17)) {
            name = propertyName + 17;
            pos = 12;
            type = ismSEC_POLICY_TOPIC;
            contextPropNameLen = strlen("TopicPolicy.Context.") + strlen(name) + 1;
            contextPropName = alloca(contextPropNameLen);
            snprintf(contextPropName, contextPropNameLen, "TopicPolicy.Context.%s", name);
            policyObject = "TopicPolicy";
        } else if (proplen >= 17 && !memcmp(propertyName, "QueuePolicy.Name.", 17)) {
            name = propertyName + 17;
            pos = 12;
            type = ismSEC_POLICY_QUEUE;
            contextPropNameLen = strlen("QueuePolicy.Context.") + strlen(name) + 1;
            contextPropName = alloca(contextPropNameLen);
            snprintf(contextPropName, contextPropNameLen, "QueuePolicy.Context.%s", name);
            policyObject = "QueuePolicy";
        } else if (proplen >= 24 && !memcmp(propertyName, "SubscriptionPolicy.Name.", 24)) {
            name = propertyName + 24;
            pos = 19;
            type = ismSEC_POLICY_SUBSCRIPTION;
            contextPropNameLen = strlen("SubscriptionPolicy.Context.") + strlen(name) + 1;
            contextPropName = alloca(contextPropNameLen);
            snprintf(contextPropName, contextPropNameLen, "SubscriptionPolicy.Context.%s", name);
            policyObject = "SubscriptionPolicy";
        } else if (proplen >= 25 && !memcmp(propertyName, "ConfigurationPolicy.Name.", 25)) {
            name = propertyName + 25;
            pos = 20;
            type = ismSEC_POLICY_CONFIG;
            policyObject = "ConfigurationPolicy";
        }

        if (pos > 0) {
            TRACE(5,"Process (flag=%d) security policy configuration item %s\n", flag, propertyName);
            /* get value */
            const char *value = ism_common_getStringProperty(props, propertyName);
            if (!value && flag != ISM_CONFIG_CHANGE_NAME) {
                TRACE(5, "Name is NULL. Can not update policy. CFG:%s\n", propertyName);
                rc = ISMRC_InvalidPolicy;
                break;
            }

            /* get engine callback to inform of messaging policy change */
            ism_config_callback_t callback;

            int sState = ism_admin_get_server_state();
            if ( ( sState == ISM_SERVER_RUNNING || sState == ISM_MESSAGING_STARTED ) &&
                ( type == ismSEC_POLICY_MESSAGING || type == ismSEC_POLICY_TOPIC || type == ismSEC_POLICY_QUEUE || type == ismSEC_POLICY_SUBSCRIPTION ) ) {
                callback = ism_config_getCallback(ISM_CONFIG_COMP_ENGINE);
            } else {
                callback = NULL;
            }

            /* get current policy */
            ismPolicyRule_t *policy = ism_security_getPolicyByName((char *)value, type);

            /* Add or update property */
            if (flag == ISM_CONFIG_CHANGE_PROPS) {
                /* add policy */
                if (!policy) {
                    rc = ism_security_createPolicyFromProps(props, type, (char *)name, (char *)propertyName, pos);
                    if (rc != ISMRC_OK) {
                        TRACE(5, "Failed to add policy %s of type %d.\n", name, type);
                        break;
                    }
                /* update policy */
                } else {
                    rc = ism_security_dynamicPolicyUpdate((char *)propertyName, pos, props, name, policy, type);
                    if (rc != ISMRC_OK) {
                        TRACE(3, "Failed to update policy: %s\n", policy->name);
                    }
                }

                /* invoke engine callback to return change ensuring the Context is passed */
                if ( rc == ISMRC_OK && callback ) {
                    pthread_rwlock_wrlock(&policylock);
                    if (policy) {
                        ism_field_t f;
                        f.type = VT_Long;
                        f.val.l = (int64_t)(policy->context);
                        ism_common_setProperty(props,contextPropName,&f);
                    }
                    int rc1 = callback(policyObject, (char *)name, props, flag);
                    if ( rc1 != ISMRC_OK )
                        TRACE(5, "Unable to send %s policy %s to engine. rc=%d\n", policy ? "update" : "create", name, rc1);
                    pthread_rwlock_unlock(&policylock);
                    if (policy) {
                        ism_common_setProperty(props,contextPropName,NULL);
                    }
                }
                continue;
            }

            /* Delete policy */
            if (flag == ISM_CONFIG_CHANGE_DELETE) {
                if (!policy) {
                    TRACE(5, "The delete from cache is ignored. Trying to delete a non-existing policy: %s\n", propertyName);
                    rc = ISMRC_OK;
                } else {
                    pthread_rwlock_wrlock(&policylock);

                    policy->deleted = 1;
                    /* invoke engine callback to return change ensuring the context is passed */
                    if ( callback ) {
                        ism_prop_t *changedProps = ism_common_newProperties(1);
                        ism_field_t f;
                        f.type = VT_Long;
                        f.val.l = (int64_t)(policy->context);
                        ism_common_setProperty(changedProps,contextPropName,&f);
                        if (needCallBack) {
                            rc = callback(policyObject, (char *)name, changedProps, flag);
                        }
                        ism_common_freeProperties(changedProps);
                    }

                    /* reset policy */
                    if ( policy->UserID ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->UserID);
                        policy->UserID = NULL;
                    }
                    if ( policy->GroupID ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->GroupID);
                        policy->GroupID = NULL;
                    }
                    if ( policy->CommonNames ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->CommonNames);
                        policy->CommonNames = NULL;
                    }
                    if ( policy->ClientID ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->ClientID);
                        policy->ClientID = NULL;
                    }
                    if ( policy->ClientAddress ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->ClientAddress);
                        policy->ClientAddress = NULL;
                    }
                    if ( policy->Protocol ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->Protocol);
                        policy->Protocol = NULL;
                    }

                    if ( policy->Destination ) {
                        ism_common_free(ism_memory_admin_misc,(void *)policy->Destination);
                        policy->Destination = NULL;
                    }

                    policy->context = NULL;

                    pthread_rwlock_unlock(&policylock);
                }
                continue;
            }
        }

    }
    return rc;
}


/* Reads a file and returns a buffer */
static char * ism_security_fileToBuffer(const char *fname, int *rc, int mode) {
    char *polstr = NULL;
    size_t len = 0, bread = 0;
    FILE *f;
    char fileName[1024];

    *rc = ISMRC_OK;

    snprintf(fileName, 1024, "%s", fname);

    len = strlen(fileName);
    TRACE(9, "Process policy file: %s\n", fileName);

    /* READ policy from policy file */
    f = fopen(fileName, "rb");
    if (!f) {
        TRACE(2, "Unable to open a policy file: %s\n", fileName);
        *rc = ISMRC_Error;
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    polstr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,116),len + 2);
    if (!polstr) {
        TRACE(2, "Unable to allocate memory for policy document.");
        fclose(f);
        *rc = ISMRC_AllocateError;
        return NULL;
    }
    rewind(f);
    bread = fread(polstr, 1, len, f);
    polstr[bread] = 0;
    polstr[bread + 1] = 0;
    if (bread != len) {
        TRACE(2, "Unable to read a policy file: %s\n", fileName);
        fclose(f);
        ism_common_free(ism_memory_admin_misc,polstr);
        return NULL;
    }

    fclose(f);

    return polstr;
}

/* Adds a policy from a file or a json string
 * - used by cunit tests
 */
XAPI int ism_security_addPolicy(const char *filename, char *polstr) {
    int rc = ISMRC_OK;

    if (!filename) {
        TRACE(6, "Policy file name or Policy string is NULL.\n");
        return ISMRC_InvalidPolicy;
    }

    if (polstr == NULL) {
        polstr = ism_security_fileToBuffer(filename, &rc, 1);
        if (polstr == NULL)
            return rc;
    }

    int type = 0;
    ism_prop_t * props = ism_security_createPolicyPropsFromJson(polstr, &type );

    if (props == NULL)
        return ISMRC_InvalidPolicy;

    rc = ism_security_createPolicyFromProps(props, type, NULL, NULL, 0);
    return rc;
}

/**************************************************************************
 *           Functions related to policy based authorization
 **************************************************************************/

/* check for Protocol Family */
int ism_security_checkProtocol(ismSecurity_t *ctx, ismPolicyRule_t *pol) {
    int match = 0;
    char *proto = (char *)ctx->transport->protocol_family;

    if (pol->protofam_count==0) {
        /*No filter for Protocol*/
        match = 1;
    } else {
        if ( proto ) {
            int count=0;
            int protoid = ism_admin_getProtocolId(proto);
            for (count =0; count < pol->protofam_count; count++) {
                if (protoid == pol->protofam[count]) {
                    match = 1;
                    break;
                }
            }
        }
    }

    if ( match == 0 ) {
        TRACE(9, "Protocol family check failed: %s : %s. Protocol Rule Count: %d\n", proto?proto:"", pol->Protocol, pol->protofam_count);
    }
    return match;
}

/* compare IN6 address */
int ism_security_cmp_in6addr(struct in6_addr * addr1, struct in6_addr * addr2) {
    int ndx;
    int cmp = 0;
    for (ndx = 0; ndx < 16; ndx++) {
        if (addr1->s6_addr[ndx] < addr2->s6_addr[ndx]) {
            cmp = -1;
            break;
        }
        if (addr1->s6_addr[ndx] > addr2->s6_addr[ndx]) {
            cmp = 1;
            break;
        }
    }

    return cmp;
}

/* match IPV4 or IPV6 address in a range */
int ism_security_matchIPAddress(ismPolicyRule_t *pol, const char *address) {
    int match = 0;

    /* check for * in pol */
    if ( pol && pol->ClientAddress && *(pol->ClientAddress) == '*' ) {
        return 1;
    }

    /* check type */
    int type = 1;  /* default ipv4 */
    if ( strstr(address, ":")) type = 2;   /* ipv6 address */

    int i = 0;
    if ( type == 1 ) {
        uint32_t caddr = ntohl(inet_addr(address));
        for ( i = 0; i < pol->ipv4Pairs; i++) {   /* BEAM suppression: operating on NULL */
            if ( caddr >= pol->low4[i] && caddr <= pol->high4[i] ) {
                match = 1;
                break;
            }
        }
    } else {
        struct in6_addr claddr;

        char buf[64];
        memset(buf, 0, 64);
        while(*address) {
            if ( *address == '[' || *address == ']' ) {
                address++;
                continue;
            }
            buf[i++] = *address;
            address++;
        }

        int rc = inet_pton(AF_INET6, buf, &claddr);
        if ( rc == 1 ) {
            for ( i = 0; i < pol->ipv6Pairs; i++) {                       /* BEAM suppression: operating on NULL */
                if ( ism_security_cmp_in6addr(&claddr, &pol->low6[i]) >= 0
                     && ism_security_cmp_in6addr(&claddr, &pol->high6[i]) <= 0) {
                    match = 1;
                    break;
                }
            }
        }
    }

    return match;
}

/* check for Client Address */
int ism_security_checkAddress(ismSecurity_t *ctx, ismPolicyRule_t *pol) {
    int match = 0;
    /* check if transport is valid */
    if ( ctx->transport->client_addr && ism_security_matchIPAddress(pol, ctx->transport->client_addr)) {
            TRACE(9, "Client address check passed: %s : %s\n",
                ctx->transport->client_addr?
                    ctx->transport->client_addr : "", pol->ClientAddress);
            match = 1;
    } else {
            TRACE(9, "Client address check failed: %s : %s\n",
                ctx->transport->client_addr?
                    ctx->transport->client_addr : "", pol->ClientAddress);
            match = 0;
    }
    return match;
}

/* Check if member belongs to OAuth Group */
int ism_security_isUserInOauhGroups(char *gid, char *groupList, char *dchar) {
    if (!gid || *gid == '\0' || !groupList || *groupList == '\0')
        return 0;

    char *p, *gtoken, *gnexttoken = NULL;
    int len = strlen(groupList);
    char *goption = alloca(len + 1);
    memcpy(goption, groupList, len);
    goption[len] = '\0';

    for (gtoken = strtok_r(goption, dchar, &gnexttoken); gtoken != NULL; gtoken = strtok_r(NULL, dchar, &gnexttoken))
    {
        /* remove leading space */
        while ( *gtoken == ' ' )
            gtoken++;
        if ( *gtoken == 0 )
            return 0;
        /* remove trailing space */
        p = gtoken + strlen(gtoken) - 1;
        while ( p > gtoken && *p == ' ' )
            p--;
        *(p+1) = 0;

        /* compare with valueStr */
        if ( ism_common_match(gtoken, gid) )  {
            return 1;
        }
    }

    return 0;
}


/* check for ID in a group */
int ism_security_checkGID(ismSecurity_t *ctx, ismPolicyRule_t *pol) {
    int match = 0;
    char *UserID = (char *)ctx->transport->userid;
    char *gid = (char *)pol->GroupID;

    if ( *gid == '*' )
        return 1;

    /* get groups for the user */
    if ( UserID ) {
        /* validate for oauthGroup if set in security context, otherwise use LDAP group to validate */
        char *groupList = (char *)ctx->oauthGroup;
        if ( groupList ) {
            char *groupDelimiter = (char *)ism_security_context_getOAuthGroupDelimiter(ctx);
            if ( ism_security_isUserInOauhGroups(gid, groupList, groupDelimiter) )  {
                match = 1;
            } else {
                TRACE(9, "Oauth Group check failed: %s : %s\n", groupList, gid);
            }
        } else {
            if ( ism_security_isMemberBelongsToGroup(ctx,UserID, (char *)pol->GroupLDAPEx) > 0 )  {
                match = 1;
            }
        }
    } else {
        TRACE(9, "Group check failed for UserID: %s : %s\n", UserID?UserID:"", pol->GroupID);
    }
    return match;
}

/*
 * Get a slot for a policy in the security context
 */
ismPolicyRule_t * * getPolicySlot(ismSecurity_t * secContext) {
    if (secContext->policy_slot_pos == secContext->policy_slot_alloc) {
        if (!secContext->policy_inheap) {
            secContext->policies = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,118),32, sizeof(ismPolicyRule_t *));
            if (!secContext->policies)
                return NULL;
            secContext->policy_inheap = 1;
            secContext->policy_slot_alloc = 32;
            memcpy(secContext->policies, secContext->policy_slots, secContext->policy_slot_pos * sizeof (ismPolicyRule_t *));
        } else {
            int newsize = secContext->policy_slot_alloc * 4;
            ismPolicyRule_t * * newp = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,119),newsize, sizeof (ismPolicyRule_t *));
            if (!newp)
                return NULL;
            memcpy(newp, secContext->policies, secContext->policy_slot_alloc * sizeof (ismPolicyRule_t *));
            ism_common_free(ism_memory_admin_misc,secContext->policies);
            secContext->policies = newp;
            secContext->policy_slot_alloc = newsize;
        }
    }
    return secContext->policies + (secContext->policy_slot_pos++);
}

/*
 * Revalidate policies in security context
 */
void ism_security_reValidatePoliciesInSecContext(ismSecurity_t *secContext) {
    int nPol = 0;
    ismPolicyRule_t * * slot;
    pthread_spin_lock(&secContext->lock);
    char * token;
    char * tmpstr;
    char   xbuf [2048];
    int    pollen;

    /* Zero out all existing policies */
    ism_transport_t *tport = secContext->transport;
    secContext->conPolicyPtr = NULL;
    secContext->topicPolicyPtr = NULL;
    secContext->queuePolicyPtr = NULL;
    secContext->subsPolicyPtr = NULL;
    if (secContext->policy_inheap) {
        ism_common_free(ism_memory_admin_misc,secContext->policies);
        secContext->policy_inheap = 0;
    }
    secContext->policy_slot_alloc = SEC_CTX_POLICY_SLOTS;
    secContext->policies = secContext->policy_slots;

    /* revalidate connection or configuration Policies */
    if (tport && tport->listener && tport->listener->conpolicies) {
        TRACEL(9, tport->trclevel, "ReValidate connection policies: \"%s\"\n",
                tport->listener->conpolicies);
        pollen = strlen(tport->listener->conpolicies);
        tmpstr = pollen < 2048 ? xbuf : alloca(pollen + 1);
        tmpstr[pollen] = 0;
        memcpy(tmpstr, tport->listener->conpolicies, pollen);
        token = nextToken(&tmpstr);
        while (token) {
            /* get policy pointer */
            ismPolicyRule_t *policy = NULL;
            if ( tport->listener->isAdmin == 1 ) {
                policy = ism_security_getPolicyByName(token, ismSEC_POLICY_CONFIG);
            } else {
                policy = ism_security_getPolicyByName(token, ismSEC_POLICY_CONNECTION);
            }
            if (!policy) {
                TRACEL(9, tport->trclevel, "Invalid policy \"%s\" is defined for the Security context.", token);
            } else {
                TRACEL(9, tport->trclevel, "Update Security context: No:%d Name:\"%s\"\n", nPol, token);
                slot = getPolicySlot(secContext);
                if (!slot) {
                    TRACE(2, "No slot available in security policy: %s\n", token);
                    break;
                } else {
                    *slot = policy;
                    nPol++;

                    /*
                     * Check if Group ID Is set. If yes, set the checkGroup Flag for this context
                     * so we can get Group from LDAP
                     */
                     if (secContext->checkGroup!=1 && policy->GroupID!=NULL && *policy->GroupID!='\0'){
                         TRACEL(9, tport->trclevel, "Authorization: Enabled Security context checkGroup.\n");
                         secContext->checkGroup=1;
                     }
                }
            }
            token = nextToken(&tmpstr);
        }
    }
    secContext->noConPolicies = nPol;

    /* revalidate Topic Policies */
    nPol = 0;
    if ( tport && tport->listener && tport->listener->topicpolicies ) {
        TRACEL(9, tport->trclevel, "ReValidate Topic Policies: \"%s\"\n", tport->listener->topicpolicies);
        pollen = strlen(tport->listener->topicpolicies);
        tmpstr = pollen < 2048 ? xbuf : alloca(pollen + 1);
        tmpstr[pollen] = 0;
        memcpy(tmpstr, tport->listener->topicpolicies, pollen);
        token = nextToken(&tmpstr);
        while (token) {
            /* get policy pointer */
            ismPolicyRule_t *policy = ism_security_getPolicyByName(token, ismSEC_POLICY_TOPIC);
            if ( !policy ) {
                TRACEL(9, tport->trclevel, "Invalid policy \"%s\" is defined for the Security context.", token);
                break;
            } else {
                TRACEL(9, tport->trclevel, "Update Security context: No:%d Name:\"%s\"\n", nPol, token);
                slot = getPolicySlot(secContext);
                if (!slot) {
                    TRACE(2, "No slot available in security policy: %s\n", token);
                } else {
                    *slot = policy;
                    nPol++;

                    /*
                     * Check if Group ID Is set. If yes, set the checkGroup Flag for this context
                     * so we can get Group from LDAP
                     */
                     if(secContext->checkGroup!=1 && ( (policy->GroupID!=NULL && *policy->GroupID!='\0') ||
                             policy->varGR != 0 )){
                         TRACEL(9, tport->trclevel, "Authorization: Enabled Security context checkGroup.\n");
                         secContext->checkGroup=1;
                     }
                }
            }
            token = nextToken(&tmpstr);
        }
    }
    secContext->noTopicPolicies = nPol;

    /* revalidate Queue Policies */
    nPol = 0;
    if ( tport && tport->listener && tport->listener->qpolicies ) {
        TRACEL(9, tport->trclevel, "ReValidate Queue policies: \"%s\"\n", tport->listener->qpolicies);
        pollen = strlen(tport->listener->qpolicies);
        tmpstr = pollen < 2048 ? xbuf : alloca(pollen + 1);
        tmpstr[pollen] = 0;
        memcpy(tmpstr, tport->listener->qpolicies, pollen);
        token = nextToken(&tmpstr);
        while (token) {
            /* get policy pointer */
            ismPolicyRule_t *policy = ism_security_getPolicyByName(token, ismSEC_POLICY_QUEUE);
            if ( !policy ) {
                TRACEL(9, tport->trclevel, "Invalid policy \"%s\" is defined for the Security context.", token);
            } else {
                TRACEL(9, tport->trclevel, "Update Security context: No:%d Name:\"%s\"\n", nPol, token);
                slot = getPolicySlot(secContext);
                if (!slot) {
                    TRACE(2, "No slot available in security policy: %s\n", token);
                } else {
                    *slot = policy;
                    nPol++;

                    /*
                     * Check if Group ID Is set. If yes, set the checkGroup Flag for this context
                     * so we can get Group from LDAP
                     */
                     if(secContext->checkGroup!=1 && ( (policy->GroupID!=NULL && *policy->GroupID!='\0') || policy->varGR != 0 )){
                         TRACEL(9, tport->trclevel, "Authorization: Enabled Security context checkGroup.\n");
                         secContext->checkGroup=1;
                     }
                }
            }
            token = nextToken(&tmpstr);
        }
    }
    secContext->noQueuePolicies = nPol;

    /* revalidate Subscription Policies */
    nPol = 0;
    if ( tport && tport->listener && tport->listener->subpolicies ) {
        TRACEL(9, tport->trclevel, "ReValidate Subscription Policies: \"%s\"\n", tport->listener->subpolicies);
        pollen = strlen(tport->listener->subpolicies);
        tmpstr = pollen < 2048 ? xbuf : alloca(pollen + 1);
        tmpstr[pollen] = 0;
        memcpy(tmpstr, tport->listener->subpolicies, pollen);
        token = nextToken(&tmpstr);
        while (token) {
            /* get policy pointer */
            ismPolicyRule_t *policy = ism_security_getPolicyByName(token, ismSEC_POLICY_SUBSCRIPTION);
            if ( !policy ) {
                TRACEL(9, tport->trclevel, "Invalid policy \"%s\" is defined for the Security context.", token);
            } else {
                TRACEL(9, tport->trclevel, "Update Security context: No:%d Name:\"%s\"\n", nPol, token);
                slot = getPolicySlot(secContext);
                if (!slot) {
                    TRACE(2, "No slot available in security policy: %s\n", token);
                } else {
                    *slot = policy;
                    nPol++;

                    /*
                     * Check if Group ID Is set. If yes, set the checkGroup Flag for this context
                     * so we can get Group from LDAP
                     */
                    if(secContext->checkGroup!=1 && ( (policy->GroupID!=NULL && *policy->GroupID!='\0') || policy->varGR != 0 )){
                        TRACEL(9, tport->trclevel, "Authorization: Enabled Security context checkGroup.\n");
                        secContext->checkGroup=1;
                    }
                }
            }
            token = nextToken(&tmpstr);
        }
    }
    secContext->noSubsPolicies = nPol;

    secContext->conPolicyPtr = secContext->policies;
    secContext->topicPolicyPtr = secContext->policies + secContext->noConPolicies;
    secContext->queuePolicyPtr = secContext->policies + secContext->noConPolicies + secContext->noTopicPolicies;
    secContext->subsPolicyPtr = secContext->policies + secContext->noConPolicies + secContext->noTopicPolicies +
            secContext->noQueuePolicies;

    TRACE(9, "ReValidated: SecurityContext:%p ConnectionPolicies=%d TopicPolicies=%d QueuePolicies=%d SubscriptionPolicies=%d\n",
        secContext, secContext->noConPolicies, secContext->noTopicPolicies, secContext->noQueuePolicies, secContext->noSubsPolicies);

    secContext->reValidatePolicy = 0;
    pthread_spin_unlock(&secContext->lock);
}


/* Validate policy */
int ism_security_policyBasedAuthorization(
    ismSecurity_t *secContext,
    ismSecurityAuthObjectType_t objectType,
    const char * objectName,
    ismSecurityAuthActionType_t actionType,
    ism_ConfigComponentType_t compType,
    void ** context,
    concat_alloc_t *output_buffer)
{

    int rc = ISMRC_NotAuthorized;
    int i;
    char    *polFailName[MAX_POLICIES_PER_SECURITY_CONTEXT];
    int32_t  polFailRC[MAX_POLICIES_PER_SECURITY_CONTEXT];

    if (!secContext) {
        TRACE(2, "Policy validation failed, NULL security context. objectType=%d objectName=%s actionType=%d compType=%d\n",
                objectType, ((objectName) ? objectName : "NULL"), actionType, compType);
        return ISMRC_InvalidSecContext;
    }

    if ( actionType < 0 || actionType >= ismSEC_AUTH_ACTION_LAST ) {
        TRACE(2, "Policy validation failed, invalid action type %d: secContext: %p\n", actionType, secContext);
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "actionType");
        return ISMRC_ArgNotValid;
    }

    int validationType = ism_security_getValidationType(actionType);
    char *actionName = ism_security_getActionName(actionType);

    /* get endpoint address from security context */
    const char *endpointName = NULL;
    ism_transport_t *tport = secContext->transport;

    if ( tport && tport->listener ) {
        endpointName = tport->listener->name;
    }

    if ( doAuthorize == 0 || secContext->adminPort == 2)
        return ISMRC_OK;

    if ( secContext->reValidatePolicy == 1 ) {
        ism_security_reValidatePoliciesInSecContext(secContext);
    }

    /* TODO: remove this when messaging policy support is removed */
    TRACE(9, "Authorize: SecurityContext:%p actionType:%d objectType:%d validationType:%d\n", secContext, actionType, objectType, validationType);

#if 0
    if ( secContext->noMsgPolicies != 0 && (validationType == ismSEC_POLICY_TOPIC || validationType == ismSEC_POLICY_QUEUE || validationType == ismSEC_POLICY_SUBSCRIPTION ))
        validationType = ismSEC_POLICY_MESSAGING;

    /* Process Messaging Policy */
    if ( validationType == ismSEC_POLICY_MESSAGING ) {
        if ( secContext->noMsgPolicies == 0 ) {
            TRACE(2, "No Messaging policy is specified for the security context: %p Endpoint: %s\n", secContext, endpointName? endpointName:"NULL");
            return ISMRC_NotAuthorized;
        }

        /* create properties and run filter */
        if ( !objectName || *objectName == '\0' ) {
            TRACE(2, "Messaging policy validation failed, invalid argument. secContext: %p Endpoint: %s\n", secContext, endpointName? endpointName:"NULL");
            ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "NULL objectName");
            return ISMRC_ArgNotValid;
        }

        pthread_rwlock_rdlock(&policylock);

        int policy_found = 0;
        int match = 0;
        int setErr = 0;

        for (i = 0; i < secContext->noMsgPolicies; i++) {
            polFailName[i] = NULL;
            polFailRC[i]   = ISMRC_NotAuthorized;

            ismPolicyRule_t *policy = secContext->msgPolicyPtr[i];

            if (policy == NULL || policy->deleted == 1 || policy->destType != objectType )
                continue;

            TRACE(9, "Check messaging policy=%s | action=%d | policy->action=%d | destination=%s | secContext=%p | endpoint=%s | GroupIDs=%d\n",
                policy->name, actionType, policy->actions[actionType], objectName, secContext, endpointName? endpointName:"NULL", policy->varGR );

            policy_found = 1;
            match = 0;

            if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                polFailName[i] = policy->name;
                setErr += 1;
            }

            if (output_buffer) rc = ISMRC_OK;

            /* check policy action type first */
            if ( policy->actions[actionType] == 1) {
                if ( policy->validate == 0 ) {
                    match = 1;
                    if (output_buffer) continue;
                    break;
                }

                int destCheck = 0;
                if ( policy->varGR == 0 ) {
                    if ( ( destCheck = ism_common_matchWithVars(objectName, policy->Destination, (ima_transport_info_t *)secContext->transport, NULL, 0, 0 )) != 0 ) { /* BEAM suppression: operating on NULL */
                        TRACE(7, "Destination check failed: %s : %s : mwrc=%d\n", objectName, policy->Destination, destCheck);
                        rc = ISMRC_TopicNotAuthorized;
                        if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                        continue;
                    } else {
                        match = 1;
                    }
                } else {
                    if ( (destCheck = ism_security_validateDestinationWithGroupID(objectName, secContext, policy->varGR, policy->Destination)) != 0 ) {
                        TRACE(7, "Destination check failed: %s : %s : mwrc=%d\n", objectName, policy->Destination, destCheck);
                        rc = ISMRC_TopicNotAuthorized;
                        if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                        continue;
                    } else {
                        match = 1;
                    }
                }

                if ( policy->UserID &&
                       (match = ism_common_match(secContext->transport->userid, policy->UserID)) == 0 ) {                    /* BEAM suppression: operating on NULL */
                    TRACE(7, "ID check failed: %s : %s\n", secContext->transport->userid?secContext->transport->userid:"", policy->UserID);
                    rc = ISMRC_UserNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->GroupID && (match = ism_security_checkGID(secContext, policy)) == 0 ) {
                    rc = ISMRC_GroupNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->ClientID &&
                     (match = ism_common_match(secContext->transport->clientID, policy->ClientID)) == 0 ) {            /* BEAM suppression: operating on NULL */
                    TRACE(7, "ClientID check failed: %s : %s\n", secContext->transport->clientID?secContext->transport->clientID:"", policy->ClientID);
                    rc = ISMRC_ClientNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->Protocol && (match = ism_security_checkProtocol(secContext, policy)) == 0 ) {
                    rc = ISMRC_ProtoNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->ClientAddress && (match = ism_security_checkAddress(secContext, policy)) == 0 ) {
                    rc = ISMRC_AddrNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->CommonNames &&
                    (match = ism_common_match(secContext->transport->cert_name, policy->CommonNames)) == 0 ) {        /* BEAM suppression: operating on NULL */
                    TRACE(7, "CommonNames check failed: %s : %s\n", secContext->transport->cert_name?secContext->transport->cert_name:"", policy->CommonNames);
                    rc = ISMRC_CNameNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

            } else {
                rc = ISMRC_AuthorityCheckError;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
            }

            if ( match == 1 ) {
                rc = ISMRC_OK;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                if ( output_buffer )
                    continue;
#ifdef AUTH_TIME_STAT_ENABLED
                __sync_add_and_fetch(&statCount->author_msgn_passed, 1);
                statCount->author_total_time = statCount->author_total_time + (ism_common_readTSC() - startTime);
#endif
                if ( context != NULL ) {
                    *context = policy->context;
                }
                if ( auditLogControl > 1 ) {
/* BEAM suppression: operating on NULL */      LOG(INFO, Security, 6105, "%-s%-s%-s%-s%-s%-s%-s%-s%-s",
                        "The messaging policy {0} has authorized a client to {1} on Destination {2}. The connection identifiers are: Endpoint={3}, UserID={4}, ClientID={5}, ClientIPAddress={6}, CertificateCommonName={7}, Protocol={8}.",
                        policy->name, actionName, objectName, endpointName ? endpointName:"NULL",
                        (char *)secContext->transport->userid?(char *)secContext->transport->userid:"NULL",
                        (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                        (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                        (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL",
                        (char *)secContext->transport->protocol_family?(char *)secContext->transport->protocol_family:"NULL");  /* BEAM suppression: operating on NULL */
                }
                break;
            }
        }

        if ( policy_found == 0 ) {
            rc = ISMRC_NoMessagingPolicy;
            setErr = 1;
            polFailRC[0] = rc;
            polFailName[0] = NULL;
        }

#ifdef AUTH_TIME_STAT_ENABLED
        if ( match != 1 ) {
            __sync_add_and_fetch(&statCount->author_msgn_failed, 1);
            statCount->author_total_time = statCount->author_total_time + (ism_common_readTSC() - startTime);
        }
#endif

        if ( rc != ISMRC_OK && auditLogControl > 0 && output_buffer == NULL ) {
            char messageBuffer[256];
            char oauthInfo[1024];
            char *userid = (char *)secContext->transport->userid;
            if (secContext->oauthGroup) {
                snprintf(oauthInfo, 1024, "%s[%s]", userid?userid:"NULL", secContext->oauthGroup);
                userid = oauthInfo;
            }

/* BEAM suppression: operating on NULL */      LOG(NOTICE, Security, 6106, "%-s%-s%-s%-s%-s%-s%-s%-s",
                    "A client is not authorized to {0} on Destination {1}. The connection identifiers are: Endpoint={2}, UserID={3}, ClientID={4}, ClientIPAddress={5}, CertificateCommonName={6}, Protocol={7}.",
                   actionName, objectName, endpointName ? endpointName:"NULL",
                    userid?userid:"NULL",
                    (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                    (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                    (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL",
                       (char *)secContext->transport->protocol_family?(char *)secContext->transport->protocol_family:"NULL");   /* BEAM suppression: operating on NULL */

            /* LOG Error codes for each policy - at MAX level */
            for (int k=0; k<setErr; k++) {
                LOG(INFO, Security, 6130, "%-s%-s%-s%-s%-s%d", "Policy evaluated for {0}={1} to {2}: PolicyName={3}, Error={4} RC={5}",
                        "ClientID", (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                        actionName, polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
            }
        }

        /* Check and update output buffer - for test authority */
        if ( output_buffer ) {
            int first = 0;
            char rbuf[1024];
            for (int k=0; k < setErr; k++ ) {
                char messageBuffer[256];
                if ( first == 0 ) {
                    snprintf(rbuf, 1024, "{");
                    ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                }
                if ( first > 0 ) {
                    snprintf(rbuf, 1024, ",");
                    ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                }
                snprintf(rbuf, 1024, "\"%s\":\"%s (RC=%d)\"", polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
                ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                first += 1;
            }
            snprintf(rbuf, 1024, "}");
            ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
        }

        pthread_rwlock_unlock(&policylock);

        /* set error code to not authorized because this code is sent to client */
        if ( rc != ISMRC_OK )
            rc = ISMRC_NotAuthorized;

    } else
#endif
    if ( validationType == ismSEC_POLICY_TOPIC  || validationType == ismSEC_POLICY_QUEUE || validationType == ismSEC_POLICY_SUBSCRIPTION ) {

        /* Process TOPIC, Queue or Subscription Policy */

        /* Handle Receive - could be in Queue or Subscription policy */
        if ( objectType == ismSEC_AUTH_SUBSCRIPTION && validationType == ismSEC_POLICY_QUEUE )
            validationType = ismSEC_POLICY_SUBSCRIPTION;


        if ( secContext->noTopicPolicies == 0 && secContext->noQueuePolicies && secContext->noSubsPolicies ) {
            TRACE(2, "No policy is specified for the security context: %p Endpoint: %s\n", secContext, endpointName? endpointName:"NULL");
            return ISMRC_NotAuthorized;
        }

        /* create properties and run filter */
        if ( !objectName || *objectName == '\0' ) {
             TRACE(2, "Policy validation failed, invalid argument. secContext: %p Endpoint: %s\n", secContext, endpointName? endpointName:"NULL");
            ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "NULL objectName");
            return ISMRC_ArgNotValid;
        }

        pthread_rwlock_rdlock(&policylock);

        int policy_found = 0;
        int match = 0;
        int setErr = 0;

        int noPolicies = 0;
        if ( validationType == ismSEC_POLICY_TOPIC )
            noPolicies = secContext->noTopicPolicies;
        else if ( validationType == ismSEC_POLICY_QUEUE )
            noPolicies = secContext->noQueuePolicies;
        else if ( validationType == ismSEC_POLICY_SUBSCRIPTION )
            noPolicies = secContext->noSubsPolicies;

        for (i = 0; i < noPolicies; i++) {
            polFailName[i] = NULL;
            polFailRC[i]   = ISMRC_NotAuthorized;

            ismPolicyRule_t *policy = NULL;
            if ( validationType == ismSEC_POLICY_TOPIC )
                policy = secContext->topicPolicyPtr[i];
            else if ( validationType == ismSEC_POLICY_QUEUE )
                policy = secContext->queuePolicyPtr[i];
            else if ( validationType == ismSEC_POLICY_SUBSCRIPTION )
                policy = secContext->subsPolicyPtr[i];

            if (policy == NULL || policy->deleted == 1 || policy->destType != objectType )
                continue;

            TRACE(9, "Check topic policy=%s | action=%d | policy->action=%d | topic=%s | secContext=%p | endpoint=%s | GroupIDs=%d\n",
                policy->name, actionType, policy->actions[actionType], objectName, secContext, endpointName? endpointName:"NULL", policy->varGR );

            policy_found = 1;
            match = 0;

            if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                polFailName[i] = policy->name;
                setErr += 1;
            }

            if (output_buffer) rc = ISMRC_OK;

            /* check policy action type first */
            if ( policy->actions[actionType] == 1) {
                if ( policy->validate == 0 ) {
                    match = 1;
                    if (output_buffer) continue;
                    break;
                }

                int destCheck = 0;
                if ( policy->varGR == 0 ) {
                    if ( ( destCheck = ism_common_matchWithVars(objectName, policy->Destination, (ima_transport_info_t *)secContext->transport, NULL, 0, 0 )) != 0 ) { /* BEAM suppression: operating on NULL */
                        TRACE(7, "Destination check failed: %s : %s : mwrc=%d\n", objectName, policy->Destination, destCheck);
                        rc = ISMRC_TopicNotAuthorized;
                        if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                        continue;
                    } else {
                        match = 1;
                    }
                } else {
                    if ( (destCheck = ism_security_validateDestinationWithGroupID(objectName, secContext, policy->varGR, policy->Destination)) != 0 ) {
                        TRACE(7, "Destination check failed: %s : %s : mwrc=%d\n", objectName, policy->Destination, destCheck);
                        rc = ISMRC_TopicNotAuthorized;
                        if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                        continue;
                    } else {
                        match = 1;
                    }
                }

                if ( policy->UserID &&
                       (match = ism_common_match(secContext->transport->userid, policy->UserID)) == 0 ) {                    /* BEAM suppression: operating on NULL */
                    TRACE(7, "ID check failed: %s : %s\n", secContext->transport->userid?secContext->transport->userid:"", policy->UserID);
                    rc = ISMRC_UserNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->GroupID && (match = ism_security_checkGID(secContext, policy)) == 0 ) {
                    rc = ISMRC_GroupNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->ClientID &&
                     (match = ism_common_match(secContext->transport->clientID, policy->ClientID)) == 0 ) {            /* BEAM suppression: operating on NULL */
                    TRACE(7, "ClientID check failed: %s : %s\n", secContext->transport->clientID?secContext->transport->clientID:"", policy->ClientID);
                    rc = ISMRC_ClientNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->Protocol && (match = ism_security_checkProtocol(secContext, policy)) == 0 ) {
                    rc = ISMRC_ProtoNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->ClientAddress && (match = ism_security_checkAddress(secContext, policy)) == 0 ) {
                    rc = ISMRC_AddrNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->CommonNames &&
                    (match = ism_common_match(secContext->transport->cert_name, policy->CommonNames)) == 0 ) {        /* BEAM suppression: operating on NULL */
                    TRACE(7, "CommonNames check failed: %s : %s\n", secContext->transport->cert_name?secContext->transport->cert_name:"", policy->CommonNames);
                    rc = ISMRC_CNameNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

            } else {
                rc = ISMRC_AuthorityCheckError;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
            }

            if ( match == 1 ) {
                rc = ISMRC_OK;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                if ( output_buffer )
                    continue;
#ifdef AUTH_TIME_STAT_ENABLED
                __sync_add_and_fetch(&statCount->author_msgn_passed, 1);
                statCount->author_total_time = statCount->author_total_time + (ism_common_readTSC() - startTime);
#endif
                if ( context != NULL ) {
                    *context = policy->context;
                }
                if ( auditLogControl > 1 ) {
/* BEAM suppression: operating on NULL */      LOG(INFO, Security, 6105, "%-s%-s%-s%-s%-s%-s%-s%-s%-s",
                        "The messaging policy {0} has authorized a client to {1} on Destination {2}. The connection identifiers are: Endpoint={3}, UserID={4}, ClientID={5}, ClientIPAddress={6}, CertificateCommonName={7}, Protocol={8}.",
                        policy->name, actionName, objectName, endpointName ? endpointName:"NULL",
                        (char *)secContext->transport->userid?(char *)secContext->transport->userid:"NULL",
                        (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                        (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                        (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL",
                        (char *)secContext->transport->protocol_family?(char *)secContext->transport->protocol_family:"NULL");  /* BEAM suppression: operating on NULL */
                }
                break;
            }
        }

        if ( policy_found == 0 ) {
            rc = ISMRC_NoMessagingPolicy;
            setErr = 1;
            polFailRC[0] = rc;
            polFailName[0] = NULL;
        }

#ifdef AUTH_TIME_STAT_ENABLED
        if ( match != 1 ) {
            __sync_add_and_fetch(&statCount->author_msgn_failed, 1);
            statCount->author_total_time = statCount->author_total_time + (ism_common_readTSC() - startTime);
        }
#endif

        if ( rc != ISMRC_OK && auditLogControl > 0 && output_buffer == NULL ) {
            char messageBuffer[256];
            char oauthInfo[1024];
            char *userid = (char *)secContext->transport->userid;
            if (secContext->oauthGroup) {
                snprintf(oauthInfo, 1024, "%s[%s]", userid?userid:"NULL", secContext->oauthGroup);
                userid = oauthInfo;
            }

/* BEAM suppression: operating on NULL */      LOG(NOTICE, Security, 6106, "%-s%-s%-s%-s%-s%-s%-s%-s",
                    "A client is not authorized to {0} on Destination {1}. The connection identifiers are: Endpoint={2}, UserID={3}, ClientID={4}, ClientIPAddress={5}, CertificateCommonName={6}, Protocol={7}.",
                   actionName, objectName, endpointName ? endpointName:"NULL",
                    userid?userid:"NULL",
                    (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                    (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                    (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL",
                       (char *)secContext->transport->protocol_family?(char *)secContext->transport->protocol_family:"NULL");   /* BEAM suppression: operating on NULL */

            /* LOG Error codes for each policy - at MAX level */
            for (int k=0; k<setErr; k++) {
                LOG(INFO, Security, 6130, "%-s%-s%-s%-s%-s%d", "Policy evaluated for {0}={1} to {2}: PolicyName={3}, Error={4} RC={5}",
                        "ClientID", (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                        actionName, polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
            }
        }

        /* Check and update output buffer - for test authority */
        if ( output_buffer ) {
            int first = 0;
            char rbuf[1024];
            for (int k=0; k < setErr; k++ ) {
                char messageBuffer[256];
                if ( first == 0 ) {
                    snprintf(rbuf, 1024, "{");
                    ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                }
                if ( first > 0 ) {
                    snprintf(rbuf, 1024, ",");
                    ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                }
                snprintf(rbuf, 1024, "\"%s\":\"%s (RC=%d)\"", polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
                ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                first += 1;
            }
            snprintf(rbuf, 1024, "}");
            ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
        }

        pthread_rwlock_unlock(&policylock);

        /* set error code to not authorized because this code is sent to client */
        if ( rc != ISMRC_OK )
            rc = ISMRC_NotAuthorized;

    } else if ( validationType == ismSEC_POLICY_CONNECTION ) {

        if ( secContext->reValidatePolicy == 1 ) {
            ism_security_reValidatePoliciesInSecContext(secContext);
        }

        if ( secContext->noConPolicies == 0 ) {
            TRACE(6, "No Connection policy is specified for the security context\n");
            return ISMRC_NotAuthorized;
        }

        pthread_rwlock_rdlock(&policylock);

        int policy_found = 0;
        int match = 0;
        int setErr = 0;

        for (i = 0; i < secContext->noConPolicies; i++) {
            polFailName[i] = NULL;
            polFailRC[i]   = ISMRC_NotAuthorized;
            match = 0;
            ismPolicyRule_t *policy = secContext->conPolicyPtr[i];
            if (policy == NULL || policy->deleted == 1 )
                continue;

            policy_found = 1;
            TRACE(9, "Check connection policy=%s | validate=%d | secContext=%p | endpoint=%s\n",
                policy->name, policy->validate, secContext, endpointName? endpointName:"NULL" );


            if ( policy->validate == 0 ) {
                match = 1;
                if (output_buffer) continue;
                break;
            }

            if (output_buffer) rc = ISMRC_OK;

            if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) {
                polFailName[i] = policy->name;
                setErr += 1;
            }

            if ( policy->UserID ) {
                   match = ism_common_match(secContext->transport->userid, policy->UserID);      /* BEAM suppression: operating on NULL */

                if ( match == 0 ) {
                    TRACE(7, "UserID check failed: %s : %s\n", secContext->transport->userid?secContext->transport->userid:"", policy->UserID);  /* BEAM suppression: operating on NULL */
                    rc = ISMRC_UserNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }
            }

            if ( policy->GroupID && (match = ism_security_checkGID(secContext, policy)) == 0 ) {
                rc = ISMRC_GroupNotAuthorized;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                continue;
            }

            if ( policy->ClientID ) {
                int clientCheck = 0;
                if ( ( clientCheck = ism_common_matchWithVars(secContext->transport->clientID, policy->ClientID, (ima_transport_info_t *)secContext->transport, NULL, 0, 0 )) != 0 ) { /* BEAM suppression: operating on NULL */
                    TRACE(7, "ClientID check failed: %s : %s : mwrc=%d\n", secContext->transport->clientID?secContext->transport->clientID:"null", policy->ClientID?policy->ClientID:"null", clientCheck);
                    rc = ISMRC_UserNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                } else {
                    match = 1;
                }
            }

            if ( policy->Protocol && (match = ism_security_checkProtocol(secContext, policy)) == 0 ) {
                rc = ISMRC_ProtoNotAuthorized;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                continue;
            }

            if ( policy->ClientAddress && (match = ism_security_checkAddress(secContext, policy)) == 0 ) {
                rc = ISMRC_AddrNotAuthorized;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                continue;
            }

            if ( policy->CommonNames &&
                (match = ism_common_match(secContext->transport->cert_name, policy->CommonNames)) == 0 ) {        /* BEAM suppression: operating on NULL */
                TRACE(9, "CommonNames check failed: %s : %s\n", secContext->transport->cert_name?secContext->transport->cert_name:"", policy->CommonNames); /* BEAM suppression: operating on NULL */
                rc = ISMRC_CNameNotAuthorized;
                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                continue;
            }

            if ( match == 1 ) {
                rc = ISMRC_OK;

                if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;

                /*set security Context AllowDurable and AllowPersistentMessages value
                 * when the policy is match
                 */
                secContext->AllowDurable            = policy->AllowDurable;
                secContext->AllowPersistentMessages = policy->AllowPersistentMessages;
                secContext->ExpectedMsgRate         = policy->ExpMsgRate;
                secContext->MaxSessionExpiry        = policy->MaxSessionExpiry;
                TRACE(8, "ism_security_policyBasedAuthorization: match policy is: %s, Set security context AllowDurable: %d, AllowPersistentMessages: %d, ExpectedMessageRate: %d, MaxSessionExpiry: %u\n",
                                policy->name, secContext->AllowDurable, secContext->AllowPersistentMessages, secContext->ExpectedMsgRate, secContext->MaxSessionExpiry);

                if ( output_buffer )
                    continue;

#ifdef AUTH_TIME_STAT_ENABLED
                __sync_add_and_fetch(&statCount->author_conn_passed, 1);
                statCount->author_total_time = statCount->author_total_time + (ism_common_readTSC() - startTime);
#endif

                if ( auditLogControl > 1 && output_buffer == NULL) {
/* BEAM suppression: operating on NULL */     LOG(INFO, Security, 6103, "%-s%-s%-s%-s%-s%-s%-s",
                        "The connection policy {0} has authorized a connection. The connection identifiers are: Endpoint={1}, UserID={2}, ClientID={3}, ClientIPAddress={4}, CertificateCommonName={5}, Protocol={6}.",
                        policy->name, endpointName ? endpointName:"NULL",
                        (char *)secContext->transport->userid?(char *)secContext->transport->userid:"NULL",
                        (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                        (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                        (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL",
                        (char *)secContext->transport->protocol_family?(char *)secContext->transport->protocol_family:"NULL"); /* BEAM suppression: operating on NULL */
                }
                break;
            }
        }


        if ( policy_found == 0 ) {
            rc = ISMRC_NoConnectionPolicy;
            setErr = 1;
            polFailRC[0] = rc;
            polFailName[0] = NULL;
        }

#ifdef AUTH_TIME_STAT_ENABLED
        if ( match != 1 ) {
            __sync_add_and_fetch(&statCount->author_conn_failed, 1);
            statCount->author_total_time = statCount->author_total_time + (ism_common_readTSC() - startTime);
        }
#endif

        if ( rc != ISMRC_OK && auditLogControl > 0 && output_buffer == NULL ) {
            char messageBuffer[256];
            char oauthInfo[1024];
            char *userid = (char *)secContext->transport->userid;
            if (secContext->oauthGroup) {
                snprintf(oauthInfo, 1024, "%s[%s]", userid?userid:"NULL", secContext->oauthGroup);
                userid = oauthInfo;
            }
/* BEAM suppression: operating on NULL */       LOG(INFO, Security, 6104, "%-s%-s%-s%-s%-s%-s",
                    "Connection is not authorized. The connection identifiers are: Endpoint={0}, UserID={1}, ClientID={2}, ClientIPAddress={3}, CertificateCommonName={4}, Protocol={5}.",
                    endpointName ? endpointName:"NULL",
                    userid?userid:"NULL",
                    (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                    (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                    (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL",
                    (char *)secContext->transport->protocol_family?(char *)secContext->transport->protocol_family:"NULL"); /* BEAM suppression: operating on NULL */

            /* LOG Error codes for each policy - at MAX level */
            for (int k=0; k<setErr; k++) {
                LOG(INFO, Security, 6130, "%-s%-s%-s%-s%-s%d", "Policy evaluated for {0}={1} to {2}: PolicyName={3}, Error={4} RC={5}",
                        "ClientID", (char *)secContext->transport->clientID?(char *)secContext->transport->clientID:"NULL",
                        "Connect", polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
            }
        }

        /* Check and update output buffer - for test authority */
        if ( output_buffer ) {
            int first = 0;
            char rbuf[1024];
            for (int k=0; k < setErr; k++ ) {
                char messageBuffer[256];
                if ( first == 0 ) {
                    snprintf(rbuf, 1024, "{");
                    ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                }
                if ( first > 0 ) {
                    snprintf(rbuf, 1024, ",");
                    ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                }
                snprintf(rbuf, 1024, "\"%s\":\"%s (RC=%d)\"", polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
                ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
                first += 1;
            }
            snprintf(rbuf, 1024, "}");
            ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
        }

        pthread_rwlock_unlock(&policylock);

        /* set error code to not authorized because this code is sent to client */
        if ( rc != ISMRC_OK )
            rc = ISMRC_NotAuthorized;

    } else if ( validationType == ismSEC_POLICY_CONFIG) {

        int policy_found = 0;
        int match = 0;
        int setErr = 0;

        if ( secContext->reValidatePolicy == 1 ) {
            ism_security_reValidatePoliciesInSecContext(secContext);
        }

        /* By pass authorization check if no configuration policy is configured */
        if ( secContext->noConPolicies == 0 ) {
            TRACE(9, "Admin Endpoint has no connection policies.\n");
            return ISMRC_OK;
        }

        /* Check for AdminUserID */
        if ( secContext->superUser == 1 ) {
            char *unm = secContext->username;
            TRACE(9, "Admin request is from superUser (%s) - bypass authorization check.\n", unm?unm:"NULL");
            return ISMRC_OK;
        }

        /* Check if request is made from a client in localhost */
        if ( secContext->transport && secContext->transport->client_addr && (strcmp(secContext->transport->client_addr, "127.0.0.1") == 0)) {
            TRACE(9, "Admin request is from localhost - bypass authorization check.\n");
            return ISMRC_OK;
        }

        pthread_rwlock_rdlock(&policylock);

        for (i = 0; i < secContext->noConPolicies; i++) {
            polFailName[i] = NULL;
            polFailRC[i]   = ISMRC_NotAuthorized;
            match = 0;
            ismPolicyRule_t *policy = secContext->conPolicyPtr[i];
            if (policy == NULL || policy->deleted == 1 )
                continue;

            policy_found = 1;
            TRACE(9, "Check configuration policy=%s | validate=%d | secContext=%p | endpoint=%s\n",
                policy->name, policy->validate, secContext, endpointName? endpointName:"NULL" );


            if ( policy->validate == 0 ) {
                match = 1;
                break;
            }

            if ( policy->actions[actionType] == 1 ) {

                if ( policy->UserID ) {
                       match = ism_common_match(secContext->transport->userid, policy->UserID);      /* BEAM suppression: operating on NULL */

                    if ( match == 0 ) {
                        TRACE(7, "UserID check failed: %s : %s\n", secContext->transport->userid?secContext->transport->userid:"", policy->UserID);  /* BEAM suppression: operating on NULL */
                        rc = ISMRC_UserNotAuthorized;
                        if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                        continue;
                    }
                }

                if ( policy->GroupID && (match = ism_security_checkGID(secContext, policy)) == 0 ) {
                    rc = ISMRC_GroupNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->ClientAddress && (match = ism_security_checkAddress(secContext, policy)) == 0 ) {
                    rc = ISMRC_AddrNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( policy->CommonNames &&
                    (match = ism_common_match(secContext->transport->cert_name, policy->CommonNames)) == 0 ) {        /* BEAM suppression: operating on NULL */
                    TRACE(9, "CommonNames check failed: %s : %s\n", secContext->transport->cert_name?secContext->transport->cert_name:"", policy->CommonNames); /* BEAM suppression: operating on NULL */
                    rc = ISMRC_CNameNotAuthorized;
                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;
                    continue;
                }

                if ( match == 1 ) {
                    rc = ISMRC_OK;

                    if ( i < MAX_POLICIES_PER_SECURITY_CONTEXT ) polFailRC[i] = rc;

                    LOG(INFO, Security, 6101, "%-s%-s", "The connection is authorized to initiate action {0} : ClientAddress={1}.",
                        actionName, (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL");
                    break;
                }
            }
        }


        if ( policy_found == 0 ) {
            rc = ISMRC_NotAuthorized;
            setErr = 1;
            polFailRC[0] = rc;
            polFailName[0] = NULL;
        }

        if ( rc != ISMRC_OK ) {

            char messageBuffer[256];

/* BEAM suppression: operating on NULL */       LOG(INFO, Security, 6102, "%-s%-s%-s%-s",
                    "Connection is not authorized. The connection identifiers are: Endpoint={0}, UserID={1}, ClientIPAddress={2}, CertificateCommonName={3}",
                    endpointName ? endpointName:"NULL",
                    (char *)secContext->transport->userid?(char *)secContext->transport->userid:"NULL",
                    (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                    (char *)secContext->transport->cert_name?(char *)secContext->transport->cert_name:"NULL" ); /* BEAM suppression: operating on NULL */

            /* LOG Error codes for each policy - at MAX level */
            for (int k=0; k<setErr; k++) {
                LOG(INFO, Security, 6130, "%-s%-s%-s%-s%-s%d", "Policy evaluated for {0}={1} to {2}: PolicyName={3}, Error={4} RC={5}",
                        "ClientAddress", (char *)secContext->transport->client_addr?(char *)secContext->transport->client_addr:"NULL",
                        "Connect", polFailName[k]?polFailName[k]:"NULL",
                        ism_common_getErrorStringByLocale(polFailRC[k], ism_common_getLocale(), messageBuffer, 255), polFailRC[k]);
            }
        }

        pthread_rwlock_unlock(&policylock);

    }

    return rc;

}


/**
 *  API for components to authorize an administrative action
 */
int32_t ism_security_validate_policy(
    ismSecurity_t *secContext,
    ismSecurityAuthObjectType_t objectType,
    const char * objectName,
    ismSecurityAuthActionType_t actionType,
    ism_ConfigComponentType_t compType,
    void ** context)
{
    concat_alloc_t *output_buffer = NULL;
    return ism_security_policyBasedAuthorization(secContext, objectType, objectName, actionType, compType, context, output_buffer);
}


/**
 * @brief  Sets context to be returned by ism_security_validate_policy()
 *
 * Stores some opaque data to be retrieved when a policy has been used
 * to authorise an action
 *
 * @param[in]     name                   name to set the context for
 * @param[in]     policyType             type of policy
 * @param[in]     compType               Component type
 * @param[in]     newContext             Context to store
 *
 * NOTE: At the moment compType is asserted to be ISM_CONFIG_COMP_ENGINE
 * only.
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int32_t ism_security_set_policyContext(const char *name,
                                            ismSecurityPolicyType_t policyType,
                                            ism_ConfigComponentType_t compType,
                                            void *newContext)
{
        int rc = ISMRC_NotFound;
        ismPolicyRule_t *policy = NULL;

        if (!name) {
                TRACE(5, "name provided is null.\n");
                rc = ISMRC_NullPointer;
                return rc;
        }

        switch(compType) {
        case ISM_CONFIG_COMP_ENGINE:
                policy = ism_security_getPolicyByName((char *) name, policyType);
                if (policy) {
                        /*found the policy and update the context*/
                        policy->context = newContext;
                        rc = ISMRC_OK;
                }
                break;
        default:
                TRACE(5, "compType %d is not supported.\n", compType);
                rc = ISMRC_InvalidParameter;
                break;
        }

    return rc;
}

/**
 * @brief  Get context of a policy
 *
 * @param[in]     name                   name to set the context for
 * @param[in]     compType               Component type
 *
 * NOTE: At the moment compType is asserted to be ISM_CONFIG_COMP_ENGINE
 * only.
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI void *ism_security_get_policyContextByName(char *name, ism_ConfigComponentType_t compType)
{
        ismPolicyRule_t *policy = NULL;

        if (!name) {
                TRACE(5, "name provided is null.\n");
                return NULL;
        }

        switch(compType) {
        case ISM_CONFIG_COMP_ENGINE:
            policy = ism_security_getPolicyByName(name, ismSEC_POLICY_MESSAGING);
        if (policy) {
                TRACE(8, "Found policy, policy name=%s\n", policy->name);
                return policy->context;
        }
            break;
        default:
                TRACE(5, "compType %d is not supported.\n", compType);
                break;
        }

        return NULL;
}

