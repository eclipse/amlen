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
#include <selector.h>
#include <regex.h>
#include <ismjson.h>
#include <ismregex.h>
#include <fendian.h>
#include <protoex.h>
#include <imacontent.h>

#include <unicode/ustring.h>
#include <unicode/utypes.h>

static int  promote_var(ism_field_t * var1, ism_field_t * var2);
static int  compare_var(ism_prop_t * props, ism_field_t * var1, ism_field_t * var2, int op);
static int  calc_var(ism_field_t * var1, ism_field_t * var2, int op);
static int  in_hash(ism_field_t * var1, ism_field_t * var2, int op);
static int  checkACL(ism_field_t * f, const char * extra);
static int  topicpart(const char * topic, const char * * part, int which);

#define CHECK_LEVEL(n) if (level<(n)) return SELECT_UNKNOWN

extern int ism_common_printSelectRule(ismRule_t * rule);
//extern void ism_common_lowerCaseUTF8(char ** dest, const char * src);

static int States[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
};

#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])

/* An array of ACLs to access without a lock */
ism_acl_t * g_acl_array [10] = {0};

#define DEBUG_SELECT

static int debugflag = 0;

/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

/*
 * Debugging aid.  This normally does nothing anyway
 */
XAPI void ism_common_setSelectorDebug(int val) {
    debugflag = val;
}


/*
 * Generate properties from the message
 */
static int propgen(ism_prop_t * xprops, ism_emsg_t * emsg, const char * name, ism_field_t * f, void * extra) {
    ism_actionbuf_t props = {0};
    props.buf = (char *)emsg->props;
    props.len = emsg->proplen;
    props.used = emsg->proplen;

    /*
     * Check for JMS special properties
     */
    if (name[0] == 'J' && name[1] == 'M' && name[2] == 'S' && name[3] != 'X') {
        switch (name[3]) {
        case '_':
            switch(name[4]) {
            case 'T':
                if (!strcmp(name+4, "Topic")) {
                    int xrc = ism_findPropertyNameIndex(&props, ID_Topic, f);
                    return xrc;
                }
                break;
            case 'A':
                if (!strcmp(name+4, "ACLCheck")) {
                    return checkACL(f, (const char *)extra);
                }
                break;
            case 'I':
                if (!strcmp(name+4, "IBM_Retain")) {
                    f->type = VT_Integer;
                    f->val.i = !!(emsg->hdr->Flags&ismMESSAGE_FLAGS_RETAINED);
                    return 0;
                }
                break;
            case 'Q':
                if (!strcmp(name+4, "QoS")) {
                    f->type = VT_Integer;
                    f->val.i = emsg->hdr->Reliability;
                    return 0;
                }
                break;
            case 'P':
                if (!strcmp(name+4, "PayloadFormat")) {
                    f->type = VT_Integer;
                    f->val.i = emsg->hdr->MessageType == MTYPE_MQTT_Text ||
                               emsg->hdr->MessageType == MTYPE_TextMessage ? 1 : 0;
                    return 0;
                }
                break;
            case 'C':
                if (!strcmp(name+4, "ContentType")) {
                    return ism_findPropertyNameIndex(&props, ID_ContentType, f);
                }
                break;
            }
        case 'C':
            if (!strcmp(name, "JMSCorrelationID"))
                return ism_findPropertyNameIndex(&props, ID_CorrID, f);
            break;

        case 'D':
            if (!strcmp(name, "JMSDeliveryMode")) {
                f->type  = VT_String;
                f->val.s = emsg->hdr->Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT ?
                            "PERSISTENT" : "NON_PERSISTENT";
                return 0;
            }
            if (!strcmp(name, "JMSDestination"))
                return ism_findPropertyNameIndex(&props, ID_Topic, f);
            break;

        case 'E':
            if (!strcmp(name, "JMSExpiration"))
                 return ism_findPropertyNameIndex(&props, ID_Expire, f);
            break;

        case 'M':
            if (!strcmp(name, "JMSMessageID"))
                return ism_findPropertyNameIndex(&props, ID_MsgID, f);
            break;

        case 'P':
            if (!strcmp(name, "JMSPriority")) {
                f->type  = VT_Integer;
                f->val.i = emsg->hdr->Priority;
                return 0;
            }
            break;

        case 'R':
            if (!strcmp(name, "JMSReplyTo"))
                 return ism_findPropertyNameIndex(&props, ID_ReplyToT, f);
            if (!strcmp(name, "JMSRedelivered")) {
                f->type  = VT_Boolean;
                f->val.i = emsg->hdr->RedeliveryCount != 0;
                return 0;
            }
            break;

        case 'T':
            if (!strcmp(name, "JMSType"))
                return ism_findPropertyNameIndex(&props, ID_JMSType, f);
            if (!strcmp(name, "JMSTimestamp"))
                return ism_findPropertyNameIndex(&props, ID_Timestamp, f);
            break;
        }
        f->type = VT_Null;
        return 1;
    }
    ism_findPropertyName(&props, name, f);
    return 0;
}


/*
 * Select a message.
 *
 * Return a selection which has the values SELECT_TRUE, SELECT_FALSE, or SELECT_UNKNOWN.
 *         Note that SELECT_TRUE has the value 0.
 */
int ism_common_selectMessage(
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        const char *               topic,
        void *                     rule,
        size_t                     rulelen) {
    int       i;
    uint32_t  proplen = 0;
    char *    propp = NULL;
    ism_emsg_t emsg;

    if (!rule) {
        return SELECT_TRUE;
    }

    /* Find the props and body */
    for (i=0; i<areas; i++) {
        if (areatype[i] == ismMESSAGE_AREA_PROPERTIES) {
            proplen = (uint32_t)areasize[i];
            propp = (char *)areaptr[i];
            break;
        }
    }

    emsg.hdr = hdr;
    emsg.props = propp;
    emsg.proplen = proplen;
    emsg.topic = topic;
    int rc = ism_common_filter(rule, NULL, propgen, &emsg);
    TRACE(9, "filter message: topic=%s rc=%d\n", topic, rc);
    return rc;
}


/*
 * Do a generic match using an asterisk wildcard which matches 0 or more characters.
 *
 * The codepoint 0xff (which is not a valid UTF-8 encoding) matches a single
 * asterisk in the string.
 *
 * @param str    The string to match
 * @param match  The match string
 * @return 0=does not match, 1=matches
 */
int ism_common_match(const char * str, const char * match) {
    const char * lastmatch = NULL;
    int   len;
    int   clen;

    /* Get the length of the string */
    len = str ? (int)strlen(str) : 0;

    /* Loop until we get a result */
    for (;;) {
        /* If at end of match, check if we need to restart */
        if (!*match) {
            if (!len || !lastmatch)
                break;
            match = lastmatch;
        }

        /* For a wildcard, match any number of characters */
        if (*match == '*') {
            lastmatch = match++;
            while (*match == '*')
                match++;
            clen = (int)strcspn(match, "*\xff");
//            printf("clen=%d *match=%2x\n", clen, (uint8_t)*match);
            if (!clen) {
                if ((uint8_t)*match != 0xff)
                    return 1;         /* Match */
                while (len && *str != '*') {
                    str++;
                    len--;
                }
                if (!len)
                    return 0;
                continue;
            }
            while (len) {
                if (*str == *match) {
                    if (len >= clen && !memcmp(str, match, clen)) {
                        match += clen;
                        break;
                    }
                }
                len--;
                str++;
            }
            if (!len)
                return 0;        /* Does not match */
            len -= clen;
            str += clen;
        }

        /* A literal 0xff matches an asterisk */
        else if ((uint8_t)*match == 0xff) {
            match++;             /* Use up one character in the match string */
//            printf("ff [%s] len=%d\n", str, len);
            if (!len || *str != '*') {
                if (lastmatch) {
                    match = lastmatch;
                    continue;
                }
                return 0;
            }
            str++;               /* Use up one character in the source string */
            len--;
        }
        /* Match a constant string */
        else {
            clen = (int)strcspn(match, "*\xff");
            if (len < clen || memcmp(str, match, clen)) {/* BEAM suppression: passing null object */
                match += clen;
                if (lastmatch) {
                    /* We might get a match if we try later */
                    match = lastmatch;
                    continue;
                }
                return 0;
            }
            len -= clen;
            str += clen; /* BEAM suppression: operating on NULL */
            match += clen;
        }
    }
    return len ? 0 : 1;
}

/*
 * Match a string with replacement values from a connection.
 *
 * The following variables are allowed in the match string:
 * <dl>
 * <dt>${ClientID}      <dd>Match the clientID from the connection
 * <dt>${CommonName}    <dd>Match the certificate common name from the connection
 * <dt>${UserID}        <dd>Match the userID from the connection
 * <dt>${GroupID}       <dd>Match the next groupID passed as an argument
 * <dt>${GroupID#}      <dd>Match the specific groupID passed as an argument
 * <dt>${*}             <dd>Match a literal asterisk
 * <dt>${$}             <dd>Match a dollar sign
 * </dl>
 *
 * @param str           The source string to compare
 * @param match         The match string which can contain variables
 * @param transport     The transport object
 * @param groupID       Array of group IDs
 * @param noGroup       Number of groups (max 10)
 * @param noCaseCheck   Make group ID comparisons case insensitive.  This only works before the first asterisk in the match string.
 * @return 0 for a match and non-zero for not matched
 */
