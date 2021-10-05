/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms.impl;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;

/* TODO: fix AND & OR */

/*
 * Implement the IBM MessageSight JMS client message selector.
 * This consists of two major components, the select compiler, and the select interpreter.
 * <ul>
 * <li>The select compiler is run when the message selector is created.  It checks for syntactic
 * errors and creates a set of rules to implement the selection.
 * <li>The select interpreter takes a message and runs these rules against it.  
 * This gives a result of TRUE, FALSE, or UNKNOWN.
 * </ul>
 * <p>
 * This implementation is designed to be an exact implementation of the JMS message selection
 * specification without subsets or supersets.
 * <p>
 * The selector is stored as a set of rules which are interpreted sequentially.
 * Variables and constants are pushed onto a stack, and operators generally take
 * one or more operands off the stack, and put the result back on the stack. 
 * The result of the selector is the value on the stack at the end of running all
 * rules.  A selector with no rules give a true result.
 * <p>
 * The values on the stack are kept as Java objects.  Strings are Java String objects, 
 * Numeric values are Java Byte, Short, Integer, Long, Float, or Double objects.
 * Boolean values are Java Boolean objects with the null value equal to UNKNOWN.
 * <p>
 * Some rules reference a constant Java object which is kept as an index into an
 * array of objects.  A variable references the name as a String object.
 */
public class ImaMessageSelector {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /*
     * The selection result is returned as a Boolean object.  
     * A null represents the value unknown.
     */
    public static final Boolean SELECT_TRUE = Boolean.TRUE;
    public static final Boolean SELECT_FALSE = Boolean.FALSE;
    public static final Boolean SELECT_UNKNOWN = null; 
    
   /*
    * The debug flag can be set from a testcase running in the same package.
    */
    static boolean debug_flag = false;
    
    /*
     * Token types from the token parser.
     * The parser also returns the token as an object, but this is null for simple tokens.
     */
    static final int TT_NONE = 0; 
    static final int TT_None       = 0;    /* End of string                  */
    static final int TT_Name       = 1;    /* Unquoted name                  */
    static final int TT_String     = 2;    /* Quoted string                  */
    static final int TT_Int        = 3;    /* Integer constant               */
    static final int TT_Long       = 4;    /* Long constant                  */
    static final int TT_Float      = 5;    /* Float constant                 */
    static final int TT_Double     = 6;    /* Double constant                */
    static final int TT_Group      = 7;    /* Group separator (comma)        */
    static final int TT_Begin      = 8;    /* Begin (left parenthesis)       */
    static final int TT_MinOp      = 9;
    static final int TT_End        = 9;    /* End (right parenthesis)        */
    static final int TT_EQ         = 10;   /* Compare equals                 */
    static final int TT_NE         = 11;   /* Compare not equals             */
    static final int TT_GT         = 12;   /* Compare greater than           */
    static final int TT_LT         = 13;   /* Compare less than              */
    static final int TT_GE         = 14;   /* Compare greater than or equal  */
    static final int TT_LE         = 15;   /* Compare less than or equal     */
    static final int TT_Minus      = 16;   /* Substract operation            */
    static final int TT_Plus       = 17;   /* Addition operation             */
    static final int TT_Multiply   = 18;   /* Multiply operation             */
    static final int TT_Divide     = 19;   /* Divide operation               */
    static final int TT_Or         = 21;   /* Logical OR                     */
    static final int TT_And        = 22;   /* Logical AND                    */
    static final int TT_In         = 25;   /* IN operator                    */
    static final int TT_Is         = 26;   /* IS operator                    */
    static final int TT_Like       = 27;   /* LIKE operator                  */
    static final int TT_Between    = 28;   /* BETWEEN operator               */
    static final int TT_Between2   = 29;   /* AND following a between        */
    static final int TT_MaxOp      = 29;
    static final int TT_True       = 31;   /* Constant TRUE                  */
    static final int TT_False      = 32;   /* Constant FALSE                 */
    static final int TT_Null       = 33;   /* NULL name                      */
    static final int TT_Not        = 35;   /* Unary not                      */
    static final int TT_Escape     = 36;   /* ESCAPE after LIKE              */
    static final int TT_Negative   = 39;   /* Unary minus                    */
    static final int TT_TooLong    = -1;   /* Token too long                 */
    static final int TT_QuoteErr   = -2;   /* Quote error                    */
    static final int TT_BadToken   = -3;   /* Bad token                      */
    
    /*
     * Operation priorities
     */
    static final byte P_NONE  = 0;    /* Not an operator        */
    static final byte P_OR    = 1;    /* OR                     */
    static final byte P_AND   = 2;    /* AND                    */
    static final byte P_OP    = 4;    /* IN, IS, LIKE, BETWEEN  */
    static final byte P_NOT   = 3;    /* NOT                    */
    static final byte P_COMP  = 5;    /* Comparison             */
    static final byte P_PLUS  = 6;    /* Binary add subtract    */
    static final byte P_MUL   = 7;    /* Binary multiply divide */
    static final byte P_UNARY = 8;    /* Negation               */
    
    /*
     * Map from token type to priority
     */
    static final byte op_priority[] = {
        P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NONE,
        P_COMP, P_COMP, P_COMP, P_COMP, P_COMP, P_COMP, P_PLUS, P_PLUS, P_MUL,  P_MUL,
        P_NONE, P_OR,   P_AND,  P_NONE, P_NONE, P_OP,   P_OP,   P_OP,   P_OP,   P_OP,
        P_NONE, P_NONE, P_NONE, P_NONE, P_NONE, P_NOT,  P_NONE, P_NONE, P_NONE, P_UNARY,  
    };
    
    /*
     * Token names
     */
    static final String TokenNames[] = {
        "none", "name", "string", "int",  "long",  "float", "double", ",",     "(",        ")",
        "=",    "<>",   ">",      "<",    ">=",    "<=",    "-",      "+",     "*",        "/",
        "20",   "or",   "and",    "23",   "24",    "in",    "is",     "like",  "between1", "between",
        "30",   "true", "false",  "null", "34",    "not",   "escape", "37",    "38",       "negate", 
    };
    
    /*
     * Invalid token names
     */
    static final String BadTokenNames[] = {
        "00", "toolong", "quoteerr", "badtoken",
    };
    
    /*
     * Compile states
     */
    enum CompileState {
        Operand, Op, Done,
    }

    /*
     * Operation codes
     */
    static final int RULE_String  = 0;    /* Load a string constant          val=index  */
    static final int RULE_Boolean = 1;    /* Load a boolean constant         val=index  */
    static final int RULE_Const   = 2;    /* Load a numeric constant         val=index  */
    static final int RULE_Var     = 3;    /* Load a variable                 val=index  */
    static final int RULE_And     = 4;    /* AND operator                    val=unused */
    static final int RULE_Or      = 5;    /* OR operator                     val=unused */
    static final int RULE_Calc    = 6;    /* Arithmetic operators            val=which  */
    static final int RULE_Not     = 7;    /* Unary not operator              val=unused */
    static final int RULE_Neg     = 8;    /* Unary minus                     val=unused */
    static final int RULE_In      = 9;    /* In operator                     val=index  */
    static final int RULE_Is      = 10;   /* Is operator                     val=which  */
    static final int RULE_Like    = 11;   /* Like operator                   val=index  */
    static final int RULE_Between = 12;   /* Between operator                val=index  */
    static final int RULE_Compare = 13;   /* Compare                         val=which  */
    
