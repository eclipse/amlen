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

/*
 * The WebSockets framer implements a simple http server with three functions:
 * 1. websocket upgrade
 * 2. Redirect to another http server
 * 3. Deliver a file (or file header)
 *
 * To implement this there are 4 config properties used to implement this.  These are
 * based on the Apache config values. All config values are in UTF-8.
 * AuthName=name        The name of the WWW authorizaton realm
 * DocumentRoot=file    The root of the file system.  All documents must be within this directory
 * Redirect.#=alias kind todir  There can be multiple redirection entries and they are procssed in order
 * Alias.#=alias tourl public   There can be multiple alias entries and they are processed in order
 *
 * The redirect can have the kinds temp, permanent, seealso, or gone.  Which can also be specified using
 * the http return codes 302, 301, 303, and 410.  If the kind is missing it is assumed to be 302 with give
 * the http Found response.   The alias name must begin with a slash. Redirections are processed before aliases.
 * If you redirect everything you will not be able to use websockets.  You can redirect to websockets on
 * this server by using a "redirect alias 101".  You could then redirect everything else in a subsequent
 * redirect.
 *
 * The alias entries give the name of an alias, the mapping, and the security.  The alias name must begin
 * with a slash.  The security is either public or not specified.  If public is not specified, you need to
 * complete WWW authorization with the realm specified by AuthName.  If the todir begins with "ws:" the
 * alias can be used for websocket upgrade.  Otherwise is is treated as a directory name relative to the
 * DocumentRoot.  The final file must be within DocumentRoot.  A .. is not allowed in the specified path name.
 *
 * If there are no alias entries, the default alias entry is created:
 *     alias  /  ws:* public
 * This creates an alias at / which allows upgrade to websockts for any protocol
 *
 * TODO: Should we use a http handshake for JMS?
 */
#ifndef TRACE_COMP
#define TRACE_COMP Tcp
#endif
#ifndef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel
#endif

#include <ismutil.h>
#ifdef IMAPROXY
#include <pxtransport.h>
#ifndef NO_PXACT
#ifndef PXACTIVITY_H_
int ism_pxactivity_get_state(void);
#endif
#endif
#else
#include <transport.h>
#endif
#include <ismjson.h>
#include <imacontent.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <byteswap.h>
#include <ctype.h>
#include <openssl/aes.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifndef htons
#define htons(x)  bswap_16(x)
#endif
#ifndef ntohs
#define ntohs(x)  bswap_16(x)
#endif
#ifndef htonll
#define htonll(x) bswap_64(x)
#endif
#ifndef ntohll
#define ntohll(x) bswap_64(x)
#endif

#ifdef IMAPROXY
extern int ism_transport_noLog(const char * client);
//#define WS_TRACE(level, fmt...) if (!ism_transport_noLog(transport->client_addr)) TRACE(level, fmt)
#define WS_TRACE(level, fmt...) if (UNLIKELY((level) <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)]) && !ism_transport_noLog(transport->client_addr)) \
    traceFunction((level), 0, __FILE__, __LINE__, fmt)
#else
#define WS_TRACE(level, fmt...) TRACE(level, fmt)
#endif

#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wformat-truncation="
#pragma GCC diagnostic ignored "-Wformat-overflow="
#endif

#ifdef HAS_BRIDGE
#include <tenant.h>
#endif
#ifdef IMAPROXY
int ism_proxy_getLicensedUsage(void);
#endif

/*
 * WebSockets frame structure during parse
 */
typedef struct ws_frame_t {
    ism_transport_t * transport;
    const char *  wskey;
    const char *  wshost;
    const char *  wsorigin;
    const char *  wwwauth;
    const char *  reqheaders;
    const char *  locale;
    const char *  imaSSO;
    char *  path;
    const char *  wsprotocol [20];
    int     content_len;
    int     maploc;
    int     protocount;
    int     protomax;
    char    upgrade_found;
    char    connection_found;
    char    http_op;
    uint8_t chunked;
    uint8_t reqmethod;
    uint8_t send_continue;
    char    resv[2];
    concat_alloc_t buf;
} ws_frame_t;

/*
 * WebSockets retained frame info
 */
typedef struct ism_frameobj {
    char    frametype[8];
    ws_frame_t * frame;
    int     wsversion;
    int     needed;
    concat_alloc_t save;
    int     data_pos;
    uint8_t chunked;
    uint8_t chunk_state;   /* 0=data, 1=len, 2=cr */
    uint8_t resv[2];
    /* Above here is zeroed on each header */
    int32_t  header_wanted_count;  /* -1 = all */
    uint32_t resv2;
    const char * * header_wanted_list;
} ism_frameobj_t;

typedef struct ws_alias_t {
    struct ws_alias_t * next;
    char * alias;
    char * mapto;
    int    kind;
    int    security;            /* 1=non-public */
    int    alias_len;
} ws_alias_t;


#define SERVER_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/*
 * Opcodes for WebSocket frames.
 * 0x0 - 0x7 are non-control frames.
 * 0x8 - 0xF are control frames.
 *
 * Ranges 0x3-0x7 and 0xB-0xF are reserved for future use.
 */
enum WS_MESSAGE_TYPE {
    WS_CONTINUATION     = 0x0,      /**< Continuation frame */
    WS_TEXT             = 0x1,      /**< Text frame         */
    WS_BINARY           = 0x2,      /**< Binary frame       */
    WS_RESERVED1        = 0x3,      /**< Reserved           */
    WS_RESERVED2        = 0x4,      /**< Reserved           */
    WS_RESERVED3        = 0x5,      /**< Reserved           */
    WS_RESERVED4        = 0x6,      /**< Reserved           */
    WS_RESERVED5        = 0x7,      /**< Reserved           */
    WS_CONNECTION_CLOSE = 0x8,      /**< Connection close   */
    WS_PING             = 0x9,      /**< Ping               */
    WS_PONG             = 0xA,      /**< Pong               */
    WS_CTL_RESERVED1    = 0xB,      /**< Reserved           */
    WS_CTL_RESERVED2    = 0xC,      /**< Reserved           */
    WS_CTL_RESERVED3    = 0xD,      /**< Reserved           */
    WS_CTL_RESERVED4    = 0xE,      /**< Reserved           */
    WS_CTL_RESERVED5    = 0xF       /**< Reserved           */
};

static int processHeader(ism_transport_t * transport, char * keyword, char * value, ws_frame_t * frame);
static int checkHeader(ism_transport_t * transport, ws_frame_t * frame);
static int canonicalizeHttp(char * buf, int len);
static int validAuthority(const char * authority);
static int parseuri(char * uri, char * * scheme, char * * authority, char * pathsep, char * * path, char * * query, char * * fragment);
static int doRedirect(ism_transport_t * transport, char * path, ws_frame_t * frame);
static int findAlias(ism_transport_t * transport, char * path, ws_frame_t * frame);
static int processRequest(ism_transport_t * transport, char * path, char * query,
        char * buffer, int len, ws_frame_t * frame, int * used);
static int continueRequest(ism_transport_t * transport, char * buffer, int len);
static char * http_unescape(char * str);
static int http_escapepath_extra(const char * str);
static char * http_escapepath(char * out, const char * in);
static int invalid_path(char * path);
static void addItem(ws_frame_t * frame, int isCookie, const char * name, const char * value);
static int hexval(char ch);
int ism_transport_httpframer(ism_transport_t * transport, char * buf, int pos, int avail, int * used);
static const char * http_status_str(int httprc);
static const char * zz_decrypt(const char * data, char * buf, int len);

int ism_transport_allowConnection(ism_transport_t * transport);

ws_alias_t *    g_redirect_table = NULL;
const char *    g_document_root = NULL;
const char *    g_auth_name = "MessageSight";
int             g_alias_count = 0;
int             ism_transport_forwarder_active = 0;
int             g_strictTransportSecurity = 0;
bool            g_sendServerHTTPHeader = true;
bool            g_sendServerHTTPErrorMessages = true;


/*
 * Add the frame to before the current buffer position for WebSockets at the RFC 6455 level.
 */
XAPI int ism_transport_addWSFrame(ism_transport_t * transport, char * buffer, int len, int kind) {
    int lenlen = 2;

    kind |= 0x80;
    if (len <= 125) {
        buffer[-2] = kind;
        buffer[-1] = (char)len;
    } else if (len < (64*1024)) {
        buffer[-4] = kind;
        buffer[-3] = 126;
        buffer[-2] = (char)(len>>8);
        buffer[-1] = (char)len;
        lenlen = 4;
    } else {
        uint64_t llen = htonll((uint64_t)len);
        buffer[-10] = kind;
        buffer[-9] = 127;
        memcpy(buffer-8, &llen, 8);
        lenlen = 10;
    }
    return lenlen;
}

/*
 * Send a WebSockets close
 */
XAPI int ism_transport_closeWS(ism_transport_t * transport, int rc) {
    char closebuf[4];
    int  flen;

    closebuf[2] = (char)(rc>>8);
    closebuf[3] = (char)rc;
    flen = ism_transport_addWSFrame(transport, closebuf+2, 2, WS_CONNECTION_CLOSE);
    transport->send(transport, closebuf+2-flen, flen+2,  0, SFLAG_SYNC|SFLAG_HASFRAME);
    WS_TRACE(8, "Send close WS: rc=%d connect=%u client=%s\n", rc, transport->index, transport->name);
    return 0;
}

/*
 * Frame for remaining data
 */
int ism_transport_frameHttpData(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    int buflen = avail-pos;

    /*
     * If connection is closing or has been closed, reject
     */
    if (transport->state == ISM_TRANST_Closed || transport->state == ISM_TRANST_Closing) {
        return -1;
    }

    *used += buflen;
    return continueRequest(transport, buffer+pos, buflen);
}

/*
 * Implement the HTTP framing.
 * This implements WebSockets at the RFC 6455 level as well as general HTTP
 */
int ism_transport_frameWS(ism_transport_t * transport, char * buffer, int pos, int avail, int * used) {
    char * bp = buffer+pos;
    char * mp;
    char * mask;
    int  buflen = avail-pos;
    int  startUsed = *used;
    uint8_t kind;
    uint8_t opcode = 0;
    int  i;
    int  len = 0;
    int  maskflag;
    int  count;
    uint16_t  slen;
    uint64_t  llen;
    int  isFragment;
    int  isFirstFragment;
    int  isLastFragment;
    int  fragStarted = 0;
    char * messageStart = bp;
    char * fragCur = NULL;
    int  totalLen = 0;

    /*
     * If connection is closing or has been closed, reject
     */
    if (transport->state == ISM_TRANST_Closed || transport->state == ISM_TRANST_Closing) {
        return -1;
    }

    /*
     * The message could be fragmented, so continue until
     * the last frame of the message is found
     */
    do {
        int consumed = *used - startUsed;

        if ((buflen - consumed) < 2) {
            return (consumed + 2);
        }

        /*
         * Check the op and length bytes
         */
        kind  = bp[0];
        len   = bp[1]&0x7f;
        count = 2;

        /*
         * Get information about fragments
         */
        isFirstFragment = !!(kind != WS_CONTINUATION && !(kind & 0x80));
        isFragment = isFirstFragment || (kind == WS_CONTINUATION && !(kind & 0x80));
        isLastFragment = !!(kind == (WS_CONTINUATION | 0x80));
        fragStarted = (fragStarted || isFirstFragment);

        maskflag = !!(bp[1]&0x80);
        bp += 2;

        /*
         * Control frames must have length < 125 and must not be fragmented.
         */
        if (kind >= WS_CONNECTION_CLOSE && kind <= WS_CTL_RESERVED5) {
            if (len > 125 || !(kind & 0x80)) {
                ism_common_setError(ISMRC_BadClientData);
                transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
                return -1;
            }
        }

        /*
         * If the frame is not masked or if reserved bits are not zero,
         * FAIL the connection.
         */
        if (!maskflag || (kind&0x70)) {
            ism_common_setError(ISMRC_BadClientData);
            transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
            return -1;
        }
        kind &= 0x7f;

        /*
         * Determine the frame length
         */
        if (len > 125) {
            if (len == 126) {
                if ((buflen - consumed) < 4) {
                    return (consumed + 4);
                }
                memcpy(&slen, bp, 2);
                bp += 2;
                len = (int)ntohs(slen);
                count = 4;
            } else {
                if ((buflen - consumed) < 10) {
                    return (consumed + 10);
                }
                memcpy(&llen, bp, 8);
                llen = ntohll(llen);

                if (llen > 16*1024*1024) {
                    ism_common_setError(ISMRC_BadClientData);
                    transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
                    return -1;
                }
                bp += 8;
                len = (int)llen;
                count = 10;
            }
        }

        /*
         * Unmask the bytes
         */
        if (maskflag) {
            count += 4;
            if ((buflen - consumed) < len + count) {
                return (consumed + len + count);
            }
            mask = bp;
            bp += 4;
            mp = bp;
            for (i=0; i<len; i++) {
                *mp = *mp ^ mask[i&3];
                mp++;
            }
        } else {
            if ((buflen - consumed) < len + count) {
                return (consumed + len + count);
            }
        }

        if (isFirstFragment || !fragStarted) {
            messageStart = bp;
            opcode = kind;
        }

        if (isFragment || !fragStarted || isLastFragment) {
            totalLen += len;
        }

        /*
         * Handle various opcodes (control frames could occur in the
         * middle of fragmented message)
         */
        switch (kind) {
        case WS_CONNECTION_CLOSE:
            {
                int clean = 0;
                int ec;
                const char * reason;

                /* Echo original close message and close the connection */
                transport->addframe = ism_transport_addWSFrame;
                transport->send(transport, bp, len,  WS_CONNECTION_CLOSE, SFLAG_SYNC|SFLAG_FRAMESPACE);
                slen = 1005;
                if (len >= 2) {
                    memcpy(&slen, bp, 2);
                    slen = ntohs(slen);
                    WS_TRACE(7, "WebSockets connection close code: %04x", slen&0xffff);
                }
                switch (transport->closestate[3]) {
                default:
                    reason = "The connection has completed normally.";
                    ec = 0;
                    clean = 1;
                    break;
                case 2:
                    reason = "The ClientID is not valid";
                    ism_common_setError(ISMRC_BadClientData);
                    ec = ISMRC_BadClientID;
                    break;
                case 3:
                    reason = "The server capacity is reached";
                    ism_common_setError(ISMRC_ServerCapacity);
                    ec = ISMRC_ServerCapacity;
                    break;
                case 5:
                    reason = "Connection not authorized";
                    ism_common_setError(ISMRC_ConnectNotAuthorized);
                    ec = ISMRC_ConnectNotAuthorized;
                }
                WS_TRACE(8, "Close WebSocket connection: rc=%d reason=\"%s\"", ec, reason);
                transport->close(transport, ec, clean, reason);
            }
            break;

        case WS_PING:
            /* Send pong back */
            transport->send(transport, bp, len, WS_PONG, SFLAG_SYNC|SFLAG_FRAMESPACE);
            break;

        case WS_PONG:
            /* Don't do anything, unsolicited */
            break;

        case WS_CONTINUATION:
        default:
            /* Perform receive on unfragmented message or on fully assembled message */
            if (isLastFragment) {
                if (!fragCur) {
                    ism_common_setError(ISMRC_BadClientData);
                    transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
                    return -1;
                }
                memmove(fragCur, bp, len);
            }
            if (!fragStarted || isLastFragment) {
                /* Not fragmented or last fragment */
                int rrc = transport->receive(transport, messageStart, totalLen, opcode);
                if (rrc) {
                    return rrc == -9 ? -9 : -1;
                }
                fragStarted = 0;
                fragCur = NULL;
            } else if (fragStarted && !isFirstFragment){
                /* Middle fragment, concatenate */
                if (!fragCur) {
                    ism_common_setError(ISMRC_BadClientData);
                    transport->close(transport, ISMRC_BadClientData, 0, "The data from the client is not valid");
                    return -1;
                }
                memmove(fragCur, bp, len);
                fragCur += len;
            } else if (isFirstFragment) {
                /* First fragment, point past data for future concatenation */
                fragCur = bp + len;
            }
            break;
        }

        bp += len;
        *used += (len+count);
    } while (fragStarted);

    return 0;
}