int ism_common_matchWithVars(const char * str, const char * match, ima_transport_info_t * transport, char * groupID[], int noGroups, int noCaseCheck) {
    int usedlen;
    int maxlen;
    int slen = (int) strlen(str);
    char * copystr = alloca(slen+1);
    char * strp = copystr;
    const char * matchp = match;
    char * buf;
    char * bufp;
    const char * subval;
    int   sublen = 0;
    int   clen;
    int   star_found;
    int   gid = 0;
    int   gidn = 0;

    /* Make a copy so we can modify it */
    strcpy(strp, str);

    /*
     * If we mismatch in the first literal area, fail.
     * If the match does not contain generics or variables this will complete the match.
     */
    while (*matchp != '*' && *matchp != '$') {
        if (!*strp)
            return *matchp ?  MWVRC_LiteralMismatch : MWVRC_Matched;
        if (*strp != *matchp) {
            return  MWVRC_LiteralMismatch;
        }
        strp++;
        matchp++;
    }

    /*
     * If the match consists of only a trailing asterisk, success
     * This handles the case of a trailing generic.
     */
    if (matchp[0] == '*' && matchp[1]==0) {
        return MWVRC_Matched;
    }

    /*
     * Scan the string to create the match string
     */
    maxlen = strlen(strp) + strlen(matchp)*2;
    usedlen = 0;
    star_found = 0;
    buf = bufp = alloca(maxlen+1);
    for (; *matchp; matchp++) {
        if (*matchp == '*') {
            /* If the rule would be too long, then it is known it must fail */
            if (usedlen >= maxlen) {
                return MWVRC_WildcardMismatch;
            }
            star_found = 1;
            noCaseCheck = 0;  /* Case independent check only works before first asterisk */
            *buf++ = '*';
            usedlen++;
            while (matchp[1] == '*')
                matchp++;
        } else if (*matchp == '$' && matchp[1]=='{') {
            matchp += 2;
            clen = (int)strcspn(matchp, "}");
            if (matchp[clen]==0) {
                return MWVRC_MalformedVariable;            /* Non-terminated variable */
            }
            subval = NULL;
            switch(clen) {
            case 1:   /* Match ${*} or ${$}  */
                if (*matchp == '*') {
                    subval = "*";
                    sublen = 1;
                } else if (*matchp == '$') {
                    subval = "$";
                    sublen = 1;
                }
                break;
            case 6:  /* Match UserID */
                if (!memcmp(matchp, "UserID", 6)) {
                    if (!transport || !transport->userid || !*transport->userid) {
                        return MWVRC_NoUserID;
                    }
                    subval = transport->userid;
                    sublen = strlen(transport->userid);
                }
                break;
            case 7: /* Match groupID */
                if (!memcmp(matchp, "GroupID", 7)) {
                    if ( gidn >= 1 ) {
                        return MWVRC_MalformedVariable;
                    }
                    if (gid > (noGroups-1)) {
                        return MWVRC_MaxGroupID;
                    }
                    if (!groupID || !groupID[gid]) {
                        return MWVRC_NoGroupID;
                    }
                    subval = groupID[gid];
                    sublen = strlen(groupID[gid]);
                    gid += 1;
                    /* Convert match and sub values to lower case */
                    if (noCaseCheck && strlen(strp) >= sublen) {
                        char * grplwr = alloca(sublen+1);
                        const char * outlwr = ism_common_lowerUTF8(grplwr, sublen+1, strp, sublen);
                        if (outlwr == grplwr)
                            memcpy(strp, outlwr, sublen);
                        subval = ism_common_lowerUTF8(grplwr, sublen+1, subval, sublen);
                    }
                }
                break;
            case 8: /* Match groupIDn */
                if (!memcmp(matchp, "GroupID", 7)) {
                    if ( gid >= 1 ) {
                        return MWVRC_MalformedVariable;
                    }
                    if (!groupID) {
                        return MWVRC_NoGroupID;
                    }
                    const char *np = matchp + 7;
                    if ( *np < 0x30 || *np > 0x39 ) {
                    	return MWVRC_NoGroupID;
                    }
                    int n = atoi(np);
                    if (n >= noGroups) {
                        return MWVRC_MaxGroupID;
                    }
                    subval = groupID[n];
                    sublen = strlen(subval);
                    gidn = 1;
                    /* Convert match and sub values to lower case */
                    if (noCaseCheck && strlen(strp) >= sublen) {
                        char * grplwr = alloca(sublen+1);
                        const char * outlwr = ism_common_lowerUTF8(grplwr, sublen+1, strp, sublen);
                        if (outlwr)
                            memcpy(strp, outlwr, sublen);
                        subval = ism_common_lowerUTF8(grplwr, sublen+1, subval, sublen);
                    }
                } else if (!memcmp(matchp, "ClientID", 8)) {
                    /* Match ClientID */
                    if (!transport || !transport->clientID || !*transport->clientID) {
                        return MWVRC_NoClientID;
                    }
                    subval = transport->clientID;
                    sublen = strlen(transport->clientID);
                }
                break;
            case 10: /* Match CommonName */
                if (!memcmp(matchp, "CommonName", 10)) {
                    if (!transport || !transport->cert_name || !*transport->cert_name) {
                        return MWVRC_NoCommonName;
                    }
                    subval = transport->cert_name;
                    sublen = strlen(transport->cert_name);
                }
                break;
            default:
                break;
            }
            /* Check if we got an unknown variable */
            if (subval == NULL) {
                return MWVRC_UnknownVariable;         /* Unknown variable */
            }
            matchp += clen;       /* Skip over the variable */

            /* If we have not yet found an asterisk, do a literal match */
            if (!star_found) {
                slen = (int) strlen(strp);

                if (slen < sublen || memcmp(strp, subval, sublen)) {
                    switch (clen) {
                    case 6:    return MWVRC_UserIDMismatch;       /* UserID mismatch */
                    case 7:    return MWVRC_GroupIDMismatch;      /* GroupID mismatch */
                    case 8:    if (gidn) return MWVRC_GroupIDMismatch; else return MWVRC_ClientIDMismatch;  /* GroupID/ClientID mismatch */
                    case 10:   return MWVRC_CommonNameMismatch;   /* CommonName mismatch */
                    default:   return MWVRC_LiteralMismatch;      /* Literal mismatch */
                    }
                }
                strp += sublen;
            } else {
                /* Put the variable value into the match string */
                if (usedlen + sublen >= maxlen) {   /* If the variable is too long it cannot match */
                    return MWVRC_WildcardMismatch;
                }
                while (*subval) {
                    if (*subval == '*')
                        *buf++ = (char)0xff;     /* Match a literal asterisk */
                    else
                        *buf++ = *subval;
                    subval++;
                }
                usedlen += sublen;
            }
        } else {
            if (!star_found) {
                if (*strp++ != *matchp) {
                    return MWVRC_LiteralMismatch;
                }
            } else {
                /* Add a single literal character */
                if (usedlen >= maxlen) {  /* If the rule would be too long, then it is known it must fail */
                    return MWVRC_WildcardMismatch;
                }
                *buf++ = *matchp;
                usedlen++;
            }
        }
    }
    *buf = 0;
    // printf("match [%s] [%s]\n", copystr, bufp);
    return ism_common_match(strp, bufp) ? MWVRC_Matched : MWVRC_WildcardMismatch;
}

struct ism_regex_t {
    regex_t   regex;
};


/*
 * Compile a regular expression.
 *
 * @param  pregex     (output) The location to return the compiled regular expression.
 * @param  regex_str  A regular expression in string form
 * @return A regex specific return code
 */
static inline int ism_regex_compile_withflags(ism_regex_t * pregex, const char * regex_str, int flags) {
    ism_regex_t regex = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,219),1, sizeof(regex_t));
    int     rc;

    rc = regcomp(&regex->regex, regex_str, flags);
    if (rc) {
        *pregex = NULL;
        ism_common_free(ism_memory_utils_misc,regex);
    } else {
        *pregex = regex;
    }
    return rc;
}
/*
 * Compile a regular expression (extended syntax but no sub expression matching).
 *
 * @param  pregex     (output) The location to return the compiled regular expression.
 * @param  regex_str  A regular expression in string form
 * @return A regex specific return code
 */
int ism_regex_compile(ism_regex_t * pregex, const char * regex_str) {
    return ism_regex_compile_withflags(pregex, regex_str, REG_EXTENDED | REG_NOSUB);
}

/*
 * Compile a regular expression (extended syntax and sub expression matching).
 *
 * @param  pregex       (output) The location to return the compiled regular expression.
 * @param  psubexprcnt  (output) The number of subexpressions contained in the regular expression
 * @param  regex_str    A regular expression in string form
 * @return A regex specific return code
 */
int ism_regex_compile_subexpr(ism_regex_t * pregex, int *psubexprcnt, const char * regex_str) {
    int rc = ism_regex_compile_withflags(pregex, regex_str, REG_EXTENDED);
    if (!rc) {
        *psubexprcnt = (int)((*pregex)->regex.re_nsub);
    } else {
        *psubexprcnt = 0;
    }
    return rc;
}

/*
 * Return the error string for a regex error
 *
 * @param rc   A return code from ism_regex_compile() or ism_regex_match()
 * @param buf  A buffer in which to return the string
 * @param len  The length of the buffer
 * @return The return error string
 */
const char * ism_regex_getError(int rc, char * buf, int len, ism_regex_t regex) {
    regerror(rc, &regex->regex, buf, len);
    return(buf);
}


/*
 * Match a regular expression.
 *
 * @param regex  A previously compiled regular expression
 * @param str    An arbitrary string to match
 * @return A return code 0=match, 1=not matched, other=error
 */
int ism_regex_match(ism_regex_t regex, const char * str) {
    return regexec(&regex->regex, str, 0, NULL, 0);
}

/*
 * Match a regular expression.
 *
 * @param regex       A previously compiled regular expression
 * @param str         An arbitrary string to match
 * @param maxMatches  max number of matches caller wants
 * @param matches     array of results of size maxMatches
 * @return A return code 0=match, 1=not matched, other=error
 */
int ism_regex_match_subexpr(ism_regex_t regex, const char * str, int maxMatches, ism_regex_matches_t *matches) {
    return regexec(&regex->regex, str, maxMatches, (regmatch_t *)matches, 0);
}

/*
 * Free a regular expression
 * @pararm regex  A compiled regular expression
 */
void ism_regex_free(ism_regex_t regex) {
    if (!regex)
        return;
   regfree(&regex->regex);
   ism_common_free(ism_memory_utils_misc,regex);
}

/*
 * Implement like.
 */
int ism_common_likeMatch(const char * str, int len, const uint8_t * match) {
    int clen;
    const char * laststr = NULL;
    int   lastlen = 0;
    const uint8_t * lastmatch = NULL;
    for (;;) {
        /*
         * If we reach the end of the like string, but not the end of the data string,
         * go back to the last generic and try again.
         */
        if (!*match) {
            if (!len || !lastmatch)
                break;
            match = lastmatch;
        }
        /*
         * For a generic, look thru the string to find the next matching string
         */
        if (*match == 0xff) {
            lastmatch = match++;
            clen = *match;
            laststr = NULL;
            if (clen == 0xff || clen == 0xfe) {
                if (clen == 0xfe  && *str) {
                    if ((uint8_t)*str < 0x80) {
                        lastlen = len-1;
                        laststr = str-1;
                    } else {
                        clen = States[((uint8_t)*str)>>3];
                        if (clen > len)
                            clen = len;
                        lastlen = len-clen;
                        laststr = str-clen;
                    }
                    laststr = str+1;
                    lastlen = len-1;
                }
                continue;
            }
            match++;
            if (!clen)
                return SELECT_TRUE;
            while (len) {
                if (*str == *match) {
                    if (len >= clen && !memcmp(str, match, clen)) {
                        match += clen;
                        break;
                    }
                }
                len--;
                str++;
            }
            if (!len)
                return SELECT_FALSE;   /* Post match not found */
            len -= clen;
            str += clen;

        }

        /*
         * Match an arbitrary character.
         * Interpret the character as UTF-8
         */
        else if (*match == 0xfe) {
            if (!len)
                return SELECT_FALSE;
            if ((uint8_t)*str < 0x80) {
                len--;
                str++;
            } else {
                clen = States[((uint8_t)*str)>>3];
                if (clen > len)
                    clen = len;
                len -= clen;
                str += clen;
            }
            match++;
        }

        /*
         * Constant string, return false if the string is too short or does not match
         */
        else {
            clen = *match++;
            if (len < clen || memcmp(str, match, clen)) {
                match += clen;
                if (lastmatch) {
                    match = lastmatch;
                    if (laststr) {
                        len = lastlen;
                        str = laststr;
                    }
                    continue;
                }
                return SELECT_FALSE;   /* Post match not found */
            }
            len -= clen;
            str += clen;
            match += clen;
        }
    }
    return len ? SELECT_FALSE : SELECT_TRUE;
}

/*
 * Get a result from the variable.
 * This acts the same as converting the variable to boolean, but does not
 * change the variable.
 */
int getResult(ism_field_t * var) {
    if (var->type == VT_Boolean)
        return var->val.i ? SELECT_TRUE : SELECT_FALSE;
    return SELECT_UNKNOWN;
}


/*
 * Set the result
 */
void setResult(ism_field_t * var, int result) {
    if (result < 0) {
        var->type = VT_Null;
    } else {
        var->type = VT_Boolean;
        var->val.i = !result;
    }
}

/*
 * Implement in.
 * An in rule consists of a set of entries with a one byte length followed
 * by the value.
 */
static int selectIn(ism_field_t * field, ismRule_t * rule) {
	int  count = rule->kind;
	if (field->type == VT_String) {
		int flen = (int)strlen(field->val.s);
		uint8_t * match = (uint8_t *)(rule+1);
		while (count-- > 0) {
			int mlen = *match++;
			if (mlen == flen && !memcmp(match, field->val.s, mlen))
				return SELECT_TRUE;
			match += mlen;
		}
		return SELECT_FALSE;
	} else {
		return SELECT_UNKNOWN;
	}
}

/*
 * Parse a topic into segments.
 * This is done in place so the topic must be a copy.
 */