    /*
     * Operation code names for debugging
     */
    static final String [] RuleNames = {
        "string", "boolean", "const", "var", "and", "or", "calc", "not", "neg", "in", "is", "like", "between", "compare",
    };
    
    /*
     * Compare names for debugging
     */
    static final String [] CompareNames = {
        "EQ", "NE", "GT", "LT", "GE", "LE",
    };
    
    /*
     * Calculation names for debugging
     */
    static final String [] CalcNames = {
        "subtract", "add", "multiply", "divide", 
    };
   
    /*
     * Type is promotion order
     */
    static final int PT_Null      = 0;
    static final int PT_String    = 1;
    static final int PT_Boolean   = 2;   
    static final int PT_Byte      = 3;    /* Byte - start of numeric types   */
    static final int PT_Short     = 4;
    static final int PT_Int       = 5;
    static final int PT_Long      = 6;
    static final int PT_Float     = 7;
    static final int PT_Double    = 8;
    
    /*
     * Promotion type names for debugging
     */
    static final String [] TypeNames = {
        "null", "string", "boolean", "byte", "short", "int", "long", "float", "double",
    };
    
    /*
     * Class variables 
     */
    String ruledef;                      /* String form of rule definition */
    ImaSession session = null;           /* Optional associated session    */
    int    rule[] = new int[1000];
    Object objects[] = new Object[1000]; /* The referenced objects         */
    int    rulepos  = 0;
    int    objpos   = 0;
    
    /* 
     * Compile time only class variables 
     */
    int    level;                        /* Current level (during compile) */
    int    oplvl[] = new int[64];        /* Operator stack during compile  */
    int    op_pos = 0;                   /* The current position in the op stack */
    byte   opstack[] = new byte[64];     /* The operator stack             */
    char   tokenbuf[] = new char[32750]; /* The token buffer               */
    Object token;                        /* Object from parse              */
    int    saveToken;                    /* Put back a token               */
    int    pos;                          /* Current position during compile */
    
    /*
     * The constructor with no arguments is not allowed.
     */
    @SuppressWarnings ("unused")
    private ImaMessageSelector() {}
    
    /*
     * Create a message selector.
     * @param ruledef  The selector string in SQL92 syntax
     * @throws InvalidSelectorException If the syntax of the selector is not correct.
     */
    public ImaMessageSelector(String ruledef) throws JMSException {
        compile(ruledef);
    }
    

    /*
     * Create a message selector associated with a session.
     * The message selector does not really need a session, but this gives
     * us a location to put properties such as allowing extensions to the
     * JMS spec. 
     * @param ruledef  The selector string in SQL92 syntax
     * @throws InvalidSelectorException If the syntax of the selector is not correct.
     */
    public ImaMessageSelector(ImaSession session, String ruledef) throws JMSException {
        this.session = session;
        compile(ruledef);
    }
    
    
    /*
     * Run the selection rule.
     * @param  The message 
     * @return The result as a Boolean true or false, or null to indicate unknown.
     */
    public Boolean select(ImaMessage msg) throws JMSException {
        if (rule.length == 0)
            return true;
        
        int rpos = 0;
        Object result;
        Object [] stack = new Object[320];
        int level = 0;
        
        /*
         * If there are no sub-rules, the result is true.
         */
        stack[0] = SELECT_TRUE;
        /*
         * Run all sub-rules 
         */
        for (rpos = 0; rpos<rule.length; rpos++) {
            int op = rule[rpos]>>16;
            int value = rule[rpos]&0xffff;
            switch (op) {

             /*
              * Load a constant.  We have three rules to allow for JMS restriction
              * testing at compile time.
              */
            case RULE_Const:
            case RULE_String:
            case RULE_Boolean:
                setResult(stack, level++, objects[value]);
                break;
                
            /* Load a field */    
            case RULE_Var:
                setResult(stack, level++, getField(msg, objects[value]));
                break;
                
            /* Implement the IS operator */    
            case RULE_Is:
                assert(level >= 1);
                result = selectIs(stack[level-1], value);
                setResult(stack, level-1, result);
                break;
                
            /* Implement the IN operator */    
            case RULE_In:
                assert(level >= 1);
                result = selectIn(stack[level-1], objects, value+1, ((Integer)objects[value]).intValue());
                setResult(stack, level-1, result);
                break;
                
            /* Implement the LIKE operator */    
            case RULE_Like:
                assert(level >= 1);
                result = selectLike(stack[level-1], objects[value]);
                setResult(stack, level-1, result);
                break;
                
            /* Implement all binary compare operators (= <> > < >= <=)  */    
            case RULE_Compare:
                assert(level >= 2);
                result = selectCompare(stack[level-2], value, stack[level-1]);
                level--;
                setResult(stack, level-1, result);
                break;
                
            /* Implement binary arithmetic operators (add, subtract, multiply, divide) */    
            case RULE_Calc:
                assert(level >= 2);
                result = selectCalc(stack[level-2], value, stack[level-1]);
                level--;
                setResult(stack, level-1, result);
                break;
                
            /* Implement the boolean AND operator */    
            case RULE_And:
                assert(level >= 2);
                result = selectAnd(stack[level-2], stack[level-1]);
                level--;
                setResult(stack, level-1, result);
                break;
                
            /* Implement the boolean OR operator */    
            case RULE_Or:
                assert(level >= 2);
                result = selectOr(stack[level-2], stack[level-1]);
                level--;
                setResult(stack, level-1, result);
                break;
                
            /* Implement the unary minus operator */    
            case RULE_Neg:
                assert(level >= 1);
                result = selectNeg(stack[level-1]);
                setResult(stack, level-1, result);
                break;
                
            /* Implement the boolean NOT operator */    
            case RULE_Not:
                assert(level >= 1);
                result = selectNot(stack[level-1]);
                setResult(stack, level-1, result);
                break;
             
            /* Implement the between op */
            case RULE_Between:
                assert(level >= 3);
                result = selectBetween(stack[level-3], stack[level-2], stack[level-1]);
                level -= 2;
                setResult(stack, level-1, result);
                break;
            }
        }
        return getResult(stack[0]);
    }

    
    /*
     * Set a result into the stack.
     */
    void setResult(Object [] stack, int level, Object value) {
        stack[level] = value;    
    }
    
