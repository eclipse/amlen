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

#ifndef _XMLPARSE_INCLUDED
#define _XMLPARSE_INCLUDED

#include <ismutil.h>


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

/** @file xmlparse.h
 * C definitions for the simple XML parser
 */

/** @class xmlparse
 * This is a very small and simple XML parser with C interfaces.  It is
 * designed to handle a subset of XML very quickly.  It is not a
 * conforming XML processor.  The following major restrictions exist:
 *
 * 1. No external entities
 * 2. Only the default internal entities and numeric entities are allowed
 * 3. All DTD declarations, comments, and processing instructions are ignored
 *
 * This parser catches most but not all well formed violations in the
 * xml file.  This code allows the user to process an xml file which is
 * not well formed.  A full xml parser (perhaps with validation) should
 * be used where more complete document checking is required.
 *
 * Only one thread may be parsing the DOM at a time as the DOM contains
 * a current position.
 */


/** Element node type.  This represents the start of an element. */
#define NODE_ELEMENT 'e'
/** Content node type. */
#define NODE_CONTENT 'c'
/** Whitespace node type. */
#define NODE_WHITESPACE 'w'

/**
 * The attribute structure
 */
typedef struct {
    char * name;       /**< The attribute name */
    char * value;      /**< The attribute value */
} xATTR;

/**
 * The encapsulated dome
 */
typedef struct xdompos_t xdompos_t;

/**
 * The node structure.
 * Since the nodes are only linked in one direction, it is necessary to have
 * the xml dom object to back up.
 */
typedef struct xnode {
    char            type;              /**< Type of node NODE_               */
    unsigned char   fileno;            /**< File number                      */
    short           count;             /**< -1 = attributes are unparsed     */
    char *          name;              /**< Name of node, content string     */
    struct xnode *  sibling;           /**< Sibling node or null             */
    struct xnode *  child;             /**< Child node or null               */
    xATTR *         attr;              /**< Attribute pointer                */
    int             line;              /**< Source line containing node      */
} xnode_t;


/**
 * Mask of callback types.
 * These allow the invoker to get control during parse.  This can be used to
 * implement simple SAX-like function.  The values are ORd together.
 */
enum NodeCallbackType {
    XT_StartElement = 1,     /**< Callback for start elements */
    XT_EndElement   = 2,     /**< Callback for end elements   */
    XT_Element      = 3,     /**< Callback for elements       */
    XT_Content      = 4,     /**< Callback for content nodes  */
    XT_All          = 7      /**< Callback for all nodes      */
};

/**
 * Option flags.  These can be ORd together.
 */
enum SXMLOptions {
    XOPT_DelayAttr    = 1,   /**< Delay attribute parse.  If this is selected, attribute errors will not be reported.*/
    XOPT_NoCheckNames = 2,   /**< Do not check names for character restrictions.*/
    XOPT_Error        = 4,   /**< Report violations as errors and stop parsing.*/
    XOPT_NoNamespace  = 8,   /**< Ignore all namespace specifications in tags and attributes.*/
    XOPT_MultiRoot    = 16,  /**< Allow multiple root elements (ignored for streams) */
    XOPT_RootContent  = 32,  /**< Allow content outside of the root tag */
    XOPT_Whitespace   = 64   /**< Keep whitespace nodes */
};


/**
 * The DOM is opaque externally
 */
#ifndef XDOM_DEFINED
#define XDOM_DEFINED
typedef struct _xdom xdom;
#endif

/**
 * The prototype for the SAX-like callback.
 * Callbacks can be done for start element, end element, and content nodes.
 * There is a bit for each of these which can be ORd together.
 * When a self ending element is found, it both the start and end element
 * callbacks will be called.
 * <p>
 * If a non-zero return code is given, the node will not be put into the
 * dom.  If the start element is supressed, the callback must also supress
 * the end element, except that the end element callback will not be made
 * for self-ending elements.  If the element is supressed by the start element
 * callback, the attributes will not be available at the end element callback.
 */
typedef int (*ism_xml_callback_t)(xdom * dom, xnode_t * node, int what);

/**
 * The prototype for logging calls
 */
typedef int (*ism_xml_log_t)(int level, const char * message);

/**
 * The prototype for extended logging calls
 */
typedef int (*ism_xml_logx_t)(xdom * dom, int level, const char * msgid, const char * message);

/**
 * The prototype for the get function.  A negative return code indicates that
 * no characters are available (such as EOF and IO Error).
 */
typedef int (*ism_xml_getch_t)(void * x);

#define LEVEL_THIS   0         /* Get next at this level only     */
#define LEVEL_CHILD  1         /* Go to child but not to parent */
#define LEVEL_PARENT 2         /* Go to parent but not to sibling */
#define LEVEL_ALL    3         /* Get any next node */
#define LEVEL_CHILDONLY 5