static int topicSegment(char * topic, char * * segments, int count) {
    int segs = 1;
    if (count > 0)
        segments[0] = topic;
    while ((topic = strchr(topic, '/'))) {
        *topic++ = 0;
        if (segs < count)
            segments[segs] = topic;
        segs++;
    }
    return segs;
}

/*
 *
 */
static int selectACLCheck(ism_field_t * field, ismRule_t * rule, ism_prop_t * props, property_gen_t generator, ism_emsg_t * emsg) {
    char * levels [32];
    char * topic;
    const uint8_t * wp;
    char * key;
    char * kp;
    ism_field_t f = {0};
    int   count;
    int   len;
    int   i;
    int   outlen;
    void * extra;

    if (generator && field->type == VT_String && field->val.s) {
        len = (int)strlen(field->val.s);
        topic = alloca(len+1);
        strcpy(topic, field->val.s);
        count = topicSegment(topic, levels, 32);

        outlen = rule->kind;          /* Space for separators */
        wp = (const uint8_t *)(rule+1);
        extra = (void *)(wp+1);
        wp += (*wp+1);
        for (i=0; i<rule->kind; i++) {
            int which = (uint8_t)wp[i];
            if (which >= count)
                return SELECT_UNKNOWN;
            outlen += strlen(levels[which]);
        }
        key = kp = alloca(outlen);
        for (i=0; i<rule->kind; i++) {
            int which = (uint8_t)wp[i];
            if (i>0)
                *kp++ = '/';
            strcpy(kp, levels[which]);
            kp += strlen(kp);
        }

        /* Call the generator function to do the ACL check */
        f.type = VT_String;
        f.val.s = key;
        generator(props, emsg, "JMS_ACLCheck", &f, extra);  /* Prop names starting JMS are reserved for system use */
        if (f.type != VT_Boolean)
            return SELECT_UNKNOWN;
        return !f.val.i;
    } else {
        return SELECT_UNKNOWN;
    }
}


/*
 * Implement like.
 */
static int selectLike(ism_field_t * field, ismRule_t * rule) {
	if (field->type == VT_String) {
		uint8_t * match = (uint8_t *)(rule+1);
		return ism_common_likeMatch(field->val.s, (int)strlen(field->val.s), match);
	} else {
		return SELECT_UNKNOWN;
	}
}


/**
 * Run a selection rule or property mapping rule
 * @param rule    The selection rule
 * @param props   The property environment
 * @return The filter result: 0=good, <0=error
 */
int ism_common_filter(ismRule_t * rule, ism_prop_t * props, property_gen_t generator, ism_emsg_t * emsg) {
    char * rp = (char *)rule;
    int   result = SELECT_TRUE;
    ism_field_t stack [320];
    int   level = 0;
    int   next;
    int   thislvl;
    ism_field_t * var;
    int64_t lval;
    double  dval;
    float   fval;
    const char * topic = NULL;
    const char * pp;
    char * ap;
    int pplen;
    xUNUSED char * startrp = rp;

    if (!rule || rule->op == SELRULE_End)
        return 0;

    next = 0;
    for (;;) {
        uint16_t rlen;
        rp += next;
        rule = (ismRule_t *)rp;
        memcpy(&rlen, &rule->len, 2);
        next = rlen;

    //  if (debugflag)
    //      ism_common_printSelectRule(rule);
        switch (rule->op) {
        /* Open parenthesis */
        case SELRULE_Begin:
            break;

        /* Close parenthesis or end of rule */
        case SELRULE_End:
            if (rule->kind == 0) {
                return getResult(stack);
            }
            break;

        /* Implement is [not] null and exists */
        case SELRULE_Is:
            CHECK_LEVEL(1);
            switch (rule->kind &0x3F) {

            /*
             * Unary operators
             */
            case TT_Null:
                /* IsNull is either
                 * unknown (VT_String, stack[level].val.s=NULL)
                 * or empty string (stack[level].val.s="")
                 */
                level--;
                switch (stack[level].type) {
                case VT_Null:
                	result = SELECT_TRUE;
                	break;
                case VT_String:
                case VT_ByteArray:
                    result = (stack[level].val.s && *stack[level].val.s);
                    break;
                default:
                    result = SELECT_FALSE;
                    break;
                }
                break;

            /* Extension from JMS, allow IS TRUE.  This can be used to convert an unknown to false */
            case TT_True:
                level--;
                switch (stack[level].type) {
                case VT_Boolean:
                	result = !stack[level].val.i;
                	break;
                default:
                    result = SELECT_FALSE;    /* false */
                    break;
                }
                break;

            /* Extension from JMS, allow IS FALSE.  */
            case TT_False:
                level--;
                switch (stack[level].type) {
                case VT_Boolean:
                    result = !!stack[level].val.i;
                    break;
                default:
                    result = SELECT_FALSE;    /* false */
                    break;
                }
                break;
            }
            if (rule->kind & 0x40)
                result = !result;
            setResult(stack+level, result);
            level++;
            break;

        /* Implement Compare */
        case SELRULE_Compare:
            CHECK_LEVEL(2);
            result = compare_var(props, stack+level-2, stack+level-1, rule->kind);
            level -= 1;
            setResult(stack+level-1, result);
            break;

        /* Load an integer value */
        case SELRULE_Int:
            stack[level].type = VT_Integer;
            stack[level].val.i = *(uint32_t *)(rule+1);
            level++;
            break;

        /* Load a boolean value */
        case SELRULE_Boolean:
            stack[level].type = VT_Boolean;
            stack[level].val.i = (int8_t)rule->kind;
            if (stack[level].val.i < 0) {
                stack[level].type = VT_Null;
            }
            level++;
            break;

        /* Load a long value */
        case SELRULE_Long:
            stack[level].type = VT_Long;
            memcpy(&lval, rule+1, sizeof lval);
            stack[level].val.l = lval;
            level++;
            break;

		/* Load a double value */
		case SELRULE_Float:
			stack[level].type = VT_Float;
			memcpy(&fval, rule+1, sizeof fval);
			stack[level].val.f = fval;
			level++;
			break;

        /* Load a double value */
        case SELRULE_Double:
            stack[level].type = VT_Double;
            memcpy(&dval, rule+1, sizeof dval);
            stack[level].val.d = dval;
            level++;
            break;

        /* Load a string value */
        case SELRULE_String:
            stack[level].type = VT_String;
            stack[level].val.s = rp+sizeof(ismRule_t);
            level++;
            break;

        /* Implement IN */
        case SELRULE_In:
            CHECK_LEVEL(1);
            result = selectIn(stack+level-1, rule);
            setResult(stack+level-1, result);
            break;

        /* Implement LIKE */
        case SELRULE_Like:
            CHECK_LEVEL(1);
            result = selectLike(stack+level-1, rule);
            setResult(stack+level-1, result);
            break;

        /* Implement Between */
        case SELRULE_Between:
            CHECK_LEVEL(3);
            result = compare_var(props, stack+level-3, stack+level-2, CMP_GE);
            if (result == SELECT_TRUE)
                result = compare_var(props, stack+level-3, stack+level-1, CMP_LE);
            level -= 3;
            setResult(stack+level, result);
            level++;
            break;

        /* Implemement AND */
        case SELRULE_And:
            CHECK_LEVEL(1);
            var = stack+level-1;
            result = getResult(var);
            if (result != SELECT_TRUE) {
                thislvl = rule->kind;
                while (rule->op != SELRULE_End || rule->kind > thislvl) {
                    memcpy(&rlen, &rule->len, 2);
                    rp += rlen;
                    rule = (ismRule_t *)rp;
                }
                next = 0;
            } else {
                level--;
            }
            break;

        /* Implemement OR */
        case SELRULE_Or:
            CHECK_LEVEL(1);
            var = stack+level-1;
            result = getResult(var);
            if (result == SELECT_TRUE) {
                thislvl = rule->kind;
                while (rule->op != SELRULE_End || rule->kind > thislvl) {
                    memcpy(&rlen, &rule->len, 2);
                    rp += rlen;
                    rule = (ismRule_t *)rp;
                }
                next = 0;
            } else {
                level--;
            }
            break;


        /* Implement unary minus */
        case SELRULE_Negative:
            CHECK_LEVEL(1);
            var = stack+level-1;
            switch (var->type) {
            case VT_Integer:    var->val.i = -var->val.i;   break;
            case VT_Byte:       var->val.i = (int)(int8_t)-var->val.i;    break;
            case VT_Short:      var->val.i = (int)(int16_t)-var->val.i;   break;
            case VT_Long:       var->val.l = -var->val.l;   break;
            case VT_Float:      var->val.f = -var->val.f;   break;
            case VT_Double:     var->val.d = -var->val.d;   break;
            default:
                var->type = VT_Null;
                break;
            };
            break;

        /* Implement NOT */
        case SELRULE_Not:
            CHECK_LEVEL(1);
            var = stack+level-1;
            if (var->type == VT_Boolean) {
                var->val.i = !var->val.i;
            } else {
            	var->type = VT_Null;
            }
            break;

        /* Implement math functions */
        case SELRULE_Calc:
            CHECK_LEVEL(2);
            result = calc_var(stack+level-2, stack+level-1, rule->kind);
            level--;
            break;


        /* Load a named field to the stack */
        case SELRULE_Var:
            if (generator)   /* Use a generator */
                generator(props, emsg, rp+sizeof(ismRule_t), stack+level, NULL);
            else             /* Use existing properties */
                ism_common_getProperty(props, rp+sizeof(ismRule_t), stack+level);
            level++;
            break;

        /* Do an ACL check */
        case SELRULE_ACLCheck:
            CHECK_LEVEL(1);
            result = selectACLCheck(stack+level-1, rule, props, generator, emsg);
            setResult(stack+level-1, result);
            break;

        /* Do a hash check */
        case SELRULE_InHash:
            CHECK_LEVEL(2);
            result = in_hash(stack+level-2, stack+level-1, rule->kind);
            level--;
            setResult(stack+level-1, result);
            break;

        /* Record options for options > 1 */
        case SELRULE_Internal:
            break;

        /* Select a topic or topic part */
        case SELRULE_Topic:
            if (!topic) {
                if (emsg)
                    topic = emsg->topic;
                if (!topic && generator) {
                    ism_field_t tf;
                    generator(props, emsg, "JMS_Topic", &tf, NULL);
                    if (tf.type == VT_String)
                        topic = tf.val.s;
                }
            }
            stack[level].type = topic ? VT_String : VT_Null;
            stack[level].val.s = (char *)topic;
            level++;
            break;

        case SELRULE_TopicPart:
            if (!topic) {
                if (emsg)
                    topic = emsg->topic;
                if (!topic && generator) {
                    ism_field_t tf;
                    generator(props, emsg, "JMS_Topic", &tf, NULL);
                    if (tf.type == VT_String)
                        topic = tf.val.s;
                }
            }
            pplen = topicpart(topic, &pp, rule->kind);
            if (pp) {
                stack[level].val.s = ap = alloca(pplen+1);
                memcpy(ap, pp, pplen);
                ap[pplen] = 0;
                stack[level].type = VT_String;
            } else {
                stack[level].type = VT_Null;
            }
            level++;
            break;

        case SELRULE_QoS:
            stack[level].type = VT_Integer;
            stack[level].val.i = emsg ? emsg->hdr->Reliability : 0;
            level++;
            break;

        case SELRULE_SmallInt:
            stack[level].type = VT_Integer;
            stack[level].val.i = rule->kind;
            level++;
            break;
        }
    }
}


/*
 * Return a part of a topic
 */
static int topicpart(const char * topic, const char * * part, int which) {
    const char * tp = topic;
    const char * endp;
    int found = 0;
    while (*tp && found < which) {
        if (*tp == '/')
            found++;
        tp++;
    }
    if (*tp) {
        *part = tp;
        endp = strchr(tp, '/');
        if (endp)
            return endp-tp;
        else
            return (int)strlen(tp);
    } else {
        *part = NULL;
        return 0;
    }
}