    /*
     * Get the result from an object.
     * In most cases we can make sure that the expression yields a boolean
     * result with compile time checks, but we do not know the type of
     * variables at compile time.  
     */
    Boolean getResult(Object obj) {
        if (obj == null || !(obj instanceof Boolean))
            return null;
        return (Boolean)obj;
    }
    
    
    /*
     * Get a field from a message.
     */
    Object getField(ImaMessage msg, Object oname) throws JMSException {
        if (oname == null || !(oname instanceof String))
            return null;
        String name = (String)oname;
        if (name.startsWith("JMS")) {
            if (name.equals("JMSType")) {
                return msg.getJMSType();
            } else if (name.equals("JMSCorrelationID")) {
                return msg.getJMSCorrelationID();
            } else if (name.equals("JMSDeliveryMode")) {
                return msg.getJMSDeliveryMode() == DeliveryMode.PERSISTENT ? 
                                "PERSISTENT" : "NON_PERSISTENT";
            } else if (name.equals("JMSMessageID")) {
                return msg.getJMSMessageID();
            } else if (name.equals("JMSPriority")) {
                return msg.getJMSPriority();
            } else if (name.equals("JMSTimestamp")) {
                return msg.getJMSTimestamp();
            } else if (name.equals("JMSDestination")) {
                Destination ds = msg.getJMSDestination();
                return (ds == null) ? ds : ds.toString();
            } else if (name.equals("JMSExpiration")) {
                return msg.getJMSExpiration();
            } else if (name.equals("JMSReplyTo")) {
            	Destination ro = msg.getJMSReplyTo();
            	return (ro == null) ? ro : ro.toString();
            } else if (name.equals("JMSRedelivered")) {
                return msg.getJMSRedelivered();
            } else if (name.equals("JMS_IBM_Retain")) {
                return msg.retain;
            } else {
                /* names starting with JMS_ and JMSX are looked up as properties */
                if (!name.startsWith("JMS_") && ! name.startsWith("JMSX"))
                    return null;
            }
        }
        return msg.getObjectProperty(name);
    }
    
    
    /*
     * Implement the like rule
     */
    Boolean selectLike(Object str, Object like) {
        /* Return UNKNOWN for any input which is not a string */
        if (str == null || !(str instanceof String) || !(like instanceof ImaLike))
            return SELECT_UNKNOWN;
        return ((ImaLike)like).match((String)str) ? SELECT_TRUE : SELECT_FALSE;
    }
    
    
    /*
     * Implement the in rule
     */
    Boolean selectIn(Object str, Object [] objlist, int start, int count) {
        int   i;
        if (str == null || !(str instanceof String))
            return SELECT_UNKNOWN;
        for (i=0; i<count; i++) {
            if (str.equals(objlist[start++]))
                return SELECT_TRUE;
        }
        return SELECT_FALSE;
    }
    
    
    /*
     * Implement the is rule
     */
    Boolean selectIs(Object obj, int kind) {
        if (kind == TT_Not) {
            return obj==null ? SELECT_FALSE : SELECT_TRUE;
        } else {
            return obj==null ? SELECT_TRUE : SELECT_FALSE;
        }
    }
    

