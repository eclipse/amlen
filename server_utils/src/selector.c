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
#define DECLARE_NAMES
#include <selector.h>
#ifndef _WIN32
#include <limits.h>
#endif

/*
 * Implement the message selector compiler.
 */

/*
 * Priorities
 */
#define P_NONE  0  /* Not an operator          */
#define P_OR    1  /* or                       */
#define P_AND   2  /* and                      */
#define P_NOT   3
#define P_OP    4  /* in, is, like, between, @ */
#define P_COMP  5  /* = <> > < >= <=           */
#define P_PLUS  6  /* binary - +               */
#define P_MUL   7  /* binary * /               */
#define P_UNARY 8  /* unary NOT - convert      */
/*
 * Map from token type to priority
 */
char op_priority[41] = {
    P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE,
    P_NONE, P_COMP, P_COMP, P_COMP, P_COMP, P_COMP, P_COMP, P_PLUS, P_PLUS, P_MUL,
    P_MUL,  P_NONE, P_OR,   P_AND,  P_NONE, P_OP,   P_OP,   P_OP,   P_OP,   P_OP,
    P_OP,   P_OP,   P_NOT,  P_NOT,  P_NONE, P_NONE,  P_NONE, P_NOT, P_NONE, P_UNARY,
    P_NONE,
};


 /*
  * Character types
  * 1 = Valid name character
  * 2 = Whitespace
  */
static char CharType[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 0, 0, /* 00           */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10           */
    2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, /* 20 $     .   */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 30 0 - 9 :   */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 50    \ ^    */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 80           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 90           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* A0           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* B0           */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* C0           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* D0           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* E0           */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, /* F0           */
};


#define isWhiteSpace(pos) ((CharType[(unsigned char)(pos)]&2))
#define isNameChar(pos)   ((CharType[(unsigned char)(pos)]&1))

/*
 * Select rule compile states
 */
enum CompileState {
    CS_Operand   = 1,   /* Looking for an operand         */
    CS_Op        = 2,   /* Looking for an op code         */
    CS_Group     = 3,   /* Start of group processing      */
    CS_Done      = 4
};

 /*
  * Rule compilation structure.
  * This keeps all values needed during the compile step.
  */
typedef struct {
    char *       ruledef;      /* The input rule definition           */
    char *       buf;          /* The allocated buffer                */
    int          buflen;       /* The allocated length of the buffer  */
    int          bufpos;       /* The used position in the buffer     */
    int          level;        /* The parenthesis level               */
    int          count;        /* The count of items in a group       */
    int          error;        /* Set non-zero if there is an error   */
    int          offset;       /* The error offset 0=no error         */
    int          rc;           /* The return code                     */
    int          op_pos;       /* The current op code stack level     */
    uint8_t      opt_internal; /* Internal option                     */
    uint8_t      resv[1];
    uint16_t     options;
    int          max_count;    /* Max count in a function group       */
    int          prevrule;
    int          savekind;
    char         savetoken[256];
    uint16_t     oplvl[64];    /* The position in the op stack per level */
    uint8_t      opstack[256];
} rulecomp_t;


/*
 * Forward references
 */
static int compile(rulecomp_t * rulec, char * token, int tokenlen);
static int  isNegative(rulecomp_t * rulec);
static int  checkNumeric(rulecomp_t * rulec, int op, int kind);
static int  checkBoolean(rulecomp_t * rulec, int op, int kind);
static int  checkCompare(rulecomp_t * rulec, int op, int kind);
static int  checkIdentifier(rulecomp_t * rulec, int kind);
static const char * opname(int op, int kind);

/*
 * Get a selector token.
 * This is the tokenizer for the Select Compiler
 * @param  token  The buffer to return the token.
 * @param  tokenlen  The length of the token buffer
 * @param  xpos   The location to parse.  This is both an input and and output
 * @return The token type (see enum tokenTypes).  Negative values indicate an error.
 */