/*
 * Promote two numeric values for comparison
 */
static int  promote_var(ism_field_t * var1, ism_field_t * var2) {
	if (var1->type == var2->type)
		return 0;
	switch (var1->type) {
	case VT_Byte:
	case VT_UByte:
	case VT_Short:
	case VT_UShort:
	case VT_Integer:
		switch (var2->type) {
		case VT_Byte:
		case VT_UByte:
		case VT_Short:
		case VT_UShort:
		case VT_Integer:
			var1->type = VT_Integer;
			var2->type = VT_Integer;
		    return 0;
		case VT_Long:
		case VT_ULong:
			var1->type = VT_Long;
			var1->val.l = var1->val.i;
			return 0;
		case VT_UInt:
			var1->type = VT_Long;
			var2->type = VT_Long;
			var1->val.l = var1->val.i;
			var2->val.l = var2->val.i&0xffffffffL;
			return 0;
		case VT_Float:
		    var1->type = VT_Float;
		    var1->val.f = (float)var1->val.i;
		    return 0;
		case VT_Double:
	        var1->type = VT_Double;
	        var1->val.d = (double)var1->val.i;
	        return 0;
		default:
			break;
		}
		break;

	case VT_Long:
	case VT_ULong:
		switch (var2->type) {
		case VT_Byte:
		case VT_UByte:
		case VT_Short:
		case VT_UShort:
		case VT_Integer:
		    var2->type = VT_Long;
		    var2->val.l = var2->val.i;
		    return 0;
		case VT_UInt:
			var2->type = VT_Long;
			var2->val.l = var2->val.i&0xffffffffL;
			return 0;
	    case VT_Float:
	        var1->type = VT_Float;
	        var1->val.f = var2->type==VT_ULong ? (float)(uint64_t)(var2->val.l) : (float)var2->val.l;
	        return 0;
	    case VT_Double:
	        var2->type = VT_Double;
	        var2->val.d = var2->type==VT_ULong ? (double)(uint64_t)(var2->val.l) : (double)var2->val.l;
	        return 0;
		default:
			break;
		}
		break;
	case VT_UInt:
		switch (var2->type) {
		case VT_Byte:
		case VT_UByte:
		case VT_Short:
		case VT_UShort:
		case VT_Integer:
			var1->type = VT_Long;
			var2->type = VT_Long;
			var2->val.l = var2->val.i;
			var1->val.l = var1->val.u;
			return 0;
		case VT_Long:
			var1->type = VT_Long;
			var1->val.l = var1->val.u;
			return 0;
	    case VT_Float:
	        var1->type = VT_Float;
	        var1->val.f = (float)var1->val.u;
	        return 0;
	    case VT_Double:
	        var1->type = VT_Double;
	        var1->val.d = (double)var1->val.u;
	        return 0;
		default:
			break;
		}
		break;

	case VT_Float:
        switch (var2->type) {
        case VT_Byte:
        case VT_UByte:
        case VT_Short:
        case VT_UShort:
        case VT_Integer:
            var2->type = VT_Float;
            var2->val.f = (float)var2->val.i;
            return 0;
        case VT_UInt:
            var1->type = VT_Float;
            var1->val.f = (float)var1->val.u;
            return 0;
        case VT_Long:
            var1->type = VT_Float;
            var1->val.f = (float)var1->val.l;
            return 0;
        case VT_ULong:
            var1->type = VT_Long;
            var1->val.f = (float)(uint64_t)var1->val.l;
            return 0;
        case VT_Double:
            var1->type = VT_Double;
		    var1->val.d = (double)var1->val.f;
		    return 0;
        default:
            break;
		}
		break;

	case VT_Double:
        switch (var2->type) {
        case VT_Byte:
        case VT_UByte:
        case VT_Short:
        case VT_UShort:
        case VT_Integer:
            var2->type = VT_Double;
            var2->val.d = (double)var2->val.i;
            return 0;
        case VT_UInt:
            var1->type = VT_Double;
            var1->val.d = (double)var1->val.u;
            return 0;
        case VT_Long:
            var1->type = VT_Double;
            var1->val.d = (float)var1->val.l;
            return 0;
        case VT_ULong:
            var1->type = VT_Double;
            var1->val.d = (double)(uint64_t)var1->val.l;
            return 0;
        case VT_Float:
		    var2->type = VT_Double;
		    var2->val.d = (double)var2->val.f;
		    return 0;
        default:
            break;
		}
		break;
	default:
		break;
	}
    return 1;
}

/*
 * Compare two variables.  The types of the two are coerced as necessary.
 */
static int  compare_var(ism_prop_t * props, ism_field_t * var1, ism_field_t * var2, int op) {
    if (var1->type != var2->type) {
        int ptype = promote_var(var1, var2);
        if (ptype) {
            return SELECT_UNKNOWN;
        }
    }

    switch (var1->type) {
    case VT_Null:                      /* If both are unknown, the result is unknown */
        return SELECT_UNKNOWN;
    case VT_String:
        if (!var1->val.s || !var2->val.s)
            return SELECT_UNKNOWN;
        switch (op) {
        case CMP_EQ: return !(strcmp(var1->val.s, var2->val.s) == 0);
        case CMP_LT: return !(strcmp(var1->val.s, var2->val.s) < 0);
        case CMP_GT: return !(strcmp(var1->val.s, var2->val.s) > 0);
        case CMP_NE: return !(strcmp(var1->val.s, var2->val.s) != 0);
        case CMP_LE: return !(strcmp(var1->val.s, var2->val.s) <= 0);
        case CMP_GE: return !(strcmp(var1->val.s, var2->val.s) >= 0);
        }
        break;

    case VT_Integer:
    case VT_Byte:
    case VT_Short:
        switch (op) {
        case CMP_EQ: return !(var1->val.i == var2->val.i);
        case CMP_LT: return !(var1->val.i < var2->val.i);
        case CMP_GT: return !(var1->val.i > var2->val.i);
        case CMP_NE: return !(var1->val.i != var2->val.i);
        case CMP_LE: return !(var1->val.i <= var2->val.i);
        case CMP_GE: return !(var1->val.i >= var2->val.i);
        }
        break;

	case VT_UInt:
    case VT_UByte:
    case VT_UShort:
	    switch (op) {
	    case CMP_EQ: return !(var1->val.u == var2->val.u);
	    case CMP_LT: return !(var1->val.u < var2->val.u);
	    case CMP_GT: return !(var1->val.u > var2->val.u);
	    case CMP_NE: return !(var1->val.u != var2->val.u);
	    case CMP_LE: return !(var1->val.u <= var2->val.u);
	    case CMP_GE: return !(var1->val.u >= var2->val.u);
	    }
	    break;

    case VT_Boolean:
        switch (op) {
        case CMP_EQ: return !(var1->val.i == var2->val.i);
        case CMP_NE: return !(var1->val.i != var2->val.i);
        default:
            return SELECT_UNKNOWN;
        }
        break;

    case VT_Long:
        switch (op) {
        case CMP_EQ: return !(var1->val.l == var2->val.l);
        case CMP_LT: return !(var1->val.l < var2->val.l);
        case CMP_GT: return !(var1->val.l > var2->val.l);
        case CMP_NE: return !(var1->val.l != var2->val.l);
        case CMP_LE: return !(var1->val.l <= var2->val.l);
        case CMP_GE: return !(var1->val.l >= var2->val.l);
        }
        break;

    case VT_Float:
        switch (op) {
        case CMP_EQ: return !(var1->val.f == var2->val.f);
        case CMP_LT: return !(var1->val.f < var2->val.f);
        case CMP_GT: return !(var1->val.f > var2->val.f);
        case CMP_NE: return !(var1->val.f != var2->val.f);
        case CMP_LE: return !(var1->val.f <= var2->val.f);
        case CMP_GE: return !(var1->val.f >= var2->val.f);
        }
        break;

    case VT_Double:
        switch (op) {
        case CMP_EQ: return !(var1->val.d == var2->val.d);
        case CMP_LT: return !(var1->val.d < var2->val.d);
        case CMP_GT: return !(var1->val.d > var2->val.d);
        case CMP_NE: return !(var1->val.d != var2->val.d);
        case CMP_LE: return !(var1->val.d <= var2->val.d);
        case CMP_GE: return !(var1->val.d >= var2->val.d);
        }
        break;
    default:
        break;
    }
    return 0;
}

/*
 * Do a calculation on two varaibles.
 */
static int  calc_var(ism_field_t * var1, ism_field_t * var2, int op) {
    int  rc;

    /*
     * Make sure the two fields are of the same type
     */
    if (var1->type != var2->type) {
	    rc  = promote_var(var1, var2);
	    if (rc)
		    return 1;       /* type mismatch */
	}

    /*
     * Do the calculation
     */
	switch (var1->type) {
	case VT_Byte:
	case VT_Short:
	case VT_Integer:
		switch (op) {
		case '-':  var1->val.i = var1->val.i - var2->val.i;      break;
		case '+':  var1->val.i = var1->val.i + var2->val.i;      break;
		case '*':  var1->val.i = var1->val.i * var2->val.i;      break;
		case '/':  var1->val.i = var1->val.i / var2->val.i;      break;
		};
		switch (var1->type) {
		case VT_Byte:   var1->val.i = (int)(int8_t)var1->val.i;  break;
		case VT_Short:  var1->val.i = (int)(int16_t)var1->val.i; break;
		default: break;
		}
		break;

	case VT_UByte:
	case VT_UShort:
	case VT_UInt:
		switch (op) {
		case '-':  var1->val.u = var1->val.u - var2->val.u;      break;
		case '+':  var1->val.u = var1->val.u + var2->val.u;      break;
		case '*':  var1->val.u = var1->val.u * var2->val.u;      break;
		case '/':  var1->val.u = var1->val.u / var2->val.u;      break;
		};
		switch (var1->type) {
		case VT_UByte:   var1->val.u &= 0xff;    break;
		case VT_UShort:  var1->val.u &= 0xffff;  break;
		default: break;
		}
		break;

	case VT_Long:
		switch (op) {
		case '-':  var1->val.l = var1->val.l - var2->val.l;      break;
		case '+':  var1->val.l = var1->val.l + var2->val.l;      break;
		case '*':  var1->val.l = var1->val.l * var2->val.l;      break;
		case '/':  var1->val.l = var1->val.l / var2->val.l;      break;
		};
		break;

	case VT_Float:
		switch (op) {
		case '-':  var1->val.f = var1->val.f - var2->val.f;      break;
		case '+':  var1->val.f = var1->val.f + var2->val.f;      break;
		case '*':  var1->val.f = var1->val.f * var2->val.f;      break;
		case '/':  var1->val.f = var1->val.f / var2->val.f;      break;
		};
		break;

	case VT_Double:
		switch (op) {
		case '-':  var1->val.d = var1->val.d - var2->val.d;      break;
		case '+':  var1->val.d = var1->val.d + var2->val.d;      break;
		case '*':  var1->val.d = var1->val.d * var2->val.d;      break;
		case '/':  var1->val.d = var1->val.d / var2->val.d;      break;
		};
		break;
	default:
		return 1;
		break;
	}
    return 0;
}

/*
 * Scan an extension
 * @return the count of items or a negative value if the extension is not valid
 */