/**
 * xml object constructor.
 * Make a new DOM which can then be filled in using parse.
 * @param systemId The identifier of the DOM (normally the filename)
 * @return An xml dom object
 */
XAPI xdom * ism_xml_new(char * systemId);

/**
 * xml object destructor.
 * Free up the memory associated with the xml dom object
 * @param dom  An xml dom object
 */
XAPI void ism_xml_free(xdom * dom);


/**
 * xml set options.
 * @param dom  An xml dom object;
 * @param options  The options flags ORd together
 */
XAPI void ism_xml_setOptions(xdom * dom, int options);

/**
 * xml include file.
 * @param dom  An xml dom object;
 * @param name The location of the source
 * @param fileno  The file number to use
 */
XAPI int ism_xml_include(xdom * dom, char * name, int fileno);

/**
 * xml get options.
 * @param dom  An xml dom object;
 * @return The option flags ORd together
 */
XAPI int ism_xml_getOptions(xdom * dom);


/**
 * xml set log message prefix.
 * The message prefix is appened to the front of the two character message IDs generated
 * by the xml parser.  This is normally a 6 byte string making the entire message ID 8 characters.
 * <p>
 * If the log message prefix is null or invalid, a default is used.
 * @param dom  An xml dom object;
 * @param msgp The message prefix
 */
XAPI void ism_xml_setLogPrefix(xdom * dom, const char * msgp);


/**
 * xml buffer parser.
 * The xml source in the specified buffer is parsed.  This parsing modifies the
 * contents of the buffer.  If this is not desired, the copy flag can be set which
 * causes the parser to make a copy of the buffer before parsing, and thus leave
 * the original buffer unmodified.  If the copy flag is not set, the buffer may not
 * be reused until ism_xml_free is called.
 * @param dom   An xml dom object which is normally a newly created object.
 * @param buf   A buffer containing the XML source to parse
 * @param len   The number of bytes in the buffer
 * @param copy  0=do not copy, !0=copy buffer
 * @return A return code, 0=good, 1=warning, >1=error
 */
XAPI int ism_xml_parse (xdom * dom, char * buf, int len, int copy);


/**
 * xml stream parser.
 * The XML source specified in the stream in parsed.  This is somewhat less efficient
 * than parsing a complete buffer, but handles the case where the invoker does not know
 * how large the document is.
 * <p>
 * This call terminates when the end tag matching the root is found, when the stream get
 * function returns an error, or when a parsing error is found.  The setting of XOPT_MultiRoot
 * is ignored as this function always terminates when the end of the root element is found.
 * <p>
 * The character returned from from getch() is a unicode character.
 *
 * @param dom    An xml dom object which is normally a newly created object
 * @param getch  A function to call to get a character from the stream
 * @param parm   A parameter to pass to getch
 */
XAPI int ism_xml_parse_stream(xdom * dom, ism_xml_getch_t getch, void * parm);


/**
 * xml parse attributes.
 * During initial parsing, the attributes of a node may not be fully parsed.
 * This call causes the attributes to be fully parsed.  If the attributes of
 * this node are already fully parsed, then take no action.
 * @param dom  An xml dom object
 * @param node An xml node object
 */
XAPI void ism_xml_parseAttributes(xdom * dom, xnode_t * node);


/**
 * xml get systemID.
 * Return the system ID associated with this dom.  This is normally the file name
 * which originally contained the xml source.
 * @param dom  An xml dom object
 * @return A string representing the ID of the dom (can be null)
 */
XAPI char * ism_xml_getSystemId(xdom * dom);


/**
 * xml get Level.
 * Indicate the level of the current position in the dom.  The root node is at level 1.
 * @param dom  An xml dom object
 * @return The current level
 */
XAPI int ism_xml_getLevel(xdom * dom);


/**
 * xml get Level.
 * Indicate the level of the current position in the dom.  The root node is at level 1.
 * @param dom  An xml dom object
 * @return The current level
 */
XAPI int ism_xml_getLine(xdom * dom);


/*
 * Return the current file number
 * @param dom  An xml dom object
 * @return The current file number
 */
XAPI int ism_xml_getFileno(xdom * dom);

/**
 * Return the soruce name of a node
 * @param dom  An xml dom object
 * @param node An xml node object
 */
XAPI const char * ism_xml_getSourceName(xdom * dom, xnode_t * node);


/*
 * Set the system identifer and location
 * @param dom  An xml dom object
 * @param systemId  The system identifier (normally the file name or URI) of the source
 * @param line      The line number
 * @param fileno    The file number (0-255)
 */
XAPI void ism_xml_setSystemId(xdom * dom, const char * systemId, int line, int fileno);

