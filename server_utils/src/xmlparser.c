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

#include <setjmp.h>
#include <ismxmlparser.h>
#include <alloca.h>
#include <assert.h>

#ifdef IGNORE
#undef IGNORE
#endif

#define WHITESPACE 1
#define ATTRIB     2
#define CONTENT    3
#define TAGSTART   4
#define IGNORE     5
#define ENCODING   6
#define ENDTAG     7
#define TAGNAME    8
#define IGNOREGT   9
#define STARTDOC  10

/**
 * Define the size of DOM chunks to allocate
 */
#define DOM_ALLOC 65500

/**
 * Define the maximum depth of start elements.
 */
#define MAX_DEPTH 100


/**
 * Internal function prototypes
 */
static int unescape(xdom * dom, char * buf, int start, int end, int utf8);
static void logit(xdom * dom, int fileno, int line, int level, const char * msgid, const char * string, const char * repl);
static void fatalerror(xdom * dom, int rc, const char * msgid, const char * string, const char * repl);
static void warnerror(xdom * dom, const char * msgid, const char * string, const char * repl);
static void * domAlloc(xdom * dom, int size);
static void * domDup(xdom * dom, const char * str);
static xnode_t * newNode(xdom * dom, short type, char * name);
static int checkName(xdom * dom, char * name);
static void doContentX(xdom * dom, char * buf, int start, int end, int kind);
static void doContent(xdom * dom, char * buf, int start, int end);
static void doWhitespace(xdom * dom, char * buf, int start, int end);
static void doWhitespace_stream(xdom * dom, char * buf, int len);
static int getEntity(xdom * dom, char * name);
static int  doStartElement(xdom * dom, char * tag, char * attr);
static void doEndElement(xdom * dom, char * tag);
static int isWhitespace(int ch);
static int isEquals(int ch);
static char * parse_match(char * match, int maxlen, char * elem, int * count, char * attr, char * value);

/**
 * Define the actual structure of the DOM.
 * This is encapsulated away from any users but is defined here.
 *
 * By keeping a single position in the dom, it is possible to
 * remove almost all of the pointers from the node, and greatly
 * simplify it.
 *
 * Memory is allocated in chunks within the DOM.  Pointer are
 * into the in memory copy of the document.
 */
struct _xdom {
    int    Line;                      /**< Current line number (for errors)   */
    int    Level;                     /**< Node level (root = 1)              */
    xnode_t * First;                  /**< First node in tree                 */
    xnode_t * Node [MAX_DEPTH+2];     /**< Node hierarchy for next            */
    char * SystemId;                  /**< Normally the file name (for errors)*/
    char * DomChunk;                  /**< First allocated chunk              */
    char * DomAlloc;                  /**< Allocate position                  */
    int    DomLeft;                   /**< Space left in chunk                */
    int    warnings;                  /**< Warning count                      */
    int    jmpset;                    /**< Is error return set ?              */
    jmp_buf env;                      /**< Error return register save area    */
    ism_xml_log_t  log;        /**< Log function                       */
    ism_xml_logx_t logx;       /**< Extended log function              */
    ism_xml_callback_t sax;    /**< SAX-like callbacks                 */
    int    callback;                  /**< Which callbacks                    */
    char * copybuf;                   /**< Copy input buffer                  */
    int    options;                   /**< Options                            */
    const char * msgid_prefix;        /**< Message ID prefix                  */
    int    fileno;                    /**< File number                        */
    void * userdata;
    char * * fileNames;               /**< File name array                    */
    char   SIDBuf[16];                /**< Buffer for short SystemID          */
    char   encoding[32];              /**< Character encoding                 */
};

#define XDOM_LEN ((sizeof(xdom)+7)&~7)

/*
 * Structure for save and restore of position
 */
struct xdompos_t {
    int    valid;                     /**< Save is valid if non-zero           */
    int    Level;                     /**< Node level (root = 1)               */
    xdom * dom;                       /**< First node in tree                  */
    xnode_t * Node [MAX_DEPTH+2];     /**< Node hierarchy for next             */
};

/*
 * Structure for buffer getch
 */
typedef struct xbufio_t {
    char * buf;
    int    len;
    int    pos;
} xbufio_t;

/*
 * Structure to map automatic encoding detection
 */
typedef struct {
    int  offset;
    int  len;
    char map[4];
    ism_xml_getch_t func;
} xdefault_map_t;

xdefault_map_t xdefault_map [] = {
    {0, 4, "\0\0\xfe\xff", ism_xml_getch_utf32be},
    {0, 4, "\xff\xfe\0\0", ism_xml_getch_utf32le},
    {0, 2, "\xfe\xff",     ism_xml_getch_utf16be},
    {0, 2, "\xff\xfe",     ism_xml_getch_utf16le},
    {0, 3, "\0\0\0",       ism_xml_getch_utf32be},
    {1, 3, "\0\0\0",       ism_xml_getch_utf32le},
    {0, 1, "\0",           ism_xml_getch_utf16be},
    {1, 1, "\0",           ism_xml_getch_utf16le},
    {1, 1, "\0",           ism_xml_getch_utf16le},
    {0, 3, "\xef\xbb\xbf", ism_xml_getch_utf8   },
    {0, 0, "", NULL }
};


void ism_xml_log(xdom * dom, xnode_t * node, int level, const char * msgid, const char * string, const char * repl) {
    logit(dom, node->fileno, node->line, level, msgid, string, repl);
}


/*
 * Log a message
 */
static void logit(xdom * dom, int fileno, int line, int level, const char * msgid, const char * string, const char * repl) {
    char logmsgid[16];
    char logbuf[1024];
    char * src = dom->SystemId;

    if (strlen(msgid)>3) {
        strcpy(logmsgid, msgid);
    } else {
        strcpy(logmsgid, dom->msgid_prefix);
        strcat(logmsgid, msgid);
    }
    if (dom->fileNames) {
        src = dom->fileNames[fileno];
        if (!src) src = dom->SystemId;
    }
    snprintf(logbuf, 1024, "%s:%d - %s%s", src, line, string, repl ? repl : "");
    dom->logx(dom, level, logmsgid, logbuf);
}


/*
 * Default logging.  The user should replace this routine.
 */
static int  logcallx(xdom * dom, int kind, const char * msgid, const char * msg) {
    char  buf[1024];
    snprintf(buf, 1024, "%s%c|%s", msgid, kind, msg);
    if (dom->log) {
        dom->log(kind, buf);
    } else {
        fputs(buf, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
    return 0;
}


/*
 * In the case of a fatal error
 */
static void fatalerror(xdom * dom, int rc, const char * msgid, const char * string, const char * repl) {
    logit(dom, dom->fileno, dom->Line, 'E', msgid, string, repl);
    longjmp(dom->env, rc);
}


/*
 * Warning or error depending on flag settings
 */
static void warnerror(xdom * dom, const char * msgid, const char * string, const char * repl) {
    if (dom->options & XOPT_Error) {
        fatalerror(dom, 3, msgid, string, repl);
    } else {
        logit(dom, dom->fileno, dom->Line, 'E', msgid, string, repl);
        dom->warnings++;
    }
}


/*
 * Allocate from DOM space
 * If the requested size is a multiple of 8 bytes, it is returned 8 byte aligned
 * Note that when running buffer based, almost everything is 8 byte multiples.
 */
static void * domAlloc(xdom * dom, int size) {
    char * p = dom->DomAlloc;
    int    pad = 0;

    /*
     * If the request size is an 8 byte multiple, align the allocation
     */
    if ((size&0x7) == 0) {
        pad = (int)((uintptr_t)p & 0x7);
        if (pad)
            pad = 8-pad;
    }

    /*
     * If the current chunk is not big enough, allocate a new one
     */
    if (dom->DomLeft < (size+pad)) {
        char * newdom = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_xml,3),DOM_ALLOC);
        if (!newdom) {
            fatalerror(dom, 5, "05", "Unable to allocate memory.", NULL);
            return NULL;
        }
        *(char * *)newdom = dom->DomChunk;
        dom->DomChunk = newdom;
        p = dom->DomAlloc = dom->DomChunk + sizeof(char *);
        pad = (int)((uintptr_t)p & 0x7);
        dom->DomLeft  = DOM_ALLOC-(sizeof(char *) + pad);
    }

    /*
     * Allocate the bytes
     */
    p = dom->DomAlloc + pad;
    memset(p, 0, size);
    dom->DomAlloc += (size + pad);
    dom->DomLeft  -= (size + pad);
    return (void *)p;
}