static int getSelectToken(char * token, int tokenlen, char * * xpos) {
    char * pos = *xpos;
    char * eos;
    int    len = 0;
    int    ret = 0;

    if (tokenlen-- < 1)       /* Leave room for the trailing null */
        return 0;
    *token = 0;

    /* Go past any while space */
    while (*pos && isWhiteSpace(*pos))
        pos++;
    if (!*pos) {
        return 0;    /* No token found */
    }

    /*
     * Look for a quoted string
     */
    if (*pos == '\'') {
        pos++;
        while (*pos) {
        	if (*pos == '\'') {
        		if (pos[1] != '\'')
        			break;
                pos++;
        	}
            if (len >= tokenlen) {
                token[len] = 0;
                return TT_TooLong;
            }
            token[len++] = *pos++;
        }
        token[len] = 0;
        if (!*pos) {
            return TT_QuoteErr;
        } else {
            pos++;
            *xpos = pos;
            return TT_String;
        }
    }
    /*
     * Parse special operators
     */
    switch (*pos) {
    case '+':  ret = TT_Plus;       break;
    case '-':  ret = TT_Minus;      break;
    case '<':
        if (pos[1]=='=') { ret = TT_LE; pos++;  break; }
        if (pos[1]=='>') { ret = TT_NE; pos++;  break; }
        ret = TT_LT;                break;
    case '>':
        if (pos[1]=='=') { ret = TT_GE; pos++;  break; }
        ret = TT_GT;                break;
    case '=':  ret = TT_EQ;         break;
    case '(':  ret = TT_Begin;      break;
    case ')':  ret = TT_End;        break;
    case ',':  ret = TT_Group;      break;
    case '*':  ret = TT_Multiply;   break;
    case '/':  ret = TT_Divide;     break;
    case '!':
        if (pos[1]=='!') {
            ret = TT_NotNot;
            pos++;
        } else {
            ret = TT_Exclam;
        }
        break;
    case '@':  ret = TT_InHash;     break;
    case '&':  ret = TT_Amp;        break;
    };
    if (ret) {
        *xpos = ++pos;
        return ret;
    }
    /*
     * Accumulate a name
     */
    while (*pos && isNameChar(*pos)) {
        if (len >= tokenlen) {
            token[len] = 0;
            return TT_TooLong;
        }
        token[len++] = *pos++;
    }
    token[len] = 0;
    *xpos = pos;
    if (len == 0)
       return TT_BadToken;

    /*
     * Check for number
     */
    if ((*token >= '0' && *token<='9') || *token=='.') {
        uint64_t lval;
        xUNUSED double   dval;
        int islong = 0;
        int isfloat = 0;
        if (*token == '0' && token[1]== 'x' && token[2]) {
            lval = strtoull(token+2, &eos, 16);
        } else {
            lval = strtoull(token, &eos, 0);
            if (*eos=='L' || *eos=='l') {
            	islong = 1;
            	eos++;
            }
            if (*eos && (*eos=='E' || *eos=='e' || *eos=='.' || *eos=='F' || *eos=='f')) {
                /*
                 * Handle the special case where a float value contains a plus or minus
                 * sign after the exponent.  This characer must be considered as part
                 * of the name value rather than as an operator.
                 */
                if ((token[len-1]=='E' || token[len-1]=='e') && (*pos == '-' || *pos == '+')) {
                    int zlen = len;
                    char * zp = pos+1;
                    token[zlen++] = *pos;
                    while(*zp && isNameChar(*zp)) {
                        if (zlen >= 255) {
                            token[zlen] = 0;
                            return TT_TooLong;
                        }
                        token[zlen++] = *zp++;
                    }
                    token[zlen] = 0;
                    dval = strtod(token, &eos);
                    if (*eos=='f' || *eos=='F') {
                        isfloat = 1;
                        eos++;
                    }
                    if (*eos) {
                        token[len] = 0;
                        return TT_BadToken;
                    }
                    *xpos = pos = zp;
                    len = zlen;
                } else {
                    dval = strtod(token, &eos);
                    if (*eos=='f' || *eos=='F') {
                    	isfloat = 1;
                    	eos++;
                    }
                    if (*eos) {
                        return TT_BadToken;
                    }
                }
                return isfloat ? TT_Float : TT_Double;
            } else {
                if (*eos)
                    return TT_BadToken;
            }
        }
        return (lval <= INT_MAX  && !islong) ? TT_Int : TT_Long;
    }

    /*
     * Check for known names
     */
    if ((*token >='A' && *token<='Z') || (*token >='a' && *token <= 'z')) {
        switch (len) {
        case 2:
            if (!strcmpi(token, "in"))
                return TT_In;
            if (!strcmpi(token, "is"))
                return TT_Is;
            if (!strcmpi(token, "or"))
                return TT_Or;
            break;
        case 3:
            if (!strcmpi(token, "and"))
                return TT_And;
            if (!strcmpi(token, "not"))
                return TT_Not;
            break;
        case 4:
            if (!strcmpi(token, "like"))
                return TT_Like;
            if (!strcmpi(token, "null"))
                return TT_Null;
            if (!strcmpi(token, "true"))
                return TT_True;
            break;
        case 5:
            if (!strcmpi(token, "false"))
                return TT_False;
            break;
        case 6:
            if (!strcmpi(token, "escape"))
                return TT_Escape;
            break;
        case 7:
            if (!strcmpi(token, "between"))
                return TT_Between;
            break;
        case 8:
            if (!strcmpi(token, "aclcheck"))
                return TT_ACLCheck;
            break;
        };
        /* Return a normal name */
        return TT_Name;
    }
    return TT_Name;
}


/*
 * Put a rule into the table
 */
static void putrule(rulecomp_t * rulec, int len, int op, int kind, void * data) {
    ismRule_t * rulei;
    int  rulelen = len;
    if (len & 0x03) {
        rulelen = (len+3)&~3;   /* Round up */
    }
    if ((rulelen + rulec->bufpos) > rulec->buflen) {
        rulec->error++;
        return;
    }
    rulei = (ismRule_t *)(rulec->buf + rulec->bufpos);
    rulei->len  = (uint16_t)rulelen;
    rulei->op   = (uint8_t)op;
    rulei->kind = (uint8_t)kind;
    if (len > 4 && data) {
	    memset((char *)(rulei+1), 0, rulelen-4);
        memcpy((char *)(rulei+1), data, len-4);
    }
    rulec->bufpos += rulelen;
    if (op != SELRULE_End)
        rulec->prevrule = op;
}


/*
 * Put out an int rule
 */
static void putrule_int(rulecomp_t * rulec, char * token) {
    int ival = strtoul(token, NULL, 0);
    if (isNegative(rulec)) {
        ival = -ival;
    }
    if (rulec->opt_internal && ival>=0 && ival<=255) {
        putrule(rulec, 4, SELRULE_SmallInt, ival, NULL);
    } else {
        putrule(rulec, 8, SELRULE_Int, 0, &ival);
    }
}


 /*
  * Put out a long rule
  */
static void putrule_long(rulecomp_t * rulec, char * token) {
    int64_t lval = strtoull(token, NULL, 0);
    if (isNegative(rulec)) {
        lval = -lval;
    }
    putrule(rulec, 12, SELRULE_Long, 0, &lval);
}

/*
 * Put out a double rule
 */
static void putrule_double(rulecomp_t * rulec, char * token) {
    double dval = strtod(token, NULL);
    if (isNegative(rulec)) {
        dval = -dval;
    }
    putrule(rulec, 12, SELRULE_Double, 0, &dval);
}

/*
 * Put out a double rule
 */
static void putrule_float(rulecomp_t * rulec, char * token) {
    float dval = (float)strtod(token, NULL);
    if (isNegative(rulec)) {
        dval = -dval;
    }
    putrule(rulec, 8, SELRULE_Float, 0, &dval);
}


/*
 * Compute the length of the match string.
 * When constructing a "like" string operand, we allocate enough
 * space for the compiled match string.  We will do the conversion
 * later since we cannot do it until we have seen the "escape" op.
 */
int ism_common_matchlen(const char * str) {
    int len = (int)(strlen(str)+2);   /* Extra len + null terminator */
	const char * cp = str;
	while (*cp) {
		if (*cp == '_' || *cp == '%')
			len++;
		cp++;
	}
	len += len/100;
    return len;
}


/*
 * Convert a matchrule.
 * This can be done in place if the space available meets the size returned
 * by matchlen().
 * @param str    The input string
 * @param match  The output match rule
 * @param escape The escape character (or 0 if none)
 */
void ism_common_convertmatch(const char * str, char * match, char escape) {
    char * mlenp = NULL;
    char    ch;
    int     kind;

    /*
     * If the input and output are at the same
     */
    if (str == match) {
        char * s = alloca(strlen(str)+1);
        strcpy(s, str);
        str = s;
    }

    /*
     * Convert characters
     */
    while (*str) {
        kind = 0;
        ch = *str;
        if (*str == escape) {          /* If this is the escape character. use it */
            if (str[1])
                ch = *++str;
        } else {                       /* Check for specials */
            if (*str == '%')
                kind = 0xff;
            else if (*str == '_')
                kind = 0xfe;
        }
        if (kind) {                    /* If this is a special */
            *match++ = (char)kind;
            mlenp = NULL;
        } else {
            if (!mlenp || *mlenp > 100) {
                mlenp = match++;
                *mlenp = 0;
            }
            *match++ = ch;
            (*mlenp)++;

        }
        str++;
    }
    *match = 0;
}


