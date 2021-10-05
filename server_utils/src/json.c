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
#define TRACE_COMP Util
#include <ismutil.h>
#include <ismjson.h>
#include <imacontent.h>
#include <limits.h>

/*
 * Tokens returned by the JSON tokenizer
 */
enum json_token_e {
    JTOK_Error          = 1,
    JTOK_StartObject    = 2,
    JTOK_EndObject      = 3,
    JTOK_StartArray     = 4,
    JTOK_EndArray       = 5,
    JTOK_Colon          = 6,
    JTOK_Comma          = 7,
    JTOK_String         = 8,
    JTOK_Number         = 9,
    JTOK_Integer        = 10,
    JTOK_True           = 11,
    JTOK_False          = 12,
    JTOK_Null           = 13,
    JTOK_End            = 14,
};

/*
 * Parser states
 */
enum json_state_e {
    JSTATE_Name,      /* In object, looking for name */
    JSTATE_Value,     /* In object or array, looking for value */
    JSTATE_Comma,     /* In object or array, looking for separator or terminator */
    JSTATE_Done,      /* Done processing due to end of message or error */
};

/*
 * Forward references
 */
static int jsonNewEnt(ism_json_t * jobj, int objtype, const char * name, const char * value, int level);
static int jsonToken(ism_json_t * jobj, char * * data);
static int jsonKeyword(ism_json_t * jobj, int otype, const char * match, int len);
static int jsonString(ism_json_t * jobj);
static int jsonNumber(ism_json_t * jobj);
static int jsonExtra(const char * str);
static int jsonExtraLen(const char * str, int len);
static void jsonEscape(char * to, const char * from, int len);

/*
 * Check if this is JSON.
 *
 * This is not a complete parse, it is asking if the first non-whitespace
 * character is a JSON object or array starter.
 *
 * @param buf     The buffer to check for JSON
 * @param len     The length of the buffer
 * @param comment If non-zero, allow C style comments
 * @return 0=not JSON, 1=JSON
 */
int ism_json_isJSON(const char * buf, int len, int comment) {
    while (len-- > 0) {
        uint8_t ch = (uint8_t)*buf++;
        switch (ch) {
        case ' ':
        case '\t':
        case '\r':
        case 0x0b:
        case 0x0c:
        case '\n':
            break;

        case '{':
        case '[':
            return 1;

        case '/':
            if (comment) {
                if (len-- <= 0)
                    return 0;
                ch = *buf++;
                if (ch != '*' && ch != '/')
                    return 0;
                if (ch == '*') {       /* slash,star comment */
                    while (len-- > 0) {
                        ch = *buf++;
                        if (ch == '*') {
                            if (len-- <= 0)
                                return 0;
                            ch = *buf++;
                            if (ch == '/')
                                break;
                        }
                    }
                    if (len < 0)
                        return 0;
                } else {               /* slash,slash comment */
                    while (len-- > 0) {
                        ch = *buf++;
                        if (ch == '\r' || ch == '\n')
                            break;
                    }
                }
            } else {
                return 0;
            }
            break;
        default:
            return 0;
        }
    }
    return 0;
}


/*
 * Parse a JSON message.
 * The parse object can be used in only one thread at a time.
 */