/*
 * Duplicate a string into DOM memory
 */
static void * domDup(xdom * dom, const char * str) {
    int    len = strlen(str)+1;
    char * mem = domAlloc(dom, len);
    memcpy(mem, str, len);
    return mem;
}

/*
 * Allocate a new node
 */
static xnode_t * newNode(xdom * dom, short type, char * name) {
    xnode_t * n = (xnode_t *) domAlloc(dom, sizeof(xnode_t));
    n->type = (char)type;
    n->name = name;
    n->line = dom->Line;
    n->fileno = (uint8_t)dom->fileno;
    return n;
}

/*
 * Character classification table
 */
static char chTable[256] =  {
/*  0 1 2 3 4 5 6 7 8 9 A B C D E F        */
    0,0,0,0,0,0,0,0,0,4,4,0,0,4,0,0,  /* 0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 1 */
    4,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,  /* 2 */
    1,1,1,1,1,1,1,1,1,1,3,0,0,0,0,0,  /* 3 */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* 4 */
    3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,3,  /* 5 */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* 6 */
    3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,  /* 7 */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* 8 */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* 9 */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* A */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* B */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* C */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* D */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* E */
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,  /* F */
};


/*
 * Check a name for valid characters.
 * As we do not keep a full Unicode type table as required by conforming XML, we
 * cheat here by only doing checking on the ASCII7 portion of the name.  Any
 * character above 0x80 is assumed to be a valid name character.
 */
static int checkName(xdom * dom, char * name) {
    /* Handle entities in names */
    if (strchr(name, '&')) {
        unescape(dom, name, 0, strlen(name)+1, 1);
    }

    /* Check names if requested */
    if (!(dom->options & XOPT_NoCheckNames)) {
        char * cp = name;
        if (!(chTable[(uint32_t)*cp]&2)) {
            warnerror(dom, "09", "Invalid start character of name: ", name);
            return 1;
        }
        while (*cp) {
            if (!(chTable[(uint32_t)*cp]&1)) {
                warnerror(dom, "10", "Invalid character in name: ", name);
                return 1;
            }
            cp++;
        }
    }

    /* Remove namespace if requested */
    if ((dom->options & XOPT_NoNamespace) && strchr(name, ':')) {
        char * np = name;
        char * ep = name+strlen(name);
        while (ep[-1] != ':') ep--;
        while (*ep) *np++ = *ep++;
        *np = 0;
    }
    return 0;
}


/*
 * Common method for settin a content like node
 */
static void doContentX(xdom * dom, char * buf, int start, int end, int kind) {
    xnode_t * n;
    int len = unescape(dom, buf, start, end, 1);
    buf[start+len] = 0;
    n = newNode(dom, kind, buf+start);
    if (dom->sax && (dom->callback&XT_Content)) {
        int rc = dom->sax(dom, n, XT_Content);
        if (rc)
            return;
    }
    if (dom->Level==0 && !(dom->options & XOPT_RootContent)) {
        warnerror(dom, "17", "Content found outside root element", NULL);
        return;
    }
    if (!dom->Node[dom->Level]->child) {
        dom->Node[dom->Level]->child = n;
    } else {
        dom->Node[dom->Level+1]->sibling = n;
    }
    dom->Node[dom->Level+1] = n;
}


/*
 * Create a content node
 */
static void doContent(xdom * dom, char * buf, int start, int end) {
    doContentX(dom, buf, start, end, 'c');
}


/*
 * Ignore whitespace
 */
static void doWhitespace(xdom * dom, char * buf, int start, int end) {
    if ((dom->options&XOPT_Whitespace) && dom->Level > 0) {
        doContentX(dom, buf, start, end, 'w');
    }
}


/*
 * Ignore whitespace
 */
static void doWhitespace_stream(xdom * dom, char * buf, int len) {
    if ((dom->options&XOPT_Whitespace) && dom->Level > 0) {
        char * mem = domAlloc(dom, len);
        memcpy(mem, buf, len);
        doContentX(dom, mem, 0, len, 'w');
    }
}


/*
 * Create an element node
 */
static int  doStartElement(xdom * dom, char * tag, char * attr) {
    xnode_t * n;
    if (checkName(dom, tag))
        return 1;
    n = newNode(dom, 'e', tag);
    if (!attr || !*attr) {
        n->count = 0;
        n->attr  = NULL;
    } else {
        n->count = -2;
        n->attr = (xATTR *)attr;
        if (!(dom->options & XOPT_DelayAttr)) {
            ism_xml_parseAttributes(dom, n);
        }
    }
    if (dom->sax && (dom->callback&XT_StartElement)) {
        int rc = dom->sax(dom, n, XT_StartElement);
        if (rc)
            return 1;
    }
    if (dom->Level >= MAX_DEPTH) {
        fatalerror(dom, 4, "06", "Too many start elements: ", tag);
    }
    if (!dom->Node[dom->Level]->child) {
        dom->Node[dom->Level]->child = n;
    } else {
        dom->Node[dom->Level+1]->sibling = n;
    }
    dom->Node[++dom->Level] = n;
    return 0;
}


/*
 * Process the end of an element
 */
static void doEndElement(xdom * dom, char * tag) {
    int  i;

    if (checkName(dom, tag))
        return;
    if (strcmp(dom->Node[dom->Level]->name, tag)) {
        for (i = dom->Level; i>0; i--) {
            if (!strcmp(dom->Node[i]->name, tag))
                break;
        }
        if (i != dom->Level) {
            char * buf = (char *)alloca(1024);
            if (i != 0) {
                snprintf(buf, 1024, "Missing end element: [%s] [%s]",
                         dom->Node[dom->Level]->name, tag);
                warnerror(dom, "07", buf, NULL);
                dom->Level = i;
            } else {
                int rc = 0;
                if (dom->sax && (dom->callback&XT_EndElement)) {
                    /*
                     * If this is an unmatched element, it may be because we
                     * did not insert the start element.  In this case call
                     * the sax end element to see if the end tag should also
                     * be ignored.
                     */
                    xnode_t * n = newNode(dom, 'e', tag);
                    rc = dom->sax(dom, n, XT_EndElement);
                }
                if (rc)
                    return;
                snprintf(buf, 1024, "Unmatched end element: [%s] [%s]",
                         dom->Node[dom->Level]->name, tag);
                warnerror(dom, "08", buf, NULL);
                return;

            }
        }
    }
    if (dom->sax && (dom->callback&XT_EndElement)) {
        int rc = dom->sax(dom, dom->Node[dom->Level], XT_EndElement);
        if (rc)
            return;
    }
    dom->Level--;
}

/*
 * Set the xml log file
 */
void   ism_xml_setlog(xdom * dom, ism_xml_log_t logcx) {
    dom->log = logcx;
}

/*
 * Set the xml log file
 */