    /*
     * Get an integer type value for the object type.
     * The integer values are arranged in Java promotion order.
     */
    int getType(Object obj) {
        if (obj == null)
            return PT_Null;
        if (obj instanceof String)
            return PT_String;
        if (obj instanceof Integer)
            return PT_Int;
        if (obj instanceof Boolean)
            return PT_Boolean;
        if (obj instanceof Long)
            return PT_Long;
        if (obj instanceof Double)
            return PT_Double;
        if (obj instanceof Byte)
            return PT_Byte;
        if (obj instanceof Float)
            return PT_Float;
        if (obj instanceof Short)
            return PT_Short;
        return PT_Null;
    }

    
    /*
     * Promote a variable
     */
    Object promote(Number num, int type) {
        switch (type) {
        case PT_Short:  return Short.valueOf(num.shortValue());
        case PT_Int:    return Integer.valueOf(num.intValue());
        case PT_Long:   return Long.valueOf(num.longValue());
        case PT_Float:  return Float.valueOf(num.floatValue());
        case PT_Double: return Double.valueOf(num.doubleValue());
        }
        return null;
    }

    
    /*
     * Compare two objects with numeric promotion.
     * String and boolean objects are not promoted and can only be
     * compared against objects of equal type.  Numeric comparisons
     * of unequal type are done using the promotion series:
     *     byte, short, int, long, float, double.
     */
    Boolean selectCompare(Object obj1, int op, Object obj2) {
        int type1 = getType(obj1);
        int type2 = getType(obj2);
        if (type1 != type2) {
            if (type1<PT_Byte || type2<PT_Byte) {
                if (type1 == PT_Null || type2 == PT_Null)
                    return SELECT_UNKNOWN;
                return SELECT_FALSE;    
            } else {
                if (type2 > type1) 
                    type1 = type2;
            }
        }
        switch (type1) {
        case PT_Null:    /* Both objects are unknown, return unknown */
            return SELECT_UNKNOWN;

        case PT_String:
            switch (op) {
            case TT_EQ: return Boolean.valueOf(((String)obj1).equals((String)obj2));
            case TT_NE: return Boolean.valueOf(!(((String)obj1).equals((String)obj2)));
            }    
            break;

        case PT_Boolean:
            switch (op) {
            case TT_EQ: return Boolean.valueOf(obj1 == obj2);
            case TT_NE: return Boolean.valueOf(obj1 != obj2);
            }  
            break;

        case PT_Byte:
        case PT_Short:
        case PT_Int:     /* Implement byte, short, and int together */
            switch (op) {
            case TT_EQ: return Boolean.valueOf(((Number)obj1).intValue() == ((Number)obj2).intValue());
            case TT_NE: return Boolean.valueOf(((Number)obj1).intValue() != ((Number)obj2).intValue());
            case TT_GT: return Boolean.valueOf(((Number)obj1).intValue() >  ((Number)obj2).intValue());
            case TT_LT: return Boolean.valueOf(((Number)obj1).intValue() <  ((Number)obj2).intValue());
            case TT_GE: return Boolean.valueOf(((Number)obj1).intValue() >= ((Number)obj2).intValue());
            case TT_LE: return Boolean.valueOf(((Number)obj1).intValue() <= ((Number)obj2).intValue());
            }
            break;

        case PT_Long:
            switch (op) {
            case TT_EQ: return Boolean.valueOf(((Number)obj1).longValue() == ((Number)obj2).longValue());
            case TT_NE: return Boolean.valueOf(((Number)obj1).longValue() != ((Number)obj2).longValue());
            case TT_GT: return Boolean.valueOf(((Number)obj1).longValue() >  ((Number)obj2).longValue());
            case TT_LT: return Boolean.valueOf(((Number)obj1).longValue() <  ((Number)obj2).longValue());
            case TT_GE: return Boolean.valueOf(((Number)obj1).longValue() >= ((Number)obj2).longValue());
            case TT_LE: return Boolean.valueOf(((Number)obj1).longValue() <= ((Number)obj2).longValue());
            }
            break;

        case PT_Float:
            switch (op) {
            case TT_EQ: return Boolean.valueOf(((Number)obj1).floatValue() == ((Number)obj2).floatValue());
            case TT_NE: return Boolean.valueOf(((Number)obj1).floatValue() != ((Number)obj2).floatValue());
            case TT_GT: return Boolean.valueOf(((Number)obj1).floatValue() >  ((Number)obj2).floatValue());
            case TT_LT: return Boolean.valueOf(((Number)obj1).floatValue() <  ((Number)obj2).floatValue());
            case TT_GE: return Boolean.valueOf(((Number)obj1).floatValue() >= ((Number)obj2).floatValue());
            case TT_LE: return Boolean.valueOf(((Number)obj1).floatValue() <= ((Number)obj2).floatValue());
            }
            break;

        case PT_Double:
            switch (op) {
            case TT_EQ: return Boolean.valueOf(((Number)obj1).doubleValue() == ((Number)obj2).doubleValue());
            case TT_NE: return Boolean.valueOf(((Number)obj1).doubleValue() != ((Number)obj2).doubleValue());
            case TT_GT: return Boolean.valueOf(((Number)obj1).doubleValue() >  ((Number)obj2).doubleValue());
            case TT_LT: return Boolean.valueOf(((Number)obj1).doubleValue() <  ((Number)obj2).doubleValue());
            case TT_GE: return Boolean.valueOf(((Number)obj1).doubleValue() >= ((Number)obj2).doubleValue());
            case TT_LE: return Boolean.valueOf(((Number)obj1).doubleValue() <= ((Number)obj2).doubleValue());
            }
            break;
        }
        return SELECT_UNKNOWN;
    }

    
    /*
     * Perform a calculation according to the Java promotion rules.
     * For calculations, Byte and Short are converted to integer.
     * The calculation results in the higher type in the series:
     *        int, long, float, double.
     * The promotion is done by Java by using the Number interface.   
     * <p>
     * An integer divide by zero results in UNKNOWN    
     */
    Object selectCalc(Object obj1, int op, Object obj2) {
        int type1 = getType(obj1);
        int type2 = getType(obj2);
        if (type1<PT_Byte || type2<PT_Byte)
            return null;
        
        /*
         * Determine result type based on Java promotion
         */
        int ptype = type1;
        if (type1 < PT_Int || type2 < PT_Int)
            ptype = PT_Int;
        if (ptype < type2)
            ptype = type2;
        
        /*
         * Implement the calculations
         */
        switch (ptype) {
        case PT_Int:
            switch (op) {
            case TT_Plus:      return Integer.valueOf(((Number)obj1).intValue() + ((Number)obj2).intValue());
            case TT_Minus:     return Integer.valueOf(((Number)obj1).intValue() - ((Number)obj2).intValue());
            case TT_Multiply:  return Integer.valueOf(((Number)obj1).intValue() * ((Number)obj2).intValue()); 
            case TT_Divide:  
                int div = ((Number)obj2).intValue();
                if (div == 0)
                    return null;        /* Divide by zero results in UNKNOWN */
                return Integer.valueOf(((Number)obj1).intValue() / div);
            default:
                return null;
            }
        case PT_Long:
            switch (op) {
            case TT_Plus:      return Long.valueOf(((Number)obj1).longValue() + ((Number)obj2).longValue());
            case TT_Minus:     return Long.valueOf(((Number)obj1).longValue() - ((Number)obj2).longValue());
            case TT_Multiply:  return Long.valueOf(((Number)obj1).longValue() * ((Number)obj2).longValue()); 
            case TT_Divide:  
                long div = ((Number)obj2).longValue();
                if (div == 0)
                    return null;      /* Divide by zero results in UNKNOWN */
                return Long.valueOf(((Number)obj1).longValue() / div);
            default:
                return null;    
            }
        case PT_Float:
            switch (op) {
            case TT_Plus:      return Float.valueOf(((Number)obj1).floatValue() + ((Number)obj2).floatValue());
            case TT_Minus:     return Float.valueOf(((Number)obj1).floatValue() - ((Number)obj2).floatValue());
            case TT_Multiply:  return Float.valueOf(((Number)obj1).floatValue() * ((Number)obj2).floatValue()); 
            case TT_Divide:    return Float.valueOf(((Number)obj1).floatValue() / ((Number)obj2).floatValue());
            default:
                return null;
            }
        case PT_Double:
            switch (op) {
            case TT_Plus:      return Double.valueOf(((Number)obj1).doubleValue() + ((Number)obj2).doubleValue());
            case TT_Minus:     return Double.valueOf(((Number)obj1).doubleValue() - ((Number)obj2).doubleValue());
            case TT_Multiply:  return Double.valueOf(((Number)obj1).doubleValue() * ((Number)obj2).doubleValue()); 
            case TT_Divide:    return Double.valueOf(((Number)obj1).doubleValue() / ((Number)obj2).doubleValue());
            default:
                return null;
            }
        }
        return null;
    }
    
    
    /*
     * Implement boolean AND.
     * Note that both subexpressions are fully evaluated, unlike the
     * C or Java && operator.
     */
    Object selectAnd(Object obj1, Object obj2) {
        if (!(obj1 instanceof Boolean)) {
            if (obj2 instanceof Boolean && !((Boolean)obj2).booleanValue())
                return SELECT_FALSE;
            return null;
        }
        if (!(obj2 instanceof Boolean)) {
            if (!((Boolean)obj1).booleanValue()) 
                return SELECT_FALSE;
            return null;
        }
        return Boolean.valueOf(((Boolean)obj1).booleanValue() & ((Boolean)obj2).booleanValue());
    }
    
    
    /*
     * Implement boolean OR.
     * Note that both subexpressions are fully evaluated, and this is thus
     * a | and not a || type of OR.  That means there is no way to check
     * if a property is null and to also check a range.
     */
    Object selectOr(Object obj1, Object obj2) {
        if (!(obj1 instanceof Boolean)) {
            if (obj2 instanceof Boolean && ((Boolean)obj2).booleanValue())
                return SELECT_TRUE;
            return null;
        }
        if (!(obj2 instanceof Boolean)) {
            if (((Boolean)obj1).booleanValue()) 
                return SELECT_TRUE;
            return null;
        }
        return Boolean.valueOf(((Boolean)obj1).booleanValue() | ((Boolean)obj2).booleanValue());
    }
    
    
    /*
     * Implement boolean NOT
     */
    Object selectNot(Object obj) {
        if (!(obj instanceof Boolean))
            return null;  
        return Boolean.valueOf(!((Boolean)obj).booleanValue());
    }
    
    
    /*
     * Implement numeric unary minus.
     * This does not strictly implement the Java calculation promotion rule
     * in that the negation of byte and short values retain their type.  This
     * is done since SQL92 does not have any form of casting.
     */
    Object selectNeg(Object obj) {
        int ptype = getType(obj);

        switch (ptype) {
        case PT_Byte:   return Byte.valueOf((byte)-((Byte)obj).byteValue());
        case PT_Short:  return Short.valueOf((short)-((Short)obj).shortValue());
        case PT_Int:    return Integer.valueOf(-((Integer)obj).intValue());
        case PT_Long:   return Long.valueOf(-((Long)obj).longValue());
        case PT_Float:  return Float.valueOf(-((Float)obj).floatValue());
        case PT_Double: return Double.valueOf(-((Double)obj).doubleValue());
        }
        return null;
    }
    
    /*
     * Compare two objects with numeric promotion.
     * String and boolean objects are not promoted and can only be
     * compared against objects of equal type.  Numeric comparisons
     * of unequal type are done using the promotion series:
     *     byte, short, int, long, float, double.
     */
    Boolean selectBetween(Object obj1, Object obj2, Object obj3) {
        Boolean ret = selectCompare(obj1, TT_GE, obj2);
        if (ret == SELECT_TRUE) {
            ret = selectCompare(obj1, TT_LE, obj3);
        }
        return ret;
    }  
 
 
 
    /******************************************************************
     *
     * Selection compiler methods
     *
     ******************************************************************/
    
