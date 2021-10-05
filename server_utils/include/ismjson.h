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
#ifndef __JSON_DEFINED
#define __JSON_DEFINED

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

/* @file ismjson.h
 *
 * Although we only use a small subset of JSON, this is a complete JSON parser
 * and low level encoder for JSON.  It is designed to allow parsing and encoding
 * of JSON with space allocated on the stack for all small messages and to overflow
 * into the heap for larger messages.
 *
 * It is designed to be very fast with very few memory allocations.  The resulting
 * parsed structure has minimal linking and is designed for linear processing.
 *
 * This parser works fully in bytes so it assumes the invoker has handled UTF-8 correctly.
 * It only honors ASCII whitespace.
 */

/*
 * JSON entry types
 */
enum json_obj_type_e {
    JSON_String   = 1,   /* JSON string, value is UTF-8              */
    JSON_Integer  = 2,   /* Number with no decimal point             */
    JSON_Number   = 3,   /* Number which is too big or has a decimal */
    JSON_Object   = 4,   /* JSON object, count is number of entries  */
    JSON_Array    = 5,   /* JSON array, count is number of entries   */
    JSON_True     = 6,   /* JSON true, value is not set              */
    JSON_False    = 7,   /* JSON false, value is not set             */
    JSON_Null     = 8    /* JSON null, value is not set              */
};


/*
 * JSON entry object
 */
typedef struct ism_json_entry_t {
    int    objtype;        /* JSON entry type                               */
    int    count;          /* Count for object and array, value for integer */
    int    level;          /* Level of JSON entry in the tree               */
    int    line;           /* The line number in the source                 */
    const char * name;     /* utf-8 entry name when in an object            */
    const char * value;    /* utf-8 entry value for string and number       */
} ism_json_entry_t;


/*
 * JSON object.
 *
 * This object is designed to allow it to be placed in the stack and to allow the
 * extended pieces to start in the stack and be moved to the heap only if they grow.
 *
 * The bytes in the source array are modified during the parse, and should be
 * copied if this is a problem.  The entries after parse will have pointers into
 * the source array.
 *
 */
 typedef struct ism_json_t {
    ism_json_entry_t *  ent;            /* Entry array         */
    char *              source;         /* Source string       */
    int                 src_len;        /* Source length       */
    int                 ent_alloc;      /* Allocated entries   */
    int                 ent_count;      /* Used entries        */
    uint8_t             free_source;    /* Source should be freed when done */
    uint8_t             free_ent;       /* Entry array should be freed when done */
    uint8_t             options;        /* 0x01=allow comments */
    uint8_t             free_parseobj;  /* Free the parse object */
    int                 rc;             /* Return code from parse */
    int                 line;           /* Offset where error was found */
    char *              data;           /* Used for data allocation during structure build */
    uint32_t            data_len;       /* Amount of allocated data */
    uint8_t             free_data;      /* Data needs to be freed */
    uint8_t             resvx [3];
    uint64_t            resv [4];       /* Reserved for later use */
    char *              pos;            /* Internal use during parse */
    int                 left;           /* Internal use during parse */
    int                 level;          /* Internal use during output */
    int8_t              indent;         /* Indentation level for output.  Valid values are 0 to 8 */
    int8_t              extra_indent;   /* Indent inherit from parent */
    uint8_t             compress;       /* Compress options for output.  Or of values in JSON_OUT_* */
    uint8_t             first;          /* Used during output, next value does not require comma */
    uint8_t             firstline;      /* Do not put nl before first line */
    uint8_t             simplearray;    /* Used on output to track a simple array */
    uint8_t             resvxi [2];
    concat_alloc_t *    buf;            /* Buffer for output. When this is set use direct to writer */
    uint32_t            data_pos;       /* Position in allocated data */
    uint32_t            resvi;
} ism_json_t;

typedef ism_json_t ism_json_parse_t;