void   ism_xml_setlogx(xdom * dom, ism_xml_logx_t logcx) {
    dom->logx = logcx ? logcx : &logcallx;
}


int    ism_xml_setCallback(xdom * dom, ism_xml_callback_t callback, int kind) {
    dom->sax = callback;
    dom->callback = kind;
    return 0;
}

/*
 * Return the value of a standard entity.  If the name is not
 * matched, return 0xffff
 */
static int getEntity(xdom * dom, char * name) {
    /*
     * Handle numeric entities.
     */
    if (name[0] == '#') {
        char * ep;
        int    val;
        if (name[1]=='x') {
            if (name[2]) {
                val = strtoul(name+2, &ep, 16);
                if (val && !*ep)
                    return val;
            }
        } else {
            if (name[1]) {
                val = strtoul(name+1, &ep, 10);
                if (val && !*ep)
                    return val;
            }
        }
    }
    /*
     * Handle standard entities.
     */
    else {
        if (!strcmp(name, "amp"))
            return '&';
        if (!strcmp(name, "lt"))
            return '<';
        if (!strcmp(name, "gt"))
            return '>';
        if (!strcmp(name, "quot"))
            return '"';
        if (!strcmp(name, "apos"))
            return '\'';
    }
    if (dom && dom->jmpset)
        warnerror(dom, "16", "Unknown entity: ", name);
    return 0xffff;
}

/*
 * Convert char to utf-8 into a buffer.
 */
static int toutf8(int ch, char * buf, int pos, int left) {
    if ((unsigned int)ch > 0x10FFFF || left<1) {
        return pos;
    }
    if (ch < 0x80) {
        buf[pos++] = ch;
    } else {
        if (ch >= 0x10000) {
            if (left < 4)
                return pos;
            buf[pos++] = 0xf0 | ((ch>>18) & 0x07);
            buf[pos++] = 0x80 | ((ch>>12) & 0x3f);
            buf[pos++] = 0x80 | ((ch>>6) & 0x3f);
        }
        else if (ch >= 0x0800) {
            if (left < 3)
                return pos;
            buf[pos++] = 0xe0 | ((ch>>12) & 0x0f);
            buf[pos++] = 0x80 | ((ch>>6) & 0x3f);
        }
        else if (ch >= 0x80) {
            if (left < 2)
                return pos;
            buf[pos++] = 0xc0 | ((ch>>6) & 0x1f);
        }
        buf[pos++] = 0x80 | (ch & 0x3f);
    }
    return pos;
}


/*
 * Unescape the content in place.  The unescaped string is always
 * equal to or shorter than the escaped string.
 */
static int unescape(xdom * dom, char * buf, int start, int end, int utf8) {
    int  ch;
    int i, tag;
    int pos = start;
    for (i=start; i<end; i++) {
        ch = buf[i];
        if (buf[i]=='&') {
            for (tag=i+2; tag<end; tag++) {
                if (buf[tag]==';') {
                    buf[tag] = 0;
                    ch = getEntity(dom, buf+i+1);
                    buf[tag] = ';';
                    if (!utf8 && ch > 0xff)
                        ch = 0xffff;
                    if (ch != 0xffff) {
                        if (!utf8) {
                            buf[pos++] = (char)ch;
                        } else {
                            /* Convert to UTF-8 */
                            int newpos = toutf8(ch, buf, pos, 4);
                            if (newpos == pos)
                                buf[pos++] = '&';
                            else {
                                i = tag;
                                pos = newpos;
                            }
                        }
                    } else
                        buf[pos++] = '&';
                    break;
                }
            }
        } else {
            buf[pos++] = ch;
        }
    }
    return pos-start;
}


/*
 * External version of unescape
 */
int ism_xml_unescape(xdom * dom, char * buf, int utf8) {
    int len = unescape(dom, buf, 0, strlen(buf), utf8);
    buf[len] = 0;
    return len;
}


/*
 * Check for whitespace
 */
static int isWhitespace(int ch) {
    return ((unsigned int)ch <= ' ') ? chTable[ch]&4 : 0;
}


/*
 * Check for equals or whitespace
 */
static int isEquals(int ch) {
    return (ch=='=') ? 1 : isWhitespace(ch);
}


/*
 * xml object constructor
 */
xdom * ism_xml_new(char * systemId) {
    xdom * dom = (xdom *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_xml,4),DOM_ALLOC);

    if (!dom) {
        return NULL;
    }
    memset(dom, 0, sizeof(xdom));
    dom->DomAlloc = ((char *)dom)+XDOM_LEN;
    dom->DomLeft  = DOM_ALLOC-XDOM_LEN;
    dom->First = newNode(dom, 'e', "");
    dom->Node[0] = dom->First;
    if (!systemId)
        systemId = "xml";
    if (strlen(systemId)<16) {
        strcpy(dom->SIDBuf, systemId);
        dom->SystemId = dom->SIDBuf;
    } else {
        dom->SystemId = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_xml,1000),systemId);
    }
    dom->logx = &logcallx;
    dom->Line = 1;
    ism_xml_setLogPrefix(dom, NULL);
    return dom;
}


/*
 * Return the systemId.  This is normally the file name, but it can be anything.
 * It is really just used for error messages.
 */
char * ism_xml_getSystemId(xdom * dom) {
    return dom->SystemId;
}

/*
 * Set the message prefix
 */
void ism_xml_setLogPrefix(xdom * dom, const char * msgp) {
    dom->msgid_prefix = (msgp && strlen(msgp)<13) ? msgp : "FMDFS0";
}


/*
 * Return the current level.
 * Level 1 is the root.
 */
int ism_xml_getLevel(xdom * dom) {
    return dom->Level;
}


/*
 * Return the current level.
 * Level 1 is the root.
 */
int ism_xml_getLine(xdom * dom) {
    return dom->Line;
}


/*
 * Return the current file number
 */
int ism_xml_getFileno(xdom * dom) {
    return dom->fileno;
}


/*
 * Set the system identifier and location
 */
void ism_xml_setSystemId(xdom * dom, const char * systemId, int line, int fileno) {
    if (fileno > 255)
        fileno = 255;
    if (systemId) {
        if (!dom->fileNames) {
            dom->fileNames = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_xml,5),sizeof(char *), 512);
            // SA_ASSUME(dom->fileNames != NULL);
            dom->fileNames[0] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_xml,1000),dom->SystemId ? dom->SystemId : "");
        }
        if (!dom->fileNames[fileno])
            dom->fileNames[fileno] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_xml,1000),systemId);
        if (!strcmp(dom->fileNames[fileno], systemId)) {
            ism_common_free(ism_memory_utils_xml,dom->fileNames[fileno]);
            dom->fileNames[fileno] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_xml,1000),systemId);
        }
    }
    if (line >= 0)
        dom->Line = line;
    if (fileno >= 0)
        dom->fileno = fileno;
}


/*
 * Return the source name for the specified file
 */
const char * ism_xml_getSourceName(xdom * dom, xnode_t * node) {
    const char * ret;
    if (!dom->fileNames)
        return dom->SystemId;
    ret = (const char *)dom->fileNames[node->fileno];
    return ret ? ret : dom->SystemId;
}


/*
 * xml Object de-constructor
 */