    /*
     * Get a selector token.
     * This is the tokenizer for the select compiler.
     * Before calling this function, the ruledef and pos class variables must be set.
     * 
     * @return the token type.  The actual token is returned in the token class variable.
     */
    int getSelectToken() {
        String tokenstr;
        if (saveToken != 0) {
            int ret = saveToken;
            saveToken = 0;
            return ret;
        }
        while (pos < ruledef.length() && Character.isWhitespace(ruledef.charAt(pos)))
            pos++;
        token = null;
        if (pos >= ruledef.length())
            return TT_None;
        
        /*
         * Look for a quoted string 
         */
        int tokenpos = 0;
        if (ruledef.charAt(pos)=='\'' ) {
            pos++;
            while (pos < ruledef.length()) {
                char ch = ruledef.charAt(pos);
                if (ch == '\'') {
                    if ((pos+1) >= ruledef.length() || ruledef.charAt(pos+1)!='\'') {
                        token = new String(tokenbuf, 0, tokenpos);
                        pos++;
                        return TT_String;
                    }
                    pos++;
                }
                if (tokenpos > tokenbuf.length)
                    return TT_TooLong;
                tokenbuf[tokenpos++] = ch;
                pos++;
            }
            return TT_QuoteErr;
        }
        
        /*
         * Look for an operator
         */
        int  ret = TT_None;
        switch (ruledef.charAt(pos)) {
        case '+':  ret = TT_Plus;     break;
        case '-':  ret = TT_Minus;    break;
        case '<':
            if (pos+1<ruledef.length() && ruledef.charAt(pos+1)=='=') {
                ret = TT_LE; 
                pos++;
                break; 
            }
            if (pos+1<ruledef.length() && ruledef.charAt(pos+1)=='>') { 
                ret = TT_NE; 
                pos++;
                break; 
            }
            ret = TT_LT;  
            break;
        case '>':
            if (pos+1<ruledef.length() && ruledef.charAt(pos+1)=='=') {
                ret = TT_GE; 
                pos++;  
                break; 
            }
            ret = TT_GT;  
            break;
        case '=':  ret = TT_EQ;           break;
        case '(':  ret = TT_Begin;        break;
        case ')':  ret = TT_End;          break;
        case ',':  ret = TT_Group;        break;
        case '*':  ret = TT_Multiply;     break;
        case '/':  ret = TT_Divide;       break;

        }
        if (ret != TT_None) {
            pos++;
            return ret;
        }
        
        /*
         * Accumulate a name.  We allow a dot in the name as this might be numeric,
         * but this is an invalid identifier so we will check for it later.
         */
        boolean containsdot = false;
        int     firstchar = 0;
        
        while (pos < ruledef.length()) {
            int codept = (int)ruledef.charAt(pos);
            if (codept >= 0xd800 && codept <= 0xdbff && (pos+1 < ruledef.length())) {
                codept = ((codept&0x3ff)<<10) + (ruledef.charAt(++pos)&0x3ff) + 0x10000; 
            }
            if (firstchar == 0)
            	firstchar = codept;

            if (codept == '.')
                containsdot = true;
            else if (!Character.isJavaIdentifierPart(codept))
                break;
            if (tokenpos >= tokenbuf.length)
                return TT_TooLong;
            if (codept >= 0x10000) {
                tokenbuf[tokenpos++] = ruledef.charAt(pos-1);
                tokenbuf[tokenpos++] = ruledef.charAt(pos);
            } else {
            	tokenbuf[tokenpos++] = (char)codept;
            }	
            pos++;
        }
        tokenbuf[tokenpos] = 0;
        tokenstr = new String(tokenbuf, 0, tokenpos);
        
        /*
         * Check for a number.
         * We will only see positive numbers here as the minus or plus signs
         * will be considered unary operators.  We will fix that up later.
         * 
         * The JMS spec is a little ambiguous here.  It says that the Java
         * numeric syntax is used but does not show any examples of numbers
         * in hex format, or with the 'L' or 'F' suffixes.  The understanding
         * of number here is that it matches Java which allows these.  Note
         * that there is no way to specify a constant byte or short.
         */
        if ((tokenbuf[0]>='0' && tokenbuf[0]<='9') || tokenbuf[0]=='.') {
            long lval = 0;
            boolean islong = false;
            /* Allow a trailing 'L' or 'l' */
            if (tokenbuf[tokenpos-1]=='L' || tokenbuf[tokenpos-1]=='l') {
                islong = true;
                tokenstr = new String(tokenbuf, 0, tokenpos-1);
            }
            try {
                /* Allow a hex or octal value */
                if (tokenpos>2 && tokenbuf[0]=='0') {
                    if (tokenbuf[1]=='x' || tokenbuf[1]=='X') 
                        lval = Long.parseLong(tokenstr.substring(2), 16);
                    else
                        lval = Long.parseLong(tokenstr, 8);  /* octal */ 
                } else {
                    lval = Long.parseLong(tokenstr);
                }    
                /* Check if this is a long or an integer */
                if (islong || (lval < Integer.MIN_VALUE || lval > Integer.MAX_VALUE)) {
                    token = (Object)Long.valueOf(lval);
                    return TT_Long;
                } else {
                    token = (Object)Integer.valueOf((int)lval);
                    return TT_Int;
                }
            } catch (NumberFormatException e) {
                /* If not a long, try making it a double */
                boolean isfloat = false;
                /*
                 * Handle the special case where a float value contains a plus or minus sign
                 * after the exponent.  This character becomes part of the value and not 
                 * an operator.
                 */
                if (tokenbuf[tokenpos-1]=='E' || tokenbuf[tokenpos-1]=='e') {
                    if (pos < ruledef.length() && (ruledef.charAt(pos)=='+' || ruledef.charAt(pos)=='-')) {
                        if (tokenpos >= tokenbuf.length)
                            return TT_TooLong;
                        tokenbuf[tokenpos++] = ruledef.charAt(pos++);
                        while (pos < ruledef.length()) {
                            char ch = ruledef.charAt(pos);
                            if (Character.isJavaIdentifierPart(ch)) {
                                if (tokenpos >= tokenbuf.length)
                                    return TT_TooLong;
                                tokenbuf[tokenpos++] = ch;
                                pos++;
                            } else {
                                break;
                            }
                        }
                    }
                    tokenstr = new String(tokenbuf, 0, tokenpos);
                }
                
                /* Specify a float by a trailing 'F' or 'f' */
                if (tokenbuf[tokenpos-1]=='F' || tokenbuf[tokenpos-1]=='f') {
                    isfloat = true;
                    tokenstr = new String(tokenbuf, 0, tokenpos-1);
                }
                try {
                    if (isfloat) {
                        float fval = Float.parseFloat(tokenstr);
                        token = (Object)Float.valueOf(fval);
                        return TT_Float;
                    } else {
                        double dval = Double.parseDouble(tokenstr);
                        token = (Object)Double.valueOf(dval);
                        return TT_Double;
                    }
                } catch (Exception ex) {
                    return TT_BadToken;  /* If it does not convert, it is an error */
                }
            }
        } 
        
        /* 
         * Final checks for a valid name
         */
        if (containsdot || !Character.isJavaIdentifierStart(firstchar)) 
            return TT_BadToken;
        ret = TT_Name;
        
        /*
         * Check for known names.
         * We use the length any only check for known names of that length.
         * Known names are matched independent of case.
         */
        token = null;
        switch (tokenpos) {
        case 2:
            if ("is".equalsIgnoreCase(tokenstr))
                ret = TT_Is;
            else if ("in".equalsIgnoreCase(tokenstr))
                ret = TT_In;
            else if ("or".equalsIgnoreCase(tokenstr))
                ret = TT_Or;
            break;
        case 3:
            if ("and".equalsIgnoreCase(tokenstr))
                ret = TT_And;
            else if ("not".equalsIgnoreCase(tokenstr))
                ret = TT_Not;
            break;
        case 4:
            if ("like".equalsIgnoreCase(tokenstr))
                ret = TT_Like;
            else if ("null".equalsIgnoreCase(tokenstr))
                ret = TT_Null;
            else if ("true".equalsIgnoreCase(tokenstr)) {
                ret = TT_True;
                token = (Object)Boolean.valueOf(true);
            }  
            break;
        case 5:
            if ("false".equalsIgnoreCase(tokenstr)) {
                ret = TT_False;
                token = (Object)Boolean.valueOf(false);
            }  
            break;
        case 6: 
            if ("escape".equalsIgnoreCase(tokenstr))
                ret = TT_Escape;
            break;
        case 7: 
            if ("between".equalsIgnoreCase(tokenstr))
                ret = TT_Between;
            break;
        }
        if (token == null)
            token = (Object) tokenstr;    
        return ret;
    }
    