int ism_common_scanExtension(const char * bufp, int len, ism_scanCallback_t callback, void * userdata) {
    int count = 0;
    int itemlen;
    uint16_t sint;
    while (len) {
        uint8_t item = *bufp;
        int kind = item>>6;
        switch (kind) {
        case 0:
            itemlen = 0;
            if (item == 0x3F && len != 1)
                return -2;
            if (callback) {
                callback(item, NULL, 1, userdata);
            }
            break;
        case 1:
            if (len < 2)
                return -2;
            memcpy(&sint, bufp+1, 2);
            itemlen = endian_int16(sint);
            bufp += 2;
            len -= 2;
            if (callback) {
                callback(item, (const char *)bufp+1, itemlen, userdata);
            }
            break;
        case 2:
            itemlen = 2;
            if (callback) {
                memcpy(&sint, bufp+1, 2);
                callback(item, NULL, (uint16_t)endian_int16(sint), userdata);
            }
            break;
        case 3:
            if (item >= 0xF0) {
                if (item >= 0xF8) {
                    int32_t balen;
                    memcpy(&balen, bufp+1, 4);
                    itemlen = 4 + endian_int32(balen);
                    if (callback) {
                        callback(item, bufp+5, endian_int32(balen), userdata);
                    }
                } else {
                    itemlen = 8;
                    if (callback) {
                        int64_t lint;
                        memcpy(&lint, bufp+1, 8);
                        callback(item, NULL, endian_int64(lint), userdata);
                    }
                }
            } else {
                itemlen = 4;
                if (callback) {
                    int32_t iint;
                    memcpy(&iint, bufp+1, 4);
                    callback(item, NULL, endian_int32(iint), userdata);
                }
            }
            break;
        }
        if (len <= itemlen)
            return -1;
        bufp += (itemlen+1);
        len -= (itemlen+1);
        count++;
    }
    return count;
}



/*
 * Get an integer value from an extension
 */
int32_t ism_common_getExtensionValue(const char * bufp, int len, int which, int deflt) {
    uint8_t   match = (uint8_t)which;
    int16_t   sint;
    int32_t   nint;
    int64_t   lint;
    int       itemlen;
    while (len) {
        uint8_t item = *bufp;
        int kind = item>>6;
        switch (kind) {
        case 0:
            itemlen = 0;
            if (item == 0x3F && len != 1)
                return -2;
            if (match == item)
                return 1;
            break;
        case 1:
            if (len < 2)
                return -2;
            memcpy(&sint, bufp+1, 2);
            itemlen = (uint16_t)(endian_int16(sint));
            bufp += 2;
            len -= 2;
            break;
        case 2:
            itemlen = 2;
            if (match == item && len>2) {
                memcpy(&sint, bufp+1, 2);
                return (int32_t)(uint16_t)endian_int16(sint);
            }
            break;
        case 3:
            if (item >= 0xF0) {
                if (item >= 0xF8) {
                    if (len < 4)
                        return -2;
                    memcpy(&nint, bufp+1, 4);
                    itemlen = 4 + endian_int32(nint);
                } else {
                    itemlen = 8;
                    if (match == item && len > 8) {
                        memcpy(&lint, bufp+1, 8);
                        return (int32_t)endian_int64(lint);
                    }
                }
            } else {
                itemlen = 4;
                if (match == item && len > 4) {
                    memcpy(&nint, bufp+1, 4);
                    return endian_int32(nint);
                }
            }
            break;
        }
        if (len <= itemlen)
            return -1;
        bufp += (itemlen+1);
        len -= (itemlen+1);
    }
    if (match < 0x40)
        return 0;
    return (int32_t)deflt;
}


/*
 * Get a long integer value from an extension
 */
int64_t ism_common_getExtensionLong(const char * bufp, int len, int which, int64_t deflt) {
    uint8_t   match = (uint8_t)which;
    int16_t   sint;
    int32_t   nint;
    int64_t   lint;
    int       itemlen;
    while (len) {
        uint8_t item = *bufp;
        int kind = item>>6;
        switch (kind) {
        case 0:
            itemlen = 0;
            if (item == 0x3F && len != 1)
                return -2;
            if (match == item)
                return 1;
            break;
        case 1:
            if (len < 2)
                return -2;
            memcpy(&sint, bufp+1, 2);
            itemlen = (uint16_t)(endian_int16(sint));
            bufp += 2;
            len -= 2;
            break;
        case 2:
            itemlen = 2;
            if (match == item && len>2) {
                memcpy(&sint, bufp+1, 2);
                return (int64_t)(uint16_t)endian_int16(sint);
            }
            break;
        case 3:
            if (item >= 0xF0) {
                if (item >= 0xF8) {
                    if (len < 2)
                        return -2;
                    memcpy(&nint, bufp+1, 4);
                    itemlen = 4 + endian_int32(nint);
                } else {
                    itemlen = 8;
                    if (match == item && len > 8) {
                        memcpy(&lint, bufp+1, 8);
                        return (int64_t)endian_int64(lint);
                    }
                }
            } else {
                itemlen = 4;
                if (match == item && len > 4) {
                    memcpy(&nint, bufp+1, 4);
                    return (int64_t)(uint32_t)endian_int32(nint);
                }
            }
            break;
        }
        if (len <= itemlen)
            return -1;
        bufp += (itemlen+1);
        len -= (itemlen+1);
    }
    if (match < 0x40)
        return 0;
    return deflt;
}


/*
 * Get a string value from an extension
 */
const char * ism_common_getExtensionString(const char * bufp, int len, int which, int * outlen) {
    uint8_t   match = (uint8_t)which;
    uint16_t  sint;
    uint32_t  nint;
    int       itemlen;
    if (outlen)
        *outlen = 0;
    while (len) {
        uint8_t item = *bufp;
        int kind = item>>6;
        switch (kind) {
        case 0:
            itemlen = 0;
            if (item == 0x3F && len != 1)
                return NULL;
            break;
        case 1:
            if (len < 2)
                return NULL;
            memcpy(&sint, bufp+1, 2);
            itemlen = endian_int16(sint);
            bufp += 2;
            len -= 2;
            if (match == item && len > itemlen) {
                if (outlen)
                    *outlen = itemlen;
                if (itemlen == 0)
                    return "";
                return bufp+1;
            }
            break;
        case 2:
            itemlen = 2;
            break;
        case 3:
            if (item >= 0xF0) {
                if (item >= 0xF8) {
                    if (len < 4)
                        return NULL;
                    memcpy(&nint, bufp+1, 4);
                    itemlen = 4 + endian_int32(nint);
                } else {
                    itemlen = 8;
                }
            } else {
                itemlen = 4;
            }
            break;
        }
        if (len <= itemlen)
            return NULL;
        bufp += (itemlen+1);
        len -= (itemlen+1);
    }
    return NULL;
}

/*
 * Get a string value from an extension
 */
const char * ism_common_getExtensionByteArray(const char * bufp, int len, int which, int * outlen) {
    uint8_t   match = (uint8_t)which;
    uint16_t  sint;
    uint32_t  nint;
    int       itemlen;
    if (outlen)
        *outlen = 0;
    while (len) {
        uint8_t item = *bufp;
        int kind = item>>6;
        switch (kind) {
        case 0:
            itemlen = 0;
            if (item == 0x3F && len != 1)
                return NULL;
            break;
        case 1:
            if (len < 2)
                return NULL;
            memcpy(&sint, bufp+1, 2);
            itemlen = endian_int16(sint) + 2;
            break;
        case 2:
            itemlen = 2;
            break;
        case 3:
            if (item >= 0xF0) {
                if (item >= 0xF8) {
                    if (len < 4)
                        return NULL;
                    memcpy(&nint, bufp+1, 4);
                    itemlen = 4 + endian_int32(nint);
                    if (match == item && len >= itemlen) {
                        if (outlen)
                            *outlen = endian_int32(nint);
                        return bufp+5;
                    }
                } else {
                    itemlen = 8;
                }
            } else {
                itemlen = 4;
            }
            break;
        }
        if (len <= itemlen)
            return NULL;
        bufp += (itemlen+1);
        len -= (itemlen+1);
    }
    return NULL;
}


/*
 * Put a string to an extension
 */
int ism_common_putExtensionString(concat_alloc_t * buf, int which, const char * value) {
    uint8_t cbuf[4];
    int     itemlen;

    if (which < 0x40 || which >= 0x80) {
        return ISMRC_Error;
    }
    cbuf[0]= (uint8_t)which;
    itemlen = value ? strlen(value)+1 : 0;
    cbuf[1] = (uint8_t)(itemlen>>8);
    cbuf[2] = (uint8_t)itemlen;
    ism_common_allocBufferCopyLen(buf, (char *)cbuf, 3);
    if (itemlen > 0)
        ism_common_allocBufferCopyLen(buf, value, itemlen);
    return 0;
}

/*
 * Put a string to an extension
 */
int ism_common_putExtensionByteArray(concat_alloc_t * buf, int which, const char * value, int itemlen) {
    uint8_t cbuf[8];

    if (which < 0xF8 || which >= 0xFF || itemlen < 0 || itemlen > 0x1000000) {
        return ISMRC_Error;
    }
    cbuf[0]= (uint8_t)which;
    cbuf[1] = (uint8_t)(itemlen>>24);
    cbuf[2] = (uint8_t)(itemlen>>16);
    cbuf[3] = (uint8_t)(itemlen>>8);
    cbuf[4] = (uint8_t)itemlen;
    ism_common_allocBufferCopyLen(buf, (char *)cbuf, 5);
    if (itemlen > 0)
        ism_common_allocBufferCopyLen(buf, value, itemlen);
    return 0;
}


/*
 * Put a non-string value to an extensions
 */
int ism_common_putExtensionValue(concat_alloc_t * buf, int which, uint64_t value) {
    uint8_t cbuf[10];
    uint16_t   sint;
    uint32_t   nint;
    uint64_t   lint;
    int        len = 0;

    cbuf[0]= (uint8_t)which;
    switch (cbuf[0]>>6) {
    case 0:
        len = 1;
        break;
    case 1:
        return ISMRC_Error;
    case 2:
        sint = endian_int16((uint16_t)value);
        memcpy(cbuf+1, &sint, 2);
        len = 3;
        break;
    case 3:
        if (which >= 0xF0) {
            if (which >= 0xF8) {
                return ISMRC_Error;
            } else {
                len = 9;
                lint = endian_int64((uint64_t)value);
                memcpy(cbuf+1, &lint, 8);
            }
        } else {
            len = 5;
            nint = endian_int32((uint32_t)value);
            memcpy(cbuf+1, &nint, 4);
        }
        break;
    }
    ism_common_allocBufferCopyLen(buf, (char *)cbuf, len);
    return 0;
}

pthread_rwlock_t acl_lock = PTHREAD_RWLOCK_PREFER_WRITER_NP;

ismHashMap * acl_list = NULL;

/*
 * Check that the key is in the ACL list
 * @param  key     The ACL key
 * @param  aclname The name of the ACL
 * @return 0=found, 1=not-found, -1=no-ACL
 */
int ism_protocol_checkACL(const char * key, const char * aclname) {
    ism_acl_t * acl = ism_protocol_findACL(aclname, 0);
    int rc;
    if (!acl)
        return -1;
    rc = ism_common_getHashMapElement(acl->hash, key, strlen(key)) == NULL;
    ism_protocol_unlockACL(acl);
    return rc;
}

/*
 * Find the ACL and create it if requested.
 */