void ism_xml_free(xdom * dom) {
    int    i;
    char * d = dom->DomChunk;
    char * nd;
    while (d) {
        nd  = *(char * *)d;
        ism_common_free(ism_memory_utils_xml,d);
        d = nd;
    }
    dom->DomChunk = 0;
    if (dom->copybuf) {
        ism_common_free(ism_memory_utils_xml,dom->copybuf);
        dom->copybuf = NULL;
    }
    if (dom->SystemId && dom->SystemId != dom->SIDBuf) {
        ism_common_free(ism_memory_utils_xml,dom->SystemId);
        dom->SystemId = NULL;
    }
    if (dom->fileNames) {
        for (i=0; i<512; i++) {
            if (dom->fileNames[i]) {
                ism_common_free(ism_memory_utils_xml,dom->fileNames[i]);
                dom->fileNames[i] = NULL;
            }
        }
        ism_common_free(ism_memory_utils_xml,dom->fileNames);
        dom->fileNames = NULL;
    }
    ism_common_free(ism_memory_utils_xml,dom);
}


/*
 * Include a file
 */
int ism_xml_include(xdom * dom, char * name, int fileno) {
    FILE * f;
    int len, bread;
    char * savesysid;
    int    saveline, savefileno, rc;
    char * buf;

    f = fopen(name, "rb");
    if (!f) {
        warnerror(dom, "21", "Unable to open file: ", name);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_xml,13),len+2);
    if (!buf) {
        warnerror(dom, "22", "Unable to allocate memory.", NULL);
        fclose(f);
        return -2;
    }
    rewind(f);
    bread = fread(buf, 1, len, f);
    buf[bread] = 0;
    buf[bread+1] = 0;                                        /* Double null*/
    if (bread != len) {
        warnerror(dom, "23", "Unable to read file: ", name);
        ism_common_free(ism_memory_utils_xml,buf);
        fclose(f);
        return -3;
    }
    fclose(f);

    /*
     *
     */
    savesysid = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_xml,1000),ism_xml_getSystemId(dom));
    saveline  = ism_xml_getLine(dom);
    savefileno = ism_xml_getFileno(dom);
    ism_xml_setSystemId(dom, name, 1, fileno);
    if (fileno < 256) {
        dom->fileNames[fileno+256] = buf;     /* Remember buffer to free it*/
    }
    rc = ism_xml_parse(dom, buf, len, 0);
    ism_xml_setSystemId(dom, savesysid, saveline, savefileno);
    ism_common_free(ism_memory_utils_xml,savesysid);
    return rc;
}


/*
 * Parse the XML file.  This parses the file into element and content node.
 */
int ism_xml_parse (xdom * dom, char * buf, int len, int copy) {
    int  ch, prevch;
    int  mode;
    int  start = 0;
    int  i;
    int  rc;
    char * encoding = NULL;
    char * tag = NULL;
    int  resetjmp = 0;
    int  savewarnings;

    /*
     * If this is not a pure UTF-8 file, process it using stream.
     */
    if (buf[0] != '<' || buf[1]==0) {
        xdefault_map_t * dm = xdefault_map;
        while (dm->len) {
            if ((dm->offset + dm->len) <= len &&
                !memcmp(buf+dm->offset, dm->map, dm->len)) {
                xbufio_t xbuf;
                xbuf.buf = buf;
                xbuf.len = len;
                xbuf.pos = (dm->offset+dm->len);
                return ism_xml_parse_stream(dom, dm->func, (void *)&xbuf);
            }
            dm++;
        }
    }

    /*
     * Check the encoding string and process iso8859-1
     */
    if (buf[1]=='?') {
        encoding = ism_xml_findEncoding(dom, buf, len);
        if (encoding) {
            if (!strcasecmp(encoding, "iso-8859-1") ||
                !strcasecmp(encoding, "iso8859-1")) {
                xbufio_t xbuf;
                xbuf.buf = buf;
                xbuf.len = len;
                xbuf.pos = 0;
                return ism_xml_parse_stream(dom, ism_xml_getch_latin1, (void *)&xbuf);
            }
        }
    }

    /*
     * Set up exception handling
     */
    if (!dom->jmpset) {
        rc = setjmp(dom->env);
        if (rc) {
            dom->jmpset = 0;
            return rc;
        }
        savewarnings = 0;
        dom->warnings = 0;
        dom->jmpset = 1;
        resetjmp = 1;
    } else {
        savewarnings = dom->warnings;
    }

    /*
     * Check for a valid encoding
     */
    if (encoding) {
        if (strcmp(encoding, "utf-8") && strcmp(encoding, "UTF-8")) {
            warnerror(dom, "20", "Unknown encoding: ", encoding);
        }
    }

    /*
     * Copy buffer if requested
     */
    if (copy) {
        dom->copybuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_xml,16),len+1);
        // SA_ASSUME(dom->copybuf != NULL);
        memcpy(dom->copybuf, buf, len);
        buf = dom->copybuf;
        buf[len] = 0;
    }

    /*
     * Loop for all characters in the source
     */
    ch = 0;
    mode = (dom->options & XOPT_RootContent) ? CONTENT : STARTDOC;
    for (i=0; i<(uint32_t)len; i++) {
        prevch = ch;
        ch = buf[i];
        if (ch == 0x0a) dom->Line++;

        /* State table parse */
        switch (mode) {
        case ATTRIB:
            if (ch == '>') {
                if (prevch == '/') {
                    buf[i-1] = 0;
                    if (!doStartElement(dom, tag, buf+start)) {
                        doEndElement(dom, tag);
                        if (dom->Level==0 && !(dom->options&XOPT_MultiRoot))
                            i = len;
                    }
                } else {
                    buf[i] = 0;
                    doStartElement(dom, tag, buf+start);
                }
                mode = WHITESPACE;
                start = i+1;
            }
            break;

        case WHITESPACE:
            if (!isWhitespace(ch)) {
                if (start<i)
                    doWhitespace(dom, buf, start, i);
                if (ch == '<') {
                    mode = TAGSTART;
                    start = -1;
                } else {
                    mode = CONTENT;
                    start = i;
                }
            }
            break;

        case STARTDOC:
            if (ch != '<') {
                warnerror(dom, "17", "Content found outside root element", NULL);
                mode = CONTENT;
            }
            /* fall thru */

        case CONTENT:
            if (ch == '<') {
                int last = i-1;
                while (last>=start && isWhitespace(buf[last]))
                    last--;
                if (start<=last)
                    doContent(dom, buf, start, last+1);
                if (last != i-1)
                    doWhitespace(dom, buf, last+1, i);
                start = -1;
                mode = TAGSTART;
            }
            break;

        case TAGSTART:
            if (ch == '/') {
                mode = ENDTAG;
                start = i+1;
            }
            else if (ch == '!' || ch=='?') {
                mode = IGNORE;
            }
            else {
                mode = TAGNAME;
                start = i;
            }
            break;

        case IGNORE:
            if (ch == '>' && (prevch=='?' || prevch=='-' || prevch==']')) {
                mode = WHITESPACE;
                start = i+1;
            }
            break;

        case IGNOREGT:
            if (ch == '>')  {
                mode = WHITESPACE;
                start = i+1;
            }
            break;

        case ENDTAG:
            if (isWhitespace(ch) || ch=='>') {
                tag = buf+start;
                buf[i] = 0;
                doEndElement(dom, tag);
                if (dom->Level==0 && !(dom->options&XOPT_MultiRoot))
                    i = len;
                mode = (ch=='>') ? WHITESPACE : IGNOREGT;
            }
            break;

        case TAGNAME:
            if (ch=='>') {
                tag = buf+start;
                buf[i] = 0;
                doStartElement(dom, tag, "");
                mode = WHITESPACE;
                start = i+1;
            }
            if (ch =='/') {
                tag = buf+start;
                buf[i] = 0;
                if (!doStartElement(dom, tag, "")) {
                    doEndElement(dom, tag);
                    if (dom->Level==0 && !(dom->options&XOPT_MultiRoot))
                        i = len;
                }
                mode = IGNOREGT;
                start = -1;
            }
            if (isWhitespace(ch)) {
                tag = buf+start;
                buf[i] = 0;
                mode = ATTRIB;
                start = i+1;
            }
            break;
        }
    }
    if (resetjmp) {
        dom->jmpset = 0;
    }
    if (dom->warnings) {
        dom->warnings += savewarnings;
        return 1;
    }
    dom->warnings += savewarnings;
    return 0;
}




