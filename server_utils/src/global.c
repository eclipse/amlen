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
#include <locale.h>
#include <fcntl.h>
#include <unicode/ures.h>
#include <unicode/unum.h>
#include <unicode/putil.h>
#include <unicode/ustring.h>
#include <unicode/ucasemap.h>
#include <unicode/utypes.h>
#include <sys/stat.h>

struct resbundle_t {
    struct resbundle_t *     next;
    struct UResourceBundle * res;
    char                     locale[32];
};

static char g_locale [64] = "en_US";     /* language_region portion of locale name */
static UResourceBundle * g_msgcatalog;
static struct resbundle_t *     g_resbundles;
static char *       g_wanted_locale = NULL;
static char         g_triad = ',';
static char         g_point = '.';
       char         g_bedrock_locale [64] = {'0'};

#ifndef HAS_BRIDGE
static const char * g_path = IMA_SVR_INSTALL_PATH "/resource/";
#else /* else we are the bridge*/
static const char * g_path = IMA_BRIDGE_INSTALL_PATH "/resource/";
#endif 
extern pthread_mutex_t     g_utillock;

void ism_common_crcinit(void);
void ism_common_crc32c_init(void);


/*
 * Set the locale from the OS or by config.
 * We do this very early so we are not yet able to log a failure.
 */
int ism_common_initLocale(ism_prop_t * props) {
    const char * locale_conf  = NULL;
    char   locale[64];
    char * loc;
    int    base;
    int    len;
    UNumberFormat * unum;
    UErrorCode status = U_ZERO_ERROR;
    UChar  ubuf[1];
    const char * respath;

    ism_common_crcinit();
    ism_common_crc32c_init();
    respath = ism_common_getStringConfig("ResourceDir");
    if (respath)
        g_path = respath;


    if (props)
        locale_conf = ism_common_getStringProperty(props, "Locale");
    if (!locale_conf) {
        locale_conf = getenv("LC_ALL");
        if (!locale_conf) {
            locale_conf = getenv("LANG");
            if (!locale_conf)
                locale_conf = "en_US";
        }
    }

    ism_common_strlcpy(locale, locale_conf, sizeof(locale));
    base =  strcspn(locale, ".@");
    locale[base] = 0;
    ism_common_strlcat(locale, ".utf8", sizeof(locale));
    TRACE(5, "The requested locale is: %s\n", locale);

    loc = setlocale(LC_ALL, locale);
    if (!loc || (!strstr(loc, "utf") && !strstr(loc, "UTF"))) {
        loc = setlocale(LC_ALL, "en_US.utf8");
        if (!loc) {
            loc = "en_US";
            TRACE(3, "The locale %s is missing, so en_US is used instead.\n", locale);
        }
        g_wanted_locale = (char *)locale_conf;
    }
    if (*locale) {
        strcpy(g_locale, locale);
    } else {
        ism_common_strlcpy(g_locale, loc, sizeof(g_locale));
        base =  strcspn(g_locale, ".@ ");
        g_locale[base] = 0;
    }
    TRACE(5, "The set locale is: %s\n", g_locale);

    /*
     * Use ICU to find the decimal point and group separator for the current locale
     */
    unum = unum_open(UNUM_DECIMAL, NULL, 0, locale, NULL, &status);
    if (U_SUCCESS(status)) {
        len = unum_getSymbol(unum, UNUM_GROUPING_SEPARATOR_SYMBOL, ubuf, 1, &status);
        if (len == 1) {
            g_triad = (char)ubuf[0];
            TRACE(8, "Set group separator to: %c\n", g_triad);
        }
        len = unum_getSymbol(unum, UNUM_DECIMAL_SEPARATOR_SYMBOL, ubuf, 1, &status);
        if (len == 1) {
            g_point = (char)ubuf[0];
            TRACE(8, "Set decimal point to: %c\n", g_point);
        }
        unum_close(unum);
    }

    setlocale(LC_NUMERIC, "C");    /* Make strtod work */

    g_msgcatalog = NULL;

    // defect 232721: causing crash?
    // pthread_mutex_unlock(&g_utillock);
    return 0;
}