ism_acl_t * ism_protocol_findACL(const char * name, int create) {
    int namelen;
    ism_acl_t * acl = NULL;

    if(name==NULL || name[0] == '\0')
    	return NULL;

    namelen = (int)strlen(name);
    if (namelen == 2 && create != 9 && *name == '_' && (name[1]>='0' && name[1]<='9')) {
        acl = g_acl_array[name[1]-'0'];
        if (!acl && create) {
            acl = ism_protocol_findACL(name, 9);
            return acl;
        }
        if (acl)
            pthread_spin_lock(&acl->lock);
        return acl;
    }
    if (create) {
        pthread_rwlock_wrlock(&acl_lock);
    } else {
        pthread_rwlock_rdlock(&acl_lock);
    }
    /*
     * If there is no ACL list, create it
     */
    if (!acl_list) {
        if (create) {
            acl_list = ism_common_createHashMap(1000, HASH_STRING);
            if (!acl_list)
                ism_common_setError(ISMRC_AllocateError);
        }
    }

    /*
     * If the ACL list exists, look  up the ACL, and create it if requested
     */
    if (acl_list) {
        acl = ism_common_getHashMapElement(acl_list, name, namelen);
        if (!acl && create) {
            acl = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,222),1, sizeof(ism_acl_t) + namelen + 1);
            if (acl) {
                acl->hash = ism_common_createHashMap(100, HASH_STRING);
                if (!acl->hash) {
                    ism_common_setError(ISMRC_AllocateError);
                    ism_common_free(ism_memory_utils_misc,acl);
                    acl = NULL;
                } else {
                    acl->name = (char *)(acl+1);
                    strcpy(acl->name, name);
                    pthread_spin_init(&acl->lock, 0);
                }
            }
            ism_common_putHashMapElement(acl_list, name, namelen, acl, NULL);
            if (create == 9 && namelen == 2 && *name == '_' && (name[1]>='0' && name[1]<='9')) {
                g_acl_array[name[1]-'0'] = acl;
            }
        }
        if (acl)
            pthread_spin_lock(&acl->lock);
    }
    pthread_rwlock_unlock(&acl_lock);
    return acl;
}

/*
 * Make the ACL no longer the active ACL
 */
void ism_protocol_unlockACL(ism_acl_t * acl) {
    if (acl) {
        pthread_spin_unlock(&acl->lock);
    }
}

/*
 * Look up in a hash table
 */
static int  in_hash(ism_field_t * var1, ism_field_t * var2, int op) {
    char xname [8];
    if (var2->type == VT_Integer && var2->val.i >= 0 && var2->val.i <= 9) {
        strcpy(xname, "_0");
        xname[1] += var2->val.i;
        var2->val.s = xname;
    } else if (var2->type != VT_String) {
        return SELECT_UNKNOWN;
    }
    if (var1->type != VT_String)
        return SELECT_UNKNOWN;

    /*
     * Do a calculation on two varaibles.
     */
    return ism_protocol_checkACL(var1->val.s, var2->val.s);
}



/*
 * Do an ACL check within the filter
 */
int checkACL(ism_field_t * f, const char * extra) {
    if (f->type != VT_String) {
        f->type = VT_Null;
        f->val.s = NULL;
        return 1;
    } else {
        int zrc = ism_protocol_checkACL(f->val.s, extra);
        if (zrc < 0) {
            f->type = VT_Null;
            f->val.s = NULL;
            return 1;
        } else {
            f->type = VT_Boolean;
            f->val.i = zrc == 0;
            return 0;
        }
    }
}

/*
 * Add an item to an ACL set.
 * This must be called with the ACL set locked.
 */
void ism_protocol_addACLitem(ism_acl_t * acl, const char * key) {
    ism_common_putHashMapElement(acl->hash,  key, strlen(key), (void *)(uintptr_t)1, NULL);
}

/*
 * Delete an item to an ACL set.
 * This must be called with the ACL set locked.
 */
void ism_protocol_delACLitem(ism_acl_t * acl, const char * key) {
    ism_common_removeHashMapElement(acl->hash,  key, strlen(key));
}

/*
 * Delete an ACL
 * @param name  The name of the ACL
 */
int ism_protocol_deleteACL(const char * name, ism_ACLcallback_f create_cb) {
    ism_acl_t * acl = NULL;
    int namelen = strlen(name);

    pthread_rwlock_wrlock(&acl_lock);
    if (acl_list)
        acl = ism_common_getHashMapElement(acl_list, name, namelen);
    if (acl) {
        pthread_spin_lock(&acl->lock);
        ism_common_removeHashMapElement(acl_list, acl->name, strlen(acl->name));
        pthread_rwlock_unlock(&acl_lock);
        TRACE(5, "Delete ACL set: %s\n", name);
        ism_common_destroyHashMap(acl->hash);
        acl->hash = NULL;
        /* Call the notification cb if it exists with the hashmap set to null */
        if (create_cb)
            create_cb(acl);
        ism_protocol_unlockACL(acl);
        ism_common_free(ism_memory_utils_misc,acl);
        return 0;
    } else {
        pthread_rwlock_unlock(&acl_lock);
        TRACE(7, "Unable to delete ACL set because it is not found: %s\n", name);
        return 1;
    }
}


/*
 * Get the count of items in the ACL
 */
int ism_protocol_getACLcount(const char * name) {
    ism_acl_t * acl;
    int  ret = -1;
    acl = ism_protocol_findACL(name, 0);
    if (acl) {
        ret = ism_common_getHashMapNumElements(acl->hash);
        ism_protocol_unlockACL(acl);
    }
    return ret;
}

/*
 * After check to remove any items which do not still exist
 */
static int hashcheck(ismHashMapEntry * hme, void * context) {
    concat_alloc_t * buf = (concat_alloc_t * ) context;
    if (hme->value == (void *)2) {
        ism_common_allocBufferCopyLen(buf, (const char *)&hme, sizeof hme);
    }
    return 0;
}

/*
 * Start up a replacement so we know what entries were not replaced
 */
static int hashstart(ismHashMapEntry * hme, void * context) {
    if (hme->value == (void *)1)
        hme->value = (void *)2;       /* Mark for pending delete */
    return 0;
}

/*
 * Fix ACLs to implement ACL replacement
 */
static int fixacl(ism_acl_t * acl, int after) {
    if (after) {
        int  count;
        int  i;
        ismHashMapEntry * hme;
        ismHashMapEntry * * hmelist;
        char xbuf[2048];
        concat_alloc_t buf = {xbuf, sizeof xbuf};

        ism_common_enumerateHashMap(acl->hash, hashcheck, &buf);

        /* Delete all names which were not updated */
        count = buf.used / sizeof (ismHashMapEntry *);
        hmelist = (ismHashMapEntry * *)buf.buf;
        for (i=0; i<count; i++) {
            hme = *hmelist++;
            ism_common_removeHashMapElement(acl->hash,  hme->key, hme->key_len);
        }
        //Free memory if in heap
        if (buf.inheap)
              ism_common_freeAllocBuffer(&buf);
    } else {
        ism_common_enumerateHashMap(acl->hash, hashstart, NULL);
    }
    return 0;
}

/*
 * Check if an acl belongs to a tenant. Specifically an application
 * acl. The acl name must begin with a-<name>
 */
static int check_acl_for_tenant(const char * name, const char * acl_name) {
    char acl_prefix[3] = "a-";
    char sep[2] = "-";
    char *acl_token;
    char *acl_name_local;

    if (acl_name && (strlen(acl_name) > 2)) {
        if (strncmp(acl_name, acl_prefix, 2) == 0) {
            acl_name_local = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),acl_name);

            /* just check second token since we know it starts with a- */
            strtok(acl_name_local, sep);
            acl_token = strtok(NULL, sep);

            if (strcmp(name, acl_token) == 0) {
                TRACE(8, "Found application acl: %s for tenant: %s\n", name, acl_name);
                ism_common_free(ism_memory_utils_misc,acl_name_local);
                return 0;
            } else {
                ism_common_free(ism_memory_utils_misc,acl_name_local);
            }
        }
    }
    return -1;

}

/*
 * filter struct for finding api acls
 */
typedef struct rlac_filter_context_t {
    const char * name;      /* the tenant name or org */
    concat_alloc_t * buf;   /* a buf where acl entries will be tracked */
} rlac_filter_context_t;

/*
 * Build a buffer of all the api rlac acls for a
 * specific tenant
 */
static int rlac_buf_callback(ismHashMapEntry * hme, void * context) {
    rlac_filter_context_t * filter_context = (rlac_filter_context_t * ) context;
    concat_alloc_t * buf = filter_context->buf;

    /* delete acls that start with a-<tenant> */
    if (check_acl_for_tenant(filter_context->name, hme->key) == 0) {
        ism_common_allocBufferCopyLen(buf, (const char *)&hme, sizeof hme);
    }
    return 0;
}

/*
 * Delete all application acls for a tenant
 *
 * @param tenant_name  The name of the tenant
 */
int ism_rlac_deleteACL(const char * tenant_name) {
    rlac_filter_context_t * filter_context;
    char xbuf[2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    int count;
    int i;
    ismHashMapEntry * hme;
    ismHashMapEntry * * hmelist;

    TRACE(5, "Deleting any existing application acls for the tenant: %s\n", tenant_name);

    if (acl_list) {
        filter_context = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,227),1, sizeof(rlac_filter_context_t));
        filter_context->name = tenant_name;
        filter_context->buf = &buf;
        ism_common_enumerateHashMap(acl_list, rlac_buf_callback, filter_context);
        count = buf.used / sizeof (ismHashMapEntry *);
        hmelist = (ismHashMapEntry * *)buf.buf;
        for (i=0; i<count; i++) {
            hme = *hmelist++;
            TRACE(5, "Deleting application acl: %s for tenant: %s\n", hme->key, tenant_name);
            ism_protocol_deleteACL(hme->key, NULL);
        }
        if (buf.inheap ) {
           ism_common_freeAllocBuffer(&buf);
        }
        ism_common_free(ism_memory_utils_misc,filter_context);
    }
    return 0;
}

/*
 * Set ACLs from an ACL source.
 * The ACL source is a set of lines terminated by an combination of CR, LF, and NUL.
 * The first character is the operator and the rest of the line is the data.
 * '/' = comment, the data is ignored
 * '@' = create or update an ACL set, the data is the name of the ACL set
 * ':' = replace an ACL set, the data is the name of the ACL set
 * '!' = delete an ACL set, the data is the name of the ACL set
 * '+' = add an ACL item to the current set, the data is the item key
 * '-' = delete an ACL item from the current set, the data is the item key
 *
 *
 * @param aclsrc  The ACL definitions
 * @paarm acllen  The length of the ACL.  If this is negative, use the string length.
 * @parse options Options for setting the ACL
 */
int ism_protocol_setACL(const char * aclsrc, int acllen, int opt, ism_ACLcallback_f create_cb) {
    ism_acl_t * acl = NULL;
    char * aclcopy;
    int    rc = 0;
    int    done = 0;
    int    fixneeded = 0;

    if (aclsrc && acllen < 0)
        acllen = strlen(aclsrc);
    if (!aclsrc || acllen<3 || acllen>4*1024*1024 || ism_common_validUTF8(aclsrc, acllen) < 0) {
        TRACE(3, "setACL invalid ACL source: length error\n");
        return ISMRC_Error;
    }

    /* Make a copy of the list and change CR or LF to NUL */
    aclcopy = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,229),acllen+2);
    memcpy(aclcopy, aclsrc, acllen);
    aclcopy[acllen] = 0;
    aclcopy[acllen+1] = 0xFF;
    char * ap = aclcopy;
    while (*ap) {
        if (*ap == '\r' || *ap == '\n')
            *ap = 0;
        ap++;
    }

    ap = aclcopy;
    for (;;) {
        while (!*ap)
            ap++;
        switch ((uint8_t)*ap) {
        /* End of ACLs */
        case 0xFF:
            done = 1;
            break;

        /* Comment */
        case '/':
            break;

        /* New set */
        case '@':
            if (acl) {
                if (fixneeded) {
                    fixacl(acl, 1);
                    fixneeded = 0;
                }
                ism_protocol_unlockACL(acl);
                acl = NULL;
            }
            acl = ism_protocol_findACL(ap+1, 1);
            if (create_cb && ism_common_getHashMapNumElements(acl->hash) == 0) {
                create_cb(acl);
            }
            break;

            /* New set */
        case ':':
            if (acl) {
                if (fixneeded) {
                    fixacl(acl, 1);
                    fixneeded = 0;
                }
                ism_protocol_unlockACL(acl);
                acl = NULL;
            }
            acl = ism_protocol_findACL(ap+1, 1);
            fixacl(acl, 0);
            fixneeded = 1;
            if (create_cb) {
                create_cb(acl);
            }
            break;

        /* Delete set */
        case '!':
            if (acl) {
                if (fixneeded) {
                    fixacl(acl, 1);
                    fixneeded = 0;
                }
                ism_protocol_unlockACL(acl);
                acl = NULL;
            }
            ism_protocol_deleteACL(ap+1, create_cb);
            break;

        /* Add item */
        case '+':
            if (!acl) {
                TRACE(3, "setACL invalid ACL source: the set must be defined before an add");
                rc = ISMRC_Error;
                done = 1;
            } else {
                ism_protocol_addACLitem(acl, ap+1);
            }
            break;
        /* Remove item */
        case '-':
            if (!acl) {
                TRACE(3, "setACL invalid ACL source: the set must be defined before a delete");
                rc = ISMRC_Error;
                done = 1;
            } else {
                ism_protocol_delACLitem(acl, ap+1);
            }
            break;
        default:
            TRACE(3, "setACL invalid ACL source: content not valid\n");

        }
        if (done)
            break;
        ap += strlen(ap);
    }
    if (acl) {
        if (fixneeded) {
            fixacl(acl, 1);
            fixneeded = 0;
        }
        ism_protocol_unlockACL(acl);
        acl = NULL;
    }
    if (aclcopy)
        ism_common_free(ism_memory_utils_misc,aclcopy);
    return rc;
}