int ism_json_parse(ism_json_t * jobj) {
    int    token;
    int    state;
    int    entnum;
    int    where [256];
    int    level = 0;
    int    inarray = 0;

    char * value = NULL;
    char * name;

    /*
     * The initial entry can be an object or an array
     */
    jobj->pos = jobj->source;
    jobj->left = jobj->src_len;
    jobj->line = 1;
    where[0] = 0;
    token = jsonToken(jobj, NULL);
    switch (token) {
    case JTOK_StartObject:  /* Outer object is an object */
        entnum = jsonNewEnt(jobj, JSON_Object, NULL, NULL, level);
        state = JSTATE_Name;
        break;
    case JTOK_StartArray:   /* Outer object is an array */
        entnum = jsonNewEnt(jobj, JSON_Array, NULL, NULL, level);
        state = JSTATE_Value;
        inarray = 1;
        name = NULL;
        break;
    case JTOK_End:           /* Object is empty */
        level = -1;
        state = JSTATE_Done;
        break;
    default:
        /* TODO: Return should indicate invalid entry.
         * And jobj->rc should probably also indicate data is
         * not valid for checks in other places.
         *
         * Forcicing rc = 2 (when rc is not yet set) to indicate
         * JSON message is not valid.
         *
         */
        if (!jobj->rc)
            jobj->rc = 2;
        return jobj->rc;
    }

    /*
     * Loop thru all content
     */
    while (state != JSTATE_Done) {
        switch (state) {

        /*
         * We are expecting the name within an object
         */
        case JSTATE_Name:
            token = jsonToken(jobj, &name);
            switch (token) {
            /*
             * This code is actually a bit broken.  We need this code to handle an empty object,
             * but it also makes a trailing comma on the last item in the object.  This is a
             * common error made when creating JSON manually.  For now I will leave it and say
             * we are being non-strict in our parsing of JSON.
             */
            case JTOK_EndObject:
                 if (inarray) {
                    state = JSTATE_Done;
                    break;
                }
                jobj->ent[where[level]].count = jobj->ent_count-where[level]-1;
                if (--level >= 0) {
                    inarray = jobj->ent[where[level]].objtype == JSON_Array;
					state = JSTATE_Comma;
                } else {
                    state = JSTATE_Done;
                }
                break;

            case JTOK_String:
                token = jsonToken(jobj, NULL);
                if (token != JTOK_Colon) {
                    jobj->rc = 2;
                    state = JSTATE_Done;
                    break;
                }
                state = JSTATE_Value;
                break;
            default:
                state = JSTATE_Done;
                break;

            }
            break;

        /*
         * We are expecting a value
         */
        case JSTATE_Value:
            token = jsonToken(jobj, &value);
            switch (token) {
            case JTOK_String:
                entnum = jsonNewEnt(jobj, JSON_String, name, value, level);
                name = NULL;
                state = JSTATE_Comma;
                break;
            case JTOK_Integer:
                entnum = jsonNewEnt(jobj, JSON_Integer, name, value, level);
                if (strlen(value)<12) {
                    int64_t lval = strtoll(value, NULL, 10);
                    if (lval > (int64_t)INT_MAX || lval < (int64_t)INT_MIN)
                        jobj->ent[entnum].objtype = JSON_Number;
                    else
                        jobj->ent[entnum].count = (int32_t)lval;
                } else {
                    jobj->ent[entnum].objtype = JSON_Number;
                }
                name = NULL;
                state = JSTATE_Comma;
                break;
            case JTOK_Number:
                entnum = jsonNewEnt(jobj, JSON_Number, name, value, level);
                name = NULL;
                state = JSTATE_Comma;
                break;
            case JTOK_StartObject:
                entnum = jsonNewEnt(jobj, JSON_Object, name, value, level);
                where[++level] = entnum;
                inarray = 0;
                state = JSTATE_Name;
                break;
            case JTOK_StartArray:
                entnum = jsonNewEnt(jobj, JSON_Array, name, NULL, level);
                where[++level] = entnum;
                state = JSTATE_Value;
                name = NULL;
                inarray = 1;
                break;
            case JTOK_True:
                entnum = jsonNewEnt(jobj, JSON_True, name, NULL, level);
                name = NULL;
                jobj->ent[entnum].count = 1;
                state = JSTATE_Comma;
                break;
            case JTOK_False:
                entnum = jsonNewEnt(jobj, JSON_False, name, NULL, level);
                name = NULL;
                state = JSTATE_Comma;
                break;
            case JTOK_Null:
                entnum = jsonNewEnt(jobj, JSON_Null, name, NULL, level);
                name = NULL;
                state = JSTATE_Comma;
                break;
            case JTOK_EndArray:
                if (!inarray) {
                    state = JSTATE_Done;
                    break;
                }
                jobj->ent[where[level]].count = jobj->ent_count-where[level]-1;
                if (--level >= 0) {
                    inarray = jobj->ent[where[level]].objtype == JSON_Array;
					state = JSTATE_Comma;
                } else {
                    state = JSTATE_Done;
                }
                break;
            default:
                state = JSTATE_Done;
                break;
            }
            break;

        /*
         * Comma or end of object
         */
        case JSTATE_Comma:
            token = jsonToken(jobj, NULL);
            switch (token) {
            /* Comma separator between objects */
            case JTOK_Comma:
                state = inarray ? JSTATE_Value : JSTATE_Name;
                break;

            /* Right brace ends an object */
            case JTOK_EndObject:
                if (inarray) {
                    state = JSTATE_Done;
                    break;
                }
                jobj->ent[where[level]].count = jobj->ent_count-where[level]-1;
                if (--level >= 0) {
                    inarray = jobj->ent[where[level]].objtype == JSON_Array;
                    state = JSTATE_Comma;
                } else {
                    state = JSTATE_Done;
                }
                break;

            /* Right bracket ends and array */
            case JTOK_EndArray:
                if (!inarray) {
                    state = JSTATE_Done;
                    break;
                }
                jobj->ent[where[level]].count = jobj->ent_count-where[level]-1;
                if (--level >= 0) {
                    inarray = jobj->ent[where[level]].objtype == JSON_Array;
                    state = JSTATE_Comma;
                } else {
                    state = JSTATE_Done;
                }
                break;

            /* End found.  This is good if level is -1 */
            case JTOK_End:
                state = JSTATE_Done;
                break;

            /* Error */
            default:
                state = JSTATE_Done;
                break;
            }
            break;
        }
    }

    /*
     * Check all objects are complete
     */
    if (level >= 0) {
        if (!jobj->rc) {
            jobj->rc = 1;
            TRACE(7, "Unexpected end of JSON message\n");
        }
    }
    return jobj->rc;
}


/*
 * Make a new entry
 */
static int jsonNewEnt(ism_json_t * jobj, int objtype, const char * name, const char * value, int level) {
    int entnum;
    ism_json_entry_t * ent;

    if (jobj->ent_count >= jobj->ent_alloc) {
        int newalloc;
        if (jobj->ent_alloc < 25)
            newalloc = 100;
        else
            newalloc = jobj->ent_alloc*4;
        if (jobj->free_ent) {
            jobj->ent = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_parser,1),jobj->ent, newalloc*sizeof(ism_json_entry_t));
        } else {
            ism_json_entry_t * tmpbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_parser,2),newalloc * sizeof(ism_json_entry_t));
            if (jobj->ent_count)
                memcpy(tmpbuf, jobj->ent, jobj->ent_count * sizeof(ism_json_entry_t));
            jobj->ent = tmpbuf;
            jobj->free_ent = 1;
        }
        jobj->ent_alloc = newalloc;
    }
    entnum = jobj->ent_count++;
    ent = jobj->ent + entnum;
    ent->objtype = objtype;
    ent->name    = name;
    ent->value   = value;
    ent->count   = 0;
    ent->level   = level;
    ent->line    = jobj->line;
    return entnum;
}


/*
 * Return the next token.
 * This also implements the optional C or Java style comment which acts like whitespace.
 */