/*
 * The server for now has a single server locale.
 */
const char * ism_common_getLocale(void) {
    return g_locale;
}


/*
 * Since we could not log the message when we detedcted the problem, try again later
 */
void ism_common_checkLocaleError(void) {
    if (g_wanted_locale) {
        /* TODO: make into a log point */
        TRACE(2, "The requested locale could not be set.  The requested locale is %s.  The actual locale is %s.\n",
                g_wanted_locale, g_locale);
    }
}

/*
 * Get the thousands separator
 */
XAPI char ism_common_getNumericSeparator(void) {
    return g_triad;
}

/*
 * Get the decimal point
 */
XAPI char ism_common_getNumericPoint(void) {
    return g_point;
}

/*
 * Get a resource bundle by locale in specified catalog dir
 */
struct UResourceBundle *  ism_common_getMessageCatalogFromCatalogPath(const char *catpath, const char * locale) {
    int llen;
    UErrorCode status = U_ZERO_ERROR;
    struct resbundle_t * rbundle;
    UResourceBundle * msgcat;
    char tlocale[64];

    if ( !catpath || *catpath == '\0' )
    	catpath = g_path;

    /* Fast return for system locale */
    if ((!locale || !strcmp(locale, g_locale)) && g_msgcatalog) {
        return g_msgcatalog;
    }
    if (!locale)
        locale = ism_common_getLocale();
    ism_common_strlcpy(tlocale, locale, sizeof tlocale);
    char * dot = strchr(tlocale, '.');
    if (dot)
        *dot = 0;

    /* Handle cases which will never work */
    llen = strlen(locale);
    if (llen<1 || llen >= sizeof(rbundle->locale))
        return NULL;

    /* Look for a locale we have already loaded */
    rbundle = g_resbundles;
    while (rbundle) {
        if (!strcmp(locale, rbundle->locale))
            return rbundle->res;
        rbundle = rbundle->next;
    }

    /* Lock the code to update the resource list */
    pthread_mutex_lock(&g_utillock);

    /* Try again with the lock held */
    rbundle = g_resbundles;
    while (rbundle) {
        if (!strcmp(locale, rbundle->locale)) {
            pthread_mutex_unlock(&g_utillock);
            return rbundle->res;
        }
        rbundle = rbundle->next;
    }

    /* Open the ICU message catalog */
    msgcat = ures_open(catpath, tlocale, &status);
    if (U_FAILURE(status)) {
        msgcat = ures_open(catpath, "root", &status);
        if (U_FAILURE(status)) {
            msgcat = NULL;
        }
    }
    if (!strcmp(locale, ism_common_getLocale())) {
        g_msgcatalog = msgcat;
    } else {
        rbundle = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,213),1, sizeof(struct resbundle_t));
        rbundle->next = g_resbundles;
        rbundle->res = msgcat;
        strcpy(rbundle->locale, locale);
        g_resbundles = rbundle;
    }
    pthread_mutex_unlock(&g_utillock);
    return msgcat;
}



/*
 * Get a resource bundle by locale
 */
struct UResourceBundle *  ism_common_getMessageCatalog(const char * locale) {
	return ism_common_getMessageCatalogFromCatalogPath(NULL, locale);
}


/*
 * Get the log message from the resource bundle
 */
const char * ism_common_getMessage(const char * msgid, char * buf, int len, int * xlen) {
    int  mlen;
    UErrorCode status = U_ZERO_ERROR;
    const char * str;

    mlen = len;
    str =  ures_getUTF8StringByKey(ism_common_getMessageCatalog(NULL), msgid, buf, &mlen, 1, &status);
    if (U_SUCCESS(status) && mlen < len) {
        if (xlen)
            *xlen = mlen;
        return buf;
    } else {
        if (xlen)
            *xlen = 0;
        return NULL;
    }
    return str;
}