/*
 * Set an ACL file
 */
int ism_protocol_setACLfile2(const char * aclfile, ism_ACLcallback_f create_cb, ism_setACLcb_f send_cb) {
    int  rc = 0;
    FILE * f = fopen(aclfile, "rb");
    if (!f) {
        TRACE(4, "Unable to open ACL file: %s\n", aclfile);
        rc = ISMRC_NotFound;
    } else {
        int  bread;
        int  len;
        char * buf = NULL;
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        if (len > 0) {
            buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,231),len+2);
        }
        if (!buf) {
            TRACE(4, "Unable to allocate memory for ACL file: %s\n", aclfile);
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
            TRACE(4, "Unable to read the ACL file:%s\n", aclfile);
            ism_common_free(ism_memory_utils_misc,buf);
            fclose(f);
            return ISMRC_NotFound;
        }
        fclose(f);

        /*
         * Set the ACL
         */
        rc = ism_protocol_setACL(buf, len, 0, create_cb);
        TRACE(4, "Set ACL rc=%d file=%s\n", rc, aclfile);
        if (send_cb)
            send_cb(buf, len);
    }
    return rc;
}


/*
 * Normat setACLfile, call internal version with null send_cb
 */
int ism_protocol_setACLfile(const char * aclfile, ism_ACLcallback_f create_cb) {
    return ism_protocol_setACLfile2(aclfile, create_cb, NULL);
}

/*
 * Initialize ACL processing.
 * This ACL file at init is mostly used for test environments.
 */
int ism_protocol_initACL(void) {
    int rc = 0;
    const char * aclfile = ism_common_getStringConfig("ACLfile");
    if (aclfile) {
        rc = ism_protocol_setACLfile(aclfile, NULL);
    }
    return rc;
}


/*
 * Print one ACL
 */
static int printACL(ism_acl_t * acl) {
    ismHashMapEntry * * items;
    ismHashMapEntry * * itemlist;
    ismHashMapEntry *   item;
    items = itemlist = ism_common_getHashMapEntriesArray(acl->hash);
    printf("@%s\n", acl->name);
    for (;;) {
        item = *items++;
        if (item == (void *)-1)
            break;
        printf("+%s\n", item->key);
    }
    ism_common_freeHashMapEntriesArray(itemlist);
    return 0;
}


/*
 * Get an ACL source into a buffer
 */
int ism_protocol_getACLs(concat_alloc_t * buf, const char * aclname) {
    int rc = 0;
    ism_acl_t * acl;
    acl = ism_protocol_findACL(aclname, 0);
    if (acl) {
        ismHashMapEntry * * items;
        ismHashMapEntry * * itemlist;
        ismHashMapEntry *   item;
        items = itemlist = ism_common_getHashMapEntriesArray(acl->hash);
        bputchar(buf, ':');
        ism_json_putBytes(buf, acl->name);
        bputchar(buf, '\n');
        for (;;) {
            item = *items++;
            if (item == (void *)-1)
                break;
            bputchar(buf, '+');
            ism_json_putBytes(buf, item->key);
            bputchar(buf, '\n');
        }
        ism_common_freeHashMapEntriesArray(itemlist);
        ism_protocol_unlockACL(acl);
        return 0;
    } else {
        rc = ISMRC_NotFound;
        TRACE(5, "No ACL found: %s\n", aclname);
    }
    return rc;
}

/*
 * Get an ACL source into a buffer
 * The invoker must send in a locked acl
 */
int ism_protocol_getACL(concat_alloc_t * buf, ism_acl_t * acl) {
    ismHashMapEntry * * items;
    ismHashMapEntry * * itemlist;
    ismHashMapEntry *   item;
    items = itemlist = ism_common_getHashMapEntriesArray(acl->hash);
    bputchar(buf, ':');
    ism_json_putBytes(buf, acl->name);
    bputchar(buf, '\n');
    for (;;) {
        item = *items++;
        if (item == (void *)-1)
            break;
        bputchar(buf, '+');
        ism_json_putBytes(buf, item->key);
        bputchar(buf, '\n');
    }
    ism_common_freeHashMapEntriesArray(itemlist);
    return 0;
}



/*
 * Implement the acl command
 */
int ism_protocol_printACL(const char * name) {
    ism_acl_t * acl;

    ismHashMapEntry * * items;
    ismHashMapEntry * * itemlist;
    ismHashMapEntry * item;

    if (!acl_list)
        return 0;

    pthread_rwlock_rdlock(&acl_lock);
    items = itemlist = ism_common_getHashMapEntriesArray(acl_list);
    for (;;) {
        item = *items++;
        if (item == (void *)-1)
            break;
        if (ism_common_match(item->key, name)) {
            if (item->value) {
                acl = (ism_acl_t *)item->value;
                pthread_spin_lock(&acl->lock);
                printACL(acl);
                pthread_spin_unlock(&acl->lock);
            }
        }
    }
    ism_common_freeHashMapEntriesArray(itemlist);
    pthread_rwlock_unlock(&acl_lock);

    return 0;
}


/*
 * Initialize the mqtt optional fields
 */
mqtt_prop_ctx_t * ism_common_makeMqttPropCtx(mqtt_prop_field_t * idtbl, int version) {
    int   max_id      = 0;
    int   max_array   = 0;
    int   more_count  = 0;
    mqtt_prop_field_t * tbl;
    mqtt_prop_ctx_t *   ctx;
    mqtt_prop_field_t * * more = NULL;

    tbl = idtbl;
    while (tbl->id) {
        if (tbl->id > max_id && tbl->version >= version)
            max_id = tbl->id;
        if (tbl->id > 127 && tbl->version >= version)
            more_count++;
        tbl++;
    }

    max_array = max_id;
    if (max_array > 127)
        max_array = 127;
    ctx = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,233),1, sizeof(mqtt_prop_ctx_t) + (sizeof(mqtt_prop_field_t) * (max_array+1+more_count)));
    ctx->version = version;
    ctx->array_id = max_array;
    ctx->max_id = max_id;
    ctx->more_count = more_count;
    ctx->fields = (mqtt_prop_field_t * *)(ctx+1);
    if (more_count)
        ctx->more = more = ctx->fields + (max_array+1);

    more_count = 0;
    tbl = idtbl;
    while (tbl->id) {
        mqtt_prop_field_t * ver = ctx->fields[tbl->id];
        if (!ver || ver->version < version) {
            if (tbl->id <= ctx->array_id)
                ctx->fields[tbl->id] = tbl;
            else
                ctx->more[more_count++] = tbl;
        }
        tbl++;
    }
    return ctx;
}


/*
 * Free the mqtt version tables.
 * This is normally done only in testing.  It is done without locks so we assume
 * it is not done while mqtt processing is active.
 */
int  ism_common_freeMqttPropCtx(mqtt_prop_ctx_t * ctx) {
    if (ctx)
        ism_common_free(ism_memory_utils_misc,ctx);
    return 0;
}

/*
 * Find a field in the context
 */
mqtt_prop_field_t * ism_common_findMqttPropID(mqtt_prop_ctx_t * ctx, int id) {
    mqtt_prop_field_t * fld;
    int    i;

    if (id < ctx->array_id)
        return ctx->fields[id];
    for (i=0; i<ctx->more_count; i++) {
        fld = ctx->more[i];
        if (fld->id == id)
            return fld;
    }
    return NULL;
}

/*
 * Check the validity of an MQTT optional field area
 *
 * @param bufp    The location of the area
 * @param len     The length of the area
 * @param ctx     The extension context
 * @oarse flags   The MqttOptIDs_e flags for this control packet
 * @param server 0=client, 1=server
 * @return A return code: 0=good, otherwise an ISMRC return code
 */
XAPI int ism_common_checkMqttPropFields(const char * bufp, int len, mqtt_prop_ctx_t * ctx, int flags,
        ism_check_ext_f check, void * userdata) {
    char * foundtab;
    int    rc;

    foundtab = alloca(ctx->max_id + 1);
    memset(foundtab, 0, ctx->max_id + 1);

    while (len) {
        mqtt_prop_field_t * fld;
        int datalen;
        int value;
        uint32_t int32;
        int id = (uint8_t)*bufp++;
        len--;
        if (id > ctx->max_id) {
            ism_common_setErrorData(ISMRC_BadClientData, "%s%u", "BadPropID", id);
            return ISMRC_BadClientData;
        }

        fld = id<=ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
        if (fld == NULL) {
            ism_common_setErrorData(ISMRC_BadClientData, "%s%u", "UnknownProp", id);
            return ISMRC_BadClientData;
        }
        if (!(fld->valid&flags)) {
            ism_common_setErrorData(ISMRC_BadClientData, "%s%s", "BadProp", fld->name);
            return ISMRC_BadClientData;
        }

        if (foundtab[id] && !(fld->valid & CPOI_MULTI)) {
            if (!(flags&CPOI_IS_ALT) || (fld->valid & CPOI_ALT_MULTI) != CPOI_ALT_MULTI) {
                ism_common_setErrorData(ISMRC_BadClientData, "%s%s", "DupProp", fld->name);
                return ISMRC_BadClientData;
            }
        }
        foundtab[id] = 1;

        value = 0;
        switch (fld->type) {
        case MPT_Boolean:
            datalen = 0;
            value = 1;
            break;
        case MPT_Int1:
            datalen = 1;
            if (len >= 1)
                value = (uint8_t)*bufp;
            break;
        case MPT_Int2:
            datalen = 2;
            if (len >= 2)
                value = (uint16_t)BIGINT16(bufp);
            break;
        case MPT_Int4:
            datalen = 4;
            if (len >= 4) {
                memcpy(&int32, bufp, 4);
                value = endian_int32(int32);
            }
            break;
        case MPT_Bytes:
        case MPT_String:
            value = 0;
            if (len<2) {
                ism_common_setErrorData(ISMRC_BadLength, "%s%u", "String", id);
                return ISMRC_BadLength;
            }
            datalen = (uint16_t)BIGINT16(bufp);
            bufp += 2;
            len -= 2;
            break;
        case MPT_NamePair:
            {
                int namelen;
                int vallen;
                /* Check the two inner lengths */
                if (len >= 4)
                    namelen = BIGINT16(bufp);
                else
                    namelen = 4;
                if (namelen > (len-4)) {
                    ism_common_setErrorData(ISMRC_BadLength, "%s%u", "Property", id);
                    return ISMRC_BadLength;
                }
                vallen = BIGINT16(bufp+namelen+2);
                if (vallen > (len-namelen-4)) {
                    ism_common_setErrorData(ISMRC_BadLength, "%s%u", "Property", id);
                    return ISMRC_BadLength;
                }
                value = namelen;
                datalen = namelen + vallen + 4;
            }
            break;
        case MPT_VarInt:
            {
                concat_alloc_t vibuf = {(char *)bufp, len, len};
                value = ism_common_getMqttVarInt(&vibuf);
                if (value < 0) {
                    ism_common_setErrorData(ISMRC_BadLength, "%s%u", "Property", id);
                    return ISMRC_BadLength;
                }
                datalen = vibuf.pos;
            }
            break;
        default:
            ism_common_setErrorData(ISMRC_BadClientData, "%s%u%u", "BadID", id, fld->type);
            return ISMRC_BadClientData;
        }
        if (len < datalen) {
            ism_common_setErrorData(ISMRC_BadLength, "%s%u", "PropertyLength", id);
            return ISMRC_BadLength;
        }
        if (check) {
            rc = check(ctx, userdata, fld, bufp, datalen, value);
            if (rc)
                return rc;
        }
        len -= datalen;
        bufp += datalen;
    }
    return 0;
}