/**
 * xml return first node (root element).
 * @param dom  An xml dom object
 * @return The root node if one exists, or null
 */
XAPI xnode_t * ism_xml_first(xdom * dom);


/**
 * Set to before the first element.
 * This position can then be used to do a next thru the entire document.
 * @param dom  An xml dom object
 */
XAPI void ism_xml_reset(xdom * dom);


/**
 * xml return next element.
 * Return the next node according the the specified rule.
 * @param dom    An xml dom object
 * @param child  Specify the rule to follow
 * <br>  LEVEL_THIS:  Get next at this level only
 * <br>  LEVEL_CHILD:  Go to child but not to parent
 * <br>  LEVEL_PARENT: Go to parent but not to sibling
 * <br>  LEVEL_ALL:    Get any next node
 * <br>  LEVEL_CHILDONLY Go to only the first child
 */
XAPI xnode_t * ism_xml_next(xdom * dom, int child);


/**
 * xml return next element limited to the specified level.
 * This is used to limit the search to within a section of the document.
 * @param dom    An xml dom object
 * @param child  Specify the rule to follow
 * <br>  LEVEL_THIS:  Get next at this level only
 * <br>  LEVEL_CHILD:  Go to child but not to parent
 * <br>  LEVEL_PARENT: Go to parent but not to sibling
 * <br>  LEVEL_ALL:    Get any next node
 * <br>  LEVEL_CHILDONLY Go to only the first child
 * @param level  The level to limit the search
 */
XAPI xnode_t * ism_xml_next_level(xdom * dom, int child, int level);


/**
 * xml find the first instance of an element in the document
 * Find the first instance of a tag at any level in the document.
 * This is normally used to find sections or simple configuration entries
 * @param dom    An xml dom object
 * @param tag    The name of an element to search for
 */
XAPI xnode_t * ism_xml_simpleFind(xdom * dom, char * tag);

/**
 * xml find the last instance of an element in the document
 * Find the last instance of a tag at any level in the document.
 * This is normally used to find sections or simple configuration entries
 * @param dom    An xml dom object
 * @param tag    The name of an element to search for
 */
XAPI xnode_t * ism_xml_findLast(xdom * dom, char * tag);

/**
 * xml find the next instance of an element in the document
 * Find the next instance of a tag at any level in the document starting from
 * the position of the node specified.
 * This is normally used to find sections or simple configuration entries
 * @param dom    An xml dom object
 * @param tag    The name of an element to search for
 */
XAPI xnode_t * ism_xml_findNext(xdom * dom, xnode_t * n, char * tag);


/**
 * xml get the value of an element
 * @param dom    An xml dom object
 * @param node   An xml node object
 * @param attr   The name of an attribute, or '*' for the first content node
 */
XAPI char * ism_xml_getNodeValue(xdom * dom, xnode_t * node, char * attr);


/**
 * xml set the node creation callback.
 * The callback is used to implement a simple SAX-like function which gets
 * control during the parsing.
 * @param dom       An xml dom object
 * @param callback  The function pointer of the callback
 * @param kind      What actions to implement callback for
 */
XAPI int    ism_xml_setCallback(xdom * dom, ism_xml_callback_t callback, int kind);


/**
 * xml set the log function for the dom.
 * Set the logging callback of the dom.
 * This call is deprecated.  New code should use ism_xml_setlogx().
 * @param dom  An xml dom object
 * @param logcall The funciton to call to log warnings and errors
 */
XAPI void   ism_xml_setlog(xdom * dom, ism_xml_log_t logcall);


/**
 * xml set the extended log function for the dom.
 * Set the extended logging callback of the dom.
 * An extended log function includes a message code.
 * @param dom  An xml dom object
 * @param logcall The funciton to call to log warnings and errors
 */
XAPI void   ism_xml_setlogx(xdom * dom, ism_xml_logx_t logcall);

/**
 * xml set user data in dom .
 * @param dom  An xml dom object
 * @param data A user data value to set
 */
XAPI void   ism_xml_setuserdata(xdom * dom, void * data);

/**
 * xml get user data from dom.
 * @param dom  An xml dom object
 * @return   A user data value
 */
XAPI void * ism_xml_getuserdata(xdom * dom);

/**
 * xml save position.
 * Save the current position in the xml dom.  If
 * @param dom  An xml dom object
 * @param dompos  An xml dom position object
 * @return  A object param dompos  An xml dom object
 */
XAPI xdompos_t * ism_xml_saveposition(xdom * dom, xdompos_t * dompos);

/**
 * xml restore position.
 * Restore a saved position in the xml dom.
 * @param dom  An xml dom object
 * @param dompos  An xml dom position object
 */