#define WEBSOCKET_RESPONSE_CONTINUE "\
HTTP/1.1 100 Continue\r\n\
%s\
Date: %s\r\n\
Connection: keep-alive\r\n\
Keep-Alive: timeout=60\r\n\
Content-Length: 0\r\n\
\r\n"

/* Response header format without origin */
#define WEBSOCKET_SERVER_RESPONSE_HEADER "\
HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Protocol: %s\r\n\
Sec-WebSocket-Accept: %s\r\n\
%s\
Date: %s\r\n\
\r\n"

/* Response header format with origin */
#define WEBSOCKET_SERVER_RESPONSE_HEADER_ORIGIN "\
HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Origin: %s\r\n\
Sec-WebSocket-Protocol: %s\r\n\
Sec-WebSocket-Accept: %s\r\n\
%s\
Date: %s\r\n\
\r\n"

#define WEBSOCKET_RESPONSE_MOVED "\
HTTP/1.1 301 Moved permanently\r\n\
Location: %s\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
\r\n"

#define WEBSOCKET_RESPONSE_FOUND "\
HTTP/1.1 302 Found\r\n\
Location: %s\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
\r\n"

#define WEBSOCKET_RESPONSE_SEEOTHER "\
HTTP/1.1 303 See other\r\n\
Location: %s\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
\r\n"

#define WEBSOCKET_RESPONSE_BAD "\
HTTP/1.1 400 Bad request\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n\
\r\n%s"

#define WEBSOCKET_AUTHENTICATE "\
HTTP/1.1 401 Unauthorized\r\n\
WWW-Authenticate: Basic realm=\"%s\"\r\n\
%s\
Date: %s\r\n\
Access-Control-Allow-Origin: %s\r\n\
Access-Control-Allow-Credentials: true\r\n\
Connection: keep-alive\r\n\
Keep-Alive: timeout=60\r\n\
Content-Type: %s\r\n\
Content-Length: %u\r\n\
\r\n%s"

#define WEBSOCKET_FORBIDDEN "\
HTTP/1.1 403 Forbidden\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
Content-Type: %s\r\n\
Content-Length: %u\r\n\
\r\n%s"

#define WEBSOCKET_RESPONSE_NOT_FOUND "\
HTTP/1.1 404 Not found\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n\
\r\n%s"


#define WEBSOCKET_RESPONSE_GONE "\
HTTP/1.1 410 Gone\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
\r\n"

#define WEBSOCKET_RESPONSE_NOTMET "\
HTTP/1.1 417 Expectation not met\r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
\r\n"

#define WEBSOCKET_RESPONSE_BAD_VER "\
HTTP/1.1 426 Upgrade Required\r\n\
Sec-WebSocket-Version: 13\r\n\
%s\
Date: %s\r\n\
\r\n"

#define WEBSOCKET_METHOD_NOT_ALLOWED "\
HTTP/1.1 405 Unknown HTTP method\r\n\
Allow: GET, PUT, POST, DELETE, OPTIONS \r\n\
%s\
Date: %s\r\n\
Connection: close\r\n\
\r\n"


#define ALLOW_ACTION "\
HTTP/1.1 200 OK\r\n\
%s\
Date: %s\r\n\
Access-Control-Allow-Origin: %s\r\n\
Access-Control-Allow-Credentials: true\r\n\
Access-Control-Allow-Methods: GET, PUT, POST, DELETE\r\n\
Access-Control-Allow-Headers: %s\r\n\
Access-Control-Max-Age: 1728000\r\n\
Connection: keep-alive\r\n\
Keep-Alive: timeout=60\r\n\
Content-Length: 0\r\n\
Content-Type: text/plain\r\n\
\r\n"

#define HTTP_RESPONSE "\
HTTP/1.1 %d %s\r\n\
%s\
Date: %s\r\n\
Access-Control-Allow-Origin: %s\r\n\
Access-Control-Allow-Credentials: true\r\n\
Connection: %s\r\n\
Keep-Alive: timeout=60\r\n\
Cache-Control: %s\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n"

#define HTTP_SIMPLE "\
HTTP/1.1 200 OK\r\n\
%s\
Date: %s\r\n\
Connection: keep-alive\r\n\
Keep-Alive: timeout=60\r\n\
Cache-Control: no-cache\r\n\
Content-Type: text/plain\r\n\
Content-Length: %d\r\n\r\n"

#define HTTP_SIMPLE_EMPTY "\
HTTP/1.1 204 No Content\r\n\
%s\
Date: %s\r\n\
Connection: keep-alive\r\n\
Keep-Alive: timeout=60\r\n\
Cache-Control: no-cache\r\n\r\n"
static inline const char *getServerHTTPHeaderString(void) {
    if (g_sendServerHTTPHeader) {
        return "Server: " IMA_PRODUCTNAME_FULL "\r\n";
    } else {
        return "";
    }
}


/*
 * Get an HTTP valid date
 */
const char * ism_http_time(char * date, int len) {
    struct tm tm;
    time_t timex;
    static char * days [] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static char * months [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    time(&timex);
    gmtime_r(&timex, &tm);
    snprintf(date, len, "%s, %02u %s %04u %02u:%02u:%02u GMT",
            days[tm.tm_wday], tm.tm_mday, months[tm.tm_mon],
            (tm.tm_year+1900), tm.tm_hour, tm.tm_min, tm.tm_sec);
    return date;
}

/*
 * Append an HTTP strict security header to an existing buffer
 */
const char * setStrictSecurity(ism_transport_t * transport, char * buf, int len) {
    static const char * strict = "\r\nStrict-Transport-Security: max-age=10368000";
    int offset;

    if (transport->secure && (transport->serverport == 443 || g_strictTransportSecurity==2)) {
        offset = strlen(buf);
        if ((offset + strlen(strict) + 1) < len) {
            strcpy(buf+offset, strict);
        }
    }
    return buf;
}

/*
 * Simple map from accept-language to locale
 */
const char * ism_http_mapLocale(const char * acceptlang, char * buf, int len) {
    char * token = NULL;
    char   locale[8];
    char * lang;
    int    tokenlen;

    locale[0] = 0;
    if (acceptlang) {
        lang = alloca(strlen(acceptlang)+ 1);
        strcpy(lang, acceptlang);
        token = ism_common_getToken(lang, " ,;", " ,;", &lang);
        if (token) {
            tokenlen = strlen(token);
            if (tokenlen == 2) {
                strcpy(locale, token);
            } else if (tokenlen == 5) {
                strcpy(locale, token);
                locale[2] = '_';
            }
        }
    }
    if (!*locale) {
        return NULL;
    } else {
        ism_common_strlcpy(buf, locale, len);
        return buf;
    }
}


/*
 * Send a WebSockets error response
 */
static int wserror(ism_transport_t * transport, int rc, const char * reason, char * value, const char * locale) {
    char buf[1024];
    char date[80];
    char locbuf[16] = {0};
    char * zbuf;
    char * rbuf;
    concat_alloc_t ebuf = {0};
    const char * contype = "text/plain;charset=utf-8";
    int   conlen = 0;
    int zrc;

    if (reason) {
        if (*reason == '{')
            contype = "application/json";
        conlen = strlen(reason);
    }

    ism_http_time(date, sizeof(date));
    if (g_strictTransportSecurity) {
       setStrictSecurity(transport, date, sizeof(date));
    }
    switch (rc) {
    case 100:
        snprintf(buf, 1024, WEBSOCKET_RESPONSE_CONTINUE,
                getServerHTTPHeaderString(), date);
        break;

    case 301:
        WS_TRACE(6, "Send HTTP moved: %s to %s: connect=%u\n", reason, value, transport->index);
        zrc = snprintf(buf, sizeof(buf), WEBSOCKET_RESPONSE_MOVED, value,
                       getServerHTTPHeaderString(), date);
        if (zrc < 0 || zrc >= sizeof(buf)) {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            return wserror(transport, 400, "The HTTP request is too large", "", NULL);
        }
        break;

    case 302:
        WS_TRACE(6, "Send HTTP redirect: %s to %s: connect=%u\n", reason, value, transport->index);
        zrc = snprintf(buf, sizeof(buf), WEBSOCKET_RESPONSE_FOUND, value,
                            getServerHTTPHeaderString(), date);
        if (zrc < 0 || zrc >= sizeof(buf)) {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            return wserror(transport, 400, "The HTTP request is too large", "", NULL);
        }
        break;

    case 303:
        WS_TRACE(6, "Send HTTP see other: %s to %s: connect=%u\n", reason, value, transport->index);
        zrc = snprintf(buf, sizeof(buf), WEBSOCKET_RESPONSE_SEEOTHER, value,
                             getServerHTTPHeaderString(), date);
        if (zrc < 0 || zrc >= sizeof(buf)) {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            return wserror(transport, 400, "The HTTP request is too large", "", NULL);
        }
        break;

    default:
    case 400:
        rc = 400;
        WS_TRACE(2, "The HTTP request is not valid: %s %s: connect=%u\n", reason, value, transport->index);

        if (g_sendServerHTTPErrorMessages) {
            zbuf = alloca(2048);
            rbuf = alloca(1024);
            ebuf.buf = zbuf;
            ebuf.len = 2048;
            if (locale)
                ism_http_mapLocale(locale, locbuf, sizeof locbuf);
            ism_common_getErrorStringByLocale(ISMRC_HTTP_BadRequest, locbuf, rbuf, 1024);
            sprintf(ebuf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_HTTP_BadRequest);
            ebuf.used = strlen(ebuf.buf);
            ism_json_putBytes(&ebuf, "\"Message\":");
            ism_json_putString(&ebuf, rbuf);
            ism_json_putBytes(&ebuf, ", \"Reason\":");
            ism_json_putString(&ebuf, reason);
            ism_common_allocBufferCopy(&ebuf, " }");
            ebuf.used--;
        }
        snprintf(buf, 1024, WEBSOCKET_RESPONSE_BAD, getServerHTTPHeaderString(),
                    date, "application/json", ebuf.used, ebuf.buf);
        break;

    case 401:
        WS_TRACE(6, "Send HTTP Not Authorized: origin=%s: connect=%u\n", value, transport->index);

        if (g_sendServerHTTPErrorMessages) {
            zbuf = alloca(2048);
            rbuf = alloca(1024);
            ebuf.buf = zbuf;
            ebuf.len = 2048;
            if (locale)
                ism_http_mapLocale(locale, locbuf, sizeof locbuf);
            ism_common_getErrorStringByLocale(ISMRC_HTTP_NotAuthorized, locbuf, rbuf, 1024);
            sprintf(ebuf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_HTTP_NotAuthorized);
            ebuf.used = strlen(ebuf.buf);
            ism_json_putBytes(&ebuf, "\"Message\":");
            ism_json_putString(&ebuf, rbuf);
            ism_common_allocBufferCopy(&ebuf, " }");
            ebuf.used--;
        }
        zrc = snprintf(buf, sizeof(buf), WEBSOCKET_AUTHENTICATE, g_auth_name,
                   getServerHTTPHeaderString(), date, value?value:"*",
                   contype, conlen, reason);
        if (zrc < 0 || zrc >= sizeof(buf)) {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            return wserror(transport, 400, "The HTTP request is too large", "", NULL);
        }
        break;

    case 403:
        WS_TRACE(6, "Send HTTP Forbidden: %s: connect=%u\n", reason, transport->index);
        snprintf(buf, 1024, WEBSOCKET_FORBIDDEN, getServerHTTPHeaderString(),
                           date, contype, conlen, reason);
        break;

    case 404:
        WS_TRACE(6, "Send HTTP not found: %s: connect=%u\n", value, transport->index);

        if (g_sendServerHTTPErrorMessages) {
            zbuf = alloca(2048);
            rbuf = alloca(1024);
            ebuf.buf = zbuf;
            ebuf.len = 2048;
            if (locale)
                ism_http_mapLocale(locale, locbuf, sizeof locbuf);
            ism_common_getErrorStringByLocale(ISMRC_HTTP_NotFound, locbuf, rbuf, 1024);
            sprintf(ebuf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_HTTP_NotFound);
            ebuf.used = strlen(ebuf.buf);
            ism_json_putBytes(&ebuf, "\"Message\":");
            ism_json_putString(&ebuf, rbuf);
            ism_common_allocBufferCopy(&ebuf, " }");
            ebuf.used--;
        }
        snprintf(buf, 1024, WEBSOCKET_RESPONSE_NOT_FOUND, getServerHTTPHeaderString(),
                      date, "application/json", ebuf.used, ebuf.buf);
        break;

    case 405:
        WS_TRACE(6, "Send HTTP not allowed: %s: connect=%u\n", reason, transport->index);
        snprintf(buf, 1024, WEBSOCKET_METHOD_NOT_ALLOWED, getServerHTTPHeaderString(), date);
        break;

    case 410:
        WS_TRACE(6, "Send HTTP gone: %s: connect=%u\n", reason, transport->index);
        snprintf(buf, 1024, WEBSOCKET_RESPONSE_GONE, getServerHTTPHeaderString(), date);
        break;

    case 417:
        WS_TRACE(6, "Send HTTP expectation not met: %s: connect=%u\n", reason, transport->index);
        snprintf(buf,1024, WEBSOCKET_RESPONSE_NOTMET, getServerHTTPHeaderString(), date);
        break;

    case 426:
        WS_TRACE(2, "Send HTTP bad version: %s = %s: connect=%u\n", reason, value, transport->index);
        snprintf(buf, 1024, WEBSOCKET_RESPONSE_BAD_VER, getServerHTTPHeaderString(), date);
        break;
    }

    /* Trace the response */
    if (SHOULD_TRACEC(9, Http) && (transport->endpoint_name[0] != '!' || SHOULD_TRACEC(9, Admin))) {
        TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf);
    }

    /* Send the response */
    transport->send(transport, buf, (int)strlen(buf), 0, SFLAG_SYNC);
    transport->at_server = 0;

    /* Log the response */
    switch (rc) {
    case 301:    /* Moved */
    case 302:    /* Found */
    case 303:    /* See other */
        LOG(INFO, Connection, 1113, "%s%d%s%d%-s%s%d%d", "The connection from {0}:{1} to "
            "{2}:{3} is redirected from {4} to {5}: ConnectionID={7} RC={6}.",
            transport->client_addr, transport->clientport,
            transport->server_addr, transport->serverport,
            reason, value, rc, transport->index);
        break;
    case 100:
    case 401:    /* Not authorized */
        break;

    default:
#ifdef IMAPROXY
#ifndef HAS_BRIDGE
    if (!ism_transport_noLog(transport->client_addr))
#endif
#endif
        LOG(WARN, Connection, 1112, "%s%d%s%d%d%d%-s%-s", "The HTTP handshake for connection from {0}:{1} to "
            "{2}:{3} is not valid: ConnectionID={5} RC={4} Reason={6} Data={7}.",
            transport->client_addr, transport->clientport,
            transport->server_addr, transport->serverport, rc, transport->index,
            reason, value);
        break;
    }
    return rc;
}


/*
 * Extract the resource from a URI
 */
xUNUSED const char * extractResource(ism_transport_t * transport, char * path) {
    char * pathx;
    char * query;
    char * fragment;
    char * pathcopy = alloca(strlen(path)+1);
    int    rc;
    char   pathsep;

    strcpy(pathcopy, path);
    rc = parseuri(pathcopy, NULL, NULL, &pathsep, &pathx, &query, &fragment);
    if (rc || fragment || (!pathx && !query)) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP URI is not valid", (char *)path, NULL);
        return NULL;
    }
    if (pathx) {
        if (pathsep == '/')
            pathx--;
    } else {
        pathx = query ? query : pathcopy;
    }
    path[strlen(pathx)] = 0;
    return path + (pathx-pathcopy);
}

/*
 * Map a file extension to a mime type.
 * Only a few types are supported
 */