/*
 * Get an MQTT optional field integer value
 *
 * @param bufp   The location of the area
 * @param len    The length of the area
 * @param which  The ID of the optional field
 * @parse deflt  The default value if the opptional field is not present
 * @return The value of the optional field, or deflt if it is not present
 */
int ism_common_getMqttPropField(const char * bufp, int len, int which, mqtt_prop_ctx_t * ctx, int deflt) {
    while (len) {
        mqtt_prop_field_t * fld;
        int datalen;
        int id = (uint8_t)*bufp++;
        len--;
        if (id > ctx->max_id)
            return deflt;

        fld = id <= ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
        if (!fld)
            return deflt;
        switch (fld->type) {
        case MPT_Boolean:
            datalen = 0;
            if (id == which) {
                return 1;
            }
            break;
        case MPT_Int1:
            datalen = 1;
            if (id == which && len >= 1) {
                return (uint8_t)*bufp;
            }
            break;
        case MPT_Int2:
            datalen = 2;
            if (id == which && len >= 2) {
                return (uint16_t)BIGINT16(bufp);
            }
            break;
        case MPT_Int4:
            datalen = 4;
            if (id == which && len >= 4) {
                uint32_t ival;
                memcpy(&ival, bufp, 4);
                return endian_int32(ival);
            }
            break;
        case MPT_String:
            if (len >= 2) {
                datalen = (uint16_t)BIGINT16(bufp) + 2;
                if (id == which)
                    return deflt;
            } else {
                return deflt;
            }
            break;
        case MPT_NamePair:
            if (len >= 4) {
                datalen = (uint16_t)BIGINT16(bufp);
                if (datalen+4 > len)
                    return deflt;
                datalen += (uint16_t)BIGINT16(bufp+datalen+2) + 4;
            } else {
                return deflt;
            }
            break;
        case MPT_VarInt:
            datalen = 1;
            if (len >= 1) {
                uint32_t value = *bufp&0x7f;
                if (*bufp & 0x80) {
                    concat_alloc_t vibuf = {(char *)bufp, len, len};
                    value = ism_common_getMqttVarInt(&vibuf);
                    if (value < 0)
                        return deflt;
                    datalen = vibuf.pos;
                }
                if (id == which)
                    return value;
            }
            break;
        default:
            return deflt;
            break;
        }
        if (len < datalen)
            return deflt;
        len -= datalen;
        bufp += datalen;
    }
    return deflt;
}

/*
 * Get an MQTT optional field string value
 *
 * Note: the returned string is not null terminated
 *
 * @param bufp   The location of the area
 * @param len    The length of the area
 * @param cmd    The MQTT command packet code
 * @param which  The ID of the optional field
 * @param ctx    The extension context
 * @param outlen The optional output length.
 */
const char * ism_common_getMqttPropString(const char * bufp, int len, int which, mqtt_prop_ctx_t * ctx, int * outlen) {

    if (outlen)
        *outlen = 0;
    while (len) {
        mqtt_prop_field_t * fld;
        int datalen;
        int id = (uint8_t)*bufp++;
        len--;
        if (id > ctx->max_id)
            return NULL;

        fld = id <= ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
        if (!fld)
            return NULL;

        switch (fld->type) {
        case MPT_Boolean:
            datalen = 0;
            break;
        case MPT_Int1:
            datalen = 1;
            if (id == which)
                return NULL;
            break;
        case MPT_Int2:
            datalen = 2;
            if (id == which)
                return NULL;
            break;
        case MPT_Int4:
            datalen = 4;
            if (id == which)
                return NULL;
            break;
        case MPT_String:
            if (len >= 2) {
                datalen = (uint16_t)BIGINT16(bufp);
                if (id == which && len >= (datalen+2) ) {
                    if (outlen)
                        *outlen = datalen;
                    return bufp+2;
                }
            } else {
                datalen = 0;
            }
            datalen += 2;
            break;
        case MPT_NamePair:
            if (len >= 4) {
                datalen = (uint16_t)BIGINT16(bufp);
                if (datalen+4 > len)
                    return NULL;
                datalen += (uint16_t)BIGINT16(bufp+datalen+2) + 4;
            } else {
                datalen = 4;
            }
            break;
        case MPT_VarInt:
            datalen = 1;
            if (len >= 1) {
                if (*bufp & 0x80) {
                    concat_alloc_t vibuf = {(char *)bufp, len, len};
                    uint32_t value = ism_common_getMqttVarInt(&vibuf);
                    if (value < 0)
                        return NULL;
                    datalen = vibuf.pos;
                }
                if (id == which)
                    return NULL;
            }
            break;

        default:
            return NULL;
            break;
        }
        if (len < datalen)
            return NULL;
        len -= datalen;
        bufp += datalen;
    }
    return NULL;
}

/*
 * Put an MQTT optional integer value
 *
 * @param buf   The buffer to put the optional field
 * @oaran id    The ID of the optional field
 * @param ctx   The extension context
 * @param value The value of the optional field
 * @return A return code, 0=good
 */
int ism_common_putMqttPropField(concat_alloc_t * buf, int id, mqtt_prop_ctx_t * ctx, int value) {
    mqtt_prop_field_t * fld;;
    fld = id <= ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
    switch (fld->type) {
    default:
        return ISMRC_Error;
    case MPT_Boolean:
        if (value)
            bputchar(buf, id);
        break;
    case MPT_Int1:
        bputchar(buf, id);
        bputchar(buf, value);
        break;
    case MPT_Int2:
        bputchar(buf, id);
        bputchar(buf, value>>8);
        bputchar(buf, value);
        break;
    case MPT_Int4:
        bputchar(buf, id);
        bputchar(buf, value>>24);
        bputchar(buf, value>>16);
        bputchar(buf, value>>8);
        bputchar(buf, value);
        break;
    case MPT_VarInt:
        bputchar(buf, id);
        ism_common_putMqttVarInt(buf, value);
        break;
    }
    return 0;
}

/*
 * Put an MQTT optional string value
 *
 * @param buf   The buffer to put the optional field
 * @oaram id    The ID of the optional field
 * @param ctx   The extension context
 * @param value The value of the optional field
 * @param len   The length of the value, or -1 to indicate strlen should be used
 * @return A return code, 0=good
 */
int ism_common_putMqttPropString(concat_alloc_t * buf, int id, mqtt_prop_ctx_t * ctx, const char * value, int len) {
    mqtt_prop_field_t * fld;;
    fld = id <= ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
    switch (fld->type) {
    default:
        return ISMRC_Error;
    case MPT_String:
    case MPT_Bytes:
        if (len < 0)
            len = strlen(value);
        bputchar(buf, id);
        bputchar(buf, len>>8);
        bputchar(buf, len);
        if (len)
            ism_common_allocBufferCopyLen(buf, value, len);
        break;
    }
    return 0;
}

/*
 * Put an MQTT optional name pair
 *
 * @param buf   The buffer to put the optional field
 * @oaram id    The ID of the optional field
 * @param ctx   The extension context
 * @param name  The name of the object
 * @param namelen The length of the name, or -1 to indicate strlen should be used
 * @param value The value of the optional field
 * @param len   The length of the value, or -1 to indicate strlen should be used
 * @return A return code, 0=good
 */
XAPI int ism_common_putMqttPropNamePair(concat_alloc_t * buf, int id, mqtt_prop_ctx_t * ctx,
        const char * name, int namelen, const char * value, int len) {
    mqtt_prop_field_t * fld;
    fld = id <= ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
    switch (fld->type) {
    default:
        return ISMRC_Error;
    case MPT_NamePair:
        if (len < 0)
            len = strlen(value);
        if (namelen< 0)
            namelen = strlen(name);
        bputchar(buf, id);
        bputchar(buf, namelen>>8);
        bputchar(buf, namelen);
        if (namelen)
            ism_common_allocBufferCopyLen(buf, name, namelen);
        bputchar(buf, len>>8);
        bputchar(buf, len);
        if (len)
            ism_common_allocBufferCopyLen(buf, value, len);
        break;
    }
    return 0;
}

/*
 * Get an MQTT variable size integer
 */
int ism_common_getMqttVarInt(concat_alloc_t * buf) {
    int  len;
    int  count = 1;
    int  buflen = buf->used-buf->pos;
    char * bp;
    int  multshift = 7;

    if (buflen < 1)
        return -1;
    bp = buf->buf+buf->pos;
    len =  (uint8_t)*bp++;
    /* Handle a multi-byte length */
    if (len & 0x80) {
        len &= 0x7f;
        do {
            count++;
            buflen--;
            if (count > 4 || buflen <= 0) {
                return -1;
            }
            len += (*bp++ & 0x7f) << multshift;
            multshift += 7;
        } while ((bp[-1]&0x80));
        /* Check that this is the shorteest encoding */
        if (bp[-1]==0)
            return -1;
    }
    buf->pos += count;
    return len;
}

/*
 * Get an MQTT variable size integer without alloc buffer
 */
int ism_common_getMqttVarIntExp(const char * buf, int buflen, int * used) {
    int  len;
    int  count = 1;
    const char * bp;
    int  multshift = 7;

    if (buflen < 1)
        return -1;
    bp = buf;
    len =  (uint8_t)*bp++;
    /* Handle a multi-byte length */
    if (len & 0x80) {
        len &= 0x7f;
        do {
            count++;
            buflen--;
            if (count > 4 || buflen <= 0) {
                return -1;
            }
            len += (*bp++ & 0x7f) << multshift;
            multshift += 7;
        } while ((bp[-1]&0x80));
        /* Check that this is the shorteest encoding */
        if (bp[-1]==0)
            return -1;
    }
    *used = count;
    return len;
}

/*
 * Put an MQTT variable length
 */
void ism_common_putMqttVarInt(concat_alloc_t * buf, int len) {
    if (UNLIKELY(len > 268435455 || len < 0)) {
        return;
    }
    if (len<=127) {
        bputchar(buf, len);
    } else if (len <= 16383) {
        bputchar(buf, (len&0x7f)|0x80);
        bputchar(buf, (char)(len>>7));
    } else if (len <= 2097151) {
        bputchar(buf, (char)((len&0x7f)|0x80));
        bputchar(buf, (char)((len>>7)|0x80));
        bputchar(buf, (char)(len>>14));
    } else {
        bputchar(buf, (char)((len&0x7f)|0x80));
        bputchar(buf, (char)((len>>7)|0x80));
        bputchar(buf, (char)((len>>14)|0x80));
        bputchar(buf, (char)(len>>21));
    }
}