/*
 * Parse the XML file.  This parses the file into element and content node.
 */
int ism_xml_parse_stream(xdom * dom, ism_xml_getch_t get_ch, void * parm) {
    int  ch, prevch;
    int  mode;
    char * tag = NULL;
    char * buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_xml,17),32700);
    char * bufend = buf+32700;
    char * start = NULL;
    char * bufp;
    int    len;
    int  resetjmp = 0;
    int  savewarnings;
    int  rc;

    if (!dom->jmpset) {
        rc = setjmp(dom->env);
        if (rc) {
            dom->jmpset = 0;
            ism_common_free(ism_memory_utils_xml,buf);
            return rc;
        }
        savewarnings = 0;
        dom->warnings = 0;
        dom->jmpset = 1;
    } else {
        savewarnings = dom->warnings;
    }

    prevch = 0;
    mode = (dom->options & XOPT_RootContent) ? CONTENT : STARTDOC;
    ch = get_ch(parm);
    bufp =  buf;
    start = buf;
    while (ch > 0) {
        if (ch == 0x0a) dom->Line++;
        /* Convert char to utf-8 */
        if (ch > 0x10ffff) {
            char xbuf[12];
            sprintf(xbuf, "0x%06X", ch);
            warnerror(dom, "19", "Invalid character: ", xbuf);
            ch = ' ';
        }
        len = toutf8(ch, bufp, 0, (int)(bufend-bufp));
        if (len == 0) {
            fatalerror(dom, 5, "18", "Working memory exceeded.", NULL);
        }

        /* State table parse */
        switch (mode) {
        case ATTRIB:
            if (ch == '>') {
                if (prevch == '/') {
                    bufp[-1] = 0;
                    tag = domDup(dom, tag);
                    while (isWhitespace(*start)) start++;
                    if (*start)
                        start = domDup(dom, start);
                    if (!doStartElement(dom, tag, start)) {
                        doEndElement(dom, tag);
                        if (dom->Level==0)
                            ch = -1;
                    }
                } else {
                    bufp[0] = 0;
                    while (isWhitespace(*start)) start++;
                    if (*start)
                        start = domDup(dom, start);
                    doStartElement(dom, domDup(dom, tag), start);
                }
                mode = WHITESPACE;
                bufp = buf;
                start = bufp;
            } else {
                bufp += len;
            }
            break;

        case WHITESPACE:
            if (!isWhitespace(ch)) {
                if (start < bufp)
                    doWhitespace_stream(dom, start, (int)(bufp-start));
                if (ch == '<') {
                    mode = TAGSTART;
                    start = buf;
                    bufp = buf;
                } else {
                    mode = CONTENT;
                    start = buf;
                    memcpy(buf, bufp, len);
                    bufp  = buf + len;
                }
            } else {
                bufp += len;
            }
            break;

        case STARTDOC:
            if (ch != '<') {
                warnerror(dom, "17", "Content found outside root element", NULL);
                mode = CONTENT;
            }
            /* fall thru */

        case CONTENT:
            if (ch == '<') {
                char * last = bufp-1;
                while (last>=start && isWhitespace(*last))
                    last--;
                last++;
                if (start<last) {
                    int    l = (int)(last-start);
                    char * mem = domAlloc(dom, l+1);
                    memcpy(mem, start, l+1);
                    doContent(dom, mem, 0, l);
                }
                if (last != (bufp-1))
                    doWhitespace_stream(dom, last, (int)(bufp-last)-1);
                start = NULL;
                mode = TAGSTART;
                bufp = buf;
                start = bufp;
            } else {
                bufp += len;
            }
            break;

        case TAGSTART:
            if (ch == '/') {
                mode = ENDTAG;
                bufp = buf;
                start = buf;
            }
            else if (ch == '!' || ch=='?') {
                mode = IGNORE;
                bufp = buf;
            }
            else {
                bufp += len;
                mode = TAGNAME;
            }
            break;

        case IGNORE:
            if (ch == '>' && (prevch=='?' || prevch=='-' || prevch==']')) {
                mode = WHITESPACE;
            }
            bufp = buf;
            start = NULL;
            break;

        case IGNOREGT:
            if (ch == '>' ) {
                mode = WHITESPACE;
            }
            bufp = buf;
            start = NULL;
            break;

        case ENDTAG:
            if (isWhitespace(ch) || ch=='>') {
                tag = start;
                bufp[0] = 0;
                doEndElement(dom, domDup(dom, tag));
                if (dom->Level==0)
                    ch = -1;
                mode = (ch=='>') ? WHITESPACE : IGNOREGT;
                bufp = buf;
            } else {
                bufp += len;
            }
            break;

        case TAGNAME:
            if (ch=='>') {
                bufp[0] = 0;
                tag = domDup(dom, start);
                doStartElement(dom, tag, NULL);
                mode = WHITESPACE;
                bufp = buf;
                start = bufp;
            }
            if (ch =='/') {
                bufp[0] = 0;
                tag = domDup(dom, start);
                if (!doStartElement(dom, tag, NULL)) {
                    doEndElement(dom, tag);
                    if (dom->Level==0)
                       ch = -1;
                }
                mode = IGNOREGT;
                ch   = '-';
                start = NULL;
                bufp = buf;
            }
            if (isWhitespace(ch)) {
                tag = start;
                *bufp++ = 0;
                mode = ATTRIB;
                start = bufp;
            } else {
                bufp += len;
            }
            break;
        }
        prevch = ch;
        if (ch >= 0)
            ch = get_ch(parm);
    }
    if (resetjmp) {
        dom->jmpset = 0;
    }
    if (dom->warnings) {
        dom->warnings += savewarnings;
        return 1;
    }
    dom->warnings += savewarnings;
    return 0;
}


/*
 * Parse the attributes.  Attributes are delay parsed and this function
 * converts from the string from to the key=value form.  The attribute string
 * is always kept in target encoding.
 */
void ism_xml_parseAttributes(xdom * dom, xnode_t * node) {
    int  count, len;
    char *  ap, * name, * value;
    char    ch;
    xATTR * atts;

    if (node->count >= 0)
        return;
    if (node->attr == NULL) {
        node->count = 0;
        return;
    }
    if (node->count == -2) {
        node->attr = (xATTR *)(char *)node->attr;
    }

    /* Determine probable attribute count */
    count = 0;
    for (ap=(char *)node->attr; *ap; ap++) {
        if (*ap=='=')
            count++;
    }
    atts = (xATTR *) domAlloc(dom, count * sizeof(xATTR));

    node->count = 0;
    ap = (char *)node->attr;
    node->attr = atts;

    while (*ap) {
        while (isWhitespace(*ap)) ap++;
        if (!*ap)
            break;
        name = ap;
        while (!isEquals(*ap) && !isWhitespace(*ap)) ap++;
        ch = *ap;
        *ap++ = 0;
        if (isWhitespace(ch)) {
            while (isWhitespace(*ap)) ap++;
            ch = *ap++;
        }
        if (!*ap) {
            if (dom->jmpset)
                warnerror(dom, "11", "Attribute name syntax error: ", name);
            break;
        }
        checkName(dom, name);
        if (ch != '=') {
            while (isWhitespace(*ap)) ap++;
            if (*ap != '=') {
                if (dom->jmpset)
                    warnerror(dom, "12", "Attribute syntax error: ", name);
                break;
            }
        }
        while (isWhitespace(*ap)) ap++;
        if (!*ap) {
            if (dom->jmpset)
                warnerror(dom, "13", "Attribute value not found: ", name);
            break;
        }
        ch = *ap;
        if (ch != '"' && ch != '\'' ) {
            if (dom->jmpset)
                warnerror(dom, "14", "Attribute value not quoted: ", name);
            break;
        }
        value = ++ap;
        while (*ap && *ap != ch) ap++;
        if (!*ap) {
            if (dom->jmpset)
                warnerror(dom, "15", "Attribute value not terminated: ", name);
            break;
        }
        *ap = 0;
        atts->name = name;
        len = unescape(dom, value, 0, strlen(value), 1);
        value[len] = 0;
        atts->value = value;
        node->count++;
        ap++;
        atts++;
    }

}