const char * ism_http_getContentType(const char * name, int * maxage) {
    const char * contype = "text/plain;charset=utf-8";
    int max_age = 86400;   /* one day */
    if (name) {
        const char * ext = strrchr(name, '.');
        if (ext) {
            if (!strcmpi(ext, ".html")) {
                max_age = 3600;  /* one hour */
                contype = "text/html;charset=utf-8";
            } else if (!strcmpi(ext, ".ico")) {
                contype = "image/x-icon";
            } if (!strcmpi(ext, ".zip")) {
                contype = "application/zip";
            } if (!strcmpi(ext, ".gz") || !strcmpi(ext, ".tgz")) {
                contype = "application/x-gzip";
            } if (!strcmpi(ext, ".js")) {
                max_age = 3600;  /* one hour */
                contype = "application/x-javascript";
            } if (!strcmpi(ext, ".json")) {
                max_age = 0;
                contype = "application/json";
            } if (!strcmpi(ext, ".css")) {
                max_age = 3600;
                contype = "text/css";
            } if (!strcmpi(ext, ".pem") || !strcmpi(ext, ".crt") || !strcmpi(ext, ".key")) {
                contype = "application/x-x509-cert";
            } if (!strcmpi(ext, ".jpg") || !strcmpi(ext, ".jpeg")) {
                contype = "image/jpeg";
            } if (!strcmpi(ext, ".gif")) {
                contype = "image/gif";
            } if (!strcmpi(ext, ".png")) {
                contype = "image/png";
            } if (!strcmpi(ext, ".mp3")) {
                contype = "audio/mpeg3";
            } if (!strcmpi(ext, ".pdf")) {
                contype = "application/pdf";
            } if (!strcmpi(ext, ".yaml")) {
                contype = "application/yaml";
            }
        }
    }
    if (maxage)
        *maxage = max_age;
    return contype;
}

/*
 * Find a file based on the locale.
 * This is only done for files of type http.  For all others the base name is returned.
 */
static const char * getFileByLocale(const char * path, const char * name, const char * locale, char * outbuf, int outlen) {
    int maxlen = (path?strlen(path):0) + strlen(name) + (locale?strlen(locale):0) + 6;
    char * fname = alloca(maxlen);
    char * mname;
    char * pos;

    if (*name=='.' || strstr(name, "/."))
        return NULL;

    if (path) {
        strcpy(fname, path);
        mname = fname + strlen(path);
        if (mname > fname && mname[-1] == '/')
            mname--;
        *mname++ = '/';
    } else {
        mname = fname;
    }

    pos = strrchr(name, '.');
    if (locale && pos && !strcmp(pos+1, "html")) {
        /* Try full locale */
        int flen = pos-name;
        memcpy(mname, name, flen);
        mname[flen] = '_';
        strcpy(mname+flen+1, locale);
        strcat(mname, pos);
        if (!access(fname, R_OK)) {
            ism_common_strlcpy(outbuf, mname, outlen);
            return outbuf;
        }
        if (strlen(locale) > 2) {
            memcpy(mname+flen+1, locale, 2);
            strcpy(mname+flen+3, pos);
            if (!access(fname, R_OK)) {
                ism_common_strlcpy(outbuf, mname, outlen);
                return outbuf;
            }
        }
    }
    strcpy(mname, name);
    if (!access(fname, R_OK)) {
        ism_common_strlcpy(outbuf, mname, outlen);
        return outbuf;
    }
    return NULL;
}



/*
 * Get the contents of a file.
 * We assume it is small and just read it all into memory.
 */