static int jsonToken(ism_json_t * jobj, char * * data) {
    while (jobj->left-- > 0) {
        uint8_t ch = *jobj->pos++;
        switch (ch) {
        case ' ':
        case '\t':
        case '\r':
        case 0x0b:
        case 0x0c:
            break;
        case '\n':
            jobj->line++;
            break;
        case '{':
            return JTOK_StartObject;
        case '}':
            return JTOK_EndObject;
        case '[':
            return JTOK_StartArray;
        case ']':
            return JTOK_EndArray;
        case ':':
            return JTOK_Colon;
        case ',':
            return JTOK_Comma;
        case '"':
            if (data)
                *data = jobj->pos;
            return jsonString(jobj);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (data) {
                *data = jobj->pos-2;
                return jsonNumber(jobj);
            }
            return JTOK_Error;
        case 't':
            return jsonKeyword(jobj, JTOK_True, "true", 3);
        case 'f':
            return jsonKeyword(jobj, JTOK_False, "false", 4);
        case 'n':
            return jsonKeyword(jobj, JTOK_Null, "null", 3);
        case '/':
            if (jobj->options&JSON_OPTION_COMMENT) {
                if (jobj->left-- <= 0)
                    return JTOK_Error;
                ch = *jobj->pos++;
                if (ch != '*' && ch != '/')
                    return JTOK_Error;

                if (ch == '*') {       /* slash,star comment */
                    while (jobj->left-- > 0) {
                        ch = *jobj->pos++;
                        if (ch == '*') {
                            if (jobj->left-- <= 0)
                                return JTOK_Error;
                            ch = *jobj->pos++;
                            if (ch == '/')
                                break;
                        }
                        if (ch == '\n')
                            jobj->line++;
                    }
                    if (jobj->left < 0)
                        return JTOK_Error; /* Error if not terminated */
                } else {               /* slash,slash comment */
                    while (jobj->left-- > 0) {
                        ch = *jobj->pos++;
                        if (ch == '\n')
                            jobj->line++;
                        if (ch == '\r' || ch == '\n')
                            break;
                    }
                }
            } else {
                return JTOK_Error;     /* Comment not allowed */
            }
            break;
        default:
            return JTOK_Error;
        }
    }
    return JTOK_End;
}


/*
 * Match a keyword
 */
static int jsonKeyword(ism_json_t * jobj, int otype, const char * match, int len) {
    if (jobj->left >= len) {
        if (!memcmp(jobj->pos, match+1, len)) {
            jobj->pos += len;
            jobj->left -= len;
            return otype;
        }
    }
    return JTOK_Error;
}


/*
 * Return the value of a hex digit or -1 if not a valid hex digit
 */
