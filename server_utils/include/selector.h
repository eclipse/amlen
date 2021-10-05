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

#include <ismutil.h>
#include <ismmessage.h>

#ifndef __SELECTOR_DEFINED
#define __SELECTOR_DEFINED

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Basic rule definition
 */
typedef struct  ismRule_t {
    uint16_t   len;
    uint8_t    op;
    uint8_t    kind;
} ismRule_t;

/* Integer extension */
typedef struct {
    ismRule_t    rule;
    int32_t       i;
} ism_Int_Rule_t;

/* String extension */
typedef struct {
    ismRule_t    rule;
    char *        s;
} ism_String_Rule_t;


/*
 * User data structure for propgen
 */
typedef struct ism_emsg_t {
    ismMessageHeader_t * hdr;
    const char *         props;
    int                  proplen;
    int                  otherprop_len;
    const char *         topic;
    void *               otherprops;
} ism_emsg_t;

/*
 * Define the values for selection.
 * Note the reversal of the common values for true and false.
 */
#define SELECT_TRUE     0
#define SELECT_FALSE    1
#define SELECT_UNKNOWN -1


/*
 * Base rules for the selection engine
 * Those labeled as internal are not supported in JMS
 */
enum BaseRules {
    SELRULE_End         = 0x00,  /* Left paren or end of string     kind=level      */
    SELRULE_Begin       = 0x01,  /* Right paren or start of string  kind=level      */
    SELRULE_In          = 0x02,  /* IN operator                     kind=count      */
    SELRULE_Like        = 0x03,  /* LIKE operator                   kind=count      */
    SELRULE_Between     = 0x05,  /* BETWEEN operator                kind=2          */
    SELRULE_Compare     = 0x06,  /* comparison operator             kind=CMP_e      */
    SELRULE_Boolean     = 0x07,  /* boolean constant                kind=value      */
    SELRULE_Int         = 0x08,  /* integer constant                kind unused     */
    SELRULE_Long        = 0x09,  /* long constant                   kind unused     */
    SELRULE_Float       = 0x0A,
    SELRULE_Double      = 0x0B,  /* double constant                 kind unused     */
    SELRULE_String      = 0x0C,  /* string constant                 kind unused     */
    SELRULE_Simple      = 0x0D,  /* Simple op                       kind=SimpleRules*/
    SELRULE_Negative    = 0x0E,  /* unary negative operator         kind unused     */
    SELRULE_Not         = 0x0F,  /* unary not operator              kind unused     */
    SELRULE_And         = 0x10,  /* AND operator                    kind=level      */
    SELRULE_Or          = 0x11,  /* OR operator                     kind=level      */
    SELRULE_Calc        = 0x12,  /* numeric operator                kind=which      */
    SELRULE_Var         = 0x13,  /* named field                     kind=unused     */
    SELRULE_Is          = 0x14,  /* IS operator                                     */
    SELRULE_ACLCheck    = 0x15,  /* Internal ALCheck operator       kind=count      */
    SELRULE_InHash      = 0x16,  /* Internal InHash                 kind=unused     */
    SELRULE_Internal    = 0x17,  /* Internal Compile Options        kind=opt        */
    SELRULE_Topic       = 0x18,  /* Internal Topic name             kind=unused     */
    SELRULE_TopicPart   = 0x19,  /* Internal Topic component        kind=which      */
    SELRULE_QoS         = 0x1A,  /* Internal Load QoS               kind=unused     */
    SELRULE_SmallInt    = 0x1B,  /* Internal Load one byte int      kind=value      */
};


/**
 * Compile a selection rule.
 * The selection rule syntax is based on SQL92 predicates.
 * The rule object must be freed using ism_common_freeSelectRule() when it is no longer needed.
 * <p>
 * The rule can be copied and used from the copied location, but it must put in at least 4 byte
 * aligned memory.  The optional outlen parameter gives the number of bytes which must be copied.
 *
 * @param xrule     The output rule
 * @param outlen    The returned length of the rule in bytes (optional)
 * @param ruledef   The string containing the selection rule
 * @return A return code: 0=good, error=LLMDE_e
 */