#define JSON_OUT_COMPACT 0x01      /* No extra spaces around JSON syntax */
#define JSON_OUT_TABS    0x02      /* Use tabs for indent */
#define JSON_OUT_ARRAY   0x04      /* Do not put array items on separate lines */

/*
 * Allow C like comments in JSON source.
 * JSON is defined without any comment form which is fine for transmission but not as good when
 * JSON is used for config files.  Allow C style comments at any place where white space is allowed.
 */
#define JSON_OPTION_COMMENT  0x01

/*
 * Check if this is JSON.
 *
 * This is not a complete parse, it is asking if the first non-whitespace
 * character is a JSON object or array starter.  Thus a 0 answer means that
 * this is not valid JSON, but a 1 does not guarantee that it is valid.
 *
 * @param buf     The buffer to check for JSON
 * @param len     The length of the buffer
 * @param comment If non-zero, allow C style comments
 * @return 0=not JSON, 1=JSON
 */
XAPI int ism_json_isJSON(const char * buf, int len, int comment);

/*
 * Initialize a JSON object for parsing
 *
 * The JSON object and it sub-objects are normally kept in the stack so that it is automatically freed
 * when no longer in use.  However, when the sub-objects grow too large they are moved to the heap and
 * must be freed.
 *
 * @param jobj    The JSON object to initialize.  If NULL the JSON object will be allocated
 * @param src     The JSON source.  This memory will be modified by a parse
 * @param src_len The length of the JSON source.
 * @param ent     The entry array
 * @param ent_alloc The number of allocated entries
 * @param options The parse options
 */
XAPI ism_json_t ism_json_newParseObject(ism_json_t jobj, char * src, int src_len, ism_json_entry_t * * ent, int ent_alloc, int options);

/*
 * Allow a JSON object to be reused for a new parse
 *
 * The existing entry array is retained but is emptied.  Other fields are set to the initial values.
 *
 * @param jobj    The JSON object to initialize.  If NULL the JSON object will be allocated
 * @param src     The JSON source.  This memory will be modified by a parse
 * @param src_len The length of the JSON source.
 */
XAPI void ism_json_clearParseObject(ism_json_t jobj, char * src, int src_len);

/*
 * Parse the JSON stream
 *
 * The JSON stream is parsed based on the settings in the parse objects.
 *
 * Before calling parse it is necessary to set up the JSON object.  This can be done using ism_json_newParseObject()
 * or ism_json_clearParseObject() or by setting the fields manually.
 * - ent - An array of ism_json_entry_t objects with a count specified by ent_count.  This is
 *         commonly allocated in the stack.  If it is allocated in the heap then free_ent should be set.
 * - ent_alloc - The array size of ent.  If zero then the entries will be allocated in the heap as required.
 * - source - The JSON source.  These bytes are modified and must be kept allocated for the life of
 *         the JSON object.  It is common to copy the source for this purpose.  If the source is copied
 *         into the heap, free_source should be set.
 * - options - The parse options.  Currently this specified if comments are allowed
 *  -
 * @param jobj   The json object.
 * @return A return code, 0=good
 */
XAPI int ism_json_parse(ism_json_t * jobj);

/*
 * Free any parts of a JSON object which are allocated in the heap
 *
 * @param jobj   The JSON object
 */
XAPI void ism_json_free(ism_json_t * jobj);


/*
 * Create a string representation of a JSON object.
 *
 * This uses the JSON object as a JSON writer object and output all entries contained in the
 * specified entry.  Various forms of formatting are allowed.  The output buffer will always
 * be null terminated (a zero byte at the buf->used position).
 *
 * @param jobj The JSON object
 * @param buf  The output buffer
 * @param entnum  Which entry to start with, 0 will output the entire JSON object
 * @param indent  The size to indent 0 to 8.  A value of 0 means that no line ends are added
 * @param options The output options (see JSON_OUT_*)
 * @return A return code: 0=good
 */