/*
 * This call should always return the root of the document
 */
xnode_t * ism_xml_first(xdom * dom) {
    dom->Level = 1;
    dom->Node[0] = dom->First;
    dom->Node[1] = dom->Node[0]->child;
    return dom->Node[1];
}


/*
 * This puts you before the first node, so that a ism_xml_next() will
 * set you at the root element.
 */
void ism_xml_reset(xdom * dom) {
    dom->Level = 0;
}

/*
 * This is the normal next call which allows return up to any level
 * if the parent bit is set.
 */
xnode_t * ism_xml_next(xdom * dom, int child) {
    return ism_xml_next_level(dom, child, 0);
}


/*
 * Get the next node.  It is possible to select whether to
 * follow the child and/or parent chains.  If the level is
 * non-zero, then do not allow the next to go up the parent
 * chain to that level.  This is used to find all elements
 * contained within a certain element.
 */
xnode_t * ism_xml_next_level(xdom * dom, int child, int level) {
    xnode_t * n = dom->Node[dom->Level];
    if ((child&LEVEL_CHILD) && n->child) {
        dom->Node[++dom->Level] = n->child;
        return n->child;
    }
    if (n->sibling && (child != LEVEL_CHILDONLY)) {
        dom->Node[dom->Level] = n->sibling;
        return n->sibling;
    }
    if (child & LEVEL_PARENT) {
        while (dom->Level>level) {
            n = dom->Node[--dom->Level];
            if (n->sibling) {
                dom->Node[dom->Level] = n->sibling;
                return n->sibling;
            }
        }
    }
    return NULL;
}


/*
 * Find the first instance of an element in the document
 * This is a helper function.
 */
xnode_t * ism_xml_simpleFind(xdom * dom, char * tag) {
    xnode_t * node = ism_xml_first(dom);

    while(node) {
        if ((node->type==NODE_ELEMENT) && !strcmp(tag, node->name))
            return node;
        node = ism_xml_next(dom, LEVEL_ALL);
    }
    return NULL;
}

/*
 * Find the next instance of an element in the document after the specified node
 * This is a helper function.
 */
xnode_t * ism_xml_findNext(xdom * dom, xnode_t * node, char * tag) {
    node = ism_xml_next(dom, LEVEL_ALL);

    while(node) {
        if ((node->type==NODE_ELEMENT) && !strcmp(tag, node->name))
            return node;
        node = ism_xml_next(dom, LEVEL_ALL);
    }
    return NULL;
}

/*
 * Find the last instance of an element in the document
 * This is a helper function.
 */
xnode_t * ism_xml_findLast(xdom * dom, char * tag) {
    xnode_t * node = ism_xml_first(dom);
    xnode_t * lastnode = NULL;

    while(node) {
        if ((node->type==NODE_ELEMENT) && !strcmp(tag, node->name))
            lastnode = node;
        node = ism_xml_next(dom, LEVEL_ALL);
    }
    return lastnode;
}

/*
 * Find the first instance of a node matching the specified string.
 * The node can be matched by element name, attribute value, and/or count.
 * <p>
 * Elements are identified by name.  Text nodes are identified as #text,
 * and attributes are identified as name=value with no quote marks.
 * If the first character of the matching string is slash (/) then the search
 * is from the beginning of the document.
 * The mathcing string can contain the following:
 * <table>
 * <tr><td>elem</td>       <td>Find the next occurance of elem at this level</td></tr>
 * <tr><td>*elem</td>      <td>Find the next occurance of elem at any level</td></tr>
 * <tr><td>elem1.elem2</td><td>Find the first occurance of elem2 within elem1</td></tr>
 * <tr><td>elem[3]</td>     <td>Find the third occurance of elem from the current node</td></tr>
 * <tr><td>elem(attr=val)</td><td>Find the next occurance of elem with the attribute of name attr and value val</td></tr>
 * <tr>
 * <p>
 * A null or empty match string returns the current node.  In the case of an error,
 * a NULL node will be returned.
 * <p>
 * A simpleFind can be described in this syntax as a find of &#x2F;*tag
 */
xnode_t * ism_xml_find(xdom *dom, char * match) {
    xnode_t * node = dom->Node[dom->Level];
    int rule = LEVEL_ALL;
    int level = dom->Level;
    char elem[128], attr[128], value[128];
    int  count;
    int  found;

    if (*match) {
        if (*match == '/') {
            node = ism_xml_first(dom);
            match++;
        }
        while (*match && node) {
            if (*match=='*')
                rule = LEVEL_ALL;
            else
                rule = LEVEL_THIS;
            match = parse_match(match, 128, elem, &count, attr, value);
            if (count<1) {
                if (count < 0)
                    node = NULL;
                break;
            }
            found = 0;
            do {
                // SA_ASSUME(node != NULL);
                if (*elem == '#') {
                    if (elem[1] == 't' && node->type == NODE_CONTENT)
                        found=1;
                    else if (elem[1] == 'w' && node->type == NODE_WHITESPACE)
                        found=1;
                } else {
                    if (!*elem && node->type == NODE_ELEMENT)
                        found=1;
                    else found = !strcmp(node->name, elem);
                }
                if (found && *attr) {
                    char * nodeval = ism_xml_getNodeValue(dom, node, attr);
                    found = ((nodeval!=NULL) && (!*value || !strcmp(value, nodeval)));
                }
                if (found)
                    count--;
                if (!found || count)
                    node = ism_xml_next_level(dom, rule, level);
            } while (count && node);
            if (*match == '.') {
                node = ism_xml_next(dom, LEVEL_CHILDONLY);
                match++;
            }
        }
    }
    return node;
}

/*
 * States for rule matching
 */
enum MATCH_STATES {
    ST_ELEMENT = 1,
    ST_COUNT   = 2,
    ST_ATTR    = 3,
    ST_VALUE   = 4,
    ST_DONE    = 5
};

/*
 * Parse the match string
 */