XAPI int ism_common_compileSelectRule(ismRule_t * * xrule, int * outlen, const char * ruledef);

/**
 * Compile a selection rule with options.
 * The selection rule syntax is based on SQL92 predicates.
 * The rule object must be freed using ism_common_freeSelectRule() when it is no longer needed.
 * <p>
 * The rule can be copied and used from the copied location, but it must put in at least 4 byte
 * aligned memory.  The optional outlen parameter gives the number of bytes which must be copied.
 *
 * @param xrule     The output rule
 * @param outlen    The returned length of the rule in bytes (optional)
 * @param ruledef   The string containing the selection rule
 * @param options   Options bits
 * @return A return code: 0=good, error=ISMRC*
 */
XAPI int ism_common_compileSelectRuleOpt(ismRule_t * * xrule, int * outlen, const char * ruledef, int options);

#define SELOPT_Internal      1

/**
 * Free up a selection rule which is no longer needed
 * @param rule  The selection rule
 */
XAPI void ism_common_freeSelectRule(ismRule_t * rule);

/**
 * Dump the select rule to the specified buffer.
 * @param rule  The select rule
 * @param buf   The buffer to return the output
 * @param len   The length of the buffer
 * @return The length of the data returned, or <0 to indicate an error
 */
XAPI int ism_common_dumpSelectRule(ismRule_t * rule, char * buf, int len);

/**
 * Select a message.
 *
 * The only message area which is used is the properties.
 *
 * @param hdr      The message header
 * @param area     The message area count
 * @param areatype The array of area types
 * @param areasize The array of area sizes
 * @parma areaptr  The array of area ponters
 * @param topic    The topic name (if null, the topic name in the properties is used)
 * @param rule     The selection rule pointer (must be 4 byte aligned)
 * @param rulelen  The rule length (generally ignored)
 */
XAPI int ism_common_selectMessage(
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        const char *               topic,
        void *                     rule,
        size_t                     rulelen);

/**
 * Compute the length of the match string.
 * This allows the creator of a match string to allocate the space for the
 * parsed match rule.
 * @param str  The match string
 * @return The length to allocate for the parsed like rule
 */

XAPI int ism_common_matchlen(const char * str);
/**
 * Convert a matchrule.
 * This can be done in place if the space available meets the size returned
 * by ism_common_matchlen().
 * @param str    The input string
 * @param match  The output like rule.  Must be the size returned by ism_common_matchlen()
 * @param escape The escape character (or 0 if none)
 */
XAPI void ism_common_convertmatch(const char * str, char * match, char escape);

/*
 * Match a like rule
 */
XAPI int ism_common_likeMatch(const char * str, int len, const uint8_t * match);

/*
 * Access control list object
 */
typedef struct ism_acl_t {
    ismHashMap *        hash;
    pthread_spinlock_t  lock;
    char *  name;
    void *  object;
} ism_acl_t;

/*
 * Check that the key is in the ACL list
 * @param  key     The ACL key
 * @param  aclname The name of the ACL
 * @return 0=found, 1=not-found, -1=no-ACL
 */
XAPI int ism_protocol_checkACL(const char * key, const char * aclname);

/*
 * Find the ACL and create it if requested.
 * @param name   The name of the ACL
 * @param create If non-zero, create the ACL if it does not exist
 * @return The locked ACL or NULL
 */
XAPI ism_acl_t * ism_protocol_findACL(const char * name, int create);

/*
 * Complete the handling of an ACL
 * The ACL is automatically locked when it is found, so this method releases it.
 * @param acl  The ACL
 */
XAPI void ism_protocol_unlockACL(ism_acl_t * acl);

/*
 * Add an item to an ACL set.
 * This must be called with the ACL set locked.
 * @param acl  The ACL
 * @param key  The key to add
 */