/*
 * Check if there is a unary minus on the top of the op stack, and remove it if there is.
 */
static int  isNegative(rulecomp_t * rulec) {
    if (rulec->op_pos > rulec->oplvl[rulec->level]) {
        if (rulec->opstack[rulec->op_pos-1] == TT_Negative) {
            rulec->op_pos--;
            return 1;
        }
    }
    return 0;
}


/*
 * Put the op on the stack, checking for overflow
 */
static int putOpStack(rulecomp_t * rulec, int op) {
	int rc = 0;
	/* There is no good reason that this should ever happen */
	if (rulec->op_pos >= 255) {
	    TRACE(1, "The selection rule is too complex: %s", rulec->ruledef);
	    rulec->rc = ISMRC_TooComplex;
	    ism_common_setError(rulec->rc);
	    return rulec->rc;
	}
	switch (op) {
	case TT_GT:
	case TT_LT:
	case TT_GE:
	case TT_LE:
		rc = checkCompare(rulec, rulec->prevrule, op);
		break;
	case TT_Plus:
	case TT_Minus:
	case TT_Multiply:
	case TT_Divide:
	case TT_Between:
		rc = checkNumeric(rulec, rulec->prevrule, op);
		break;
	case TT_And:
	case TT_Or:
		rc = checkBoolean(rulec, rulec->prevrule, op);
		break;
	}

	rulec->opstack[rulec->op_pos++] = op;
	return rc;
}


/*
 * Pop all operators at a lower level
 */
int popOpStack(rulecomp_t * rulec, int kind) {
    int op;
    while (rulec->op_pos > rulec->oplvl[rulec->level]) {
        op = rulec->opstack[--rulec->op_pos];
        if (op_priority[op] < op_priority[kind]) {
            rulec->op_pos++;
            break;
        }
		switch (op) {
		case TT_EQ:
		case TT_NE:
			putrule(rulec, 4, SELRULE_Compare, op-TT_EQ, NULL);
			break;
		case TT_GT:
		case TT_LT:
		case TT_GE:
		case TT_LE:
			if (checkCompare(rulec, rulec->prevrule, op)) {
				return CS_Done;
			}
            putrule(rulec, 4, SELRULE_Compare, op-TT_EQ, NULL);
			break;
		case TT_Between:
            rulec->rc = ISMRC_BetweenNotValid;
            ism_common_setError(rulec->rc);
            return CS_Done;
        case TT_Plus:
            putrule(rulec, 4, SELRULE_Calc, '+', NULL);
            break;
        case TT_Minus:
            putrule(rulec, 4, SELRULE_Calc, '-', NULL);
            break;
        case TT_Multiply:
            putrule(rulec, 4, SELRULE_Calc, '*', NULL);
            break;
        case TT_Divide:
            putrule(rulec, 4, SELRULE_Calc, '/', NULL);
            break;
        case TT_Negative:
            putrule(rulec, 4, SELRULE_Negative, 0, NULL);
            break;
        case TT_Not:
            putrule(rulec, 4, SELRULE_Not, 0, NULL);
		    break;
        case TT_Exclam:
            putrule(rulec, 4, SELRULE_Is, TT_True|0x40, NULL);
            break;
        case TT_NotNot:
            putrule(rulec, 4, SELRULE_Is, TT_False|0x40, NULL);
            break;
        case TT_Between2:
			checkNumeric(rulec, rulec->prevrule, op);
            putrule(rulec, 4, SELRULE_Between, 2, NULL);
            if (rulec->op_pos > rulec->oplvl[rulec->level] &&
                rulec->opstack[rulec->op_pos-1] == TT_Not) {
                rulec->op_pos--;
                putrule(rulec, 4, SELRULE_Not, 0, NULL);
            }
            break;
        case TT_InHash:
            putrule(rulec, 4, SELRULE_InHash, 0, NULL);
            break;
		default:
			break;
        }
    }
    return (kind == TT_None)? CS_Done : CS_Operand;
}


/*
 * Put out an operand
 * Returns the new state
 */
static int  putOperand(rulecomp_t * rulec, int kind, char * token) {
    switch (kind) {
    case TT_Name:
        if (rulec->opt_internal && token) {
            int len = strlen(token);
            if (len == 3 && !strcmp(token, "QoS" )) {
                putrule(rulec, 4, SELRULE_QoS, 0, NULL);
                break;
            } else if (len >= 5 && !memcmp(token, "Topic", 5)) {
                if (len == 5) {
                    putrule(rulec, 4, SELRULE_Topic, 0, NULL);
                    break;
                }
                if (len == 6 && token[5]>='0' && token[5]<= '9') {
                    putrule(rulec, 4, SELRULE_TopicPart, token[5]-'0', NULL);
                    break;
                }
            } else {
                int found = 0;
                switch (len) {
                case 2:
                    if (!strcmp(token, "ID")) {
                        putrule(rulec, 4, SELRULE_TopicPart, 5, NULL);
                        found = 1;
                    }
                    break;
                case 3:
                    if (!strcmp(token, "Org")) {
                        putrule(rulec, 4, SELRULE_TopicPart, 1, NULL);
                        found = 1;
                    }
                    else if (!strcmp(token, "Fmt")) {
                        putrule(rulec, 4, SELRULE_TopicPart, 9, NULL);
                        found = 1;
                    }
                    break;
                case 4:
                    if (!strcmp(token, "Type")) {
                        putrule(rulec, 4, SELRULE_TopicPart, 3, NULL);
                        found = 1;
                    }
                    break;
                case 5:
                    if (!strcmp(token, "Event")) {
                        putrule(rulec, 4, SELRULE_TopicPart, 7, NULL);
                        found = 1;
                    }
                    break;
                }
                if (found)
                    break;
            }
        }
        putrule(rulec, (int)(5+strlen(token)), SELRULE_Var, 0, token);
        break;

    case TT_Int:
        putrule_int(rulec, token);
        break;

    case TT_Long:
        putrule_long(rulec, token);
        break;

    case TT_Float:
        putrule_float(rulec, token);
        break;

    case TT_Double:
        putrule_double(rulec, token);
        break;

    case TT_String:
        putrule(rulec, (int)(strlen(token)+5), SELRULE_String, 0, token);
        break;

    case TT_True:
        putrule(rulec, 4, SELRULE_Boolean, 1, NULL);
        break;

    case TT_False:
        putrule(rulec, 4, SELRULE_Boolean, 0, NULL);
        break;

    default:
        TRACE(1, "Invalid field or constant: %s", rulec->ruledef);
        rulec->rc = ISMRC_OperandNotValid;
        ism_common_setError(rulec->rc);
        return CS_Done;
    }
    return CS_Op;
}