static int getFileContent(const char * path, const char * name, char * * xbuf) {
    FILE * f;
    long   len;
    char * fname;
    char * buf;
    int    bread = 0;

    /* Prohibit any file with a leading . */
    if (*name=='.' || strstr(name, "/."))
        return 0;

    if (path) {
        fname = alloca(strlen(path)+strlen(name)+2);
        strcpy(fname, path);
        strcat(fname, "/");
        strcat(fname, name);
    } else {
        fname = (char *)name;
    }

    *xbuf = NULL;
    f = fopen(fname, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    len =  ftell(f);
    if (len >= 0 && len < 16*1024*1024) {
        buf =ism_common_malloc(ISM_MEM_PROBE(ism_memory_http, 1),len+1);
        if (buf) {
            buf[len] = 0;
            rewind(f);
            *xbuf = buf;
            bread = fread(buf, 1, len, f);
        }
    }
    fclose(f);
    return bread;
}

/*
 * Replace variables in the content
 */
static int substituteContent(ism_transport_t * transport, char * * sbuf, const char * fbuf, int len) {
    char xbuf[4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    int  inlen = len;
    const char * pos;
    const char * start = fbuf;

    while (len) {
        pos = strstr(start, "${");
        if (!pos) {
            ism_common_allocBufferCopyLen(&buf, start, len);
            break;
        } else {
            const char * token = pos+2;
            const char * token_end = strchr(token, '}');
            if (token_end) {
                int valuelen = 0;
                const char * value;
                const char * value2;
                int token_len = token_end-token;
                ism_common_allocBufferCopyLen(&buf, start, pos-start);
                len -= (token_end+1)-start;
                start = token_end + 1;

                if (token_len == 10 && !memcmp(token, "BuildLevel", 10)) {
                    value = ism_common_getBuildLabel();
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else if (token_len == 13 && !memcmp(token, "LicensedUsage", 13)) {
#ifndef IMAPROXY
                    ism_licenseType_t ltype = ism_common_getPlatformLicenseType();
                    value = "Developers";
                    if (ltype == PLATFORM_LICENSE_TEST)
                        value = "Non-Production";
                    if (ltype == PLATFORM_LICENSE_PRODUCTION)
                        value = "Production";
                    if (ltype == PLATFORM_LICENSE_STANDBY)
                        value = "IdleStandby";
#else
                    int ltype = ism_proxy_getLicensedUsage();
                    switch (ltype) {
                    default:                    value = "None";           break;
                    case LICENSE_Developers:    value = "Developers";     break;
                    case LICENSE_Production:    value = "Production";     break;
                    case LICENSE_NonProduction: value = "Non-Production"; break;
                    }
#endif
                    valuelen = strlen(value);
                } else if (token_len == 13 && !memcmp(token, "ContainerName", 13)) {
                    value = getenv("MESSAGESIGHT_CONTAINER_NAME");
                    if (!value || !*value)
                        value = ism_common_getProcessName();
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else if (token_len == 12 && !memcmp(token, "BuildVersion", 12)) {
                    value = ism_common_getVersion();
                    value2 = ism_common_getBuildLabel();
                    if (!value) {
                        value = value2;
                        value2 = NULL;
                    } else {
                        if (!value)
                            value = "";
                    }
                    if (value2) {
                        char * xvalue = alloca(strlen(value)+strlen(value2)+8);
                        sprintf(xvalue, "%s - %s", value, value2);
                        value = (const char *)xvalue;
                    }
                    valuelen = strlen(value);
                } else if (token_len == 13 && !memcmp(token, "ServerNameUID", 13)) {
                    value = ism_common_getServerName();
                    if (!value)
                        value = "";
                    value2 = ism_common_getServerUID();
                    if (!value2)
                        value2 = "";
                    if (ism_transport_forwarder_active && strcmp(value, value2)) {
                        char * xvalue = alloca(strlen(value)+strlen(value2)+8);
                        sprintf(xvalue, "%s  (%s)", value, value2);
                        value = (const char *)xvalue;
                    }
                    valuelen = strlen(value);
                } else if (token_len == 10 && !memcmp(token, "ServerName", 10)) {
                    value = ism_common_getServerName();
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else if (token_len == 9 && !memcmp(token, "ServerUID", 9)) {
                    value = ism_common_getServerUID();
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else if (token_len == 8 && !memcmp(token, "Endpoint", 8)) {
                    value = transport->endpoint_name;
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else if (token_len == 8 && !memcmp(token, "Hostname", 8)) {
                    value = ism_common_getHostnameInfo();
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else if (token_len == 7 && !memcmp(token, "Version", 7)) {
                    value = ism_common_getVersion();
                    if (!value)
                        value = "";
                    valuelen = strlen(value);
                } else {      /* Unknown, leave as is */
                     valuelen = token_end-token+3;
                     value = pos;
                }
                ism_common_allocBufferCopyLen(&buf, value, valuelen);
            } else {
                ism_common_allocBufferCopyLen(&buf, start, len);
                break;
            }
        }
    }
    if (buf.inheap) {
        *sbuf = buf.buf;
        // The caller expects the memory to be allocated to ism_memory_http so move it accross
        ism_common_transfer_memory(ism_memory_alloc_buffer, ism_memory_http , buf.buf );
    } else {
        char * outbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_http, 5),buf.used);
        if (outbuf) {
            memcpy(outbuf, buf.buf, buf.used);
        } else {
            buf.used = inlen;
        }
        *sbuf = outbuf;
    }
    return buf.used;
}


/*
 * Check for a simple file which we can get without authentication
 */
static int simplePath(const char * path) {
    const char * pos;
    if (!path || *path != '/')
        return 0;
    pos = strchr(path+1, '/');
    if (pos) {
        if (pos-path == 8 && !memcmp(path, "/license", 8))
            return 1;
    } else {
        if (!strcmp(path, "/license"))
            return 1;
#ifdef IMAPROXY
        if (path[1] == '#')
            return 1;
#endif
        if (path[1] == 0 || strchr(path+1, '.'))
            return 1;
    }
    return 0;
}

#ifdef IMAPROXY
void ism_proxy_getAuthLBStats(ism_transport_t * transport, char * buf, int len);
#endif
/*
 * Parse and validate the WebSockets handshake
 * The return value is the http return code
 */
static int parseWSHandshake(ism_transport_t * transport, char * buffer, int buflen, int * used) {
    char *  path;
    char *  query = NULL;
    char *  cp;
    int     rc;
    int     i;
    char *  value;
    int     keysize;
    char    key[64];
    uint8_t sbuf[EVP_MAX_MD_SIZE];
    char    locbuf[16];
    char    b64buf[128];
    char *  outbuf;
    int     outlen;
    ws_frame_t frame;
    char    xbuf[1024];
    char date[80];
    int    httprc = 0;
    int    http_op;
    uint32_t mdLen;
    int    clen;
    int    hdrlen;
    int    len = buflen;
    int    oplen = 4;

    /*
     * Check that we have the complete http header in the buffer
     */
    cp = buffer;
    clen = len-2;
    for (i=0; i<clen; i++) {
        if (*cp == '\n' && cp[1]=='\r')     /* Look for blank line */
            break;
        cp++;
    }
    if (i >= clen)
        return -1;

    hdrlen = i+3;

    /*
     * Create the frame local object
     */
    memset(&frame, 0, sizeof(frame));
    frame.transport = transport;
    frame.buf.buf = xbuf;
    frame.buf.len = sizeof(xbuf);
    if (!transport->fobj) {
        transport->fobj = (ism_frameobj_t *)ism_transport_allocBytes(transport, sizeof(ism_frameobj_t), 1);
        memset(transport->fobj, 0, sizeof(ism_frameobj_t));
        strcpy(transport->fobj->frametype, "wstcp");
        transport->fobj->header_wanted_count = -1;
    } else {
        memset(transport->fobj, 0, offsetof(ism_frameobj_t, header_wanted_count));
    }
    transport->fobj->frame = &frame;
    frame.protomax = 20;


    /*
     * Trace the http header
     */
    if (SHOULD_TRACEC(9, Http)) {
        *++cp = 0;
        TRACEX(9, Http, 0, "httpin connect=%u:[\n%s]\n", transport->index, buffer);
        *cp = '\r';
    }

    /*
     * Determine the http operation
     */
    http_op = 0;
    if (hdrlen >= 8) {
        switch (*buffer) {
        case 'G':
            if (!memcmp(buffer, "GET ", 4))
                http_op = 'G';
            break;
        case 'P':
            if (!memcmp(buffer, "PUT ", 4)) {
                http_op = 'U';
            } else if (!memcmp(buffer, "POST ", 5)) {
                http_op = 'P';
                oplen = 5;
            }
            break;
        case 'D':
            if (!memcmp(buffer, "DELETE ", 7)) {
                http_op = 'D';
                oplen = 7;
            }
            break;
        case 'O':
            if (!memcmp(buffer, "OPTIONS ", 8)) {
                http_op = 'O';
                oplen = 8;
            }
        }
    }
    if (http_op == 0) {
        wserror(transport, 405, "Only GET, PUT, POST, DELETE, and OPTIONS are allowed", NULL, NULL);
        return 405;
    }
    frame.http_op = http_op;

    path = buffer + oplen;
    len -= oplen;
    cp = path;
    while (len-- > 0 && (uint8_t)*cp > ' ')
        cp++;
    if (len <= 0 || *cp != ' ') {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP path is not valid", path, NULL);
        return 400;
    }
    if (len < 11 || memcmp(cp, " HTTP/1.1\r\n", 10)) {
        cp[8] = 0;
#ifdef IMAPROXY
        if (!ism_transport_noLog(transport->client_addr))
#endif
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP version must be HTTP/1.1", cp+1, NULL);
        return 400;
    }
    *cp = 0;
    len -= 10;    /* already path space */
    cp += 11;

    len = canonicalizeHttp(cp, hdrlen-(cp-buffer));
    if (len < 0) {
        char * mbuf = alloca(100);
        sprintf(mbuf, "The HTTP request is not valid at offset (%d)", len);
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, mbuf, buffer, NULL);
        return 400;
    }

    /*
     * The resource can be a relative URI or an absolute URI, but cannot contain a fragment.
     * In any case, select extract the resource name from the path.
     */
    frame.path = alloca(strlen(path)+1);
    strcpy(frame.path, path);
    {
        char * pathx;
        char * fragment;
        char * pathcopy = alloca(strlen(path)+1);
        int    zrc;
        char   pathsep;

        strcpy(pathcopy, path);
        zrc = parseuri(pathcopy, NULL, NULL, &pathsep, &pathx, &query, &fragment);
        if (zrc || fragment || (!pathx && !query)) {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            wserror(transport, 400, "The HTTP URI is not valid", (char *)path, NULL);
            return 400;
        }
        if (pathx) {
            if (pathsep == '/')
                pathx--;
        } else {
            pathx = query ? query : pathcopy;
        }
        path[strlen(pathx)] = 0;
        path += (pathx-pathcopy);
    }
    path = http_unescape(path);   /* Convert to UTF-8 */
    if (!path) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP path is not valid", frame.path, NULL);
        return 400;
    }
    if (query) {
        query = http_unescape(query);
        if (!query) {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            wserror(transport, 400, "The HTTP query is not valid", frame.path, NULL);
            return 400;
        }
    }

    /*
     * Check for redirection
     */
    rc = doRedirect(transport, path, &frame);
    if (rc) {
        return rc;
    }

    /*
     * Put op, path, and query into the
     */
    // printf("httpuri: path='%s' query='%s'\n", path, query?query:"");
    {
        ism_field_t field;
        field.type = VT_Integer;
#ifndef IMAPROXY
        if (!transport->monitor_id) {
            ism_transport_addMonitorNow(transport);
        }
#endif
        field.val.i = transport->monitor_id;
        ism_common_putProperty(&frame.buf, NULL, &field);   /* h0 = connect */
        field.type = VT_Byte;
        field.val.i = frame.http_op;
        ism_common_putProperty(&frame.buf, NULL, &field);   /* h1 = http op */
        field.type = VT_String;
        field.val.s = path;
        ism_common_putProperty(&frame.buf, NULL, &field);   /* h2 = path */
        field.val.s = query;
        ism_common_putProperty(&frame.buf, NULL, &field);   /* h3 = query */
        frame.maploc = frame.buf.used;
        frame.buf.used += 4;                                /* p = map */
    }

    /*
     * Process all header lines
     */
    while (len > 0) {
        char * colon;
        char * endp = strchr(cp, '\n');
        if (!endp || endp == cp)
            break;
        *endp = 0;
        colon = strchr(cp, ':');
        if (colon) {
            int vlen;
            *colon = 0;
            value = colon+1;
            if (*value==' ')
                value++;
            vlen = strlen(value);                  /* Remove trailing blank */
            if (vlen && value[vlen-1] == ' ')
                value[vlen-1] = 0;
            rc = processHeader(transport, cp, value, &frame);
            if (rc) {
                return rc;
            }
        } else {
            ism_common_setError(ISMRC_HTTP_BadRequest);
            wserror(transport, 400, "The HTTP request is not valid", cp, NULL);
            ism_common_freeAllocBuffer(&frame.buf);
            return 400;
        }
        len -= (endp-cp+1);
        cp = endp+1;
    }

    /*
     * Check the header
     */
    if (!frame.wshost) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The Host header must be specified", "Host", NULL);
        ism_common_freeAllocBuffer(&frame.buf);
        return 400;
    }

#ifdef IMAPROXY
    if((transport->endpoint->secure != 1) && (transport->endpoint->enableAbout != 1) &&
            ((transport->endpoint->protomask & PMASK_Admin) == 0) && path && strncmp(path, "/#", 2)) {
        char delimiters[] = ".:";
        char* hostName = alloca(strlen(frame.wshost)+1);
        strcpy(hostName, frame.wshost);
        char* tenantName = strtok(hostName, delimiters);
        ism_tenant_t * tenant = NULL;
        ism_tenant_lock();
        if(tenantName) {
            tenant = ism_tenant_getTenant(tenantName);
        } else {
            tenant = ism_tenant_getTenant(frame.wshost);
        }
        ism_tenant_unlock();
        if(!tenant || tenant->require_secure == 1) {
            TRACEX(5, Http, 0, "A non-secure connection is not allowed. connect=%u org=%s From:%s:%u\n", transport->index, tenantName?tenantName:frame.wshost, transport->client_addr, transport->clientport);
            ism_common_setErrorData(ISMRC_NotAuthorized, "%s", "A non-secure connection is not allowed");
            transport->close(transport, ISMRC_NotAuthorized, 0, "The HTTP request is not authorized.");
            ism_common_freeAllocBuffer(&frame.buf);
            return 901; //returning high number as per design
        }
    }
#endif

    /*
     * Check if this is an options
     */
    if (frame.http_op == 'O') {
        if (frame.reqmethod && frame.wsorigin) {
            char buf[2048];
            int zrc;

            ism_http_time(date, sizeof(date));
            if (g_strictTransportSecurity) {
               setStrictSecurity(transport, date, sizeof(date));
            }
            WS_TRACE(7, "Send allowed response: connect=%u\n", transport->index);
            zrc = snprintf(buf, sizeof(buf), ALLOW_ACTION,
                    getServerHTTPHeaderString(),
                    date, frame.wsorigin,
                    frame.reqheaders ? frame.reqheaders : "");
            if (zrc < 0 || zrc >= sizeof(buf)) {
                ism_common_setError(ISMRC_HTTP_BadRequest);
                return wserror(transport, 400, "The HTTP request is too large", "", NULL);
            }
            /* Trace the response */
            if (SHOULD_TRACEC(9, Http) && (transport->endpoint_name[0] != '!' || SHOULD_TRACEC(9, Admin))) {
                TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf);
            }

            /* Send the response */
            transport->send(transport, buf, (int)strlen(buf), 0, SFLAG_SYNC);
        } else {
            ism_common_freeAllocBuffer(&frame.buf);
            ism_common_setError(ISMRC_HTTP_BadRequest);
            return wserror(transport, 400, "The options request must have an origin", "", NULL);
        }
        *used += hdrlen;
        return 200;
    }


    /*
     * Check if we require authorization.
     * We do not challenge for authentication if this is from websockets.
     */
#ifndef IMAPROXY
    if ((transport->www_auth || transport->listener->usePasswordAuth) && !frame.upgrade_found) {
#else
    if ((transport->www_auth || transport->endpoint->authorder[0]==AuthType_Basic) && !frame.upgrade_found) {
#endif
        if (frame.wwwauth || transport->userid) {
            if (frame.imaSSO && !transport->userid)
                WS_TRACE(6, "imaSSO credentials not used because basic authentication is in use\n");
        } else {
            if (frame.imaSSO) {
                int  zzlen = strlen(frame.imaSSO);
                char * zzbuf = alloca(zzlen);
                char * pos = NULL;
                zzbuf = (char *)zz_decrypt(frame.imaSSO, zzbuf, zzlen);
                if (zzbuf) {
                    pos = strchr(zzbuf, '!');
                }
                if (pos) {
                    ism_ts_t * ts;
                    *pos++ = 0;

                    ts = ism_common_openTimestamp(ISM_TZF_UTC);
                    // WS_TRACE(5, "imaSSO ts=%s uid=%s\n", zzbuf, pos);
                    if (ism_common_setTimestampString(ts, zzbuf) == 0) {
                        // WS_TRACE(5, "imaSSO now=%lu expire=%lu\n", ism_common_currentTimeNanos(), ism_common_getTimestamp(ts));
                        if (ism_common_currentTimeNanos() < ism_common_getTimestamp(ts)) {
                            frame.wwwauth = pos;
                            WS_TRACE(6, "Use imaSSO credentials: index=%u\n", transport->index);
                        } else {
                            WS_TRACE(4, "The imaSSO credentials have expired: \"%s\". index=%u\n", zzbuf, transport->index);
                        }
                    } else {
                        WS_TRACE(4, "The imaSSO credentials are not valid: index=%u\n", transport->index);
                    }
                    ism_common_closeTimestamp(ts);
                } else {
                    WS_TRACE(4, "The imaSSO credentials are not correct: index=%u\n", transport->index);
                }
            }
        }
#ifndef IMAPROXY
        if (!frame.wwwauth  && !transport->userid) {
#else
        if (!frame.wwwauth  && !transport->userid && (transport->requireClientCert==3 || !transport->cert_name)) {
#endif
            if (frame.http_op != 'G' || !simplePath(path)) {
                ism_common_setError(ISMRC_HTTP_NotAuthorized);
                wserror(transport, 401, "Not authorized", (char *)frame.wsorigin, frame.locale);
                ism_common_freeAllocBuffer(&frame.buf);
                *used += hdrlen;
                /*
                 * Skip over the content if there is any and we can skip over it,
                 * otherwise close the connection.
                 */
                if (frame.chunked || (buflen-hdrlen) < frame.content_len)
                    transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "The HTTP request is not authorized.");
                else
                    *used += frame.content_len;
                return 401;
            }
        }
    }

    /*
     * Some browsers do not send the client certificate on the OPTIONS so we allow it without.
     * In proxy we allow optional client certs so this must be checked later.
     */
#ifndef IMAPROXY
    if (transport->secure && transport->listener->useClientCert && transport->cert_name==NULL) {
        if (g_sendServerHTTPErrorMessages) {
            frame.buf.used = 0;
            ism_http_mapLocale(frame.locale, locbuf, sizeof locbuf);
            ism_common_getErrorStringByLocale(ISMRC_NoCertificate, locbuf, xbuf+512, sizeof(xbuf)-512);
            sprintf(frame.buf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_NoCertificate);
            frame.buf.used = strlen(frame.buf.buf);
            ism_json_putBytes(&frame.buf, "\"Message\":");
            ism_json_putString(&frame.buf, xbuf+512);
            ism_common_allocBufferCopy(&frame.buf, " }");
            frame.buf.used--;
        }
        ism_common_setError(ISMRC_NoCertificate);
        wserror(transport, 403, frame.buf.buf, NULL, NULL);
        ism_common_freeAllocBuffer(&frame.buf);
        return 403;
    }
#endif

    /*
     * Store the userid:password if we have one
     */
    if (frame.wwwauth) {
        char * colon;
        if (transport->authenticated) {
            colon = strchr(frame.wwwauth, ':');
            if (colon)
                *colon = 0;
            if (!transport->userid || strcmp(frame.wwwauth, transport->userid)) {
                if (g_sendServerHTTPErrorMessages) {
                    ism_http_mapLocale(frame.locale, locbuf, sizeof locbuf);
                    ism_common_getErrorStringByLocale(ISMRC_ChangedAuthentication, locbuf, xbuf+512, sizeof(xbuf)-512);
                    sprintf(frame.buf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_ChangedAuthentication);
                    frame.buf.used = strlen(frame.buf.buf);
                    ism_json_putBytes(&frame.buf, "\"Message\":");
                    ism_json_putString(&frame.buf, xbuf+512);
                    ism_common_allocBufferCopy(&frame.buf, " }");
                    frame.buf.used--;
                }
                ism_common_setError(ISMRC_ChangedAuthentication);
                wserror(transport, 403, "frame.buf.buf", (char *)frame.wwwauth, NULL);
                ism_common_freeAllocBuffer(&frame.buf);
                return 403;
            }
        } else {
            int    alen = (int)strlen(frame.wwwauth);
            char * uid;
            transport->userid = uid = ism_transport_allocBytes(transport, alen+2, 0);
            strcpy(uid, frame.wwwauth);
            colon = strchr(uid, ':');
            if (colon) {
                *colon++ = 0;
                while (*colon) {
                    *colon ^= 0xFD;    /* xor must be invalid char */
                    colon++;
                }
            } else {
                uid[alen+1] = 0;   /* Make sure there is a second null */
            }
        }
    }


    /*
     * Check for an alias
     */
    if (findAlias(transport, path, &frame)) {
        ism_common_setError(ISMRC_HTTP_NotFound);
        wserror(transport, 404, "Not found", frame.path, frame.locale);
        return 404;
    }


    /*
     * If not an upgrade
     */
    if (!frame.upgrade_found) {
        int prc;
        *used += hdrlen;
        hdrlen = i+3;

        /* Send a continue if expected and continue processing */
        if (frame.send_continue) {
            wserror(transport, 100, "Continue", NULL, NULL);
            frame.send_continue = 0;
        }
        if (!transport->ready)
            transport->ready = 1;

        /*
         * Send a simple file.
         * We assume these files are small and reading them into memory is not a problem.
         */
#ifndef IMAPROXY
        if (transport->listener->enableAbout && frame.http_op == 'G' && simplePath(path) &&
                (transport->authenticated || !strcmp(transport->protocol, "http"))) {
#else
        if (path && path[0]=='/' && path[1]=='#') {
            if (!strcmp(path, "/#authstats")) {
                int flen;
                ism_proxy_getAuthLBStats(transport, xbuf+512, 256);
                flen = strlen(xbuf+512);
                ism_http_time(date, sizeof date);
                if (flen == 0) {
                    sprintf(xbuf, HTTP_SIMPLE_EMPTY, getServerHTTPHeaderString(), date);
                } else {
                    sprintf(xbuf, HTTP_SIMPLE, getServerHTTPHeaderString(), date, flen);
                    strcat(xbuf, xbuf+512);
                }
                flen = strlen(xbuf);
                if (SHOULD_TRACEC(9, Monitoring)) {
                    TRACEX(9, Http, 0, "http response connect=%u: [\n%s]\n", transport->index, xbuf);
                }
                transport->send(transport, xbuf, flen, 0, 0);
                return 200;
            }
            /*
             * Simple unauthorized REST request to see if the server is running.
             * Always returns a status of 204 (No Content)
             */
            if (!strcmp(path, "/#running")) {
                ism_http_time(date, sizeof date);
                sprintf(xbuf, HTTP_SIMPLE_EMPTY, getServerHTTPHeaderString(), date);
                int flen = strlen(xbuf);
                transport->send(transport, xbuf, flen, 0, 0);
                return 200;
            }
#ifndef NO_PXACT
            if (!strcmp(path, "/#activity")) {
                int act = ism_pxactivity_get_state();
                ism_http_time(date, sizeof date);
                sprintf(xbuf, HTTP_SIMPLE, getServerHTTPHeaderString(), date, 1);
                int flen = strlen(xbuf);
                xbuf[flen++] = (act%10) + '0';
                transport->send(transport, xbuf, flen, 0, 0);
                return 200;
            }
#endif
        }
        if (transport->endpoint->enableAbout==1 && frame.http_op == 'G' && simplePath(path) &&
                (transport->authenticated || !strcmp(transport->protocol, "http"))) {
#endif
            const char * origin;
            if (*path=='/') {
                char   cachebuf[32];
                char * fbuf;
                char * stbuf = NULL;
                const char * contype = "text/plain";
                int  http_rc = 404;
                int  flen = 0;
                const char * fname;
                const char * cachectl = "no-cache";
                int  max_age = -1;
                int  pathlen = strlen(path);

                /* Default to index.html */
                if (!path[1]) {
                    path = "/index.html";
                } else {
                    if ((pathlen == 8 && !memcmp(path, "/license", 8)) ||
                        (pathlen == 9 && !memcmp(path, "/license/", 9))) {
#ifndef IMAPROXY
                        ism_licenseType_t ltype = ism_common_getPlatformLicenseType();
                        if (ltype == PLATFORM_LICENSE_DEVELOPMENT)
#else
                        int ltype = ism_proxy_getLicensedUsage();
                        if (ltype == LICENSE_Developers)
#endif
                            path = "/license/developer/LA.html";
                        else
                            path = "/license/LA.html";
                    }
                }

                /* Find the actual file name */
                fname = getFileByLocale(g_document_root, path+1,
                        ism_http_mapLocale(frame.locale, locbuf, sizeof locbuf),
                        xbuf, sizeof xbuf);
                if (fname) {
                    flen = getFileContent(g_document_root, fname, &fbuf);
                    if (flen) {
                        http_rc = 200;
                        contype = ism_http_getContentType(path+1, &max_age);
                        if (!memcmp(contype, "text/", 5) && strstr(fbuf, "${")) {
                            int oldlen = flen;
                            flen = substituteContent(transport, &stbuf, fbuf, flen);
                            if (flen != oldlen)
                                max_age = 60;
                        }
                    } else {
                        ism_common_setError(ISMRC_HTTP_NotFound);
                        wserror(transport, 404, "Not found", NULL, frame.locale);
                        return 404;
                    }
                } else {
                    ism_common_setError(ISMRC_HTTP_NotFound);
                    wserror(transport, 404, "Not found", NULL, frame.locale);
                    return 404;
                }
                ism_http_time(date, sizeof date);
                if (g_strictTransportSecurity) {
                   setStrictSecurity(transport, date, sizeof(date));
                }
                origin = frame.wsorigin;
                if (!origin)
                    origin = "*";

                if (max_age >= 0) {
                    cachectl = cachebuf;
                    sprintf(cachebuf, "max-age=%d", max_age);
                }
                int zrc = snprintf(xbuf, sizeof(xbuf), HTTP_RESPONSE, http_rc, http_status_str(http_rc),
                                   getServerHTTPHeaderString(), date, origin, "keep-alive", cachectl,
                                   contype, flen);
                if (zrc < 0 || zrc >= (sizeof(xbuf) - 2)) {
                    ism_common_setError(ISMRC_HTTP_BadRequest);
                    wserror(transport, 400, "The HTTP request is too large", "", NULL);
                    return 400;
                }
                strcat(xbuf, "\r\n");
                if (SHOULD_TRACEC(9, Http)) {
                    TRACEX(9, Http, 0, "http response connect=%u: [\n%s]\n", transport->index, xbuf);
                }
                transport->send(transport, xbuf, strlen(xbuf), 0, 0);
                if (flen) {
                    transport->send(transport, stbuf ? stbuf : fbuf, flen, 0, 0);
                    ism_common_free(ism_memory_http, fbuf);
                    if (stbuf)
                        ism_common_free(ism_memory_http,stbuf);
                }
                return 200;
            }
        }

        if (transport->ready == 1 && ism_transport_allowConnection(transport) < 0) {
            if (g_sendServerHTTPErrorMessages) {
                ism_http_mapLocale(frame.locale, locbuf, sizeof locbuf);
                ism_common_getErrorStringByLocale(ISMRC_ConnectNotAuthorized, locbuf, xbuf+512, sizeof(xbuf)-512);
                sprintf(frame.buf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_ConnectNotAuthorized);
                frame.buf.used = strlen(frame.buf.buf);
                ism_json_putBytes(&frame.buf, "\"Message\":");
                ism_json_putString(&frame.buf, xbuf+512);
                ism_common_allocBufferCopy(&frame.buf, " }");
                frame.buf.used--;
            }
            ism_common_setError(ISMRC_ConnectNotAuthorized);
            wserror(transport, 403, frame.buf.buf, NULL, NULL);
            return 403;
        }
        transport->ready = 2;


        /* Process the request */
        prc = processRequest(transport, path, query, buffer+hdrlen, buflen-hdrlen, &frame, used);
        ism_common_freeAllocBuffer(&frame.buf);
        return prc;
    }
    ism_common_freeAllocBuffer(&frame.buf);

    /*
     * If we got here we are doing a WebSockets update
     */
    rc = checkHeader(transport, &frame);
    if (rc) {
        return rc;
    }

    /*
     * Find the protocol
     */
    if (frame.protocount == 0) {
        frame.wsprotocol[frame.protocount++] = *path=='/' ? path+1 : path;
    }
    for (i=0; i<frame.protocount; i++) {
        transport->protocol = frame.wsprotocol[i];
        if (!ism_transport_findProtocol(transport)) {
            break;
        }
        transport->protocol = "unknown";
    }

    if (!strcmp(transport->protocol, "unknown")) {
        ism_common_setError(ISMRC_HTTP_NotFound);
        wserror(transport, 404, "Unknown protocol", NULL, frame.locale);
        return 404;
    }

    if (ism_transport_allowConnection(transport) < 0) {
        if (g_sendServerHTTPErrorMessages) {
            ism_http_mapLocale(frame.locale, locbuf, sizeof locbuf);
            ism_common_getErrorStringByLocale(ISMRC_ConnectNotAuthorized, locbuf, xbuf+512, sizeof(xbuf)-512);
            sprintf(frame.buf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_ConnectNotAuthorized);
            frame.buf.used = strlen(frame.buf.buf);
            ism_json_putBytes(&frame.buf, "\"Message\":");
            ism_json_putString(&frame.buf, xbuf+512);
            ism_common_allocBufferCopy(&frame.buf, " }");
            frame.buf.used--;
        }
        ism_common_setError(ISMRC_ConnectNotAuthorized);
        wserror(transport, 403, frame.buf.buf, NULL, NULL);
        return 403;
    }

    /*
     * Send the response
     */
    keysize = (int)(strlen(frame.wskey) + strlen(SERVER_GUID));
    strcpy(key, frame.wskey);
    strcat(key, SERVER_GUID);
    mdLen = ism_ssl_SHA1(key,keysize,sbuf);
    ism_common_toBase64((char *)sbuf, b64buf, mdLen);

    ism_http_time(date, sizeof(date));
    if (g_strictTransportSecurity) {
       setStrictSecurity(transport, date, sizeof(date));
    }
    if (frame.wsorigin) {
        outlen = strlen(WEBSOCKET_SERVER_RESPONSE_HEADER_ORIGIN) + strlen(b64buf) +
                 strlen(getServerHTTPHeaderString()) +
                 strlen(date)+ strlen(transport->protocol) + strlen(frame.wsorigin);
        outbuf = alloca(outlen);
        sprintf(outbuf, WEBSOCKET_SERVER_RESPONSE_HEADER_ORIGIN, frame.wsorigin,
            transport->protocol, b64buf, getServerHTTPHeaderString(), date);
    } else {
        outlen = strlen(WEBSOCKET_SERVER_RESPONSE_HEADER) + strlen(b64buf) +
                 strlen(getServerHTTPHeaderString()) +
                 strlen(date) + strlen(transport->protocol);
        outbuf = alloca(outlen);
        sprintf(outbuf, WEBSOCKET_SERVER_RESPONSE_HEADER, transport->protocol, b64buf,
                getServerHTTPHeaderString(), date);

    }
    *used += buflen;
    httprc = 101;
    if (SHOULD_TRACEC(9, Http) && (transport->endpoint_name[0] != '!' || SHOULD_TRACEC(9, Admin))) {
        TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, outbuf);
    }
    transport->send(transport, outbuf, strlen(outbuf), 0, 0);
    return httprc;
}


/*
 * Process redirections.
 * The path is in UTF-8 without escape encodings.
 */
static int doRedirect(ism_transport_t * transport, char * path, ws_frame_t * frame) {
    int  xpathlen;
    char * location;
    int  len = (int)strlen(path);
    ws_alias_t * redir = g_redirect_table;
    while (redir) {
        /* Check if we match this alias */
        if (len >= redir->alias_len && !memcmp(redir->alias, path, redir->alias_len)) {
            /* Make sure we are at end of path, or at a separator */
            if (len == redir->alias_len || path[redir->alias_len]=='/' || path[redir->alias_len-1]=='/') {
                if (redir->kind == 101)
                    return 0;
                http_unescape(frame->path);
                xpathlen = http_escapepath_extra(frame->path);
                if (xpathlen < 0) {
                    frame->path = path;
                    xpathlen = 0;
                }
                location = alloca(strlen(redir->mapto) + http_escapepath_extra(redir->mapto) +
                                  strlen(frame->path) + xpathlen + 1);
                http_escapepath(location, redir->mapto);
                if (redir->mapto[strlen(redir->mapto)-1] == '/')
                    http_escapepath(location+strlen(location), frame->path+redir->alias_len);
                wserror(transport, redir->kind, path, location, frame->locale);
                return redir->kind;
            }
        }
        redir = redir->next;
    }
    return 0;
}


/*
 * Find an alias.
 * The path is in UTF-8 without escape encodings
 */
static int findAlias(ism_transport_t * transport, char * path, ws_frame_t * frame) {
    char * pos;
    char * proto;
    if (*path != '/')
        return 1;

    /*
     * Parse off the protocol portion of the path
     */
    pos = strchr(path + 1, '/');
    if (!pos) {
        pos = path + strlen(path);
    }
    if (pos-path > 1023)
        return 1;
    if (frame->http_op == 'G' && simplePath(path))
        return 0;

    proto = alloca(pos-path + 1);
    memcpy(proto, path, pos-path);
    proto[pos-path] = 0;

    /*
     * Check if the new protocol is the same as any existing protocol
     */
    if (*transport->protocol == '/') {
        if (strcmp(transport->protocol, proto)) {
            WS_TRACE(5, "A different protocol has been used in an HTTP connection: index=%u new=%s old=%s\n",
                    transport->index, proto, transport->protocol);
            return 1;
        } else {
            return 0;
        }
    } else {
        transport->protocol = proto;     /* protocol will make it constant if it is accepted */
    }

    if (ism_transport_findProtocol(transport))
        transport->protocol = "http";
    return 0;
}

#define CST_Data     0
#define CST_Len      1
#define CST_CR       2
#define CST_FirstLen 3
#define CST_FirstCR  4
#define CST_TrailCR  5
#define CST_TrailLF  6
#define CST_Error    10
#define CST_Done     11


/*
 * Process a request.
 * This sends a request to the plugin
 */
static int processRequest(ism_transport_t * transport, char * path, char * query,
        char * buffer, int len, ws_frame_t * frame, int * used) {
    int maplen;
    ism_field_t field;
    if (frame->wsorigin) {
        if (!transport->origin) {
            transport->origin = ism_transport_putString(transport, frame->wsorigin);
        }
    }
    WS_TRACE(7, "processRequest connect=%d op=%c path='%s' query='%s' len=%d\n",
            transport->index, frame->http_op, path, query ? query : "", frame->content_len);
    maplen = frame->buf.used - frame->maploc - 4;
    frame->buf.buf[frame->maploc]   = 0x4B;
    frame->buf.buf[frame->maploc+1] = (char)(maplen >> 16);
    frame->buf.buf[frame->maploc+2] = (char)((maplen >> 8) & 0xff);
    frame->buf.buf[frame->maploc+3] = (char)(maplen & 0xff);
    if (frame->http_op=='G' || frame->http_op=='D') {
        frame->content_len = 0;
        frame->chunked = 0;
    }

    if (*transport->protocol != '/') {
        ism_common_setError(ISMRC_HTTP_NotFound);
        wserror(transport, 404, "Not found", path, frame->locale);
        return 404;
    }
    *used += len;

    /*
     * Implement chunked
     */
    if (frame->chunked) {
        transport->fobj->chunked = 1;
        transport->fobj->data_pos = frame->buf.used;
        ism_common_allocBufferCopyLen(&frame->buf, "\0\0\0", 4);
        transport->fobj->save = frame->buf;
        transport->fobj->chunk_state = CST_FirstLen;
        return continueRequest(transport, buffer, len);
    }

    /* Implement more data needed */
    if (len >= frame->content_len) {
        field.type = VT_ByteArray;
        field.val.s = buffer;
        field.len = frame->content_len;
        buffer[frame->content_len] = 0;
        ism_common_putProperty(&frame->buf, NULL, &field);
        if (transport->at_server == 0)
            transport->at_server = 1;
        if (transport->receive(transport, frame->buf.buf, frame->buf.used, 0)) {
            return -1;
        }
        return 200;
    } else {
        transport->fobj->data_pos = frame->buf.used;
        ism_common_allocBufferCopyLen(&frame->buf, "\0\0\0", 4);
        ism_common_allocBufferCopyLen(&frame->buf, buffer, len);
        transport->frame = ism_transport_frameHttpData;
        transport->fobj->chunk_state = CST_Data;
        transport->fobj->needed = frame->content_len - len;
        transport->fobj->save = frame->buf;
        /* Ensure buffer is in heap */
        if (!frame->buf.inheap) {
            // printf("was in stack\n");
            transport->fobj->save.buf =ism_common_malloc(ISM_MEM_PROBE(ism_memory_alloc_buffer, 2),frame->buf.len);
            memcpy(transport->fobj->save.buf, frame->buf.buf, frame->buf.used);
            transport->fobj->save.inheap = 1;
        }
        memset(&frame->buf, 0, sizeof(frame->buf));
        // printf("need more buf=%p len=%u used=%u inheap=%u\n", frame->buf.buf, frame->buf.len, frame->buf.used, frame->buf.inheap);
        return 0;    /* more data */
    }
}

/*
 * Free up the save buffer for an http transport
 */
void ism_transport_freeSaveBuffer(ism_transport_t * transport) {
    if (transport && transport->fobj && !strcmp(transport->fobj->frametype, "wstcp")) {
        ism_common_freeAllocBuffer(&transport->fobj->save);
        memset(&transport->fobj->save, 0, sizeof (concat_alloc_t));
    }
}


/*
 * Continue a request.
 * This is also used as the only implementation of chunked
 */
static int continueRequest(ism_transport_t * transport, char * buffer, int len) {
    int state = transport->fobj->chunk_state;
    concat_alloc_t buf = transport->fobj->save;
    char  ch;
    char * cp;

    memset(&transport->fobj->save, 0, sizeof(concat_alloc_t));
    if (SHOULD_TRACEC(9, Http) || SHOULD_TRACEC(9, Admin)) {
        char xbuf[64];
        sprintf(xbuf, "http more in connnect=%u (%d)", transport->index, len);
        traceDataFunction(xbuf, 0, __FILE__, __LINE__, buffer, len, ism_common_getTraceMsgData());
    }

    /*
     * Implement chunked
     */
    cp = buffer;
    if (transport->fobj->chunked) {
        int chunklen = transport->fobj->needed;
        while (len > 0 && state < CST_Error) {
            switch (state) {
            case CST_FirstLen:
                chunklen = 0;
                if (*cp == '\r') {
                    cp++;
                    len--;
                    state = CST_FirstCR;
                } else {
                    state = CST_Len;
                }
                break;

            case CST_Len:
                ch = *cp++;
                len--;
                if (ch == '\r') {
                    state = CST_CR;
                } else {
                    int val = hexval(ch);
                    if (val < 0)
                        state = CST_Error;
                    else
                        chunklen = (chunklen<<4) + val;
                }
                break;

            case CST_CR:
                if (*cp == '\n') {
                    cp++;
                    len--;
                    state = CST_Data;
                } else {
                    state = CST_Error;
                }
                break;

            case CST_FirstCR:
                if (*cp == '\n') {
                    state = CST_Done;
                } else {
                    state = CST_Error;
                }
                break;

            case CST_Data:
                if (len >= chunklen) {
                    ism_common_allocBufferCopyLen(&buf, cp, chunklen);
                    cp += chunklen;
                    len -= chunklen;
                    state = CST_TrailCR;
                } else {
                    ism_common_allocBufferCopyLen(&buf, cp, len);
                    cp += len;
                    len = 0;
                    transport->fobj->needed = chunklen-len;
                }
                break;

            case CST_TrailCR:
                if (*cp == '\r') {
                    cp++;
                    len--;
                    state = CST_TrailLF;
                } else {
                    state = CST_Error;
                }
                break;

            case CST_TrailLF:
                if (*cp == '\n') {
                    cp++;
                    len--;
                    state = CST_FirstLen;
                } else {
                    state = CST_Error;
                }
                break;
            }
        }
    } else {   /* Not chunked */
        if (len >= transport->fobj->needed) {
            state = CST_Done;
            ism_common_allocBufferCopyLen(&buf, cp, transport->fobj->needed);
        } else {
            ism_common_allocBufferCopyLen(&buf, cp, len);
            transport->fobj->needed -= len;
        }
    }

    /*
     * If complete, receive the data
     */
    if (state == CST_Done) {
        int rrc;
        int pos = transport->fobj->data_pos;
        int datalen = buf.used - pos - 4;
        if (datalen < 0 || datalen >= 16*1024*1024) {
            return -1;
        }
        buf.buf[pos] = 0x93;   /* S_ByteArray + 3 */
        buf.buf[pos+1] = (char)(datalen>>16);
        buf.buf[pos+2] = (char)(datalen>>8);
        buf.buf[pos+3] = (char)datalen;
        if (transport->at_server == 0)
            transport->at_server = 1;
        rrc = transport->receive(transport, buf.buf, buf.used, 0);
        ism_common_freeAllocBuffer(&buf);
        if (rrc)
            return -1;
        /* Back to HTTP frame */
        transport->frame = ism_transport_httpframer;
        return 200;
    }

    /*
     * If a chunk error, close the connection
     */
    else if (state == CST_Error) {
        WS_TRACE(2, "The HTTP content is not valid: conenct=%d clientid=%s\n", transport->index, transport->name);
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP content is not valid", NULL, NULL);
        ism_common_freeAllocBuffer(&buf);
        return 400;
    }

    /*
     * More content data needed
     */
    else {
        transport->frame = ism_transport_frameHttpData;
        transport->fobj->chunk_state = CST_Data;
        transport->fobj->save = buf;
        if (!buf.inheap) {
            transport->fobj->save.buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_http, 4),buf.len);
            memcpy(transport->fobj->save.buf, buf.buf, buf.used);
            transport->fobj->save.inheap = 1;
        }
    }
    return 0;
}

/*
 * Given a value string, create a canonical value list.
 *
 * The result value list will be separated by null characters.
 *
 * RFC 2616 (http) allows the list to contain any number of empty entries
 * (adjacent commas and spaces) which are ignored.
 *
 * @param  value  The value string from the header
 * @return the number of entries in the list.
 */
static int commalist(char * value) {
    int  count = 0;
    char * outp = value;
    char ch = *value;
    while (ch) {
        while (ch == ',' || ch == ' ') {
            value++;
            ch = *value;
        }
        if (!ch)
            break;
        count++;
        while (*value && *value != ' ' && *value != ',') {
            *outp++ = *value++;
        }
        ch = *value;
        *outp++ = 0;
    }
    return count;
}

/*
 * Check if a list contains the case independent string.
 * @return 1 if the string is in the list, 0 otherwise
 */
static int listcontains(char * list, int count, const char * str) {
    int  i;
    for (i=0; i<count; i++) {
        if (!strcmpi(list, str))
            return 1;
        list += strlen(list)+1;
    }
    return 0;
}


/*
 * Parse a cookie assuming that canonical processing is already done.
 * TODO: parse quoted value
 * @param str   Input string
 * @param token Token address (output, only set if rc==0)
 * @param value Value address (output, only set if rc==0)
 * @param more  More cookies to parse, only set if rc==0)
 * @retrun 0=cookie found, !0=no cookie
 */
static int parseCookie(char * str, const char * * token, const char * * value, char * * more) {
    if (!str || !*str)
        return 1;
    if (token)
        *token = (const char *)str;
    while (*str && *str != ' ' && *str != '=')
        str++;
    if (*str == '=') {
        *str++ = 0;
    } else if (*str == ' ' && str[1] == '=') {
        *str = 0;
        str += 2;
    } else {
        return 2;
    }
    if (!*str)
        return 3;
    if (*str == ' ')
        str++;
    if (value)
        *value = (const char *)str;
    while (*str && *str != ' ')
        str++;
    if (!*str) {         /* Last cookie */
        if (more)
            *more = NULL;
    } else {             /* More cookies */
        *str++ = 0;
        if (more)
           *more = str;
    }
    return 0;
}

XAPI int32_t ism_security_setLTPAToken(ism_transport_t * transport, struct ismSecurity_t * sContext, char * ltpaToken, int ltpaToken_len);

/*
 * Simple lower case (ASCII7 only)
 */
static char * my_strlwr(char * cp) {
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

/*
 * Process a WebSockets header fields.
 * Check the validity of an http header field.
 */
static int  processHeader(ism_transport_t * transport, char * xkeyword, char * value, ws_frame_t * frame) {
    int  count;
    int  i;
    int  found = 0;
    int  rc;

    char keyword[256];

    ism_common_strlcpy(keyword, xkeyword, sizeof(keyword));
    my_strlwr(keyword);

    switch (*keyword) {
    case 'a':
    /* Authorization is commonly returned after a not-authorized is sent */
        if (!strcmp(keyword, "authorization")) {
            int vlen = strlen(value)-6;
            if (vlen > 0 && value[5] == ' ') {
                value[5] = 0;
                if (!strcmpi(value, "basic")) {
                    if (ism_common_fromBase64(value+6, value, (int)strlen(value+6))>0 && strchr(value, ':')) {
                        frame->wwwauth = value;
                    }
                }
            }
            if (!frame->wwwauth) {
                ism_common_setError(ISMRC_HTTP_BadRequest);
                wserror(transport, 400, "The Authorization header is not valid", "", NULL);
                return 400;
            }
            if (!frame->connection_found)
                addItem(frame, 0, "Authorization", value);      /* BEAM suppression: no effect */
        }

        else if (!strcmp(keyword, "accept-language")) {
            frame->locale = value;
            found = 1;
            if (!frame->connection_found)
                addItem(frame, 0, "Accept-Language", value);      /* BEAM suppression: no effect */
        }

        else if (!strcmp(keyword, "access-control-request-method")) {
            frame->reqmethod = 1;
            found = 1;
        }
        else if (!strcmp(keyword, "access-control-request-headers")) {
            frame->reqheaders = value;
            found = 1;
        }
        break;

    case 'c':
        /* The Connection header field is required and must have the value "upgrade" */
        if (!strcmp(keyword, "connection")) {
            count = commalist(value);
            frame->connection_found = listcontains(value, count, "upgrade");
            if (listcontains(value, count, "close")) {
                transport->at_server = 2;
            }
            if (!frame->connection_found)
                addItem(frame, 0, "Connection", value);             /* BEAM suppression: no effect */
            found = 1;
        }
        /* Content length for POST and PUT */
        else if (!strcmp(keyword, "content-length")) {
            char * eos;
            frame->content_len = strtoul(value, &eos, 10);
            if (*eos) {
                ism_common_setError(ISMRC_HTTP_BadRequest);
                wserror(transport, 400, "The HTTP content length is not valid", value, NULL);
                return 400;
            }
            found = 1;
        }

        /* Content type */
        else if (!strcmp(keyword, "content-type")) {
            if (!frame->connection_found)
                addItem(frame, 0, "Content-Type", value);           /* BEAM suppression: no effect */
            found = 1;
        }

        /*
         * The Cookie header filed is optional.
         * If we find invalid syntax in the Cookie header we ignore the rest of the header.
         * This allows upward compatibility where we did not parse cookies
         */
        else if (!strcmp(keyword, "cookie")){
            const char * cookie;
            const char * cvalue;
            /* Parse cookies */
            while (value) {
                int prc = parseCookie(value, &cookie, &cvalue, &value);
                if (prc != 0)
                    break;
                if (!frame->upgrade_found)
                    addItem(frame, 1, cookie, cvalue);
                if (!strcmpi(cookie, "LtpaToken2") || !strcmpi(cookie, "LtpaToken")) {
                    WS_TRACE(8, "WSTCP: Received LTPA Cookie: index=%u name=%s value=%s\n", transport->index, cookie, cvalue);
                    //ism_security_setLTPAToken(transport, transport->security_context, (char *)cvalue, strlen(cvalue));
                }
            }
        }
        break;
    case 'e':
        if (!strcmp(keyword, "expect")) {
            if (!strcmpi(value, "100-continue")) {
                frame->send_continue = 1;
            } else {
                return wserror(transport, 417, "Expectation not met", value, NULL);
            }
        }
        found = 1;
        break;

    case 'h':
        /* The Host header field is required and must be a valid authority */
        if (!strcmp(keyword, "host")) {
            /* We only validate that the host is an acceptable URI, not that it matches this server */
            if (transport->clientport && !validAuthority(value)) {
                ism_common_setError(ISMRC_HTTP_BadRequest);
                wserror(transport, 400, "The HTTP Host header is not valid", value, NULL);
                return 400;
            }
            frame->wshost = value;
            addItem(frame, 0, "Host", frame->wshost);
            found = 1;
        }
        break;

    case 'i':
        /* The iamSSO header is used in MessageSight for single sign-on */
        if (!strcmp(keyword, "imasso")) {
            frame->imaSSO = value;
            found = 1;
        }
        break;

    case 'o':
        /* The Origin header field is optional and if present must be a valid URL */
        if (!strcmp(keyword, "origin")) {
            char * copyuri;
            char * scheme;
            char * authority;
            char * path;
            char   pathsep;
            char * query;
            char * fragment;

            if (strcmp(value, "null")) {
                copyuri = alloca(strlen(value)+1);
                strcpy(copyuri, value);
                rc = parseuri(copyuri, &scheme, &authority, &pathsep, &path, &query, &fragment);
                /* We only check that the origin URI is well formed, we do not check the origin domain */
                if (rc || !scheme || pathsep || path || query || fragment || !validAuthority(authority)) {
                    ism_common_setError(ISMRC_HTTP_BadRequest);
                    wserror(transport, 400, "The Origin header value is not valid", value, NULL);
                    return 400;
                }
            }
            frame->wsorigin = value;
            addItem(frame, 0, "Origin", frame->wsorigin);
            found = 1;
        }
        break;

    /* The Sec=Websocket-Key header field is required and must have a base64 string */
    case 's':
        if (!strcmp(keyword, "sec-websocket-key")) {
            char * to = alloca(strlen(value));
            int zrc = ism_common_fromBase64(value, to, (int)strlen(value));
            if (zrc != 16) {
                ism_common_setError(ISMRC_HTTP_BadRequest);
                wserror(transport, 400, "The WebSockets key is not valid", value, NULL);
                return 400;
            }
            frame->wskey = value;
            found = 1;
        }

        /* The Sec-WebSocketProtocol header field is optional and contains a comma separated list of protocols */
        else if (!strcmp(keyword, "sec-websocket-protocol")) {
            count = commalist(value);
            for (i=0; i<count; i++) {
                if (frame->protocount < frame->protomax)
                    frame->wsprotocol[frame->protocount++] = value;
                value += strlen(value)+1;
            }
            found = 1;
        }

        /* The Sec-WebSocket-Version header field is required and must have the value 13 */
        else if (!strcmp(keyword, "sec-websocket-version")) {
            char * eos;
            int ver;
            ver = transport->fobj->wsversion = strtoul(value, &eos, 10);
            /* TEMP: allow version 8 for FireFox 10 */
            if ((ver != 13 && ver != 8) || *eos) {
                wserror(transport, 426, "The WebSockets version must be 13", value, NULL);
                return 426;
            }
            found = 1;
        }
        break;

    case 't':
        /* We are required to support chunked transfer encoding */
        if (!strcmp(keyword, "transfer-encoding")) {
            count = commalist(value);
            if (listcontains(value, count, "chunked")) {
                frame->chunked = 1;
            }
            found = 1;
        }
        break;

    case 'u':
        /* The Upgrade header field is required and must have the value "websocket" */
        if (!strcmp(keyword, "upgrade")) {
            count = commalist(value);
            if (frame->http_op == 'G')
                frame->upgrade_found = listcontains(value, count, "websocket");
            found = 1;
        }
        if (!strcmp(keyword, "user-agent")) {
#ifndef IMAPROXY
            /* Add User-Agent to the headers with canonical spelling */
            addItem(frame, 0, "User-Agent", value);
#endif
            found = 1;
        }
        break;
    }

    if (!found) {
        if (!frame->upgrade_found)
            addItem(frame, 0, xkeyword, value);
    }

    return 0;
}

/*
 * Set the header wanted list
 */
void ism_transport_setHeaderList(ism_transport_t * transport, int count, const char * * wanted_list) {
    if (transport->protocol[0] == '/' && transport->fobj) {
        transport->fobj->header_wanted_count = count;
        transport->fobj->header_wanted_list = wanted_list;
    }
}

/*
 * Add an item to the props to send
 */
static void addItem(ws_frame_t * frame, int isCookie, const char * name, const char * value) {
    ism_field_t field = {0};
    int hcount;
    if (frame->upgrade_found)
        return;

    hcount = frame->transport->fobj->header_wanted_count;
    if (hcount >= 0) {
        int i;
        int wanted = 0;
        const char * * header_wanted_list = frame->transport->fobj->header_wanted_list;
        for (i=0; i<hcount; i++) {
            if (!strcmp(name, header_wanted_list[i]))
                wanted = 1;
        }
        if (!wanted)
            return;
    }
    // printf("addItem %s '%s'='%s'\n", (isCookie?"cookie":"header"), name, value);
    if (value) {
        field.type = VT_String;
        field.val.s = (char *)value;
    }
    if (!isCookie) {
        char * hname = alloca(strlen(name) + 2);
        hname[0] = ']';
        strcpy(hname+1, name);
        ism_common_putProperty(&frame->buf, hname, &field);
    } else {
        ism_common_putProperty(&frame->buf, name, &field);
    }
}

/*
 * Validity check the header.
 * This is done after the parse and is a place to check that required fields are present.
 */
static int  checkHeader(ism_transport_t * transport, ws_frame_t * frame) {
    ism_frameobj_t * fobj = transport->fobj;

    if (!frame->connection_found) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The Connection header must be specified", "", NULL);
        return 400;
    }

    if (!frame->wskey) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The Sec-WebSocket-Key header must be specified", "", NULL);
        return 400;
    }
    if (!fobj->wsversion) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The Sec-WebSocket-Version header must be specified", "", NULL);
        return 400;
    }
    return 0;
}