XAPI xnode_t * ism_xml_restoreposition(xdom * dom, xdompos_t * dompos);

/**
 * xml get the current node
 * Return the node at the current position in the dom.
 * @param dom  An xml dom object
 * @return The current node at the current level.
 */
XAPI xnode_t * ism_xml_node(xdom * dom);

/**
 * xml free a dom position object.
 * @param dompos  An xml dom position object
 */
XAPI void ism_xml_freeposition(xdompos_t * dompos);

/**
 * xml log a message.
 * @param dom      An xml dom object
 * @param node     The current node
 * @param level    The log severity level
 * @param msgid    The message id
 * @param string   The message text
 * @param repl     The replacement string
 */
XAPI void ism_xml_log(xdom * dom, xnode_t * node, int level, const char * msgid, const char * string, const char * repl);

/**
 * xml get a single character from ISO8859-1.
 * @param xbuf The xml buffer structure
 * @return The next character from the buffer intepreted as ISO8859-1.
 */
XAPI int ism_xml_getch_latin1(void * xbuf);

/**
 * xml get a single character from UTF-8.
 * @param xbuf The xml buffer structure
 * @return The next character from the buffer intepreted as UTF-8.
 */
XAPI int ism_xml_getch_utf8(void * xbuf);


/**
 * xml get a single character from UTF-16 little endian.
 * @param xbuf The xml buffer structure
 * @return The next character from the buffer intepreted as UTF-16 little endian.
 */
XAPI int ism_xml_getch_utf16le(void * xbuf);

/**
 * xml get a single character from UTF-16 big endian.
 * @param xbuf The xml buffer structure
 * @return The next character from the buffer intepreted as UTF-16 big endian.
 */
XAPI int ism_xml_getch_utf16be(void * xbuf);

/**
 * xml get a single character from UTF-32 little endian.
 * @param xbuf The xml buffer structure
 * @return The next character from the buffer intepreted as UTF-32 little endian.
 */
XAPI int ism_xml_getch_utf32le(void * xbuf);

/**
 * xml get a single character from UTF-32 big endian.
 * @param xbuf The xml buffer structure
 * @return The next character from the buffer intepreted as UTF-32 big endian.
 */
XAPI int ism_xml_getch_utf32be(void * xbuf);

/**
 * xml get the encoding attribute from the xml processing instruction.
 * This is normally called after reading a file and before parsing the file to
 * determine the encoding of the XML file.
 * @param dom   An xml dom object
 * @param buf   A buffer containing the xml processing instruction
 * @param len   The length of the area to search
 * @return A string containing the name of the encoding, or NULL if there is none
 */
XAPI char * ism_xml_findEncoding(xdom * dom, char * buf, int len);

/**
 * Unescape the content in place.  The unescaped string is always
 * equal to or shorter than the escaped string.
 * @param dom       An xml dom object
 * @param buf  The location of the string to unescape
 * @param utf8 Character encoding: 0=ISO8859-1  1=UTF-8
 * @return The number of bytes in the output stream
 */
XAPI int ism_xml_unescape(xdom * dom, char * buf, int utf8);

/*
 * Scan a string and determine the additional buffer required to do escape
 * processing.  If the return is zero, no escape processing is required.
 * This is not guaranteed to be exact, but this value plus the input string size
 * will always be at least large enough to hold the escaped string.
 * @param buf  The string to scan
 * @return The number of additional bytes needed, or 0 if no escape processing is required
 */
XAPI int ism_xml_extraescape(const char * buf);

/*
 * Put out standard XML escape sequences.
 * The required output buffer size can be determined using ism_xml_extraescape().
 * @param out  The output buffer for the escaped string.
 * @param str  The string to escape.
 * @param utf8 Character encoding: 1=utf8, 0=iso8859-1
 * @return The output buffer
 */
XAPI char * ism_xml_escape(char * out, const char * str, int utf8);

/**
 * Check for attributes which are not in the specified list.
 * Return the name of the first attribute of a node which does not exist in the list.
 * This is commonly used to detect invalid attributes.
 * @param dom  An xml dom object
 * @param node An xml node object
 * @param attrlist  A list of attribute names which is terminated by a NULL pointer.
 * @return The first attribute not in the list, or NULL if all attributes are in the list.
 */
XAPI char * ism_xml_checkAttributes(xdom * dom, xnode_t * node, const char * * attrlist);

/**
 * xml dump of the DOM for debugging.
 * The low level IO file number is used to indicate the file so that the
 * code works when it is compiled with a different version of the C runtime
 * than the invoking code.
 * @param dom       An xml dom object
 * @param fileno    The low level file number to write the dump to.
 */
XAPI void ism_xml_dump(xdom * dom, int fileno);

#ifdef __cplusplus
}
#endif

#endif