char * parse_match(char * match, int maxlen, char * elem, int * countp, char * attr, char * value) {
    char * cp = elem;
    int    count = 1;
    int    state;
    int    ch;
    elem[0] = 0;
    attr[0] = 0;
    value[0] = 0;
    state = ST_ELEMENT;
    while (*match && *match != '.') {
        ch = *match;

        /* Start of count */
        if (ch=='[') {
            *cp = 0;
            if (count == 1) {
                count = 0;
                state = ST_COUNT;
            } else {
                count = -1;
                state = ST_DONE;
            }
        }

        /* Start of attribute */
        if (ch=='(') {
            *cp = 0;
            if (!*attr) {
                state = ST_ATTR;
                cp = attr;
            } else {
                count = -1;
                state = ST_DONE;
            }
        }

        /* Start of value */
        if (ch=='=') {
            *cp = 0;
            if (!*value) {
                state = ST_VALUE;
                cp = value;
            } else {
                count = -1;
                state = ST_DONE;
            }
        }

        switch (state) {
        case ST_ELEMENT:
            if (cp-elem < maxlen)
                *cp++ = ch;
            break;

        case ST_COUNT:
            if (ch==']') {
                state = ST_DONE;
            }
            else if (ch < '0' || ch >'9') {
                count = -1;
                state = ST_DONE;
            }
            else {
                count = count*10 + (ch-'0');
            }
            break;

        case ST_ATTR:
            if (ch==')') {
                *cp = 0;
                state = ST_DONE;
            } else {
                if (cp-attr < maxlen)
                    *cp++ = ch;
            }
            break;

        case ST_VALUE:
            if (ch==')') {
                *cp = 0;
                state = ST_DONE;
            } else {
                if (cp-value < maxlen)
                    *cp++ = ch;
            }
            break;

        case ST_DONE:
            count = -1;
            break;
        }
        match++;
    }
    *cp = 0;
    *countp = count;
    return match;
}


/*
 * Return a label of the current position in the dom.
 * This label can be used to do a ism_xml_find() and return to the same node.
 */
char * ism_xml_getLabel(xdom * dom, char * out, int maxlen) {
    return NULL;
}


/*
 * Get the value of an element content or attribute by name
 */
char * ism_xml_getValue(xdom * dom, char * tag, char * where, char * alt) {
    char * ret;
    xnode_t * node = ism_xml_simpleFind(dom, tag);
    if (!node)
        return NULL;

    ret = ism_xml_getNodeValue(dom, node, where);
    if (!ret)
        ret = ism_xml_getNodeValue(dom, node, alt);
    return ret;
}


/*
 * Given a node, give the value for an attribute or child
 */
char * ism_xml_getNodeValue(xdom * dom, xnode_t * node, char * attr) {
    if (!strcmp(attr, "*")) {
        xnode_t * child = node->child;
        if (child && child->type == NODE_CONTENT) {
            return child->name;
        }
    } else {
        int i;
        if (node->count < 0)
            ism_xml_parseAttributes(dom, node);
        for (i=0; i<node->count; i++) {
            if (!strcmp(attr, node->attr[i].name)) {
                return node->attr[i].value;
            }
        }
    }
    return NULL;
}

/*
 * xml set options.
 */
XAPI void ism_xml_setOptions(xdom * dom, int options) {
    dom->options = options;
}

/*
 * xml get options.
 */
XAPI int ism_xml_getOptions(xdom * dom) {
    return dom->options;
}

/*
 * xml set user data in dom .
 */
XAPI void   ism_xml_setuserdata(xdom * dom, void * val) {
    dom->userdata = val;
}

/*
 * xml get user data from dom.
 */
XAPI void *  ism_xml_getuserdata(xdom * dom) {
    return dom->userdata;
}

/*
 * xml save position.
 */
XAPI xdompos_t * ism_xml_saveposition(xdom * dom, xdompos_t * dompos) {
    if (!dompos)
        dompos = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_xml,19),1, sizeof(xdompos_t));
    // SA_ASSUME(dompos != NULL);
    memcpy(dompos, dom, sizeof(xdompos_t));
    dompos->dom = dom;
    return dompos;
}


/*
 * xml restore position.
 * Restore a saved position in the xml dom.
 */
XAPI xnode_t * ism_xml_restoreposition(xdom * dom, xdompos_t * dompos) {
    xnode_t * savefirst;
    if (!dompos || dompos->dom != dom || dompos->valid != dom->Line)
        return NULL;

    savefirst = dom->First;
    memcpy(dom, dompos, sizeof(xdompos_t));
    dom->First = savefirst;
    return dom->Node[dom->Level];
}


/*
 * Return the current node.
 */
XAPI xnode_t * ism_xml_node(xdom * dom) {
    return dom->Node[dom->Level];
}


/*
 * xml free a dom position object.
 */
XAPI void ism_xml_freeposition(xdompos_t * dompos) {
    ism_common_free(ism_memory_utils_xml,dompos);
}


/*
 * xml get a single character from ISO8859-1
 */
XAPI int ism_xml_getch_latin1(void * xbufv) {
    struct xbufio_t * xbuf = xbufv;
    if (xbuf->pos >= xbuf->len)
        return -1;
    return (int)(uint8_t)(xbuf->buf[xbuf->pos++]);
}

/*
 * Constants used for decoding UTF-8
 */
static int States[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
};
static int StateMask[] = {0, 0, 0x1F, 0x0F, 0x07};


/*
 * Check for valid second byte of UTF-8
 */
static int validSecond(int state, int first, int second) {
    /*
     * Anything outside the standard range is bad
     */
    if (second<0x80 || second >0xbf)
        return 0;

    /*
     * Check legal ranges based on output size
     */
    switch (state) {
    case 2:                              /* 0x0080 - 0x07ff     */
        if (first<2)                     /* Disallow < 0x0080   */
            return 0;
        break;
    case 3:                              /* 0x0800 - 0xffff     */
        if (first==0 && second < 0xa0)   /* Disallow < 0x0800   */
            return 0;
        break;
    case 4:                              /* 0x10000 - 0x200000  */
        if (first==0 && second < 0x90)   /* Disallow < 0x10000  */
            return 0;
        if (first==4 && second > 0x8f)   /* Disallow > 0x10ffff */
            return 0;
        if (first>4)                     /* Disallow > 0x10ffff */
            return 0;
        break;
    }
    return 1;
}


/*
 * xml get a single character from UTF-8
 */
XAPI int ism_xml_getch_utf8(void * xbufv) {
    struct xbufio_t * xbuf = xbufv;
    int ch;
    int state;
    int val;
    if (xbuf->pos >= xbuf->len)
        return -1;
    ch = (int)(uint8_t)(xbuf->buf[xbuf->pos++]);
    if (ch >= 0x80) {
        state = States[ch>>3];
        val = ch & StateMask[state];
        if (state == 1)
            return -2;
        ch = (int)(uint8_t)(xbuf->buf[xbuf->pos]);
        if (!validSecond(state, val, ch))
            return -2;
        xbuf->pos++;
        val = (val<<6) + (ch&0x3f);
        if (state>2) {
            ch = (int)(uint8_t)(xbuf->buf[xbuf->pos]);
            if (ch < 0x80 || ch > 0xbf)
                return -2;
            xbuf->pos++;
            val = (val<<6) + (ch&0x3f);
            if (state>3) {
                ch = (int)(uint8_t)(xbuf->buf[xbuf->pos]);
                if (ch < 0x80 || ch > 0xbf)
                    return -2;
                xbuf->pos++;
                val = (val<<6) + (ch&0x3f);
            }
        }
        return val;
    }
    return ch;
}


/*
 * xml get a single character from UTF-16 little endian
 */
XAPI int ism_xml_getch_utf16le(void * xbufv) {
    struct xbufio_t * xbuf = xbufv;
    int ch;
    if (xbuf->pos+1 >= xbuf->len)
        return -1;
    ch = ((((int)(uint8_t)xbuf->buf[xbuf->pos+1])<<8) | (uint8_t)xbuf->buf[xbuf->pos]);
    xbuf->pos += 2;
    return ch;
}


/*
 * xml get a single character from UTF-16 big endian
 */
XAPI int ism_xml_getch_utf16be(void * xbufv) {
    struct xbufio_t * xbuf = xbufv;
    int ch;
    if (xbuf->pos+1 >= xbuf->len)
        return -1;
    ch = ((((int)(uint8_t)xbuf->buf[xbuf->pos])<<8) | (uint8_t)xbuf->buf[xbuf->pos+1]);
    xbuf->pos += 2;
    return ch;
}