/*
 * Get the log message from the resource bundle specified by catalog path
 */
const char * ism_common_getMessageByLocaleAndCatalogPath(const char *path, const char * msgid, const char * locale, char * buf, int len, int * xlen) {
    int  mlen;
    UErrorCode status = U_ZERO_ERROR;
    const char * str;

    mlen = len;
    if ( path ) {
        str =  ures_getUTF8StringByKey(ism_common_getMessageCatalogFromCatalogPath(path, locale), msgid, buf, &mlen, 1, &status);
    } else {
    	str =  ures_getUTF8StringByKey(ism_common_getMessageCatalog(locale), msgid, buf, &mlen, 1, &status);
    }
    if (U_SUCCESS(status) && mlen < len) {
        if (xlen)
            *xlen = mlen;
        return buf;
    } else {
        return NULL;
        if (xlen)
            *xlen = 0;
    }
    return str;
}

/*
 * Get the log message from the resource bundle
 */
const char * ism_common_getMessageByLocale(const char * msgid, const char * locale, char * buf, int len, int * xlen) {
	return ism_common_getMessageByLocaleAndCatalogPath(NULL, msgid, locale, buf, len, xlen );
}


/*
 * Get the log message from the resource bundle
 */
const char * ism_common_getLocaleMessage(const char * locale, const char * msgid, char * buf, int len, int * xlen) {
    int  mlen;
    UErrorCode status = U_ZERO_ERROR;
    const char * str;
    UResourceBundle * res;

    res = ism_common_getMessageCatalog(locale);
    if (!res)
        res = ism_common_getMessageCatalog(NULL);
    mlen = len;
    str =  ures_getUTF8StringByKey(res, msgid, buf, &mlen, 1, &status);
    if (U_SUCCESS(status) && mlen < len) {
        if (xlen)
            *xlen = mlen;
        return buf;
    } else {
        return NULL;
        if (xlen)
            *xlen = 0;
    }
    return str;
}



/* Starter states for UTF8 */
static int States[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
};


/* Initial byte masks for UTF8 */
static int StateMask[5] = {0, 0, 0x1F, 0x0F, 0x07};


/*
 * Check for valid second byte of UTF-8
 */
static XINLINE int validSecond(int state, int byte1, int byte2) {
    int ret = 1;

    if (byte2 < 0x80 || byte2 > 0xbf)
        return 0;

    switch (state) {
    case 2:
        if (byte1 < 2)
            ret = 0;
        break;
    case 3:
        if ((byte1== 0 && byte2 < 0xa0) || (byte1 == 13 && byte2 > 0x9f))
            ret = 0;
        break;
    case 4:
        if (byte1 == 0 && byte2 < 0x90)
            ret = 0;
        else if (byte1 == 4 && byte2 > 0x8f)
            ret = 0;
        else if (byte1 > 4)
            ret = 0;
        break;
    }
    return ret;
}

/*
 * Scan a UTF-8 string for validity.
 * Return a count of the characters in the string.
 */
int ism_common_validUTF8(const char * s, int len) {
    int  byte1 = 0;
    int  state = 0;
    int  count = 0;
    int  inputsize = 0;
    uint8_t * sp = (uint8_t *)s;
    uint8_t * endp = (uint8_t *)(s+len);

    while (sp < endp) {
        if (state == 0) {
            /* Fast loop in single byte mode */
            for (;;) {
                if (*sp >= 0x80)
                    break;
                count++;
                if (++sp >= endp)
                    return count;
            }

            count++;
            state = States[*sp >>3];
            byte1 = *sp & StateMask[state];
            sp++;
            inputsize = 1;
            if (state == 1)
                return -1;
        } else {
            int byte2 = *sp++;
            if ((inputsize==1 && !validSecond(state, byte1, byte2)) ||
                (inputsize > 1 && (byte2 < 0x80 || byte2 > 0xbf)))
                return -1;
            if (inputsize+1 >= state) {
                state = 0;
            } else {
                inputsize++;
            }
        }
    }
    if (state)
        return -1;        /* Incomplete character */
    return count;
}