    /*
     * Output a token for debugging.
     * We also use this is error messages.
     */
    String dumpToken(int kind, Object xtoken) {
        String ret = "";
        if (kind < 0) {
            kind = -kind;
            if (kind<BadTokenNames.length)
                ret = BadTokenNames[kind];
            else
                ret = "token:-" + kind;
        } else {
            if (kind<TokenNames.length)
                ret = TokenNames[kind];
            else
                ret = "token:" + kind;
        }
        if (xtoken != null && kind<TT_EQ) {
            if (xtoken instanceof String)
                ret += " '"+xtoken+"'";
            else 
                ret += " " + xtoken;
        }
        return ret;
    }
    
    /*
     * Compile the rule.
     * The compile strategy is to parse all valid operands and operators within the JMS
     * subset of SQL92.  Since we do not know the type of a variable (or identifier), 
     * we assume it is valid.  For constants and operators we know the type and can reject
     * operands which do not match the required type.
     * <p>
     * The output of the compile is a set of rules, and a set of objects.  The rules are
     * excecuted in order by the select() method.  Some of the rules have references to
     * objects in the object array.
     * <p>
     * During the compile step we use a number of class variables which are not needed after
     * the compile.  These are set to null at the end of the compile to allow garbage 
     * collection.
     */
    void compile(String ruledef) throws JMSException {
        if (ruledef == null)
            ruledef = "";
        this.ruledef = ruledef;
        pos = 0;
        CompileState state = CompileState.Operand;
        
  
        while (state != CompileState.Done) {
            /*
             * Find the next token.  The tokenizer will also skip white space but
             * we do it here to get the error offset.
             */
            while (pos < ruledef.length() && Character.isWhitespace(ruledef.charAt(pos)))
                pos++;
 //          error_offset = pos;
            int kind = getSelectToken();
            if (debug_flag)
                System.out.println(dumpToken(kind, token));
            if (kind < 0) {
                state = CompileState.Done;
                break;
            } 
            switch (state) {
            default:
                break;
            /*
             * We are expecting an operand, which includes unary operators and the left parenthesis.
             */
            case Operand:
                /*
                 * We can ignore unary plus as we define it to change neither the value
                 * nor the type of the operand.  This differs from Java in which the
                 * unary plus promotes byte and short values to int.  In Java you can fix
                 * this specification bug using casting, but there is no similar facility
                 * in SQL92.
                 */
                if (kind == TT_Minus || kind == TT_Plus || kind == TT_Not) {
                    if (kind == TT_Minus)
                        kind = TT_Negative;
                    if (kind != TT_Plus)       /* Ignore unary plus */ 
                        putOpStack(kind);
                    break;
                }
                
                if (kind == TT_None) {
                    state = CompileState.Done;
                    break;
                }
                
                if (kind == TT_Begin) {
                    if (level >= 63) {
                    	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0241", "A request to create a JMS message consumer failed because the selection rule is too complex. A selection rule can have only have 63 levels of nested parentheses.");
                        ImaTrace.traceException(2, jex);
                        throw jex;
                    }
                    level++;
                    oplvl[level] = op_pos;
                } else {
                    state = putOperand(kind);
                }
                break;
               
            /*
             * We are expecting an operator    
             */
            case Op:
                if (kind == TT_Not) {
                    putOpStack(TT_Not);
                }
                if ((kind>= TT_MinOp && kind <= TT_MaxOp) || kind==TT_None ) {
                    
                    /*
                     * For an and token, determine if this is the logical and
                     * operator, or the separator between the second and third
                     * arguments to the between operator.
                     */
                    if (kind == TT_And && level > 0 && oplvl[level] >0 &&
                                    opstack[oplvl[level]-1] == TT_Between) {
                        /*
                         * If this AND forms the final portion of the between
                         * operator, close the level we started with the BETWEEN.
                         */
                        opstack[oplvl[level]-1] = TT_Between2;
                        level--;
                        state = popOpStack(TT_Multiply);  /* close all numeric ops */
                        state = CompileState.Operand;
                        break;
                    }
                    
                    /*
                     * Create rules for all higher priority items
                     */
                    state = popOpStack(kind);
                    if (state == CompileState.Done)
                        break;
                    
                    switch (kind) {
                    
                    /*
                     * The is op is followed by either null, or not null.
                     */
                    case TT_Is:
                        kind = getSelectToken();
                        if (debug_flag)
                            System.out.println(dumpToken(kind, token));
                        boolean notflag = false;
                        if (kind == TT_Not) {
                            notflag = true;
                            kind = getSelectToken();
                            if (debug_flag)
                                System.out.println(dumpToken(kind, token));
                        }
                        if (kind != TT_Null) {
                        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0242", "A request to create a JMS message consumer failed because an IS expression in the selection rule is not valid.");
                            ImaTrace.traceException(2, jex);
                            throw jex;
                        }
                        addRule(RULE_Is, notflag ? TT_Not : TT_Null);
                        state = CompileState.Op;
                        break;
                    
                    /*
                     * The in operator is followed by a group, each member of which
                     * is a constant string, and is separated by comma and surrounded
                     * by parenthesis.  The preceding argument must be a variable.    
                     */
                    case TT_In:
                        checkIdentifier(TT_In);
                        kind = getSelectToken();
                        if (debug_flag)
                            System.out.println(dumpToken(kind, token));
                        if (kind != TT_Begin) {
                        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0243", "A request to create a JMS message consumer failed because an IN operator in the selection rule is not followed by a group.");
                            ImaTrace.traceException(2, jex);
                            throw jex;
                        }
                        kind = TT_Group;
                        int myobjpos = objpos++;
                        int count = 0;
                        while (kind == TT_Group) {
                            kind = getSelectToken();
                            if (debug_flag)
                                System.out.println(dumpToken(kind, token));
                            if (kind != TT_String) {
                            	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0244", "A request to create a JMS message consumer failed because an IN group in the selection rule is not valid.");
                                ImaTrace.traceException(2, jex);
                                throw jex;
                            }
                            addObject(token);
                            count++;
                            kind = getSelectToken();
                            if (debug_flag)
                                System.out.println(dumpToken(kind, token));
                        }
                        if (kind != TT_End) {
                        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0245", "A request to create a JMS message consumer failed because the separator in an IN group of the selection rule is missing, or is not valid.");
                            ImaTrace.traceException(2, jex);
                            throw jex;
                        }
                        addRule(RULE_In, myobjpos);
                        objects[myobjpos] = Integer.valueOf(count);
                        state = CompileState.Op;
                        break;
                    
                    /*
                     * Like is followed by a pattern which must be a constant string.
                     * This is optionally followed by a escape clause where the data
                     * must be a constant string of length 1.
                     */
                    case TT_Like:
                        checkIdentifier(TT_Like);
                        kind = getSelectToken();
                        if (kind != TT_String) {
                        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0246", "A request to create a JMS message consumer failed because the LIKE operator in the selection rule is not followed by a string.");
                            ImaTrace.traceException(2, jex);
                            throw jex;
                        }
                        String pattern = (String)token;
                        char escape = 0;
                        kind = getSelectToken();
                        if (kind != TT_Escape) {
                            saveToken = kind;
                        } else {
                            kind = getSelectToken();
                            if (kind != TT_String || ((String)token).length() != 1) {
                            	ImaInvalidSelectorException jex = new ImaInvalidSelectorException( "CWLNC0247", "A request to create a JMS message consumer failed because the ESCAPE within a LIKE expression of the selection rule is not a single character string.");
                                ImaTrace.traceException(2, jex);
                                throw jex;
                            }
                            escape = ((String)token).charAt(0);
                        }
                        ImaLike likerule = new ImaLike(pattern, escape);
                        addRule(RULE_Like, objpos);
                        addObject(likerule);
                        state = CompileState.Op;
                        break;
                        
                    case TT_End:
                        if (level == 0) {
                        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0258", "A request to create a JMS message consumer failed because there are too many right parentheses in the selection rule.");
                            ImaTrace.traceException(2, jex);
                            throw jex;
                        }
                        level--;
                        state = popOpStack(TT_Multiply);
                        if (state == CompileState.Operand)
                            state = CompileState.Op;
                        break;
                        
                    /*
                     * For between, we add an implied level since the first clause
                     * can be a numeric expression, and we need to differentiate the
                     * and which starts the second clause from an and operator.    
                     */
                    case TT_Between:
                        putOpStack(kind);
                        if (level >= 63) {
                        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0241", "A request to create a JMS message consumer failed because the selection rule is too complex. A selection rule can have only have 63 levels of nested parentheses.");
                            ImaTrace.traceException(2, jex);
                            throw jex;
                        }
                        level++;
                        oplvl[level] = op_pos;
                        break;
                        
                    /* 
                     * For all other operators, just push the operator on the stack.
                     * This also checks for valid constant types before the op.    
                     */
                    default:
                        putOpStack(kind);
                    }
                }
            }
        }
        
        /*
         * We have reached the end of the definition string, so validate
         * the end state.
         */
        if (level != 0) {
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0251", "A request to create a JMS message consumer failed because there are too many left parentheses in the selection rule.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (op_pos != 0) {
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0252", "A field or constant is expected at the end of a selection ruleA request to create a JMS message consumer failed because the selection rule ended unexpectedly.  A field or constant is expected at the end of a selection rule.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (rulepos > 0) {
            switch (rule[rulepos-1]>>16) {
            case RULE_Const:
            case RULE_Calc:
            	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0256", "A request to create a JMS message consumer failed because a Boolean result is required for a selector.");
                ImaTrace.traceException(2, jex);
                throw jex;
            }
        }
        
        /*
         * Cleanup things we only need during the compilation.
         */
        tokenbuf = null;
        oplvl = null;
        opstack = null;
        token = null;
        /* Shorten rule and object array */
        Object [] newobjects = new Object [objpos];
        System.arraycopy(objects, 0, newobjects, 0, objpos);
        objects = newobjects;
        int [] newrules = new int[rulepos];
        System.arraycopy(rule, 0, newrules, 0, rulepos);
        rule = newrules;
    }
    
    
    /*
     * Put an operand on the stack.  
     * This will put out all rules of higher priority.  It also checks that 
     * the preceding rule is of the correct type. 
     */
    void putOpStack(int kind) throws JMSException {
        /* There is no good reason that this should ever happen */
        if (op_pos >= opstack.length-1) {
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0248", "A request to create a JMS message consumer failed because a string in the selection rule is too complex. Only 63 levels of nested operators area allowed in a selection rule.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        int prevrule = rulepos == 0 ? 0 : rule[rulepos-1]>>16;
        switch (kind) {
        case TT_GT:
        case TT_LT:
        case TT_GE:
        case TT_LE:
            checkCompare(prevrule, kind);
            break;
        case TT_Plus:
        case TT_Minus:
        case TT_Multiply:
        case TT_Divide:
        case TT_Between:
            checkNumeric(prevrule, kind);
            break;
        case TT_And:
        case TT_Or:
            checkBoolean(prevrule, kind);
            break;
        }
        opstack[op_pos++] = (byte)kind;
    }
    
    
    /* 
     * Check if there is a unary negative on the top of the stack when we 
     * are loading a constant.  If there is, we will change the value to
     * negative.  This is done since the parser sees a negative number as
     * a unary minus and positive numeric value.
     */
    boolean isNegative() {
        if (op_pos > oplvl[level]) {
            if (opstack[op_pos-1] == (byte)TT_Negative) {
                op_pos--;
                return true;
            }
        }
        return false;
    }
    
    
    /*
     * Put an operand on the rule set.
     * The only reason for having separate rules for string, boolean, and numeric
     * is so that we can check on the JMS semantic restrictions at compile time.
     */
    CompileState putOperand(int kind) throws JMSException {
        switch (kind) {
        case TT_Name:
            addRule(RULE_Var, objpos);
            addObject(token);
            break;
        case TT_Int:
            if (isNegative()) 
                token = Integer.valueOf(-((Integer)token).intValue());
            addRule(RULE_Const, objpos);
            addObject(token);
            break;
        case TT_Long:
            if (isNegative()) 
                token = Long.valueOf(-((Long)token).longValue());
            addRule(RULE_Const, objpos);
            addObject(token);
            break;
        case TT_Double:
            if (isNegative()) 
                token = Double.valueOf(-((Double)token).doubleValue());
            addRule(RULE_Const, objpos);
            addObject(token);
            break;
        case TT_Float:
            if (isNegative()) 
                token = Float.valueOf(-((Float)token).floatValue());
            addRule(RULE_Const, objpos);
            addObject(token);
            break;
            
        case TT_String:
            addRule(RULE_String, objpos);
            addObject(token);
            break;
            
        case TT_True:
        case TT_False:
            addRule(RULE_Boolean, objpos);
            addObject(token);
            break;
 
        default:
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0249", "A request to create a JMS message consumer failed because an identifier or constant in the selection rule is not valid: {0}", 
                            dumpToken(kind, token));
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        return CompileState.Op;
    }
    
    
    /*
     * Add a rule, extending the rule array as required.
     * No rule should ever really be big enough to get into this code.
     */
    void addRule(int op, int value) {
        if (rulepos >= rule.length) {
            int rulecount = rule.length*2;
            int [] newrule = new int[rulecount];
            System.arraycopy(rule, 0, newrule, 0, rule.length);
            rule = newrule;
        }
        rule[rulepos++] = (op<<16) | (value&0xffff);
    }
    
    
    /*
     * Add an object, extending the object array as required.
     * No rule should ever really be big enough to get into this code.
     */
    void addObject(Object obj) {
        if (objpos >= objects.length) {
            int objcount = objects.length*2;
            Object [] newobjects = new Object[objcount];
            System.arraycopy(objects, 0, newobjects, 0, objects.length);
            objects = newobjects;
        }
        objects[objpos++] = obj;
    }
    
    
    /*
     * Pop the operand stack.
     * This implements the operator precedence and checks for expression type.
     */
    CompileState popOpStack(int kind) throws JMSException {
        int op = 0;
        int prevrule;
        while (op_pos > oplvl[level]) {
            op = opstack[--op_pos];
            if (op_priority[op] < op_priority[kind]) {
                op_pos++;
                break;
            }
            prevrule = rulepos == 0 ? 0 : rule[rulepos-1]>>16;
            switch (op) {
            case TT_EQ:
            case TT_NE:
                /*
                 * Perform a special check to pass the TCK.  This is actually a 
                 * syntactically valid construct, but it is meaningless and will
                 * always generate UNKNOWN.  The TCK wants it flagged as invalid.
                 * If the TCK is fixed, remove this check.
                 */
                int pprule = rulepos < 1 ? 0 : rule[rulepos-2]>>16;
                if (pprule == RULE_Boolean && prevrule == RULE_Const) {
                	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0291", ">A request to create a JMS message consumer failed because the selection rule compares Boolean and numeric constants. The compare of Boolean and numeric constants always gives an UNKNOWN.");
                    ImaTrace.traceException(2, jex);
                    throw jex;
                }
                addRule(RULE_Compare, op);
                break;
            case TT_GT:
            case TT_LT:
            case TT_GE:
            case TT_LE:
                checkCompare(prevrule, op);
                addRule(RULE_Compare, op);
                break;
            case TT_Plus:
            case TT_Minus:
            case TT_Multiply:
            case TT_Divide:
                checkNumeric(prevrule, op);
                addRule(RULE_Calc, op);
                break;
            case TT_Negative:
                checkNumeric(prevrule, op);
                addRule(RULE_Neg, 0);
                break;
            case TT_Or:
                checkBoolean(prevrule, op);
                addRule(RULE_Or, 0);
                break;
            case TT_And:
                checkBoolean(prevrule, op);
                addRule(RULE_And, 0);
                break;
            case TT_Not:
                checkBoolean(prevrule, op);
                addRule(RULE_Not, 0);
                break;
            case TT_Between:
            	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0250", "A request to create a JMS message consumer failed because a BETWEEN operator in the selection rule is not valid. This indicates a syntax error in the selection rule.");
                ImaTrace.traceException(2, jex);
                throw jex;
            case TT_Between2:
                checkNumeric(prevrule, op);
                addRule(RULE_Between, 0);
            }
        }
        return (kind == TT_None) ? CompileState.Done : CompileState.Operand;
    }
    
    
    /*
     * Check that the preceding operator is not boolean.
     * The JMS spec does not allow boolean or string fields to be converted to boolean.
     */
    void checkNumeric(int op, int kind) throws JMSException {
        switch (op) {
        case RULE_Const:
        case RULE_Var:
        case RULE_Calc:
        case RULE_Neg:    
            break;
        default:
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0253", "A request to create a JMS message consumer failed because the {0} operator in a selection rule does not allow a Boolean argument.",
                            dumpToken(kind, null));
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }    
    
    
    /*
     * Check that the preceding operator is not numeric.
     * The JMS spec does not allow numeric values to be converted to boolean.
     */
    void checkBoolean(int op, int kind) throws JMSException {
        switch (op) {
        case RULE_Const:
        case RULE_Calc:
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0254", "A request to create a JMS message consumer failed because the {0} operator in a selection rule does not allow a numeric argument.",
                            dumpToken(kind, null));
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }   
    
    /*
     * Check that the operand for a range compare is not a constant string, boolean, or boolean op
     */
    void checkCompare(int op, int kind) throws JMSException {
        switch (op) {
        case RULE_Var:
        case RULE_Const:
        case RULE_Calc:
        case RULE_Neg:    
            break;
        default:
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0259", "A request to create a JMS message consumer failed because the {0} operator in a selection rule does not allow a string or Boolean argument.", 
                            dumpToken(kind, null));
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * The JMS spec requires that the argument preceding LIKE or IN be a variable 
     */
    void checkIdentifier(int kind) throws JMSException {
        int prevrule = rulepos == 0 ? 0 : rule[rulepos-1]>>16;
        if (prevrule != RULE_Var) {
        	ImaInvalidSelectorException jex = new ImaInvalidSelectorException("CWLNC0255", "A request to create a JMS message consumer failed because the {0} operator in a selection rule requires an identifier as an argument.",
                            dumpToken(kind, null)); 
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    
    
    /*
     * Dump a rule for debugging.
     * This is also used for unit testing so care should be taken in modifying the result.
     */
    public String dumpSelector() {
        String ret = "Rule: " + this + "\n";
     
        for (int i=0; i<rulepos; i++) {
            String str;
            int op  = (rule[i]>>16) & 0xffff;
            int val = rule[i]&0xffff;
            if (op >= RuleNames.length) {
                str = "Bad "+op+" "+val+"\n";
            } else {
                str = RuleNames[op];
                switch (op) {
                case RULE_String:
                case RULE_Like:
                    try {
                        str += " '"+objects[val]+"'";
                    } catch (Exception e) {
                        str += " badobject:"+val;
                    }
                    break;
                    
                case RULE_Boolean:
                case RULE_Const:
                case RULE_Var:
                    try {
                        str += " "+objects[val];
                    } catch (Exception e) {
                        str += " badobject:"+val;
                    }
                    break;

                case RULE_Calc:
                    if (val >= TT_Minus && val <= TT_Divide)
                        str += " "+CalcNames[val-TT_Minus];
                    else
                        str += " BAD:"+val;
                    break;
                    
                case RULE_Compare:
                    if (val >= TT_EQ && val <= TT_LE) 
                        str += " "+CompareNames[val-TT_EQ];
                    else
                        str += " BAD:"+val;
                    break;
                    
                case RULE_Is:
                    switch (val) {
                    case TT_Not:   str += " not null";  break;
                    case TT_Null:  str += " null";      break;
                    default:       str += " BAD:"+val;  break;
                    }
                    break;
                    
                case RULE_In: 
                    int j = 0;
                    try {
                        int count = ((Number)objects[val]).intValue();
                        for (j=0; j<count; j++) {
                            str += " '"+objects[val+j+1]+"'";
                        }
                    } catch (Exception e) {
                        str += " BAD:"+val+j;
                    }
                    break;
                }
            }    
            ret += str + "\n";
        }
        return ret;
    }

    /*
     * Return the definition string as the name of the object
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return ruledef;
    }
}