XAPI int ism_json_toString(ism_json_t * jobj, concat_alloc_t * buf, int entnum, int indent, int options);

/*
 * Return a string from a JSON entry even for things without a value.
 * This is intended to be used for error handling.
 * @param ent  A JSON entry object
 * @return A string containing the value or type of the entry
 */
const char * ism_json_getJsonValue(ism_json_entry_t * ent);

/*
 * Check if a string is a valid JSON number.
 *
 * JSON allows a subset of the valid number strings used by C or Java
 * @param str   The input string
 * @return A return value:  0=invalid, 1=integer, 2=number
 */
XAPI int ism_json_isValidNumber(const char * str);

/*
 * Check if a string is a valid JSON string
 *
 * A valid JSON string must be valid UTF-8
 * @param str   The input string
 * @return A return value:  0=invalid, 1=valid
 */
XAPI int ism_json_isValidString(const char * str);

/*******************************************************************************
 * JSON get functions which allow a simple lookup of a name.  These are useful
 * when the JSON represents a simple structure like a properties array.
 *******************************************************************************/

/*
 * Get a field from a JSON object
 * @param jobj   The JSON object
 * @param entum  The starting entry number for the search.  This must be an object.
 * @param name   The name to search for
 * @return The entry in the parse object, or -1 to indicate it is not found
 */
XAPI int ism_json_get(ism_json_t * jobj, int entnum, const char * name);

/*
 * Get a string from a JSON object
 */
XAPI const char * ism_json_getString(ism_json_t * jobj, const char * name);

/*
 * Get an integer from a JSON object
 */
XAPI int ism_json_getInt(ism_json_t * jobj, const char * name, int deflt);

/*
 * Get an integer from a JSON object
 */
XAPI int ism_json_getInteger(ism_json_t * jobj, const char * name, int deflt);


/*******************************************************************************
 * JSON structure creation and output methods.  These methods are used to build
 * up a JSON structure into a JSON object, or to directly output as a string.
 * They define the JSON structure and must be processed in order.  The most common
 * use of these methods is to convert a data structure into either a JSON string
 * or JSON parsed object.
 *
 * These methods have minimal validity checking.  If the JSON is being constructed
 * from untrusted data then additional checking provided by ism_json_isValidNumber()
 * and ism_json_isValidString() should be done before calling these methods.
 *******************************************************************************/

/*
 * Initialize a JSON object for writing
 *
 * The json object is used for both parsing and output and in both cases can be allocated
 * in the stack which allows for automatic cleanup and fast processing.  When used only
 * as a writer no cleanup is required.
 *
 * @param jobj    The JSON object.  If this is NULL, a JSON object will be allocated
 * @param buf     The buffer to write to
 * @param indent  The size to indent 0 to 8.  A value of 0 means that no line ends are added
 * @param options The output options (see JSON_OUT_*)
 * @return The JSON object
 */
XAPI ism_json_t * ism_json_newWriter(ism_json_t * jobj, concat_alloc_t * buf, int indent, int options);

/*
 * If the json object allocated a buffer then free it
 */
XAPI void ism_json_closeWriter(ism_json_t * jobj);

/*
 * Start an array
 * @param jobj   The JSON object
 * @param name   The name of the item which can be null
 */
XAPI void ism_json_startArray(ism_json_t * jobj, const char * name);

/*
 * Start an object
 * @param jobj   The JSON object
 * @param name   The name of the item which can be null
 */
XAPI void ism_json_startObject(ism_json_t * jobj, const char * name);

/*
 * Put a boolean item which is true, false, or null
 * @param jobj   The JSON object
 * @param name   The name of the item which can be null
 * @param value  true=>0, false=0, null=<0
 */
XAPI void ism_json_putBooleanItem(ism_json_t * jobj, const char * name, int value);