/*
 * Fix an http header buffer to a canonical representation.
 * Most clients will only give the canonical http, but run this code to fix up any places
 * where they do not.  Also change CR/LF to a single LF to make parsing easier.
 * 1. Change all runs of linear white space to a single space.
 * 2. Remove continuation lines
 * 3. Change all CR/LF to LF character.
 * 4. Detect invalid control characters
 * This conversion is done in place.
 * For an invalid string, return a negative value
 */
static int canonicalizeHttp(char * buf, int len) {
    char * in = buf;
    char * out = buf;
    int    lastchar = 0;
    int    wasspace = 0;
    int    i;

    for (i=0; i<len; i++) {
        char ch = *in++;

        /* Handle control characters and space */
        if ((uint8_t)ch <= ' ') {
            if (ch == ' ' || ch == '\t') {
                ch = ' ';
                if (lastchar == '\n') {
                    if (!wasspace) {
                        *out++ = ' ';
                        wasspace = 1;
                    }
                    lastchar = ch;
                    continue;
                }
                if (!wasspace) {
                    *out++ = ' ';
                    wasspace = 1;
                }
            } else if (ch == '\r') {
                if (lastchar == '\n')
                    *out++ = '\n';
            } else if (ch == '\n') {
                if (lastchar != '\r')
                    return -1;          /* LF not valid without a CR */
            } else {
                return -2;              /* Invalid control char      */
            }
        }
        /* All characters above space */
        else {
            wasspace = 0;
            if (lastchar == '\n')
                *out++ = '\n';
            *out++ = ch;
        }
        lastchar = ch;
    }
    if (lastchar == '\r')
        return -3;                     /* Trailing CR */
    if (lastchar == '\n')
        *out++ = '\n';
    *out = 0;
    return out-buf;
}