/*
 * Scan a UTF-8 string for validity and do not allow noncharacters.
 *
 * In unicode, the codepoints 0xfffe and 0xfff in each of the 17 planes
 * and the codepoints 0xfdd0 to 0xfdef are noncharacters.
 *
 * Return a count of the characters in the string or a negative value to indicate an error.
 */
int ism_common_validUTF8Restrict(const char * s, int len, int notallowed) {
    int  byte1 = 0;
    int  state = 0;
    int  codept = 0;
    int  count = 0;
    int  inputsize = 0;
    uint8_t * sp = (uint8_t *)s;
    uint8_t * endp;

    if (len < 0)
        len = (int)strlen(s);
    endp = (uint8_t *)(s+len);

    while (sp < endp) {
        if (state == 0) {
            /* Fast loop in single byte mode */
            for (;;) {
                if (*sp >= 0x80)
                    break;
                if (*sp <= '/') {
                    switch(*sp) {
                    case ' ':
                        if (notallowed & UR_NoSpace)
                            return -7;
                        break;
                    case '!':
                    case '"':
                    case '$':
                    case '%':
                    case '&':
                    case '\'':
                    case '(':
                    case ')':
                    case '*':
                    case ',':
                    case '-':
                    case '.':
                        break;
                    case '\n':
                    case '\r':
                    case '\t':
                        if (notallowed & UR_NoC0Line)
                            return -2;
                        break;
                    case 0:
                        if (notallowed & UR_NoNull)
                            return -1;
                        break;
                    case '+':
                        if (notallowed & UR_NoPlus)
                            return -6;
                        break;
                    case '#':
                        if (notallowed & UR_NoHash)
                            return -6;
                        break;
                    case '/':
                        if (notallowed & UR_NoSlash)
                            return -8;
                        break;
                    default:
                        if (notallowed & UR_NoC0Other)
                            return -2;
                        break;
                    }
                }
                count++;
                if (++sp >= endp)
                    return count;
            }

            count++;
            state = States[*sp >>3];
            byte1 = *sp & StateMask[state];
            sp++;
            inputsize = 1;
            if (state == 1)
                return -5;
            codept = byte1;
        } else {
            int byte2 = *sp++;
            if ((inputsize==1 && !validSecond(state, byte1, byte2)) ||
                (inputsize > 1 && (byte2 < 0x80 || byte2 > 0xbf)))
                return -1;
            codept = (codept<<6) | (byte2&0x3f);
            if (inputsize+1 >= state) {
                if (codept < 0x00a0 && (notallowed & UR_NoC1)) {
                    return -3;
                }
                if (codept >= 0xfdd0 && (notallowed & UR_NoNonchar)) {
                    if (codept < 0xfffe) {
                        if (codept>=0xfdd0 && codept<=0xfdef)
                            return -4;
                    } else {
                        codept &= 0xffff;
                        if (codept==0xfffe || codept==0xffff)
                            return -4;
                    }
                }
                state = 0;
            } else {
                inputsize++;
            }
        }
    }
    if (state)
        return -5;        /* Incomplete character */
    return count;
}

/*
 * Put out a UTF-8 character
 */
static int putUTF8Char(int c, uint8_t * str) {
    int len;

    if (c <= 0x007F) {
        if (str)
           str[0] = (uint8_t)c;
        len = 1;
    } else if (c > 0x07FF) {
        if (c >= 0x10000) {
            if (str) {
                str[0] = (uint8_t)(0xF0 | ((c >> 18) & 0x07));
                str[1] = (uint8_t)(0x80 | ((c >> 12) & 0x3F));
                str[2] = (uint8_t)(0x80 | ((c >>  6) & 0x3F));
                str[3] = (uint8_t)(0x80 | (c & 0x3F));
            }
            len = 4;
        } else {
            if (str) {
                str[0] = (uint8_t)(0xE0 | ((c >> 12) & 0x0F));
                str[1] = (uint8_t)(0x80 | ((c >>  6) & 0x3F));
                str[2] = (uint8_t)(0x80 | (c & 0x3F));
            }
            len = 3;
        }
    } else {
        if (str) {
            str[0] = (uint8_t)(0xC0 | ((c >>  6) & 0x1F));
            str[1] = (uint8_t)(0x80 | (c & 0x3F));
        }
        len = 2;
    }
    return len;
}