static int hexValue(char ch) {
    if (ch <= '9' && ch >= '0')
        return ch-'0';
    if (ch >='A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >='a' && ch <= 'f')
        return ch-'a'+10;
    return -1;
}


/*
 * Match a string
 * A string is any number of bytes ending with an unescaped double quote.
 * We unescape the string in place.
 */
static int jsonString(ism_json_t * jobj) {
    char * ip = jobj->pos;
    char * op = ip;
    int    left = jobj->left;
    int    i;
    int    val;
    int    digit;
    int    needcheck = 0;

    while (left > 0) {
        char ch = *ip++;
        if (ch == '"') {
            *op = 0;
            /*
             * If we have any suspicious characters, check for valid UTF-8.
             */
            if (needcheck) {
                if (ism_common_validUTF8(jobj->pos, op-jobj->pos) < 0) {
                    jobj->rc = 2;
                    return JTOK_Error;
                }
            }
            jobj->pos = ip;
            jobj->left = left-1;
            return JTOK_String;
        } else if (ch=='\\') {
            if (left < 1)
                return JTOK_Error;
            ch = *ip++;
            left--;
            switch (ch) {
            case 'b':   ch = 0x08;   break;
            case 'f':   ch = 0x0c;   break;
            case 'n':   ch = '\n';   break;
            case 'r':   ch = '\r';   break;
            case 't':   ch = '\t';   break;

            case 'u':
                if (left < 4)
                    return JTOK_Error;
                /* Parse the hex value.  This must be four hex digits */
                val = 0;
                for (i=0; i<4; i++) {
                    digit = hexValue(*ip++);
                    if (digit < 0)
                        return JTOK_Error;
                    val = val<<4 | digit;
                }
                left -= 4;
                /* Do the UTF-8 expansion */
                if (val <= 0x7f) {
                    ch    = (char)val;
                } else if (val > 0x7ff) {
                    *op++ = (char)(0xe0 | ((val>>12) & 0x0f));
                    *op++ = (char)(0x80 | ((val>>6)  & 0x3f));
                    ch    = (char)(0x80 | (val & 0x3f));
                } else {
                    *op++ = (char)(0xc0 | ((val>>6) &0x1f));
                    ch    = (char)(0x80 | (val & 0x3f));
                }
                break;

            case '/':
            case '"':
            case '\\':
                break;
            default:
                return JTOK_Error;
            }
			*op++ = ch;
        } else{
            *op++ = ch;
            if ((signed char)ch < 0x20) {   /* C0 or > 0x7F */
                if ((uint8_t)ch < 0x20) {
                    jobj->rc = 2;
                    return JTOK_Error;      /* Control characters are not allowed in a string except when escaped */
                } else {
                    needcheck = 1;
                }
            }
        }
        left--;
    }
    return JTOK_Error;
}


/*
 * Match a number.
 *
 * The JSON number production is quite specific and a subset of what is allowed in C or Java.
 * We check for any violations of the production. and give an error, even though in many casses
 * the correct  * JSON behavior would be to end the production at the point of the error.
 * This would just cause the following separator we be in error and give a less usable error code.
 *
 * Numbers are commonly not terminated before the separator, so we do not have a location to put
 * the null character string terminator.  However, there is always at least one unused byte before
 * the number in the buffer.  Therefore we move the bytes of the number one byte to the left so we
 * have a place to put the terminator.
 */
static int jsonNumber(ism_json_t * jobj) {
    char * dp = jobj->pos-2;
    int    left = jobj->left+1;
    int    state   = dp[1]=='-' ? 0 : 1;
    int    waszero = 0;
    int    digits  = 0;

    while (left > 0) {
        char ch = dp[1];
        switch (ch) {
        case '0':
            if (state==1 && waszero)
               return JTOK_Error;
            waszero = 1;
            if (state == 4)
                state = 5;
            digits++;
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (state == 1)
                state = 2;
            if (state == 4)
                state = 5;
            waszero = 0;
            digits++;
            break;

        case '.':
            if (state != 1 && state != 2)
                return JTOK_Error;
            state = 3;
            digits = 0;
            break;

        case '-':
            if (state != 0 && state != 4)
                return JTOK_Error;
            state++;
            break;

        case 'E':
        case 'e':
            if (state==0 || state>3)
                return JTOK_Error;
            state = 4;
            digits = 0;
            break;

        case '+':
            if (state != 4)
                return JTOK_Error;
            state = 5;
            digits = 0;
            break;
        default:
            *dp++ = 0;
            if ((state==3 && digits == 0) || (state==5 && digits == 0))
                return JTOK_Error;
            jobj->pos = dp;
            jobj->left = left;
            return state > 2 ? JTOK_Number : JTOK_Integer;
        }
        *dp++ = ch;
        left--;
    }
    *dp++ = 0;
    jobj->pos = dp;
    jobj->left = 0;
    return JTOK_Error;
}

/*
 * Check if a string is a valid JSON number.
 *
 * JSON allows a subset of the valid number strings used by C or Java
 * @param str   The input string
 * @return A return value:  0=invalid, 1=integer, 2=number
 */
int ism_json_isValidNumber(const char * str) {
    const char * dp = str;
    int    state   = *dp=='-' ? 0 : 1;
    int    waszero = 0;
    int    digits  = 0;

    while (*dp) {
        char ch = *dp;
        switch (ch) {
        case '0':
            if (state==1 && waszero)
               return 0;
            waszero = 1;
            if (state == 4)
                state = 5;
            digits++;
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (state == 1)
                state = 2;
            if (state == 4)
                state = 5;
            waszero = 0;
            digits++;
            break;

        case '.':
            if (state != 1 && state != 2)
                return 0;
            state = 3;
            digits = 0;
            break;

        case '-':
            if (state != 0 && state != 4)
                return 0;
            state++;
            break;

        case 'E':
        case 'e':
            if (state==0 || state>3)
                return 0;
            state = 4;
            digits = 0;
            break;

        case '+':
            if (state != 4)
                return 0;
            state = 5;
            digits = 0;
            break;
        default:
            return 0;
        }
        dp++;
    }
    if ((state==3 && digits == 0) || (state==5 && digits == 0))
        return 0;
    if (state > 2)
        return 2;
    int64_t val = strtoll(str, NULL, 10);
    if (val <= INT_MAX && val >= INT_MIN)
        return 1;
    return 2;
}

/*
 * Check if a string is a valid JSON string
 *
 * A valid JSON string must be valid UTF-8
 * @param str   The input string
 * @return A return value:  0=invalid, 1=valid
 */
int ism_json_isValidString(const char * str) {
    if (!str)
        return 1;
    return ism_common_validUTF8(str, strlen(str)) >= 0;
}

/*
 * Encode a field in JSON.
 *
 * @param buf   The buffer to write to
 * @param name  The name of the field.  If this is NULL, only the value is written
 * @param value The value of the field
 * @param notfirst  If this is non-zero, add a comma before the field
 */
void ism_json_put(concat_alloc_t * buf, const char * name, ism_field_t * value, int notfirst) {
    char  ibuf[400];
    int   compact = buf->compact&0xF;
    int   lf = buf->compact&0x10 && notfirst == 1;

    if (notfirst) {
        if (lf)
            ism_json_putBytes(buf, ",\n");
        else
            ism_json_putBytes(buf, compact>1 ? "," : ", ");
    }
    if (buf->compact&0x20 && notfirst <= 1)
        ism_json_putBytes(buf, "    ");
    if (buf->compact&0x40 && notfirst <= 1)
        ism_json_putBytes(buf, "    ");
    if (buf->compact&0x80 && notfirst <= 1)
        ism_json_putBytes(buf, "    ");
    if (name) {
        ism_json_putString(buf, name);
        ism_json_putBytes(buf, compact ? ":" : ": ");
    }
    if (!value) {
        ism_json_putBytes(buf, "null");
    } else {
        switch (value->type) {
        default:
        case VT_Null:         /* Null value                    */
            ism_json_putBytes(buf, "null");
            break;

        case VT_String:       /* Type of String                */
            ism_json_putString(buf, value->val.s);
            break;

        case VT_ByteArray:    /* Type of Byte Array            */
            ism_json_putBytes(buf, "\"");    /* starting quote */
            ism_json_putEscapeBytes(buf, value->val.s, value->len);
            ism_json_putBytes(buf, "\"");    /* ending quote   */
            break;

        case VT_Boolean:      /* Type of boolean               */
            ism_json_putBytes(buf, value->val.i ? "true" : "false");
            break;

        case VT_Byte:         /* Signed 8 bit value            */
            ism_json_putInteger(buf, (int)(int8_t)value->val.i);
            break;

        case VT_UByte:        /* Unsigned 8 bit value          */
            ism_json_putInteger(buf, (int)(uint8_t)value->val.i);
            break;

        case VT_Short:        /* Signed 16 bit value           */
            ism_json_putInteger(buf, (int)(int16_t)value->val.i);
            break;

        case VT_UShort:       /* Unsigned 16 bit value         */
            ism_json_putInteger(buf, (int)(uint16_t)value->val.i);
            break;

        case VT_Integer:      /* Type of Integer               */
            ism_json_putInteger(buf, value->val.i);
            break;

        case VT_UInt:         /* Type of unsigned integer      */
            sprintf(ibuf, "%u", (uint32_t)value->val.i);
            ism_json_putBytes(buf, ibuf);
            break;

        case VT_Long:         /* Type of Long Integer          */
            sprintf(ibuf, "%lld", (long long)value->val.l);
            ism_json_putBytes(buf, ibuf);
            break;

        case VT_ULong:        /* Type of Long Integer          */
            sprintf(ibuf, "%llu", (unsigned long long)value->val.l);
            ism_json_putBytes(buf, ibuf);
            break;

        case VT_Float:        /* Type of Float                 */
            sprintf(ibuf, "%f", value->val.f);
            ism_json_putBytes(buf, ibuf);
            break;

        case VT_Double:       /* Type of Double                */
            sprintf(ibuf, "%f", value->val.d);
            ism_json_putBytes(buf, ibuf);
            break;
        }
    }
}


/*
 * Compute the extra length needed for JSON escapes of a string.
 * Control characters and the quote and backslash are escaped.
 * No mulitbyte characters are escaped.
 */
static int jsonExtra(const char * str) {
    int extra = 0;
    while (*str) {
        uint8_t ch = (uint8_t)*str++;
        if (ch >= ' ') {                 /* Normal characters */
            if (ch=='"' || ch=='\\')
                extra++;
        } else {
            switch (ch) {                /* Control characters */
            case '\n':
            case '\r':
            case 0x08:    /* BS */
            case 0x09:    /* HT */
            case 0x0c:    /* FF */
                extra++;
                break;
            default:
                extra += 5;              /* unicode escape */
                break;
            }
        }
    }
    return extra;
}


/*
 * Compute the extra length needed for JSON escapes of a byte array.
 * This is the same as jsonExtra, but takes a length instead of a null terminator.
 */
static int jsonExtraLen(const char * str, int len) {
    int  extra = 0;
    int  i;
    for (i=0; i<len; i++) {
        uint8_t ch = (uint8_t)*str++;
        if (ch >= ' ') {                /* Normal character */
            if (ch=='"' || ch=='\\')
                extra++;
        } else {                        /* Control characters */
            switch (ch) {
            case '\n':
            case '\r':
            case 0x08:    /* BS */
            case 0x09:    /* HT */
            case 0x0c:    /* FF */
                extra++;
                break;
            default:
                extra += 5;             /* Unicode escape */
                break;
            }
        }
    }
    return extra;
}

static char * hexChar = "0123456789ABCDEF";

/*
 * Copy a byte array with JSON escapes.
 * Escape the doublequote, backslash, and all control characters.
 * Do not escape any multibyte character.
 */
static void jsonEscape(char * to, const char * from, int len) {
    int i;
    for (i=0; i<len; i++) {
        uint8_t ch = (uint8_t)*from++;
        if (ch >= ' ') {
            if (ch=='"' || ch=='\\')
                *to++ = '\\';
            *to++ = ch;
        } else {
            *to++ = '\\';
            switch (ch) {
            case '\n':  *to++ = 'n';    break;
            case '\r':  *to++ = 'r';    break;
            case 0x08:  *to++ = 'b';    break;
            case 0x09:  *to++ = 't';    break;
            case 0x0c:  *to++ = 'f';    break;
            default:
                *to++ = 'u';
                *to++ = '0';
                *to++ = '0';
                *to++ = hexChar[(ch>>4)&0xf];
                *to++ = hexChar[ch&0xf];
                break;
            }
        }
    }
}


/*
 * Encode a string in JSON with escaping.
 *
 * @param buf    The buffer to write to
 * @param str    The string to encode
 */
void ism_json_putString(concat_alloc_t * buf, const char * str) {
    if (!str) {
        ism_common_allocBufferCopy(buf, "null");
    } else {
        int extra = jsonExtra(str);
        ism_json_putBytes(buf, "\"");    /* starting quote */
        if (extra == 0) {
            ism_common_allocBufferCopyLen(buf, str, (int)strlen(str));
        } else {
            int len = (int)strlen(str);
            char * tp = ism_common_allocAllocBuffer(buf, len+extra, 0);
            jsonEscape(tp, str, len);
        }
        ism_json_putBytes(buf, "\"");    /* ending quote   */
    }
}


/*
 * Write out a set of bytes in JSON
 *
 * These bytes are directly written to the JSON stream and are not interpreted in any way.
 * @param buf   The buffer to write to
 * @param str   The string of bytes to write
 */
void ism_json_putBytes(concat_alloc_t * buf, const char * str) {
    ism_common_allocBufferCopyLen(buf, str, (int)strlen(str));
}

/*
 * Ensure the buffer is null terminated
 */
void ism_json_putNull(concat_alloc_t * buf) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used] = 0;
    } else {
        ism_common_allocBufferCopyLen(buf, "", 1);
        buf->used--;
    }
}