/*
 * Framer used only while reading the http header (normally for WebSockets).
 *
 * WebSockets starts with a GET command which is an http upgrade request.
 * Once we have started serving http, we stay in that mode.
 */
int ism_transport_httpframer(ism_transport_t * transport, char * buf, int pos, int avail, int * used) {
    int buflen = avail-pos;
    int rc;

    //if (SHOULD_TRACEC(9, Http)) {
    //    traceDataFunction("httpframer", 0, __FILE__, __LINE__, buf+pos, avail, avail);
    //}
    rc = parseWSHandshake(transport, buf+pos, buflen, used);
    switch (rc) {
    case -1:
        if (transport->rcvState || (buflen < MAX_FIRST_MSG_LENGTH))
            return buflen+1;
        ism_common_setError(ISMRC_FirstPacketTooBig);
        transport->close(transport, ISMRC_FirstPacketTooBig, 0, "Invalid first message length");
        return -1;
        break;
    case 901:
        return -rc;
        break;
    case 101:   /* Switching protocol */
        transport->frame = ism_transport_frameWS;
        if (transport->addframep)
            transport->addframe = transport->addframep;
        else
            transport->addframe = ism_transport_addWSFrame;
        transport->rcvState = 1;
        break;
    case 0:
    case 100:   /* Continue */
    case 200:   /* Good */
    case 401:   /* Not authorized */
    case 426:   /* Update version */
        break;  /* Keep connection open */
    case 400:
        transport->close(transport, ISMRC_HTTP_BadRequest, 0, "The HTTP request is not valid.");
        return -rc;
    case 404:
        transport->close(transport, ISMRC_HTTP_NotFound, 0, "The HTTP request is for an object which does not exist.");
        return -rc;
    default:    /* Cause a close */
        ism_common_setError(ISMRC_BadHTTP);
        transport->close(transport, ISMRC_BadHTTP, 0, "HTTP handshake failed");
        return -rc;
    }
    return 0;
}

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace


/*
 * Configure a redirection entry
 */
static void configRedirect(int i, const char * str) {
    char * scopy;
    char * alias = NULL;
    char * redirect = NULL;
    char * kind = NULL;
    char * more;
    int    rc = 302;

    if (str) {
        scopy = alloca(strlen(str)+1);
        strcpy(scopy, str);
        alias = ism_common_getToken(scopy, " \t", " \t", &more);
        if (alias) {
            kind  = ism_common_getToken(more, " \t", " \t", &more);
            if (kind) {
                if (!strcmpi(kind, "temp") || !strcmp(kind, "302")) {
                    rc = 302;
                } else if (!strcmpi(kind, "permanent") || !strcmp(kind, "301")) {
                    rc = 301;
                } else if (!strcmpi(kind, "seeother") || !strcmp(kind, "303")) {
                    rc = 303;
                } else if (!strcmpi(kind, "gone") || !strcmp(kind, "410")) {
                    rc = 410;
                } else if (!strcmpi(kind, "upgrade") || !strcmp(kind, "101")) {
                    rc = 101;
                } else {
                    redirect = kind;
                }
                if (!redirect) {
                    redirect = ism_common_getToken(more, " \t", " \t", &more);
                }
            }
        }
    }
    if (!redirect)
        redirect = "";
    if (!alias || *alias != '/' || invalid_path(alias) ||
            (!*redirect && (rc != 410 && rc != 101)) || invalid_path(redirect)) {
        TRACE(2, "The alias entry %d is not valid\n", i);
    } else {
        ws_alias_t * aent = ism_common_calloc(ISM_MEM_PROBE(ism_memory_http, 6),1, sizeof(ws_alias_t)+strlen(alias)+strlen(redirect)+2);
        aent->alias = (char *)(aent+1);
        strcpy(aent->alias, alias);
        aent->alias_len = (uint16_t)strlen(aent->alias);
        aent->mapto = aent->alias+strlen(aent->alias)+1;
        strcpy(aent->mapto, redirect);
        aent->kind = rc;
        /* Link it in, last one first */
        aent->next = g_redirect_table;
        g_redirect_table = aent;
        TRACE(6, "Add http redirect %s = %s kind=%d\n", aent->alias, aent->mapto?aent->mapto:"", aent->kind);
    }
}