/*
 * Replace any invalid bytes in a UTF-8 string.
 *
 * Return a count of the bytes in the output sting.
 */
int ism_common_validUTF8Replace(const char * s, int len, char * outc, int notallowed, uint32_t repl) {
    int  byte1 = 0;
    int  state = 0;
    int  codept = 0;
    int  count = 0;
    int  inputsize = 0;
    uint8_t * sp = (uint8_t *)s;
    uint8_t * out  = (uint8_t *)outc;
    uint8_t * outp = (uint8_t *)outc;
    uint8_t * endp = (uint8_t *)(s+len);
    int  rep = 0;

    while (sp < endp) {
        if (state == 0) {
            /* Fast loop in single byte mode */
            for (;;) {
                if (*sp >= 0x80)
                    break;
                if (*sp <= '+') {
                    rep = *sp;
                    switch(*sp) {
                    case ' ':
                    case '!':
                    case '"':
                    case '$':
                    case '%':
                    case '&':
                    case '\'':
                    case '(':
                    case ')':
                    case '*':
                        break;
                    case '\n':
                    case '\r':
                    case '\t':
                        if (notallowed & UR_NoC0Line)
                            rep = repl;
                        break;
                    case 0:
                        if (notallowed & UR_NoNull)
                            rep = repl;
                        break;
                    case '+':
                    case '#':
                        if (notallowed & UR_NoWildcard)
                            rep = repl;
                        break;
                    default:
                        if (notallowed & UR_NoC0Other)
                            rep = repl;
                        break;
                    }
                    if (rep != 0)
                        outp += putUTF8Char(rep, out ? outp : out);
                } else {
                    if (out)
                        *outp = *sp;
                    outp++;
                }
                count++;
                if (++sp >= endp) {
                    if (outp == (uint8_t *)out)
                        outp += putUTF8Char(repl ? repl : '?', out ? outp : out);
                    if (out)
                        *outp = 0;
                    return outp-out;
                }
            }

            count++;
            state = States[*sp >>3];
            byte1 = *sp & StateMask[state];
            sp++;
            inputsize = 1;
            if (state == 1) {
                if (repl != 0)
                    outp += putUTF8Char(repl, out ? outp : out);
                state = 0;
            }
            codept = byte1;
        } else {
            int byte2 = *sp++;
            if ((inputsize==1 && !validSecond(state, byte1, byte2)) ||
                (inputsize > 1 && (byte2 < 0x80 || byte2 > 0xbf))) {
                if (repl != 0)
                    outp += putUTF8Char(repl, out ? outp : out);
                state = 0;
            } else {
                codept = (codept<<6) | (byte2&0x3f);
                if (inputsize+1 >= state) {
                    if (codept < 0x00a0 && (notallowed & UR_NoC1)) {
                        codept = repl;
                    }
                    if (state && codept >= 0xfdd0 && (notallowed & UR_NoNonchar)) {
                        if (codept < 0xfffe) {
                            if (codept>=0xfdd0 && codept<=0xfdef)
                                codept = repl;
                        } else {
                            codept &= 0xffff;
                            if (codept==0xfffe || codept==0xffff)
                                codept = repl;
                        }
                    }
                    if (codept)
                        outp += putUTF8Char(codept, out ? outp : out);
                    state = 0;
                } else {
                    inputsize++;
                }
            }
        }
    }
    if (state) {
        if (repl)
            outp += putUTF8Char(repl, out ? outp :out);
    }
    if (outp == (uint8_t *)out)
        outp += putUTF8Char(repl ? repl : '?', out ? outp : out);
    if (out)
        *outp = 0;
    return outp-out;
}
/*
 * Scan a UTF-8 string for validity and allow no control characters except LF, CR, and HT.
 * Return a count of the characters in the string or a negitive value to indicate an error.
 * This is used to determine if the string in text or binary.
 */