/*
 * Put an integer item
 *
 * JSON does not distinguish between signed 32 bit integers like this and any other
 * number, but they are common in small JSON and are handled separately for
 * convenience.
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The value
 */
XAPI void ism_json_putIntegerItem(ism_json_t * jobj, const char * name, int value);

/*
 * Put an long item
 *
 * If a long can be represented as a signed 32 bit integer, this method will do so,
 * otherwise it will create a base number item.  This method is commonly used for
 * statistics.
 *
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The value
 */
void ism_json_putLongItem(ism_json_t * jobj, const char * name, int64_t value);

/*
 * Put an ulong item
 *
 * If a long can be represented as a signed 32 bit integer, this method will do so,
 * otherwise it will create a base number item.  This method is commonly used for
 * statistics.
 *
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The value
 */
void ism_json_putULongItem(ism_json_t * jobj, const char * name, uint64_t value);
/*
 * Put a number item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The string form of the number.  This is not checked for validity.
 */
XAPI void ism_json_putNumberItem(ism_json_t * jobj, const char * name, const char * value);

/*
 * Put a string item
 * @param jobj   The JSON object
 * @param name   The name of the item
 * @param value  The value as a UTF-8 string.  This is not checked for valid UTF-8.
 */
XAPI void ism_json_putStringItem(ism_json_t * jobj, const char * name, const char * value);

/*
 * Handle the comma separator, indentation, and putting out the name of an item.
 * The indent can be 0 to 8 bytes where zero also implies no line end.
 */
XAPI void ism_json_putIndent(ism_json_t * jobj, int comma, const char * name);

/*
 * End the current array
 *
 * This will also null terminate the buffer if it is used for direct C output.
 * @param The JSON object
 * @return The new level
 */
XAPI int ism_json_endArray(ism_json_t * jobj);

/*
 * End the current object
 *
 * This will also null terminate the buffer if it is used for direct C output.
 * @param The JSON object
 * @return A return code 0=good
 */
XAPI int ism_json_endObject(ism_json_t * jobj);

/*******************************************************************************
 * JSON low level put functions which allow a simple output in JSON format.
 * These are used to implement a logical level methods.
 * However, the invoker is responsible for formatting so these are not best the
 * best way to output complex JSON objects.
 *******************************************************************************/
/*
 * Encode a field in JSON.
 *
 * @param buf   The buffer to write to
 * @param name  The name of the field.  If this is NULL, only the value is written
 * @param value The value of the field
 * @param notfirst
 */
XAPI void ism_json_put(concat_alloc_t * buf, const char * name, ism_field_t * value, int notfirst);


/*
 * Encode a string in JSON with escaping.
 *
 * @param buf    The buffer to write to
 * @param str    The string to encode
 */
XAPI void ism_json_putString(concat_alloc_t * buf, const char * str);

/*
 * Write out a set of bytes in JSON
 *
 * These bytes are directly written to the JSON stream and are not interpreted in any way.
 * @param buf   The buffer to write to
 * @param str   The string of bytes to write
 */
XAPI void ism_json_putBytes(concat_alloc_t * buf, const char * str);

/*
 * Make sure the buffer is null terminated
 * @param buf   The buffer to write to
 */
XAPI void ism_json_putNull(concat_alloc_t * buf);

/*
 * Put out a set of bytes with JSON string escapes
 * @param buf   The buffer to write to
 * @param str   The address of the bytes to write
 * @param len   The length of the bytes to write
 */
XAPI void ism_json_putEscapeBytes(concat_alloc_t * buf, const char * str, int len);

/*
 * Encode an integer in JSON.
 * @param buf    The buffer to write to
 * @param value  The value of the integer to write.
 */
XAPI void ism_json_putInteger(concat_alloc_t * buf, int value);

void ism_json_convertMemoryStatistics(ism_json_t * jobj, ism_MemoryStatistics_t * stats);

#ifdef __cplusplus
}
#endif
#endif