/*
 * xml get a single character from UTF-32 little endian
 */
XAPI int ism_xml_getch_utf32le(void * xbufv) {
    struct xbufio_t * xbuf = xbufv;
    int ch;
    if (xbuf->pos+3 >= xbuf->len)
        return -1;
    ch = (((int)(uint8_t)xbuf->buf[xbuf->pos+3])<<24) |
         (((int)(uint8_t)xbuf->buf[xbuf->pos+2])<<16) |
         (((int)(uint8_t)xbuf->buf[xbuf->pos+1])<<8)  |
         (uint8_t)xbuf->buf[xbuf->pos];
    xbuf->pos += 4;
    return ch;
}


/*
 * xml get a single character from UTF-32 big endian
 */
XAPI int ism_xml_getch_utf32be(void * xbufv) {
    struct xbufio_t * xbuf = xbufv;
    int ch;
    if (xbuf->pos+3 >= xbuf->len)
        return -1;
    ch = (((int)(uint8_t)xbuf->buf[xbuf->pos])<<24) |
         (((int)(uint8_t)xbuf->buf[xbuf->pos+1])<<16) |
         (((int)(uint8_t)xbuf->buf[xbuf->pos+2])<<8)  |
         (uint8_t)xbuf->buf[xbuf->pos+3];
    xbuf->pos += 4;
    return ch;
}





/*
 * Find the encoding string in the leading XML processing instruction.
 * This search is done before we make a copy of the input, so we must
 * copy the XML processing instruction.  The returned value is in the dom.
 */
char * ism_xml_findEncoding(xdom * dom, char * buf, int len) {
    int i, max;
    char * enc;
    char * tbuf;

    if (len<18)
        return NULL;
    if (memcmp(buf, "<?xml", 5))
        return NULL;

    max = len-1;
    for (i=5; i<max; i++) {
        if (buf[i]=='?' && buf[i+1]=='>') {
            xnode_t n;
            int savejmpset = dom->jmpset;
            tbuf = alloca(i+1);
            if (!tbuf)
                return NULL;
            memcpy(tbuf, buf, i);
            tbuf[i] = 0;
            n.count = -2;
            n.attr  = (xATTR *)(tbuf+5);
            dom->jmpset = 0;
            ism_xml_parseAttributes(dom, &n);
            dom->jmpset = savejmpset;
            enc = ism_xml_getNodeValue(dom, &n, "encoding");
            if (!enc || strlen(enc) > 31)
                return NULL;
            strcpy(dom->encoding, enc);
            return dom->encoding;
        }
    }
    return NULL;
}


/*
 * Scan a string and determine the additional buffer required to do escape
 * processing.  If the return is zero, no escape processing is required.
 */
int ism_xml_extraescape(const char * field) {
    int len  = 0;
    while (*field) {
        switch (*field) {
        case '>':  len += 3;  break;
        case '<':  len += 3;  break;
        case '"':  len += 5;  break;
        case '\'': len += 5;  break;
        case '&':  len += 4;  break;
        default:
            if (*field < ' ' || (*field&0x80)) {
                len += 5;
            }
            break;
        }
        field++;
    }
    return len;
}


/*
 * Put out standard XML escape sequences.
 */
char * ism_xml_escape(char * out, const char * str, int utf8) {
    char * saveout = out;
    int    ch;
    int    state;
    int    val;

    while (*str) {
        ch = (uint8_t)*str;

        /*
         * Convert from UTF-8 to UTF-32
         */
        if (utf8 && ch >= 0x80) {
            state = States[ch>>3];
            if (state > 1) {
                val = ch & StateMask[state];
                ch = (int)(uint8_t)(*++str);
                if (!validSecond(state, val, ch)) {
                    ch = 0xfffd;
                    if (ch == 0)
                        str--;
                } else {
                    val = (val<<6) + (ch&0x3f);
                    if (state > 2) {
                        ch = (int)(uint8_t)(*++str);
                        if (ch < 0x80 || ch > 0xbf) {
                            ch =0xfffd;
                            if (ch == 0)
                                 str--;
                        } else {
                            val = (val<<6) + (ch&0x3f);
                            if (state > 3) {
                                ch = (int)(uint8_t)(*++str);
                                if (ch < 0x80 || ch > 0xbf) {
                                    ch =0xfffd;
                                    if (ch == 0)
                                        str--;
                                }
                                val = (val<<6) + (ch&0x3f);
                            }
                        }
                    }
                    ch = val;
                }
            } else {
                ch = 0xfffd;
            }
        }

        switch (ch) {
        case '>':  memcpy(out, "&gt;", 4);  out += 4;  break;
        case '<':  memcpy(out, "&lt;", 4);  out += 4;  break;
        case '"':  memcpy(out, "&quot;", 6);out += 6;  break;
        case '\'': memcpy(out, "&apos;", 6);out += 6;  break;
        case '&':  memcpy(out, "&amp;", 5); out += 5;  break;
        default:
            if (ch < ' ' || ch > 0x7E) {
                sprintf(out, "&#x%X;", ch);
                out += strlen(out);
            } else {
                *out++ = (char)ch;
            }
            break;
        }
        str++;
    }
    *out = 0;
    return saveout;
}

/*
 * Check if a name is in the specified list.
 */
static int isInList(const char * * list, const char * name) {
    while (*list) {
        if (!strcmp(*list, name)) {
            return 1;
        }
        list++;
    }
    return 0;
}

/*
 * Check for attributes which are not in the specified list.
 */
char * ism_xml_checkAttributes(xdom * dom, xnode_t * node, const char * * attrlist) {
    int i;
    for (i=0; i<node->count; i++) {
        if (!isInList(attrlist, node->attr[i].name)) {
            return node->attr[i].name;
        }
    }
    return NULL;
}


/*
 * Dump out the DOM
 */
void ism_xml_dump(xdom * dom, int fileno) {
    char buf[20000];
    xnode_t * n = ism_xml_first(dom);
    int     i;
    DEBUG_ONLY ssize_t bytes_written=0; //this is a debug function so we don't do much if writes fail

    while (n) {
        snprintf(buf, 1000, "line %4d: level %d: ", n->line, ism_xml_getLevel(dom));
        bytes_written = write(fileno, buf, strlen(buf));
        assert(bytes_written == strlen(buf));

        switch (n->type) {
        /*
         * Element node
         */
        case 'e':
            snprintf(buf, 1000, "<%s ", n->name);
            bytes_written = write(fileno, buf, strlen(buf));
            assert(bytes_written == strlen(buf));

            if (n->count<0)
                ism_xml_parseAttributes(dom, n);
            for (i=0; i<n->count; i++) {
                snprintf(buf, 1000, "%s=\"%s\" ", n->attr[i].name, n->attr[i].value);
                bytes_written = write(fileno, buf, strlen(buf));
                assert(bytes_written == strlen(buf));
            }
            bytes_written =  write(fileno, ">\n", 2);
            assert(bytes_written == strlen(buf));
            break;

        /*
         * Content node
         */
        case 'c':
            snprintf(buf, sizeof(buf), "[%s] \n", n->name);
            buf[sizeof(buf)-1] = 0;
            bytes_written = write(fileno, buf, strlen(buf));
            assert(bytes_written == strlen(buf));
            break;

        /*
         * Whitespace node
         */
        case 'w':
            snprintf(buf, 1000, "#whitespace\n");
            bytes_written = write(fileno, buf, strlen(buf));
            assert(bytes_written == strlen(buf));
            break;
        }
        n = ism_xml_next(dom, LEVEL_ALL);
    }
}