int ism_common_validUTF8NoCC(const char * s, int len) {
    return ism_common_validUTF8Restrict(s, len, UR_NoC0Other|UR_NoNull|UR_NoC1);
}

/*
 * Truncate a UTF-8 string at a character boundary.
 */
void ism_commmon_truncateUTF8(char * str, int maxlen) {
    char * cp;
    int    state;
    int    len;

    len = strlen(str);
    if (len < maxlen-1)
        return;
    cp = str+maxlen-1;
    *cp = 0;
    if ((uint8_t)cp[-1] < 0x80)
        return;
    cp--;
    while (cp >= str && (uint8_t)*cp < 0xc0)
        cp--;
    state = States[((uint8_t)*cp) >> 3];
    switch (state) {
    case 0:  *++cp = 0;   break;         /* This should not happen */
    default:
    case 1:  *cp = 0;     break;         /* This should not happen */
    case 2:
    case 3:
    case 4:
        if (strlen(cp) != state)
            *cp = 0;
        break;
    }
}

/*
 * Make lower case the uppercase characters of a UTF-8 string. The src str is assumed to be null-terminated.
 */
int ism_common_lowerCaseUTF8(char ** dest, const char * src) {
    int rc = ISMRC_OK;
    struct UCaseMap * lowerCaseMap = NULL;
    UErrorCode pErrorCode = U_ZERO_ERROR;
    int bytes_needed = 0;

    if (!dest) {
        rc = ISMRC_NullPointer;
        return rc;
    }

    lowerCaseMap = ucasemap_open(ism_common_getLocale(), 0, &pErrorCode);
    if (!lowerCaseMap) {
        TRACE(3, "Could not open case map object. Unicode API Error: %s\n", u_errorName(pErrorCode));
        rc = ISMRC_UnicodeNotValid;
    } else {
        bytes_needed = ucasemap_utf8ToLower(lowerCaseMap, NULL, 0, src, -1, &pErrorCode) + 1;
        *dest = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_to_lower,1),bytes_needed);
        pErrorCode = U_ZERO_ERROR;
        ucasemap_utf8ToLower(lowerCaseMap, *dest, bytes_needed, src, -1, &pErrorCode);
        if (pErrorCode) {
            ism_common_free(ism_memory_utils_to_lower, *dest);
            *dest = NULL;
            TRACE(3, "Could not lowercase UTF-8 string %s. Unicode API Error: %s\n", src, u_errorName(pErrorCode));
            rc = ISMRC_UnicodeNotValid;
        }
        ucasemap_close(lowerCaseMap);
    }

    return rc;
}

/*
 * Lowercase a UTF-8 string using ICU
 */
const char * ism_common_lowerUTF8(char * buf, int buflen, const char * src, int srclen) {
    UErrorCode pErrorCode = U_ZERO_ERROR;
    struct UCaseMap * lowerCaseMap = lowerCaseMap = ucasemap_open(NULL, 0, &pErrorCode);
    int outlen = ucasemap_utf8ToLower(lowerCaseMap, buf, buflen, src, srclen, &pErrorCode);
    if (lowerCaseMap) {
        ucasemap_close(lowerCaseMap);
    }
    return (pErrorCode || outlen >= buflen) ? src : buf;
}


/*
 * CRC tables
 */
static uint32_t g_crctab[256];
static uint32_t g_crc32c_table[256];

/*
 * Initialize the CRC algorithm.  This matches the algorithm from zip
 */