/*
 * Initialize the WebSockets framer.
 *
 * All config values are in UTF-8
 */
void  ism_transport_wsframe_init(void) {
    int     i;
    const char *  value;
    char    name[16];

    if (g_alias_count)
        return;

    g_document_root = ism_common_getStringConfig("HttpDir");
    if (!g_document_root)
        g_document_root = "./http";
    TRACE(6, "Set http document root to [%s]\n", g_document_root);

    g_auth_name = ism_common_getStringConfig("AuthName");
    if (!g_auth_name) {
        g_auth_name = ism_common_getServerName();
        if (!g_auth_name)
            g_auth_name = IMA_PRODUCTNAME_FULL;
    }
    TRACE(6, "Set http authentication realm to [%s]\n", g_auth_name);

    /*
     * Return an HTTP strict security header for any HTTPS connection.
     * This should be used with great care
     */
    g_strictTransportSecurity = ism_common_getIntConfig("HttpStrictTransportSecurity", 0);
    if (g_strictTransportSecurity) {
        TRACE(5, "Set strict transport security to: %d\n", g_strictTransportSecurity);
    }

    /*
     * Do we tell clients what the server software is using HTTP Headers?
     * Useful during debug but in production, can leak info used to choose exploits to
     * attack us (see also IncludeServerHTTPErrorMessages)
     */
    int includeServerHTTPHeader = ism_common_getIntConfig("IncludeServerHTTPHeader", 1);

    if (includeServerHTTPHeader) {
        g_sendServerHTTPHeader = true;
    } else {
        TRACE(5, "Disabling Server HTTP Header (IncludeServerHTTPHeader = 0)\n");
        g_sendServerHTTPHeader = false;
    }

    /*
     * Do we send useful error messages (with error codes) back in HTTP responses?
     * Useful during debug but in production, can leak info used to choose exploits to
     * attack us (see also IncludeServerHTTPHeader)
     */
    int includeServerHTTPErrorMesssages = ism_common_getIntConfig("IncludeServerHTTPErrorMessages", 1);

    if (includeServerHTTPErrorMesssages) {
        g_sendServerHTTPErrorMessages = true;
    } else {
        TRACE(5, "Disabling Server HTTP Error Messages (IncludeServerHTTPErrorMessages = 0)\n");
        g_sendServerHTTPErrorMessages = false;
    }

    /*
     * Loop thru all properties looking for redirect
     */
    for (i=15; i>=0; i--) {
        sprintf(name, "Redirect.%d", i);
        value = ism_common_getStringConfig(name);
        if (value) {
            configRedirect(i, value);
        }
    }

}

/*
 * Check if the authority is valid.
 */
static int validAuthority(const char * authority) {
    char * pos;
    char * eos;

    /*
     * Check for invalid chars
     */
    pos = (char *)authority;
    while (*pos) {
        if (((uint8_t)*pos) <= ' ' || *pos == '/' || *pos == '?' || *pos=='#')
            return 0;
        pos++;
    }
    pos = (char *)authority;

    /*
     * Allow IPv6 format
     */
    if (*pos == '[') {
        pos = strchr(pos, ']');
        if (!pos)
            return 0;
        pos++;
        if (!*pos)
            return 1;
    } else {
        pos = strchr(authority, ':');
        if (pos == authority)
            return 0;
    }

    /*
     * Check the port
     */
    if (pos) {
        int val = strtol(pos+1, &eos, 10);
        if (*eos || val<1 || val>=(64*1024))
            return 0;
    }
    return 1;
}


/*
 * Parse a URI into its pieces.
 * This does not do any unescape of the content.
 */
static int  parseuri(char * uri, char * * scheme, char * * authority, char * pathsep, char * * path, char * * query, char * * fragment) {
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
        while (*uri) {
            *uri = tolower(*uri);
            uri++;
        }

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


/*
 * Return a hex value
 */
static int hexval(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    return -1;
}


/*
 * Unescape an http string and check the result is a valid UTF-8 string.
 */
static char * http_unescape(char * str) {
    int  val, val2;
    int  isUTF8 = 0;
    char * out = str;
    char * ret = str;
    while (*str) {
        uint8_t ch = (uint8_t)*str++;
        if (ch == '%') {
            val = hexval(*str++);
            if (val >= 0) {
                val2 = hexval(*str++);
                if (val2 >= 0)
                    val = val*16 + val2;
                else
                    val = -1;
            }
            if (val < 0)
                return NULL;
            if (val >= 0x80)
                isUTF8 = 1;
            *out++ = (char)val;
        } else {
            if (ch < 0x20)
                isUTF8 = 1;
            *out++ = (char)ch;
        }
    }
    *out = 0;
    if (isUTF8) {
        if (!ism_common_validUTF8Restrict(out, out-str, UR_NoControl | UR_NoNonchar))
            str = NULL;
    }
    return ret;
}

/*
 * Count the extra bytes needed for http path escape.
 */
static int http_escapepath_extra(const char * str) {
    int extra = 0;
    while (*str) {
        uint8_t ch = (uint8_t)*str++;
        if (ch<=0x20 || ch>0x7f || ch=='%' || ch=='?' || ch=='#')
            extra += 2;
    }
    return extra;
}


/*
 * Escape the http to escapes for the path section.
 * This assume the output buffer is large enough.  This is normally ensured by
 * first calling http_escapepath_extra to determine the final size.
 *
 * Return null if any control characters (less than space) since these are not
 * allowed characters.
 */
static char * http_escapepath(char * out, const char * in) {
    char * ret = out;

    while (*in) {
        uint8_t ch = (uint8_t)*in++;
        if (ch < ' ') {
            ret = NULL;
        } else {
            if (ch==' ' || ch>0x7f || ch=='%' || ch=='?' || ch=='#') {
                *out++ = '%';
                *out++ = "0123456789ABCDEF"[ch>>4&0x0f];
                *out++ = "0123456789ABCDEF"[ch&0x0f];
            } else {
                *out++ = ch;
            }
        }
    }
    *out = 0;
    return ret;
}

const unsigned char keybase [] = {
    0x0b, 0xbd, 0xd7, 0x89, 0x41, 0xa0, 0xd1, 0xc0,
    0x74, 0x1b, 0x6d, 0x12, 0x56, 0xa7, 0x50, 0xb9,
    0x11, 0x4b, 0x64, 0xd9, 0x46, 0x01, 0x49, 0x0b,
    0xfd, 0xc5, 0xb8, 0xe7, 0xe8, 0xeb, 0xd9, 0xb0,
    0x48, 0x62, 0x5d, 0x6b, 0xa2, 0x8d, 0xcb, 0xf7,
    0x5d, 0x4a, 0xe9, 0xde, 0x19, 0x76, 0x1b, 0xb0,
    0x27, 0x04, 0xac, 0xc5, 0x89, 0xf4, 0x67, 0x84,
    0x92, 0x8a, 0xa4, 0xd7, 0x1d, 0x70, 0x24, 0xbd,
};

const unsigned char ivec [] = {
    0x7b, 0xea, 0x60, 0x06, 0x66, 0x9f, 0x15, 0x66,
    0x61, 0xd2, 0xdf, 0x3d, 0xcc, 0x96, 0xee, 0x50,
};

static const char b64digit [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 *  Encrypt a string using AES128-CBC with variable key and salt
 */
xUNUSED static const char * zz_encrypt(const char * data, char * buf, int data_len) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    uint8_t iv [16];
    int  datalen = (int)strlen(data);
    int  blklen  = (datalen+21) & ~0x0f;
    unsigned char * blkdata = alloca(blklen);
    unsigned char * encdata = alloca(blklen+6);
    unsigned char * b64data = alloca(blklen*2);
    AES_KEY dkey;
    int offset;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    offset = tv.tv_usec%43;
    AES_set_encrypt_key(keybase+offset, 128, &dkey);
    memcpy(iv, ivec, 16);
    memset(blkdata, 0, blklen);
    blkdata[0] = b64digit[keybase[offset+16]&0x3f];
    blkdata[1] = b64digit[keybase[offset+17]&0x3f];
    blkdata[2] = b64digit[keybase[offset+18]&0x3f];
    blkdata[3] = b64digit[keybase[offset+19]&0x3f];
    blkdata[4] = ':';
    memcpy(blkdata+5, data, datalen);
    encdata[0] = (char)offset;
    AES_cbc_encrypt(blkdata, encdata+1, blklen, &dkey, iv, AES_ENCRYPT);
    ism_common_toBase64((char *)encdata, (char *)b64data, blklen+1);
    ism_common_strlcpy(buf, (const char *)b64data, len);
    return buf;

#else

    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        TRACE(7, "EVP_CIPHER_CTX_new failed\n");

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, keybase, ivec))
        TRACE(7, "EVP_aes_256_cbc failed\n");

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, (unsigned char *) buf, &len, (const unsigned char *) data, data_len))
        TRACE(7, "EVP_EncryptUpdate failed\n");
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, (unsigned char *)buf + len, &len))
        TRACE(7, "EVP_EncryptFinal_ex failed\n");
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return buf;
#endif

}

const char * ism_transport_makepw(const char * data, char * buf, int len, int dir) {
    if (dir && *data == '!') {
        return zz_decrypt(data+1, buf, len);
    } else {
        buf[0] = '!';
        return zz_encrypt(data, buf+1, len-1);
    }
}


/*
 *  Decrypt a string using AES128-CBC with variable key and salt
 */
xUNUSED static const char * zz_decrypt(const char * data, char * buf, int data_len) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    uint8_t iv[16];
    int  datalen = (int)strlen(data);
    uint8_t * bindata = alloca(datalen);
    uint8_t * decdata = alloca(datalen);
    uint8_t * pos;
    AES_KEY dkey;
    int     offset;
    int blen;

    blen = ism_common_fromBase64((char *)data, (char *)bindata, datalen);
    if (blen < 5)
         return NULL;
    blen--;

    offset = bindata[0]%43;
    memcpy(iv, ivec, 16);
    AES_set_decrypt_key(keybase+offset, 128, &dkey);
    AES_cbc_encrypt(bindata+1, decdata, blen, &dkey, iv, AES_DECRYPT);

    pos = (uint8_t *)strchr((const char *)decdata, ':');
    if (pos) {
        ism_common_strlcpy(buf, (const char *)(pos+1), len);
        return buf;
    }
    return NULL;
#else
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        TRACE(7, "EVP_CIPHER_CTX_new failed\n");

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, keybase, ivec))
        TRACE(7, "EVP_DecryptInit_ex failed\n");

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, (unsigned char *) buf, &len, (const unsigned char *)data, data_len))
        TRACE(7, "EVP_DecryptUpdate failed\n");
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, (unsigned char *)buf + len, &len))
        TRACE(7, "EVP_DecryptFinal_ex failed\n");
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return buf;
#endif
}

/*
 * Check that the string is valid UTF-8 and does not contain control characters
 */
static int invalid_path(char * path) {
    return ism_common_validUTF8Restrict(path, strlen(path), UR_NoC0) < 0;
}


/*
 * We only allow some HTTP errors, get the known text for them
 */
static const char * http_status_str(int httprc) {
    const char * error_str;
    switch (httprc) {
    case 200:  error_str = "OK";                  break;
    case 201:  error_str = "Created";             break;
    case 202:  error_str = "Accepted";            break;
    case 203:  error_str = "Non-authoritative";   break;
    case 204:  error_str = "No content";          break;
    case 205:  error_str = "Reset content";       break;
    case 400:  error_str = "Bad request";         break;
    case 403:  error_str = "Not allowed";         break;
    case 404:  error_str = "Not found";           break;
    case 405:  error_str = "Method not allowed";  break;
    case 406:  error_str = "Not acceptable";      break;
    case 409:  error_str = "Conflict";            break;
    case 410:  error_str = "Gone";                break;
    case 413:  error_str = "Request too large";   break;
    case 415:  error_str = "Unsupported media type"; break;
    case 500:  error_str = "Server error";        break;
    case 501:  error_str = "Not implemented";     break;
    case 503:  error_str = "Server unavailable";  break;
    default:   error_str = "Bad request";         break;
    }
    return error_str;
}

/*
 * Free the http object
 */
void ism_http_free(ism_http_t * http) {
    if (http->outbuf.inheap)
        ism_common_freeAllocBuffer(&http->outbuf);
    if (http->content && (http->content != &http->single_content)) {
        ism_common_free(ism_memory_http,http->content);
        http->content = NULL;
    }
    if (http->header_props) {
        ism_common_freeProperties(http->header_props);
        http->header_props = NULL;
    }
    if (http->out_headers.inheap) {
        ism_common_freeAllocBuffer(&http->out_headers);
    }
    if (http->transport) {
        http->transport->http = NULL;
    }
    ism_common_free(ism_memory_http, http);
}

/*
 * Canonicalize a MIME.
 *
 * Lower case all parts except the value portion of a parameter.
 * Also lowercase the charset parameter
 * The input and output locations an be the same.
 *
 * @param out  The output location
 * @param in   The input location;
 */
const char * ism_http_canonicalHeader(char * out, const char * in) {
    char * op = out;
    char * param = out;
    int    state = 0;
    int    extparm = 0;
    while (*in) {
        if (*in >= 'A' && *in <= 'Z' && state==0) {
            *op++ = (char)(*in|0x20);
        } else if (*in != ' ') {
            *op++ = *in;
            if (*in == ';') {
                state = 0;
                extparm = 0;
                param = op;
            } else if (*in == '=') {
                state = 1;
                if ((op-param)==8 && !memcmp(param, "charset", 7))   /* lowercase charset */
                    state = 0;
                if ((op-param)>1 && op[-2]=='*') {
                    extparm = 1;
                    state = 0;
                } else {
                    const char * np = in+1;
                    while (*np == ' ')
                        np++;
                    if (*np == '"') {    /* Quoted value */
                        np++;
                        *op++ = '"';
                        while (*np && *np != '"') {
                            if (*np == '\\') {
                                if (np[1])
                                    *op++ = *++np;
                            } else {
                                *op++ = *np;
                            }
                            np++;
                        }
                        *op++ = '"';
                        state = 0;
                        in = np;
                    }
                }
            } else if (extparm && *in == '\'') {
                if (extparm++ == 2) {
                    state = 3;           /* In data portion of extended parameter */
                }
            }
        }
        in++;
    }
    *op = 0;
    return out;
}

/*
 * Get a value of a header parameter.
 * The value can be unquoted, quoted, or in extended format.
 * In extended format, only UTF-8 is supported.
 */