/*
 * Put out a set of bytes with JSON string escapes
 * @param buf   The buffer to write to
 * @param str   The address of the bytes to write
 * @param len   The length of the bytes to write
 */
void ism_json_putEscapeBytes(concat_alloc_t * buf, const char * str, int len) {
    int extra = jsonExtraLen(str, len);
    if (extra == 0) {
        ism_common_allocBufferCopyLen(buf, str, len);
    } else {
        char * tp = ism_common_allocAllocBuffer(buf, len+extra, 0);
        jsonEscape(tp, str, len);
    }
}


/*
 * Encode an integer in JSON.
 * @param buf    The buffer to write to
 * @param value  The value of the integer to write.
 */
void ism_json_putInteger(concat_alloc_t * buf, int value) {
    char ibuf[16];
    sprintf(ibuf, "%d", value);
    ism_json_putBytes(buf, ibuf);
}


/*
 * Get a field from a JSON object
 */
int ism_json_get(ism_json_t * jobj, int entnum, const char * name) {
    int maxent;

    if (entnum < 0 || entnum >= jobj->ent_count || jobj->ent[entnum].objtype != JSON_Object) {
        return -1;
    }
    /* Allow the entry to be directly sent */
    if ((uintptr_t)name < jobj->ent_count) {
    	return (int)(uintptr_t)name;
    }
    maxent = entnum + jobj->ent[entnum].count;
    entnum++;
    while (entnum <= maxent) {
        ism_json_entry_t * ent = jobj->ent+entnum;
        if (!strcmp(name, ent->name)) {
            return entnum;
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array) {
            entnum += ent->count+1;
        } else {
            entnum++;
        }
    }
    return -1;
}