XAPI void ism_protocol_addACLitem(ism_acl_t * acl, const char * key);

/*
 * Delete an item to an ACL set.
 * This must be called with the ACL set locked.
 * @param acl   The ACL
 * @param key   The key to remove4
 */
XAPI void ism_protocol_delACLitem(ism_acl_t * acl, const char * key);

/*
 * Callback when ACL is created
 */
typedef void (* ism_ACLcallback_f)(ism_acl_t * acl);
typedef void (* ism_setACLcb_f)(const char * aclsrc, int acllen);

/*
 * Set ACLs from an ACL source.
 * The ACL source is a set of lines terminated by an combination of CR, LF, and NUL.
 * The first character is the operator and the rest of the line is the data.
 * '/' = comment, the data is ignored
 * '@' = create or update an ACL set, the data is the name of the ACL set
 * '!' = delete an ACL set, the data is the name of the ACL set
 * '+' = add an ACL item to the current set, the data is the item key
 * '-' = delete an ACL item from the current set, the data is the item key
 *
 *
 * @param aclsrc  The ACL definitions
 * @paarm acllen  The length of the ACL.  If this is negative, use the string length.
 * @parse options Options for setting the ACL
 * @param create_cb   A callback for when an ACL is created.  Used to add the object itself to its ACL.
 */
XAPI int ism_protocol_setACL(const char * aclsrc, int acllen, int opt, ism_ACLcallback_f create_cb);

XAPI int ism_protocol_setACLfile(const char * name, ism_ACLcallback_f create_cb);
XAPI int ism_protocol_setACLfile2(const char * name, ism_ACLcallback_f create_cb, ism_setACLcb_f send_cb);

/*
 * Get the count of items in the ACL
 * @param name  The name of the ACL
 * @return The count of items in the ACL or -1 if the ACL does not exist
 */
int ism_protocol_getACLcount(const char * name);

/*
 * Delete an ACL
 * @param name  The name of the ACL
 * @return 0=deleeted, 1=does not exist
 */
XAPI int ism_protocol_deleteACL(const char * name, ism_ACLcallback_f create_cb);

/*
 * Get the ACL source into a buffer
 * @param buf      The output buffer
 * @param aclname  The name of the ACL
 * @return  An ISMRC return code, 0=good
 */
XAPI int ism_protocol_getACLs(concat_alloc_t * buf, const char * aclname);

/*
 * Get the ACL source
 */
XAPI int ism_protocol_getACL(concat_alloc_t * buf, ism_acl_t * acl);

/*
 * Callback to generate properties.
 *
 * This callback can be used to generate one or all properties.  This is called to fill in and/or return
 * a property by name.  This could be used to generate each item, or to generate all items on first use.
 * @param props    The properties object.  The generator can update this object
 * @param userdata The userdata send on the filter
 * @param name     The name of the property to generate
 * @param field    The location of the field to return
 * @param extra    A field with additional data.  This is based on the name to generate
 */
typedef int (* property_gen_t)(ism_prop_t * props, ism_emsg_t * emsg, const char * name,
        ism_field_t * field, void * extra);

/*
 * Do message selection.
 *
 * The properties can either be available in a properties
 * @param rule   The compiled selection rule
 * @param props  The properties object.  If this is NULL, the generator must create each property on request.
 * @param generator The method to generate properties on first use.  If this is NULL, all values properties
 *                  must be set before making the call.
 * @param emsg    Data for the properties generator
 *
 */
XAPI int ism_common_filter(ismRule_t * rule, ism_prop_t * props, property_gen_t generator, ism_emsg_t * emsg);



#ifdef DECLARE_NAMES
/*
 * Base SEL rule names (used for dump)
 */