void ism_common_crcinit(void) {
    uint32_t crc;
    int i, j;

    if (g_crctab[1])
        return;   /* Already inited */

    for (i=0; i< 256; i++) {
        crc = (uint32_t) i;
        for (j=0; j<8; j++) {
            if (crc & 1) {
                crc = 0xedb88320L ^ (crc >> 1);
            } else {
                crc >>= 1;
            }
        }
        g_crctab[i] = crc;
    }
}


/*
 * Initialize the CRC32C algorithm.  This matches the algorithm from iSCSI
 * and is used by Kafka message format 2 and can be used with the Intel
 * hardware support.
 *
 * The algorithm is the same as the zip CRC but the polynomial is different
 */
void ism_common_crc32c_init(void) {
    int i;
    int j;
    uint32_t crc;

    if (g_crc32c_table[1])
        return;   /* Already inited */

    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78L : crc >> 1;
        }
        g_crc32c_table[i] = crc;
    }
}


/*
 * Compute the crc32c for an array of bytes
 */
uint32_t ism_common_crc32c(uint32_t crc, char * buf, int len) {
    uint32_t c = crc ^ 0xffffffff;
    int  i;

    for (i=0; i<len; i++) {
        c = g_crc32c_table[(c ^ *buf++) & 0xff] ^ (c >> 8);
    }
    return c ^ 0xffffffffL;
}


/*
 * Compute the crc for an array of bytes.
 */
uint32_t ism_common_crc(uint32_t crc, char * buf, int len) {
    uint32_t c = crc ^ 0xffffffff;
    int  i;

    for (i=0; i<len; i++) {
        c = g_crctab[(c ^ *buf++) & 0xff] ^ (c >> 8);
    }
    return c ^ 0xffffffffL;
}

#define FNV_OFFSET_BASIS_32 0x811C9DC5
#define FNV_PRIME_32 0x1000193

/*
 * Fast 32 bit hash for a string
 */
uint32_t ism_strhash_fnv1a_32(const char * in) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    const uint8_t * in8 = (const uint8_t *) in;

    while (*in8) {
        hash ^= *(in8++);
        hash *= FNV_PRIME_32;
    }
    return hash;
}

/*
 * Fast 32 bit hash for a string continued
 */
uint32_t ism_strhash_fnv1a_32_more(const char * in, uint32_t hash) {
    const uint8_t * in8 = (const uint8_t *) in;

    while (*in8) {
        hash ^= *(in8++);
        hash *= FNV_PRIME_32;
    }
    return hash;
}

/*
 * Fast 32 bit hash for a byte array
 */
uint32_t ism_memhash_fnv1a_32(const void * in, size_t len) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    const uint8_t * in8 = (const uint8_t *) in;

    while (len--) {
        hash ^= *(in8++);
        hash *= FNV_PRIME_32;
    }
    return hash;
}

/*
 * Fast 32 bit hash for a byte array contineu
 */
uint32_t ism_memhash_fnv1a_32_more(const void * in, size_t len, uint32_t hash) {
    const uint8_t * in8 = (const uint8_t *) in;

    while (len--) {
        hash ^= *(in8++);
        hash *= FNV_PRIME_32;
    }
    return hash;
}

/**
 * Set the request locale
 * Assumed that the localekey is initialized from the caller.
 */
int ism_common_setRequestLocale(pthread_key_t localekey, const char * locale)
{
	const char *tLocale;
	/*If locale is NULL or empty. Set it to default.*/
	if (locale==NULL || *locale=='\0' ){
		tLocale = ism_common_getLocale();
	} else {
		tLocale = locale;
	}
	return pthread_setspecific(localekey, tLocale);

}
/**
 * Set the request locale
 * Assumed that the localekey is initialized from the caller.
 */
const char * ism_common_getRequestLocale(pthread_key_t localekey)
{
	/*Get Locale from thread local storage*/
	const char * locale = (const char *) pthread_getspecific(localekey);
	if (locale==NULL){
		/*If no locale set in thread local storage, set to default*/
		locale = ism_common_getLocale();
	}
	return locale;
}