/*
 * Get a string from a JSON object
 */
const char * ism_json_getString(ism_json_t * jobj, const char * name) {
    ism_json_entry_t * ent;
    int    entnum;

    entnum = ism_json_get(jobj, 0, name);
    if (entnum < 0)
        return NULL;
    ent = jobj->ent+entnum;
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
 * Get an integer from a JSON object
 */
int ism_json_getInt(ism_json_t * jobj, const char * name, int deflt) {
    ism_json_entry_t * ent;
    int    entnum;
    int    val;
    char * eos;

    entnum = ism_json_get(jobj, 0, name);
    if (entnum < 0)
        return deflt;
    ent = jobj->ent+entnum;
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

/*
 * Get an integer from a JSON object.
 * Only JSON numbers with an integer value are allowed.  However it is allowed
 * to have a decimal component which is zero.
 */
int ism_json_getInteger(ism_json_t * jobj, const char * name, int deflt) {
    ism_json_entry_t * ent;
    int    entnum;
    int    val;
    double dval;

    entnum = ism_json_get(jobj, 0, name);
    if (entnum < 0)
        return deflt;
    ent = jobj->ent+entnum;
    switch (ent->objtype) {
    	case JSON_Integer:
    		return ent->count;
    	case JSON_Number:
    	    dval = strtod(ent->value, NULL);
    	    val = (int)dval;
    	    if (dval == (double)val)
    	        return val;
    	    else
    	        return deflt;
    	default:
    		return deflt;
    }
}

/*
 * Get a JSON number as a double
 */
double ism_json_getNumber(ism_json_t * jobj, const char * name, double deflt) {
    ism_json_entry_t * ent;
    int    entnum;

    entnum = ism_json_get(jobj, 0, name);
    if (entnum < 0)
        return deflt;
    ent = jobj->ent+entnum;
    switch (ent->objtype) {
        case JSON_Integer:
            return (double)ent->count;
        case JSON_Number:
            return strtod(ent->value, NULL);
        default:
            return deflt;
    }
}

/*
 * Return a string from a JSON entry even for things without a value.
 */
const char * ism_json_getJsonValue(ism_json_entry_t * ent) {
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
 * Copy strings into json object storage
 */
const char * ism_json_const(ism_json_t * jobj, const char * name) {
    /* TODO: fix this */
    return name;
}

void ism_json_closeWriter(ism_json_t * jobj) {
	if( jobj->free_parseobj)
	{
		ism_common_free(ism_memory_utils_misc, jobj);
	}
}

/*
 *
 */
ism_json_t * ism_json_newWriter(ism_json_t * jobj, concat_alloc_t * buf, int indent, int options) {
    if (!jobj) {
        jobj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,174),1, sizeof(ism_json_t));
        jobj->free_parseobj = 1;
    }
    jobj->buf = buf;
    jobj->indent = 0;
    if (indent > 0 && indent <= 8)
        jobj->indent = indent;
    jobj->compress = options;
    jobj->extra_indent = (uint8_t)((options>>8) & 7);
    if (options & JSON_OUT_TABS)
        jobj->indent = 4;         /* Just to force it non-zero */
    jobj->level = 0;
    jobj->first = 1;
    jobj->firstline = 1;
    return jobj;
}

/*
 * Check if we should do indent
 * We indent if the indent size is >= 0 and we are in an object but not in an uncompressed simple array
 */
static int shouldIndent(ism_json_t * jobj) {
    if (jobj->indent <= 0)
        return 0;
    if (jobj->firstline)
        return 1;
    if (jobj->compress & JSON_OUT_ARRAY) {
        if (jobj->simplearray)
            return 0;
    }
    return 1;
}

/*
 * Handle the comma separator, indentation, and putting out the name of an item.
 * The indent can be 0 to 8 bytes where zero also implies no line end.
 */
void ism_json_putIndent(ism_json_t * jobj, int comma, const char * name) {
    if (comma) {
        if (jobj->first) {
            jobj->first = 0;
        } else {
            ism_json_putBytes(jobj->buf, ",");
            if ((jobj->indent==0 || jobj->simplearray) && !(jobj->compress&JSON_OUT_COMPACT))
                ism_json_putBytes(jobj->buf, " ");
        }
    }
    if (shouldIndent(jobj)) {
        if (jobj->firstline)
            jobj->firstline = 0;
        else
            ism_json_putBytes(jobj->buf, "\n");
        if (jobj->level) {
            if (jobj->indent > 8)
                jobj->indent = 8;
            int len = jobj->level + jobj->extra_indent;
            if (jobj->compress & JSON_OUT_TABS) {
                ism_protocol_ensureBuffer(jobj->buf, len);
                memset(jobj->buf->buf+jobj->buf->used, '\t', len);
                jobj->buf->used += len;
            } else {
                len *= jobj->indent;
                ism_protocol_ensureBuffer(jobj->buf, len);
                memset(jobj->buf->buf+jobj->buf->used, ' ', len);
                jobj->buf->used += len;
            }
        }
    }
    if (name) {
        ism_json_putString(jobj->buf, name);
        ism_json_putBytes(jobj->buf, (jobj->compress&JSON_OUT_COMPACT) ? ":" : ": ");
    }
}


/*
 * Start an array
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param constname  True if the name is in a location which will not change over the life of the json object
 *
 */
void ism_json_startArray(ism_json_t * jobj, const char * name) {
    if (jobj->buf) {
        ism_json_putIndent(jobj, 1, name);
        ism_json_putBytes(jobj->buf, "[");
        jobj->first = 1;
        jobj->simplearray = 1;
    } else {
        name = ism_json_const(jobj, name);
        jsonNewEnt(jobj, JSON_Object, name, NULL, jobj->level);
    }
    jobj->level++;
}

/*
 * Start an object
 * @param jobj   The JSON object
 * @param name   The name of the item
 *
 */
void ism_json_startObject(ism_json_t * jobj, const char * name) {
    if (jobj->buf) {
        ism_json_putIndent(jobj, 1, name);
        ism_json_putBytes(jobj->buf, "{");
        jobj->first = 1;
        jobj->simplearray = 0;
    } else {
        jsonNewEnt(jobj, JSON_Object, name, NULL, jobj->level);
    }
    jobj->level++;
}

/*
 * Put a number item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The string form of the number.  This is not checked for validity.
 */
void ism_json_putNumberItem(ism_json_t * jobj, const char * name, const char * value) {
    if (jobj->buf) {
        ism_json_putIndent(jobj, 1, name);
        ism_json_putBytes(jobj->buf, value);
    } else {
        name = ism_json_const(jobj, name);
        value = ism_json_const(jobj, value);
        jsonNewEnt(jobj, JSON_Number, name, value, jobj->level);
    }
}

/*
 * Put a boolean item which is true, false, or null
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  true=>0, false=0, null=<0
 */
void ism_json_putBooleanItem(ism_json_t * jobj, const char * name, int value) {
    if (jobj->buf) {
        const char * svalue = value > 0 ? "true" : (value == 0) ? "false" : "null";
        ism_json_putIndent(jobj, 1, name);
        ism_json_putBytes(jobj->buf, svalue);
    } else {
        name = ism_json_const(jobj, name);
        if (value > 0) {
            int entnum = jsonNewEnt(jobj, JSON_True, name, NULL, jobj->level);
            jobj->ent[entnum].count = 1;
        } else {
            if (value == 0)
                jsonNewEnt(jobj, JSON_False, name, NULL, jobj->level);
            else
                jsonNewEnt(jobj, JSON_Null, name, NULL, jobj->level);
        }
    }
}

/*
 * Put an integer item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  true=>0, false=0, null=<0
 */
void ism_json_putIntegerItem(ism_json_t * jobj, const char * name, int value) {
    char svalue [20];
    sprintf(svalue, "%d", value);
    if (jobj->buf) {
        ism_json_putIndent(jobj, 1, name);
        ism_json_putBytes(jobj->buf, svalue);
    } else {
        name = ism_json_const(jobj, name);
        int where = jsonNewEnt(jobj, JSON_Integer, name, ism_json_const(jobj, svalue), jobj->level);
        jobj->ent[where].count = value;
    }
}

/*
 * Put an long item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  true=>0, false=0, null=<0
 */
void ism_json_putLongItem(ism_json_t * jobj, const char * name, int64_t value) {
    char svalue [20];
    if (value <= INT_MAX && value >= INT_MIN) {
        ism_json_putIntegerItem(jobj, name, (int)value);
    } else {
        sprintf(svalue, "%ld", value);
        if (jobj->buf) {
            ism_json_putIndent(jobj, 1, name);
            ism_json_putBytes(jobj->buf, svalue);
        } else {
            name = ism_json_const(jobj, name);
            jsonNewEnt(jobj, JSON_Number, name, ism_json_const(jobj, svalue), jobj->level);
        }
    }
}

/*
 * Put a ulong item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The value
 */
void ism_json_putULongItem(ism_json_t * jobj, const char * name, uint64_t value) {
    char svalue [22];
    if (value <= INT_MAX) {
        ism_json_putIntegerItem(jobj, name, (int)value);
    } else {
        sprintf(svalue, "%lu", value);
        if (jobj->buf) {
            ism_json_putIndent(jobj, 1, name);
            ism_json_putBytes(jobj->buf, svalue);
        } else {
            name = ism_json_const(jobj, name);
            jsonNewEnt(jobj, JSON_Number, name, ism_json_const(jobj, svalue), jobj->level);
        }
    }
}

/*
 * Put a string item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  true=>0, false=0, null=<0
 */
void ism_json_putStringItem(ism_json_t * jobj, const char * name, const char * value) {
    if (jobj->buf) {
        ism_json_putIndent(jobj, 1, name);
        ism_json_putString(jobj->buf, value);
    } else {
        name = ism_json_const(jobj, name);
        value = ism_json_const(jobj, value);
        jsonNewEnt(jobj, JSON_String, name, value, jobj->level);
    }
}


/*
 * End the current object
 * @param The JSON object
 * @return A return code 0=good
 */
int ism_json_endObject(ism_json_t * jobj) {
    if (jobj->level <= 0) {
        return ISMRC_Error;
    } else {
        jobj->level--;
        if (jobj->buf) {
            ism_json_putIndent(jobj, 0, NULL);
            ism_json_putBytes(jobj->buf, "}");
            ism_json_putNull(jobj->buf);
            jobj->first = 0;
            jobj->simplearray = 0;
        } else {
            /* TODO */
        }
    }
    return 0;
}

/*
 * End the current array
 * @param The JSON object
 * @return A return code 0=good
 */
int ism_json_endArray(ism_json_t * jobj) {
    if (jobj->level <= 0) {
        return ISMRC_Error;
    } else {
        jobj->level--;
        if (jobj->buf) {
            ism_json_putIndent(jobj, 0, NULL);
            ism_json_putBytes(jobj->buf, "]");
            ism_json_putNull(jobj->buf);
            jobj->first = 0;
            jobj->simplearray = 0;
        } else {
            /* TODO */
        }
    }
    return 0;
}



/*
 * Write a JSON parsed object to a buffer.
 * This uses the specified JSON object as a writer object and scans all entries.
 * This is normally used with entnum=0 to write the entire JSON object, but can be used
 * to output only a single object or array.
 */
int ism_json_toString(ism_json_t * jobj, concat_alloc_t * buf, int entnum, int indent, int options) {
    ism_json_entry_t * ent;
    int maxent;
    int level = 0;
    int startlevel = 0;
    int starttype;
    uint8_t levels [256];

    /* Null terminate the buffer in case we throw an error */
    ism_common_allocBufferCopyLen(buf, "", 1);
    buf->used--;

    if (jobj->rc)
        return jobj->rc;
    if (entnum < 0 || entnum >= jobj->ent_count)
        return ISMRC_Error;
    ent = jobj->ent + entnum;
    if (ent->objtype != JSON_Object && ent->objtype != JSON_Array)
        return ISMRC_Error;

    jobj->buf = buf;
    jobj->indent = (uint8_t)indent;
    jobj->compress = (uint8_t)options;
    jobj->extra_indent = (uint8_t)((options >> 8) & 7);
    startlevel = ent->level;
    starttype = ent->objtype;
    maxent = entnum + ent->count;
    jobj->first = 1;
    jobj->firstline = 1;

    for (; entnum <= maxent; entnum++) {
        ent = jobj->ent + entnum;
        while (ent->level < level) {
            if (levels[--level] == JSON_Array)
                ism_json_endArray(jobj);
            else
                ism_json_endObject(jobj);
        }
        level = ent->level;
        switch (ent->objtype) {
        case JSON_Object:
            ism_json_startObject(jobj, ent->name);
            levels[ent->level] = ent->objtype;
            break;
        case JSON_Array:
            ism_json_startArray(jobj, ent->name);
            levels[ent->level] = ent->objtype;
            break;
        case JSON_String:
            ism_json_putStringItem(jobj, ent->name, ent->value);
            break;
        case JSON_Integer:
            ism_json_putIntegerItem(jobj, ent->name, ent->count);
            break;
        case JSON_Number:
            ism_json_putNumberItem(jobj, ent->name, ent->value);
            break;
        default:
            ism_json_putBooleanItem(jobj, ent->name, ent->objtype==JSON_True ? 1 : (ent->objtype==JSON_False ? 0 : -1));
            break;
        }
    }

    /* Write end of objects and arrays */
    while (startlevel < level) {
        if (levels[--level] == JSON_Array)
            ism_json_endArray(jobj);
        else
            ism_json_endObject(jobj);
    }
    if (starttype == JSON_Array)
        ism_json_endArray(jobj);
    else
        ism_json_endObject(jobj);

    /* Null terminate the buffer */
    if (jobj->indent)
        ism_json_putBytes(buf, "\n");
    ism_json_putNull(buf);
    jobj->buf = NULL;
    return jobj->rc;
}

void ism_json_convertMemoryStatistics(ism_json_t * jobj, ism_MemoryStatistics_t * memoryStats) {
#ifdef COMMON_MALLOC_WRAPPER
    ism_json_putULongItem(jobj, "FFDCs", ism_common_get_ffdc_count());

    ism_json_startObject(jobj, "Groups");

    uint32_t groupId = 0;
    for (groupId = 0; groupId < ism_common_mem_numgroups; groupId++) {
        ism_json_startObject(jobj, ism_common_getMemoryGroupName(groupId));

        ism_json_putULongItem(jobj, "Total", memoryStats->groups[groupId]);
        uint32_t typeId = 0;
        for (typeId = 0; typeId < ism_common_mem_numtypes; typeId++) {
            if (ism_common_getMemoryGroupFromType(typeId) == groupId) {
                ism_json_putULongItem(jobj,
                        ism_common_getMemoryTypeName(typeId),
                        memoryStats->types[typeId]);
            }
        }
        ism_json_endObject(jobj);
    }
    ism_json_endObject(jobj);
#endif
}