/*
 * Compile the rule.
 * The compile strategy is to parse all valid operands and operators within the JMS
 * subset of SQL92.  Since we do not know the type of a variable (or identifier),
 * we assume it is valid.  For constants and operators we know the type and can reject
 * operands which do not match the required type.
 * <p>

 * The rule object must be freed using ism_common_freeRule() when it is no longer needed.
 * @param transport The transport object associated with this selection rule
 * @param rule      The location to return the rule
 * @param ruledef   The string containing the selection rule
 * @return A return code: 0=good
 */
int ism_common_compileSelectRule(ismRule_t * * xrule, int * outlen, const char * ruledef) {
    return ism_common_compileSelectRuleOpt(xrule, outlen, ruledef, 0);
}

/*
 * Internal compile with options
 */
int ism_common_compileSelectRuleOpt(ismRule_t * * xrule, int * outlen, const char * ruledef, int options) {
	int         rc;
	rulecomp_t  rulex;
	rulecomp_t * rulec = &rulex;
	char  *      token;
	int          tokenlen = 32750;
    ismRule_t * ret;

	*xrule = NULL;
	if (outlen)
	    *outlen = 0;
	memset(rulec, 0, sizeof(rulex));
	rulec->ruledef = (char *)ruledef;
	rulec->buflen = 64*1024;
	rulec->buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,116),rulec->buflen);
	if (options&SELOPT_Internal)
	    rulec->opt_internal = 1;
	rulec->options = (uint16_t)options;
	token = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,117),tokenlen);

	if (ruledef == NULL)
	    ruledef = "";
	rulec->ruledef = (char *)ruledef;
	rc = compile(rulec, token, tokenlen);
	if (rc == 0) {
	    ret = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,118),rulec->bufpos);
	    memcpy(ret, rulec->buf, rulec->bufpos);
	    *xrule = ret;
	    if (outlen)
	        *outlen = rulec->bufpos;
	}
	ism_common_free(ism_memory_utils_misc,rulec->buf);
	ism_common_free(ism_memory_utils_misc,token);
	return rc;
}


/*
 * Compile the rule
 */