const char * ism_http_getParameterValue(const char * hdr, const char * match, char * buf, int len) {
    int    state = 0;
    int    selected = 0;
    int    matchlen;
    const char * in;
    char * value = NULL;
    char * vp;
    char * op;
    int    extparm = 0;
    const char * param = hdr;
    char * out;

    if (!hdr || !match || !(matchlen = strlen(match)) || !buf)
        return NULL;
    out = alloca(strlen(hdr)+1);
    op = out;
    in = hdr;
    while (*in) {
        if (*in >= 'A' && *in <= 'Z' && state==0) {
            *op++ = (char)(*in|0x20);
        } else if ((uint8_t)*in > ' ') {
            *op++ = *in;
            if (*in == ';') {
                if (selected) {
                    op[-1] = 0;
                    break;
                } else {
                    state = 0;
                    value = NULL;
                }
                param = op;
            } else if (*in == '=') {
                value = op;
                if ((op-param) == (matchlen+1) && !memcmp(param, match, matchlen)) {
                    selected = 1;
                }
                if ((op-param)>1 && op[-2]=='*') {
                    extparm = 1;
                } else {
                    const char * np = in+1;
                    while (*np == ' ')
                        np++;
                    if (*np == '"') {    /* Quoted value */
                        np++;
                        value = vp = out;
                        while (*np && *np != '"') {
                            if (*np == '\\') {
                                if (np[1]) {
                                    /* If the next char is >0x80, just ignore the backslash */
                                    if ((uint8_t)np[1] >= 0x80 || (uint8_t)np[1]<0x20) {
                                        np++;
                                    } else {
                                        *vp++ = *++np;
                                    }
                                }
                            } else {
                                if ((uint8_t)*np >= 0x20)
                                    *vp++ = *np;
                            }
                            np++;
                        }
                        if (selected) {
                            op = vp;
                            break;
                        } else {
                            value = NULL;
                            selected = 0;
                        }
                    }
                }
            } else if (extparm && *in == '\'') {
                if (extparm++ == 2) {
                    state = 3;           /* In data portion of extended parameter */
                }
            }
        }
        in++;
    }

    if (value && selected) {
        *op = 0;
        /* TODO: process extended param */
        if (extparm) {
            int valuelen = strlen(value);
            if (valuelen > 6 && !memcmp(value, "utf-8'", 6)) {
                value = strchr(value+6, '\'');
                if (!value)
                    return NULL;
                value++;
                value = http_unescape(value);
            } else if (valuelen > 10 && !memcmp(value, "iso8859-1'", 10)) {
                /* TODO */
                return NULL;
            } else {
                return NULL;
            }

        }
        ism_common_strlcpy(buf, value, len);
        return buf;
    }
    return NULL;
}


/*
 * Create an http object
 */
ism_http_t * ism_http_newHttp(int http_op, const char * path, const char * query, const char * locale,
        char * data, int datalen, const char * datatype, char * hdrs, int hdrlen, int buflen) {
    ism_http_t * http;
    char * pos;
    int plen;
    int qlen;
    int llen;
    int tlen;
    int httplen;

    /* Get the length of the string params */
    plen = path ? strlen(path) : 0;
    qlen = query ? strlen(query) : 0;
    llen = locale ? strlen(locale) : 0;
    tlen = datatype ? strlen(datatype): 0;

    /*
     * The HTTP object is allocated as a single malloc so it can normally be freed with a single free.
     * In the case of multiple contents or expanded headers we will separately allocate and then must
     * separately free.
     */
    httplen = sizeof(ism_http_t) + buflen + datalen + plen + qlen + llen + tlen + 4 + hdrlen;
    http = ism_common_malloc(ISM_MEM_PROBE(ism_memory_http, 3),httplen);
    if (!http) {
        ism_common_setError(ISMRC_AllocateError);
        return NULL;
    }
    memset(http, 0, sizeof(ism_http_t));
    pos = (char *)(http+1);      /* Position after base object */

    http->http_op = http_op;
    http->max_age = -1;
    http->outbuf.buf = pos;
    http->outbuf.len = buflen;
    pos += buflen;

    /* Handle a single content.  Multiple content is handled later */
    if (datalen) {
        http->content = &http->single_content;
        http->single_content.content = pos;
        http->single_content.content_len = datalen;
        http->content_count = 1;
        memcpy(pos, data, datalen);
        pos += datalen;
    }

    /* Copy the path */
    if (path) {
        http->path = pos;
        memcpy(pos, path, plen+1);
        pos += (plen+1);
    }

    /* Copy the query */
    if (query) {
        http->query = pos;
        memcpy(pos, query, qlen+1);
        pos += (qlen+1);
    }

    /* Copy the locale */
    if (locale) {
        http->locale = pos;
        memcpy(pos, locale, llen+1);
        pos += (llen+1);
    }

    /* Copy the content type */
    if (tlen) {
        http->single_content.content_type = pos;
        ism_http_canonicalHeader(pos, datatype);
        pos += (tlen+1);
    }

    /* Copy the headers */
    if (hdrlen) {
        http->headers.buf = pos;
        http->headers.len = hdrlen;
        http->headers.used = hdrlen;
        memcpy(pos, hdrs, hdrlen);
        pos += hdrlen;
    }
    TRACE(8, "http %p len=%d %d\n", http, httplen, (int)(pos-(char *)http));
    return http;
}

/*
 * Put out custom headers
 */
static void putHeaders(ism_http_t * http, concat_alloc_t * buf) {
    ism_field_t field;
    const char * expose_hdr[32];
    int    expose_cnt = 0;
    int    i;
    concat_alloc_t map = http->out_headers;

    map.pos = 0;
    while (map.pos < map.used) {
        int rc = ism_protocol_getObjectValue(&map, &field);
        if (!rc && field.type == VT_Name) {
            const char * name = field.val.s;
            ism_protocol_getObjectValue(&map, &field);
            if (field.type == VT_String) {
                ism_common_allocBufferCopyLen(buf, name, strlen(name));
                ism_common_allocBufferCopyLen(buf, ": ", 2);
                ism_common_allocBufferCopyLen(buf, field.val.s, strlen(field.val.s));
                ism_common_allocBufferCopyLen(buf, "\r\n", 2);
                if (expose_cnt < 32)
                    expose_hdr[expose_cnt++] = name;
            }
        }
    }
    if (expose_cnt > 0) {
        ism_common_allocBufferCopy(buf, "Access-Control-Expose-Headers: ");
        buf->used--;
        for (i = 0; i < expose_cnt; i++) {
            if (i != 0)
                ism_common_allocBufferCopyLen(buf, ", ", 2);
            ism_common_allocBufferCopyLen(buf, expose_hdr[i], strlen(expose_hdr[i]));
        }
        ism_common_allocBufferCopyLen(buf, "\r\n", 2);
    }
}


/*
 * Respond to an http request.
 *
 * Send the response on the connection.  After this call the http object no longer belongs
 * to the invoker and can be freed up at any time.
 *
 * The data is http->outbuf is used as the content data.
 * If ism_http_sendPart() has been called, the contents of http->outbuf are ignored.
 *
 * This can be called in another thread than the one which received the request.
 *
 * The actual sending of the response may be asynchronous and the http object exists
 * until the ressponse is complete, but since the invoker cannot know when this will happen
 * it must not use this object after this call is made.
 *
 * @param  http     The http object
 * @param  http_rc  The http status code
 * @param  content_type  The http content type
 * @return An ISMRC return code
 */
int ism_http_respond(ism_http_t * http, int http_rc, const char * content_type) {
    ism_transport_t * transport = http->transport;
    char         xbuf[8192];
    char         cachebuf[32];
    char         date[80];
    const char * origin;
    const char * cachectl = "no-cache";
    const char * disp = (http->will_close || transport->at_server == 2) ? "close" : "keep-alive";

    concat_alloc_t buf = {xbuf, sizeof xbuf};

    if (http->outbuf.used == 0 && (http_rc == 404 || http_rc == 401)) {
        transport->fobj->frame->locale = http->locale;
        ism_common_setError(http_rc);
        wserror(transport, http_rc, http_status_str(http_rc), NULL, http->locale);
        ism_http_free(http);
        return 0;
    }

    /*
     * Default the content type
     */
    if (!content_type) {
        content_type = "text/plain;charset=utf-8";
        if (http->outbuf.used > 0 && ( http->outbuf.buf[0]=='{' || http->outbuf.buf[0]=='['))
            content_type = "application/json";
    }

    /*
     * Send the response
     */
    ism_http_time(date, sizeof date);
    if (g_strictTransportSecurity) {
       setStrictSecurity(transport, date, sizeof(date));
    }
    origin = transport->origin;
    if (!origin)
        origin = "*";
    if (http->max_age >= 0) {
        cachectl = cachebuf;
        sprintf(cachebuf, "max-age=%d", http->max_age);
    }
    int zrc = snprintf(xbuf, sizeof(xbuf), HTTP_RESPONSE, http_rc, http_status_str(http_rc),
                       getServerHTTPHeaderString(), date, origin, disp, cachectl, content_type,
                       http->outbuf.used);
    if (zrc < 0 || zrc >= (sizeof(xbuf))) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP request is too large", "", NULL);
        ism_http_free(http);
        return 0;
    }
    buf.used = strlen(xbuf);
    if (http->out_headers.used) {
        putHeaders(http, &buf);
    }
    ism_common_allocBufferCopyLen(&buf, "\r\n", 2);

    /* If it fits in the buffer send it as one write, otherwise use two */
    if ((http->outbuf.used + buf.used) < buf.len) {
        ism_common_allocBufferCopyLen(&buf, http->outbuf.buf, http->outbuf.used);
        if (SHOULD_TRACEC(9, Http)) {
            ism_common_allocBufferCopyLen(&buf, "", 1);
            buf.used--;
            TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf.buf);
        }
        transport->send(transport, buf.buf, buf.used, 0, 0);
    } else {
        if (SHOULD_TRACEC(9, Http)) {
            ism_common_allocBufferCopyLen(&buf, "", 1);
            buf.used--;
            TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf.buf);
        }
        transport->send(transport, buf.buf, buf.used, 0, 0);
        transport->send(transport, http->outbuf.buf, http->outbuf.used, 0, 0);
    }

    /*
     * Free up the http object
     */
    ism_http_free(http);
    if (transport->at_server == 2) {
        transport->close(transport, 0, 1, "HTTP connection close");
    }
    transport->at_server = 0;
    return 0;
}


/*
 * Respond to an http request with a file.
 *
 * This method is designed to allow large files to be written over http.
 * We start the send during this call, but we only complete it when there
 * is room in the send buffers.
 */
int ism_http_respondFile(ism_http_t * http, const char * filename) {
    ism_transport_t * transport = http->transport;
    char         xbuf[8192];
    char         date[80];
    char         cachebuf[32];
    char *       fbuf = NULL;
    const char * origin;
    const char * contype = "text/plain";
    const char * cachectl = "no-cache";
    const char * disp = transport->at_server == 2 ? "close" : "keep-alive";
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    int max_age = 0;
    int len = 0;
    const char * fname;
    int  http_rc = 404;

    /* Find the actual file name */
    fname = getFileByLocale(g_document_root, filename, NULL, xbuf, sizeof xbuf);
    if (fname) {
        len = getFileContent(g_document_root, fname, &fbuf);
        if (len) {
            http_rc = 200;
            contype = ism_http_getContentType(filename, &max_age);
            if (http->max_age >= 0)
                max_age = http->max_age;
        } else {
            ism_common_setError(ISMRC_HTTP_NotFound);
            wserror(transport, 404, "Not found", NULL, http->locale);
            return 0;
        }
    }

    /*
     * Send the response
     */
    ism_http_time(date, sizeof date);
    if (g_strictTransportSecurity) {
       setStrictSecurity(transport, date, sizeof(date));
    }
    origin = ism_http_getHeader(http, "Origin");
    if (!origin)
        origin = "*";
    if (max_age >= 0) {
        cachectl = cachebuf;
        sprintf(cachebuf, "max-age=%d", max_age);
    }
    int zrc = snprintf(xbuf, sizeof(xbuf), HTTP_RESPONSE, http_rc, http_status_str(http_rc),
            getServerHTTPHeaderString(), date, origin, disp, cachectl, contype,
            http->outbuf.used);
    if (zrc < 0 || zrc >= (sizeof(xbuf))) {
        ism_common_setError(ISMRC_HTTP_BadRequest);
        wserror(transport, 400, "The HTTP request is too large", "", NULL);
        if (fbuf)
            ism_common_free(ism_memory_http,fbuf);
        ism_http_free(http);
        return 0;
    }
    buf.used = strlen(xbuf);
    if (http->out_headers.used) {
        putHeaders(http, &buf);
    }
    ism_common_allocBufferCopyLen(&buf, "\r\n", 2);

    /* If it fits in the buffer send it as one write, otherwise use two */
    if ((len + buf.used + 1) < buf.len) {
        ism_common_allocBufferCopyLen(&buf, fbuf, len);
        if (SHOULD_TRACEC(9, Http)) {
            ism_common_allocBufferCopyLen(&buf, "", 1);
            buf.used--;
            TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf.buf);
        }
        transport->send(transport, buf.buf, buf.used, 0, 0);
    } else {
        if (SHOULD_TRACEC(9, Http)) {
            ism_common_allocBufferCopyLen(&buf, "", 1);
            buf.used--;
            TRACEX(9, Http, 0, "httpout connect=%u: [\n%s]\n", transport->index, buf.buf);
        }
        transport->send(transport, buf.buf, buf.used, 0, 0);
        transport->send(transport, fbuf, len, 0, 0);
    }

    /*
     * Free up the http object
     */
    if (fbuf)
        ism_common_free(ism_memory_http,fbuf);
    ism_http_free(http);
    if (transport->at_server == 2) {
        transport->close(transport, 0, 1, "HTTP connection close");
    }
    transport->at_server = 0;
    return 0;
}

/*
 * Get the value of a header field.
 *
 * Those header fields processed by the http parser are not included in this list.
 *
 * @param http   An http object
 * @param name  The name of the header
 * @return The value of the header or NULL to indicate it does not exist
 */
const char * ism_http_getHeader(ism_http_t * http, const char * name) {
    ism_field_t  f;
    char * xname;

    if (!http || !name)
        return NULL;
    xname = alloca(strlen(name))+2;
    *xname = ']';
    strcpy(xname+1, name);
    ism_common_findPropertyName(&http->headers, xname, &f);
    return f.type == VT_String ? f.val.s : NULL;
}

/*
 * Set a header on output
 */
void ism_http_setHeader(ism_http_t * http, const char * name, const char * value) {
    ism_field_t field = {0};
    if (value) {
        field.type = VT_String;
        field.val.s = (char *)value;
    }
    ism_common_putProperty(&http->out_headers, name, &field);
}


/*
 * Get the value of a cookie.
 *
 * @param http   An http object
 * @param name  The name of the cookie
 * @return The value of the cookie or NULL to indicate it does not exist
 */
const char * ism_http_getCookie(ism_http_t * http, const char * name) {
    ism_field_t  f;

    if (!http || !name)
        return NULL;
    ism_common_findPropertyName(&http->headers, name, &f);
    return f.type == VT_String ? f.val.s : NULL;
}

/*
 * Get the name of a header field by index.
 *
 * This is used to iterate thru the list of headers.  You should start with 0
 * and end with the first NULL response.
 * @param http   An http object
 * @param index  Which header
 * @return The name of the header or NULL to indicate there are not that many
 */
const char * ism_http_getHeaderByIndex(ism_http_t * http, int index) {
    const char * name;
    int   i = 0;

    if (!http || index < 0 || index >= http->header_count)
        return NULL;
    if (!http->header_props) {
        http->header_props = ism_common_newProperties(http->header_count + http->cookie_count);
        ism_common_deserializeProperties(&http->headers, http->header_props);
    }

    ism_common_getPropertyIndex(http->header_props, i++, &name);
    while (name) {
        if (*name == ']') {
            if (index == 0)
                return name+1;
            index--;
        }
        ism_common_getPropertyIndex(http->header_props, i++, &name);
    }
    return NULL;
}


/*
 * Get the name of a cookie by index.
 *
 * This is used to iterate thru the list of cookies.  You should start with 0
 * and end with the first NULL response.
 * @param http   An http object
 * @param index  Which cookie
 * @return The name of the header or NULL to indicate there are not that many
 */
const char * ism_http_getCookieByIndex(ism_http_t * http, int index) {
    const char * name;
    int   i = 0;

    if (!http || index < 0 || index >= http->header_count)
        return NULL;
    if (!http->header_props) {
        http->header_props = ism_common_newProperties(http->header_count + http->cookie_count);
        ism_common_deserializeProperties(&http->headers, http->header_props);
    }

    ism_common_getPropertyIndex(http->header_props, i++, &name);
    while (name) {
        if (*name != ']') {
            if (index == 0)
                return name;
            index--;
        }
        ism_common_getPropertyIndex(http->header_props, i++, &name);
    }
    return NULL;
}