char * BaseRuleName [29] = {
    "End",
    "Begin",
    "In",
    "Like",
    "4",
    "Between",
    "Compare",
    "Boolean",
    "Int",
    "Long",
    "Float",
    "Double",
    "String",
    "Simple",
    "Negative",
    "Not",
    "And",
    "Or",
    "Calc",
    "Var",
    "Is",
    "ACLCheck",
    "InHash",
    "Options",
    "Topic",
    "TopicPart",
    "QoS",
    "SmallInt",
    NULL
};
#endif

/*
 * Token types returned by getSelectToken.
 * These are also used in the operator stack.
 */
enum tokenType {
    TT_None       = 0,    /* End of string                  */
    TT_Name       = 1,    /* Unquoted name                  */
    TT_String     = 3,    /* Quoted string                  */
    TT_Int        = 4,    /* Integer constant               */
    TT_Long       = 5,    /* Long constant                  */
    TT_Float      = 6,
    TT_Double     = 7,    /* Double constant                */
    TT_Group      = 8,    /* Group separator (comma)        */
    TT_Begin      = 9,    /* Begin (left parenthesis)       */

    TT_MinOp      = 10,
    TT_End        = 10,   /* End (right parenthesis)        */
    TT_EQ         = 11,   /* Compare equals                 */
    TT_NE         = 12,   /* Compare not equals             */
    TT_GT         = 13,   /* Compare greater than           */
    TT_LT         = 14,   /* Compare less than              */
    TT_GE         = 15,   /* Compare greater than or equal  */
    TT_LE         = 16,   /* Compare less than or equal     */
    TT_Minus      = 17,   /* Substract operation            */
    TT_Plus       = 18,   /* Addition operation             */
    TT_Multiply   = 19,   /* Multiply operation             */
    TT_Divide     = 20,   /* Divide operation               */
    TT_Or         = 22,   /* Logical OR                     */
    TT_And        = 23,   /* Logical AND                    */

    TT_Exists     = 24,   /* Unary postfix exists op        */
    TT_In         = 25,   /* IN operator                    */
    TT_Is         = 26,   /* IS operator                    */
    TT_Like       = 27,   /* LIKE operator                  */
    TT_Between    = 28,   /* BETWEEN operator               */
    TT_Between2   = 29,   /* AND following a between        */
    TT_ACLCheck   = 30,   /* ACL check                      */
    TT_InHash     = 31,   /* InHash operator (@)            */
    TT_MaxOp      = 31,
    TT_NotNot     = 32,   /* Unary is not true              */
    TT_Exclam     = 33,   /* Unary is not false             */
    TT_True       = 34,   /* Constant TRUE                  */
    TT_False      = 35,   /* Constant FALSE                 */
    TT_Null       = 36,   /* NULL name                      */
    TT_Not        = 37,   /* Unary not                      */
    TT_Escape     = 38,   /* ESCAPE after LIKE              */
    TT_Negative   = 39,   /* Unary negation                 */
    TT_Amp        = 40,   /* Amersand                       */ 

    TT_TooLong    = -1,
    TT_QuoteErr   = -2,
    TT_BadToken   = -3
};

/*
 * Kind of compares.
 * Compare rules take two values, coerse them as required, and compare them.
 * The result can be true, false, or unknown.
 */
enum CompareRules {
    CMP_EQ       = 0,
    CMP_NE       = 1,
    CMP_GT       = 2,
    CMP_LT       = 3,
    CMP_GE       = 4,
    CMP_LE       = 5,
};
#define MAX_COMPARE_OP 5

#ifdef DECLARE_NAMES
/*
 * Simple rule names (used to dump)
 */
char * SimpleRuleName [12]  = {
    "IsNull",
    "IsNotNull",
    "Exists",
    "NotExists",
};

/*
 * Compare names (used to dump)
 */
char * CmpRuleName [8] =  { "EQ",  "NE",   "GT",  "LT",  "GE",   "LE"  };
char * CmpRuleNameX [8] = { " = ", " <> ", " > ", " < ", " >= ", " <= " };
#endif

#ifdef __cplusplus
}
#endif
#endif