static int compile(rulecomp_t * rulec, char * token, int tokenlen) {
	int         state;
	int         kind;
	int         tlen;
	char *      pos;
	ismRule_t * saverule;
	int         savepos;
	int         count;
	char        escape;
	int         notflag;

    pos = rulec->ruledef;
    state = CS_Operand;

    if (rulec->options > 1)
        putrule(rulec, 4, SELRULE_Internal, rulec->options, NULL);

    while (state != CS_Done) {
        /*
         * Find the next token.  The tokenizer will also skip white space but
         * we do it here to get the error offset.
         */
        while (isWhiteSpace(*pos))
            pos++;

        /* Return a saved token */
        if (rulec->savekind) {
        	kind = rulec->savekind;
        	rulec->savekind = 0;
        	ism_common_strlcpy(token, rulec->savetoken, tokenlen);
        } else {
            kind = getSelectToken(token, tokenlen, &pos);

            /* Fix for internal options */
            if (kind == TT_ACLCheck && !rulec->opt_internal)
                kind = TT_Name;
            if (!rulec->opt_internal && (kind == TT_Exclam || kind == TT_NotNot || kind == TT_InHash))
                kind = TT_BadToken;
            if (kind == TT_Amp)
                kind = rulec->opt_internal ? TT_And : TT_BadToken;
        }

        if (kind < 0) {
            state = CS_Done;
            rulec->rc = ISMRC_OperandNotValid;
            ism_common_setError(rulec->rc);
            return rulec->rc;
        }
        switch (state) {
        /*
         * We are expecting an operand, which includes unary operators and the left parenthesis.
         */
        case CS_Operand:
            /*
             * Process unary operators
             *
             * We can ignore unary plus as we define it to change neither the value
             * nor the type of the operand.  This differs from Java in which the
             * unary plus promotes byte and short values to int.  In Java you can fix
             * this specification bug using casting, but there is no similar facility
             * in SQL92.
             */
            if (kind == TT_Minus || kind == TT_Plus || kind == TT_Not || kind == TT_Exclam || kind == TT_NotNot) {
                if (kind == TT_Minus)
                    kind = TT_Negative;
                if (kind != TT_Plus)       /* Ignore unary plus */
                    if (putOpStack(rulec, kind))
                    	return rulec->rc;
                break;
            }

            if (kind == TT_None) {
                state = CS_Done;
                break;
            }

            if (kind == TT_Begin) {
                if (rulec->level >= 63) {
                    rulec->rc = ISMRC_TooComplex;
                    ism_common_setError(rulec->rc);
                    return rulec->rc;
                }
                rulec->level++;
                rulec->oplvl[rulec->level] = rulec->op_pos;
            } else {
                state = putOperand(rulec, kind, token);
            }
            break;

        /*
         * We are expecting an operator
         */
        case CS_Op:
            if (kind == TT_Not) {
                if (putOpStack(rulec, TT_Not))
                	return rulec->rc;
            }
            if ((kind>= TT_MinOp && kind <= TT_MaxOp) || kind==TT_None ) {

                /*
                 * For an and token, determine if this is the logical and
                 * operator, or the separator between the second and third
                 * arguments to the between operator.
                 */
                if (kind == TT_And && rulec->level > 0 && rulec->oplvl[rulec->level] >0 &&
                                rulec->opstack[rulec->oplvl[rulec->level]-1] == TT_Between) {
                    /*
                     * If this AND forms the final portion of the between
                     * operator, close the level we started with the BETWEEN.
                     */
                    rulec->opstack[rulec->oplvl[rulec->level]-1] = TT_Between2;
                    rulec->level--;
                    state = popOpStack(rulec, TT_Multiply);  /* close all numeric ops */
                    break;
                }

                /*
                 * Create rules for all higher priority items
                 */
                state = popOpStack(rulec, kind);
                if (state == CS_Done)
                    break;

                switch (kind) {

                /*
                 * The is op is followed by either null, or not null.
                 */
                case TT_Is:
                    kind = getSelectToken(token, tokenlen, &pos);
                    notflag = 0;
                    if (kind == TT_Not) {
                        notflag = 0x40;
                        kind = getSelectToken(token, tokenlen, &pos);
                    }
                    if (kind != TT_Null &&
                        (!rulec->opt_internal || (kind != TT_True && kind != TT_False))) {
                        rulec->rc = ISMRC_IsNotValid;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    putrule(rulec, 4, SELRULE_Is, kind+notflag, NULL);
                    state = CS_Op;
                    break;

                /*
                 * The in operator is followed by a group, each member of which
                 * is a constant string, and is separated by comma and surrounded
                 * by parenthesis.  The preceding argument must be a variable.
                 */
                case TT_In:
                    if (checkIdentifier(rulec, TT_In))
                    	return rulec->rc;
                    kind = getSelectToken(token, tokenlen, &pos);
                    if (kind != TT_Begin) {
                        rulec->rc = ISMRC_InRequiresGroup;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    saverule = (ismRule_t *)(rulec->buf+rulec->bufpos);
                    savepos = rulec->bufpos;
                    putrule(rulec, 4, SELRULE_In, 0, NULL);
                    kind = TT_Group;
                    count = 0;
                    while (kind == TT_Group) {
                        kind = getSelectToken(token, tokenlen, &pos);
                        if (kind != TT_String) {
                            rulec->rc = ISMRC_InGroupNotValid;
                            ism_common_setError(rulec->rc);
                            return rulec->rc;
                        }
                        count++;
                        tlen = (int)strlen(token);
                        if (count >= 250 || tlen>255) {
                        	rulec->rc = ISMRC_TooComplex;
                            ism_common_setError(rulec->rc);
                        	return rulec->rc;
                        }
                        if ((rulec->bufpos + tlen + 1) < rulec->buflen) {
                        	rulec->buf[rulec->bufpos++] = (char)tlen;
                        	memcpy(rulec->buf+rulec->bufpos, token, tlen);
                        	rulec->bufpos += tlen;
                        } else {
                        	rulec->rc = ISMRC_TooComplex;
                            ism_common_setError(rulec->rc);
                        	return rulec->rc;
                        }
                        kind = getSelectToken(token, tokenlen, &pos);
                    }
                    if (kind != TT_End || count==0) {
                        rulec->rc = ISMRC_InSeparator;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    if (((uintptr_t)rulec->bufpos)&3)
                    	rulec->bufpos += (4-(((uintptr_t)rulec->bufpos)&3));
                    saverule->len = rulec->bufpos - savepos;
                    saverule->kind = count;
                    state = CS_Op;
                    break;

                /*
                 * Like is followed by a pattern which must be a constant string.
                 * This is optionally followed by a escape clause where the data
                 * must be a constant string of length 1.
                 */
                case TT_Like:
                    if (checkIdentifier(rulec, TT_Like))
                    	return rulec->rc;
                    kind = getSelectToken(token, tokenlen, &pos);
                    if (kind != TT_String) {
                        rulec->rc = ISMRC_LikeSyntax;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    escape = 0;
                    kind = getSelectToken(rulec->savetoken, sizeof rulec->savetoken, &pos);
                    if (kind != TT_Escape) {
                        rulec->savekind = kind;
                    } else {
                        kind = getSelectToken(rulec->savetoken, sizeof rulec->savetoken, &pos);
                        if (kind != TT_String || strlen(rulec->savetoken) != 1 ||
                        		((uint8_t)rulec->savetoken[0]) > 0x7f) {
                            rulec->rc = ISMRC_EscapeNotValid;
                            ism_common_setError(rulec->rc);
                            return rulec->rc;
                        }
                        escape = *rulec->savetoken;
                    }
					saverule = (ismRule_t *)(rulec->buf + rulec->bufpos);
                    putrule(rulec, 4+ism_common_matchlen(token), SELRULE_Like, 1, NULL);
                    ism_common_convertmatch(token, (char *)(saverule+1), escape);
                    state = CS_Op;
                    break;

                case TT_And:
					if (checkBoolean(rulec, rulec->prevrule, kind)) {
						return rulec->rc;
					}
                	putrule(rulec, 4, SELRULE_And, rulec->level, NULL);
			  		state = CS_Operand;
                	break;

                case TT_Or:
					if (checkBoolean(rulec, rulec->prevrule, kind)) {
						return rulec->rc;
					}
                	putrule(rulec, 4, SELRULE_Or, rulec->level, NULL);
			  		state = CS_Operand;
                	break;

                case TT_End:
                    if (rulec->level == 0) {
                        rulec->rc = ISMRC_TooManyRightParen;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    putrule(rulec, 4, SELRULE_End, rulec->level, NULL);
                    rulec->level--;
                    state = popOpStack(rulec, TT_Multiply);
                    if (state == CS_Operand)
                        state = CS_Op;
                    break;

                /*
                 * For between, we add an implied level since the first clause
                 * can be a numeric expression, and we need to differentiate the
                 * and which starts the second clause from an and operator.
                 */
                case TT_Between:
                    if (putOpStack(rulec, kind))
                    	return rulec->rc;
                    if (rulec->level >= 63) {
                        rulec->rc = ISMRC_TooComplex;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    rulec->level++;
                    rulec->oplvl[rulec->level] = rulec->op_pos;
                    break;
                /*
                 * The aclcheck operator is followed by a group, each member of which
                 * is a constant integer from 0 to 31.
                 */
                case TT_ACLCheck:
                    kind = getSelectToken(token, tokenlen, &pos);
                    if (kind != TT_Begin) {
                        rulec->rc = ISMRC_InRequiresGroup;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    saverule = (ismRule_t *)(rulec->buf+rulec->bufpos);
                    savepos = rulec->bufpos;
                    putrule(rulec, 4, SELRULE_ACLCheck, 0, NULL);
                    kind = getSelectToken(token, tokenlen, &pos);
                    if (kind != TT_String) {
                        rulec->rc = ISMRC_InGroupNotValid;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    tlen = strlen(token);
                    if (tlen>254) {
                        rulec->rc = ISMRC_TooComplex;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    rulec->buf[rulec->bufpos++] = (char)(tlen+1);
                    memcpy(rulec->buf+rulec->bufpos, token, tlen);
                    rulec->bufpos += tlen;
                    rulec->buf[rulec->bufpos++] = 0;
                    count = 0;
                    kind = getSelectToken(token, tokenlen, &pos);
                    while (kind == TT_Group) {
                        int which;
                        kind = getSelectToken(token, tokenlen, &pos);
                        if (kind == TT_Int)
                            which = strtoul(token, NULL, 0);
                        if (kind != TT_Int || which<0 || which>31 ) {
                            rulec->rc = ISMRC_InGroupNotValid;
                            ism_common_setError(rulec->rc);
                            return rulec->rc;
                        }
                        count++;
                        tlen = (int)strlen(token);
                        if (count >= 32) {
                            rulec->rc = ISMRC_TooComplex;
                            ism_common_setError(rulec->rc);
                            return rulec->rc;
                        }
                        if ((rulec->bufpos + 1) < rulec->buflen) {
                            rulec->buf[rulec->bufpos++] = (char)which;
                        } else {
                            rulec->rc = ISMRC_TooComplex;
                            ism_common_setError(rulec->rc);
                            return rulec->rc;
                        }
                        kind = getSelectToken(token, tokenlen, &pos);
                    }
                    if (kind != TT_End || count==0) {
                        rulec->rc = ISMRC_InSeparator;
                        ism_common_setError(rulec->rc);
                        return rulec->rc;
                    }
                    if (((uintptr_t)rulec->bufpos)&3)
                        rulec->bufpos += (4-(((uintptr_t)rulec->bufpos)&3));
                    saverule->len = rulec->bufpos - savepos;
                    saverule->kind = count;
                    state = CS_Op;
                    break;

                /*
                 * For all other operators, just push the operator on the stack.
                 * This also checks for valid constant types before the op.
                 */
                default:
                    if (putOpStack(rulec, kind))
                    	return rulec->rc;
                }
            }
			break;
        default:
			state = CS_Done;
			break;
        }
    }

    /*
     * We have reached the end of the definition string, so validate
     * the end state.
     */
    if (rulec->level != 0) {
        rulec->rc = ISMRC_TooManyLeftParen;
        ism_common_setError(rulec->rc);
        return rulec->rc;
    }
    if (rulec->op_pos != 0 || rulec->prevrule == SELRULE_Or || rulec->prevrule == SELRULE_And) {
        rulec->rc = ISMRC_OperandMissing;
        ism_common_setError(rulec->rc);
        return rulec->rc;
    }
	switch (rulec->prevrule) {
	case SELRULE_Int:
	case SELRULE_Long:
	case SELRULE_Float:
	case SELRULE_Double:
	case SELRULE_Calc:
		rulec->rc = ISMRC_NotBoolean;
        ism_common_setError(rulec->rc);
		return rulec->rc;
		break;
	default:
		break;
    }

    /* Return the rule */
    if (!rulec->rc) {
    	putrule(rulec, 4, SELRULE_End, 0, NULL);
    }

    return rulec->rc;
}


/*
 * Check that the preceding operator is not boolean.
 * The JMS spec does not allow boolean or string fields to be converted to boolean.
 */
static int checkNumeric(rulecomp_t * rulec, int op, int kind) {
    switch (op) {
    case SELRULE_Int:
    case SELRULE_Long:
    case SELRULE_Float:
    case SELRULE_Double:
    case SELRULE_Var:
    case SELRULE_Calc:
    case SELRULE_Negative:
    case SELRULE_QoS:
    case SELRULE_SmallInt:
        break;
    default:
        rulec->rc = ISMRC_OpNotBoolean;
        ism_common_setErrorData(rulec->rc, "%s", opname(op, kind));
        return rulec->rc;
    }
    return 0;
}


/*
 * Check that the preceding operator is not numeric.
 * The JMS spec does not allow numeric values to be converted to boolean.
 */
static int checkBoolean(rulecomp_t * rulec, int op, int kind) {
    switch (op) {
    case SELRULE_Int:
    case SELRULE_SmallInt:
    case SELRULE_QoS:
    case SELRULE_Long:
    case SELRULE_Float:
    case SELRULE_Double:
    case SELRULE_Calc:
        rulec->rc = ISMRC_OpNotNumeric;
        ism_common_setErrorData(rulec->rc, "%s", opname(op, kind));
        return rulec->rc;
    }
    return 0;
}


/*
 * Check that the operand for a range compare is not a constant string, boolean, or boolean op
 */
static int checkCompare(rulecomp_t * rulec, int op, int kind) {
    switch (op) {
    case SELRULE_Var:
    case SELRULE_Int:
    case SELRULE_Long:
    case SELRULE_Float:
    case SELRULE_Double:
    case SELRULE_Calc:
    case SELRULE_Negative:
    case SELRULE_Topic:
    case SELRULE_TopicPart:
    case SELRULE_QoS:
    case SELRULE_SmallInt:
        break;
    default:
        rulec->rc = ISMRC_OpNoString;
        ism_common_setErrorData(rulec->rc, "%s", opname(op, kind));
        return rulec->rc;
    }
    return 0;
}


/*
 * The JMS spec requires that the argument preceding LIKE or IN be a variable
 */
static int checkIdentifier(rulecomp_t * rulec, int kind) {
    if (rulec->prevrule != SELRULE_Var &&
        rulec->prevrule != SELRULE_Topic &&
        rulec->prevrule != SELRULE_TopicPart) {
        rulec->rc = ISMRC_OpRequiresID;
        ism_common_setErrorData(rulec->rc, "%s", opname(0, kind));
        return rulec->rc;
    }
    return 0;
}


static const char * opname(int op, int kind) {
	return BaseRuleName[op];
}


/*
 * Free up a selection rule which is no longer needed
 * @param rule  The selection rule
 */
void ism_common_freeSelectRule(ismRule_t * rule) {
    if (rule) {
        ism_common_free(ism_memory_utils_misc,rule);
    }
}


/*
 * Convert a match string to a character string for display
 */
static void matchtostring(char * mrule, char * buf, int len) {
    int   pos = 0;
    len--;
    while (*mrule && pos<len) {
        if ((uint8_t)*mrule==0xff) {
            buf[pos++] = '%';
        } else if ((uint8_t)*mrule==0xfe) {
            buf[pos++] = '_';
        } else {
            if (((uint8_t)*mrule) + pos < len ) {
                memcpy(buf+pos, mrule+1, (uint8_t)*mrule);
                pos   += (uint8_t)*mrule;
                mrule += (uint8_t)*mrule;
            } else break;
        }
        mrule++;
    }
    buf[pos] = 0;
}

/*
 * Dump a rule to a string for debugging
 */
static int  dumprule(ismRule_t * rule, char * buf, int len) {
    char tbuf[4096], kbuf[256], *tp = NULL;
    uint64_t lval;
    int      i;
    double   dval;
    float    fval;
    char *   kp;
    uint16_t rlen;

    memcpy(&rlen, &rule->len, 2);
    switch (rule->op) {
    case SELRULE_Compare:
        strcpy(kbuf, CmpRuleName[rule->kind]);
        break;
    case SELRULE_Simple:
        strcpy(kbuf, SimpleRuleName[rule->kind]);
        break;
    case SELRULE_Calc:
        kbuf[0] = (char)rule->kind;
        kbuf[1] = 0;
        break;
    case SELRULE_Boolean:
        strcpy(kbuf, (int8_t)rule->kind < 0 ? "Unknown" : (rule->kind==0 ? "False" : "True"));
        break;
    case SELRULE_Int:
    case SELRULE_Long:
    case SELRULE_Float:
    case SELRULE_Double:
    case SELRULE_String:
    case SELRULE_Negative:
    case SELRULE_Not:
    case SELRULE_Var:
    case SELRULE_Topic:
    case SELRULE_QoS:
    case SELRULE_Is:
    case SELRULE_InHash:
        *kbuf = 0;
        break;
    default:
        ism_common_itoa(rule->kind, kbuf);
        break;
    };
    if (rule->op == SELRULE_In) {
    	sprintf(tbuf, "%3d %s (", rlen, BaseRuleName[rule->op]);
    	kp = ((char *)rule)+4;
    	for (i=0; i<rule->kind; i++) {
    		int slen = (int)(uint8_t)*kp++;
    		int tlen = (int)strlen(tbuf);
    		if (tlen + slen + 4 < sizeof(tbuf)) {
    			if (i != 0)
            		tbuf[tlen++] = ',';
    			tbuf[tlen++] = '\'';
    			memcpy(tbuf+tlen, kp, slen);
    			kp += slen;
    			tlen += slen;
    			tbuf[tlen++] = '\'';
    			tbuf[tlen] = 0;
    		}
			tp = tbuf+ strlen(tbuf);
    	}
    	ism_common_strlcat(tbuf, ")", sizeof(tbuf));
    } else {
        snprintf(tbuf, sizeof(tbuf), *kbuf ? "%3d %s %s " : "%3d %s ", rlen, BaseRuleName[rule->op], kbuf);
        tp = tbuf + strlen(tbuf);
    }
    switch (rule->op) {
    case SELRULE_String:
    case SELRULE_Var:      strcpy(tp, (char *)(rule+1));               break;
    case SELRULE_Boolean:  *--tp = 0;                                  break;
    case SELRULE_Int:      ism_common_itoa(*(int32_t *)(rule+1), tp);  break;
    case SELRULE_Long:
        memcpy(&lval, rule+1, sizeof lval);
        ism_common_ltoa(lval, tp);
        break;
    case SELRULE_Float:
        memcpy(&fval, rule+1, sizeof fval);
        ism_common_ftoa(fval, tp);
        break;
    case SELRULE_Double:
        memcpy(&dval, rule+1, sizeof dval);
        ism_common_dtoa(dval, tp);
        break;
    case SELRULE_Like:
        matchtostring((char *)(rule+1), tp, (int)(sizeof(tbuf)-(tp-tbuf)));
        break;

	case SELRULE_Is:
        strcpy(tp, (rule->kind & 0x40) ? "not" : "");
        switch(rule->kind&0x3F) {
        case TT_Null:   strcat(tp, " null");    break;
        case TT_True:   strcat(tp, " true");    break;
        case TT_False:  strcat(tp, " false");   break;
        default:        strcat(tp, " unknown"); break;
        }
        break;
	case SELRULE_ACLCheck:
	    kp = (char *)(rule+1);
	    sprintf(kbuf, " %s", kp+1);
	    strcat(tp, kbuf);
	    kp += (*kp+1);
	    for (i=0; i<rule->kind; i++) {
	        sprintf(kbuf, " %u", (uint8_t)*kp++);
	        strcat(tp, kbuf);
	    }
	    break;

    default:
    	*--tp = 0;
		break;
    }
    if (tp) {
        tp = tp + strlen(tp);
        ism_common_strlcpy(buf, tbuf, len);
        return (int)(tp-tbuf);
    }
    return 0;
}

/*
 * Print a select rule
 */
XAPI int ism_common_printSelectRule(ismRule_t * rule) {
    char tbuf[4096];
    dumprule(rule, tbuf, sizeof(tbuf));
    printf("%s\n", tbuf);
    return 0;
}

/*
 * Dump the select rule to the specified buffer.
 * @param rule  The select rule
 * @param buf   The buffer to return the output
 * @param len   The length of the buffer
 * @return The length of the data returned, or <0 to indicate an error
 */
int ism_common_dumpSelectRule(ismRule_t * rule, char * buf, int len) {
    int  outlen = 0;
    char tbuf[8192];

    *buf = 0;
    for (;;) {
        uint16_t rlen;
        memcpy(&rlen, &rule->len, 2);
        if ((rlen&3) || (rlen<4)) {
            outlen = (int)ism_common_strlcat(buf, "ERROR: Invalid rule\n", len);
            return -1;
        }
        dumprule(rule, tbuf, sizeof(tbuf));
        ism_common_strlcat(buf, tbuf, len);
        outlen = (int)ism_common_strlcat(buf, "\n", len);
        if (rule->op == SELRULE_End && !rule->kind)
            break;
        rule = (ismRule_t *)((char *)rule+rlen);
    }
    return outlen;
}

struct ts_stack_t {
    ismRule_t * stack [512];
    int   level;
};

/*
 * Put out a string
 */
void putString(const char * str, int slen, concat_alloc_t * buf) {
    char * tempstr;
    char * tp;
    int  len = 3;
    int  escape = 0;
    const char * cp = str;
    int   i;

    if (slen < 0)
        slen = (int)strlen(str);


    for (i=0; i<slen; i++) {
        len++;
        if (*cp == '\'') {
            len++;
            escape++;
        }
        cp++;
    }

    tempstr = alloca(len);
    tp = tempstr;
    cp = str;
    *tp++ = '\'';
    for (i=0; i<slen; i++) {
        if (*cp == '\'')
            *tp++ = '\'';
        *tp++ = *cp++;
    }
    *tp++ = '\'';
    ism_common_allocBufferCopyLen(buf, tempstr, tp-tempstr);
}


/*
 * Put out an operand
 */
void putStrOperand(struct ts_stack_t * ts, ismRule_t * rule, concat_alloc_t * buf) {
    char *   tp;
    char     tbuf[64];
    uint64_t lval;
    double   dval;
    float    fval;
    char *   kp;
    int      i;
    char cop [4] = " x ";
    concat_alloc_t op2;
    concat_alloc_t op3;
    char     op2buf[256];
    char     op3buf[128];

    tp = tbuf;
    switch (rule->op) {
    case SELRULE_String:
        putString((char *)(rule+1), -1, buf);
        tp = NULL;
        break;
    case SELRULE_Var:
        tp = (char *)(rule+1);
        break;
    case SELRULE_Boolean:
        tp = rule->kind ? "true" : "false";
        break;
    case SELRULE_Int:
        ism_common_itoa(*(int32_t *)(rule+1), tp);
        break;
    case SELRULE_Long:
        memcpy(&lval, rule+1, sizeof lval);
        ism_common_ltoa(lval, tp);
        break;
    case SELRULE_Float:
        memcpy(&fval, rule+1, sizeof fval);
        ism_common_ftoa(fval, tp);
        break;
    case SELRULE_Double:
        memcpy(&dval, rule+1, sizeof dval);
        ism_common_dtoa(dval, tp);
        break;
    case SELRULE_In:         /* IN operator                     kind=count      */
        putStrOperand(ts, ts->stack[ts->level--], buf);
        ism_common_allocBufferCopyLen(buf, " in (", 5);
        kp = (char *)(rule+1);
        for (i=0; i<rule->kind; i++) {
            int slen = (int)(uint8_t)*kp++;
            if (i != 0)
                ism_common_allocBufferCopyLen(buf, ",", 1);
            putString(kp, slen, buf);
            kp += slen;
        }
        tp = ")";
        break;

    case SELRULE_Like:       /* LIKE operator                   kind=count      */
        putStrOperand(ts, ts->stack[ts->level--], buf);
        ism_common_allocBufferCopyLen(buf, " like ", 6);
        matchtostring((char *)(rule+1), tbuf, sizeof(tbuf));
        putString(tbuf, -1, buf);
        tp = NULL;
        break;

    case SELRULE_Between:    /* BETWEEN operator                               */
        memset(&op3, 0, sizeof op3);
        op3.buf = op3buf;
        op3.len = sizeof op3buf;
        putStrOperand(ts, ts->stack[ts->level--], &op3);

        memset(&op2, 0, sizeof op2);
        op2.buf = op2buf;
        op2.len = sizeof op2buf;
        putStrOperand(ts, ts->stack[ts->level--], &op2);

        putStrOperand(ts, ts->stack[ts->level--], buf);

        ism_common_allocBufferCopyLen(buf, " between ", 9);
        ism_common_allocBufferCopyLen(buf, op2.buf, op2.used);
        if (op2.inheap)
            ism_common_freeAllocBuffer(&op2);
        ism_common_allocBufferCopyLen(buf, " and ", 5);
        ism_common_allocBufferCopyLen(buf, op3.buf, op3.used);
        if (op3.inheap)
            ism_common_freeAllocBuffer(&op3);
        tp = NULL;
        break;

    case SELRULE_Compare:    /* comparison operator             kind=CMP_e      */
        memset(&op2, 0, sizeof op2);
        op2.buf = op2buf;
        op2.len = sizeof op2buf;
        putStrOperand(ts, ts->stack[ts->level--], &op2);
        putStrOperand(ts, ts->stack[ts->level--], buf);
        ism_common_allocBufferCopy(buf, CmpRuleNameX[rule->kind]);
        buf->used--;
        ism_common_allocBufferCopyLen(buf, op2.buf, op2.used);
        if (op2.inheap)
            ism_common_freeAllocBuffer(&op2);
        tp = NULL;
        break;
    case SELRULE_Negative:   /* unary negative operator         kind unused    */
        tp = "-";
        break;
    case SELRULE_Not:        /* unary not operator              kind unused     */
        tp = " not ";
        break;

    case SELRULE_Calc:       /* numeric operator                kind=which      */
        memset(&op2, 0, sizeof op2);
        op2.buf = op2buf;
        op2.len = sizeof op2buf;
        putStrOperand(ts, ts->stack[ts->level--], &op2);
        putStrOperand(ts, ts->stack[ts->level--], buf);
        cop[1] = rule->op;
        ism_common_allocBufferCopyLen(buf, cop, 3);
        ism_common_allocBufferCopyLen(buf, op2.buf, op2.used);
        if (op2.inheap)
            ism_common_freeAllocBuffer(&op2);
        tp = NULL;
        break;
    case SELRULE_Is:
        strcpy(tp, (rule->kind & 0x40) ? " is not" : " is");
        switch(rule->kind&0x3F) {
        case TT_Null:   strcat(tp, " null");    break;
        case TT_True:   strcat(tp, " true");    break;
        case TT_False:  strcat(tp, " false");   break;
        default:        strcat(tp, " unknown"); break;
        }
        break;
    }
    if (tp) {
        ism_common_allocBufferCopy(buf, tp);
        buf->used--;
    }
}


/*
 * Return a human readable string for a selection rule
 */
int ism_common_toStringSelectRule(ismRule_t * rule, concat_alloc_t * buf) {
#if 0
    struct ts_stack_t ts;
    ts.level = 0;

    for (;;) {
        uint16_t rlen;
        memcpy(&rlen, &rule->len, 2);
        printf("rule=%d rlen=%d\n", rule->op, rlen);
        if ((rlen&3) || (rlen<4)) {
            return -1;
        }
        switch (rule->op) {
        default:
        case SELRULE_Boolean:
        case SELRULE_Int:
        case SELRULE_Long:
        case SELRULE_Float:
        case SELRULE_Double:
        case SELRULE_String:
        case SELRULE_Var:
            ts.stack[++ts.level] = rule;
            break;
        case SELRULE_End:        /* Left paren or end of string     kind=level      */
            while (ts.level > 0 && ts.stack[ts.level]) {
                putStrOperand(&ts, ts.stack[ts.level--], buf);
            }
            if (ts.level > 0 && ts.stack[ts.level]==NULL) {
                ts.level--;
            }
            if (rule->kind == 0) {
                ism_common_allocBufferCopy(buf, "");
                return 0;
            }
            ism_common_allocBufferCopyLen(buf, ")", 1);
            break;

        case SELRULE_Begin:      /* Right paren or start of string  kind=level      */
            ism_common_allocBufferCopyLen(buf, "(", 1);
            ts.stack[ts.level++] = NULL;
            break;
        case SELRULE_And:        /* AND operator                    kind=level      */
            while (ts.level > 0 && ts.stack[ts.level]) {
                putStrOperand(&ts, ts.stack[ts.level--], buf);
            }
            ism_common_allocBufferCopyLen(buf, " and ", 5);
            break;
        case SELRULE_Or:         /* OR operator                     kind=level      */
            while (ts.level > 0 && ts.stack[ts.level]) {
                putStrOperand(&ts, ts.stack[ts.level--], buf);
            }
            ism_common_allocBufferCopyLen(buf, " or ", 4);
            break;
        }
        rule = (ismRule_t *)((char *)rule+rlen);
    }
#endif
    return 0;
}
