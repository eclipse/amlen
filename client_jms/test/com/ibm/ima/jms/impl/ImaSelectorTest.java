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

import javax.jms.InvalidSelectorException;

import com.ibm.ima.jms.impl.ImaMessage;
import com.ibm.ima.jms.impl.ImaMessageSelector;
import com.ibm.ima.jms.impl.ImaSession;

import junit.framework.TestCase;

/*
 * Test the message selector.
 * There are two major functions, the selector compiler, and the selector interpreter.
 */
public class ImaSelectorTest extends TestCase {
    public ImaSelectorTest() {
        super("IMA JMS Client Selector test");
    }
    
    /*
     * Copy the constants from IMAMessageSelector 
     */
    static final Boolean SELECT_TRUE    = ImaMessageSelector.SELECT_TRUE;
    static final Boolean SELECT_FALSE   = ImaMessageSelector.SELECT_FALSE;
    static final Boolean SELECT_UNKNOWN = ImaMessageSelector.SELECT_UNKNOWN;
    
    /*
     * Function to try creating a selector we think is bad.
     */
    public String badSelector(String ruledef) {
       try {
           ImaMessageSelector sel = new ImaMessageSelector(ruledef);
           return sel.dumpSelector();
       } catch (InvalidSelectorException ise) {
           return ise.getErrorCode();
       } catch (Exception e) {
           return e.toString();
       }
    }
    
    /*
     * Check the parsing of constants in the compiler.
     * The strategy for testing the compilation is to create the object, and then
     * dump it and see that the expected items exist.  This depends on the syntax
     * of the dump which is not specified.
     */
    public void testConst() throws Exception {
        ImaMessageSelector sel;

        /* Check boolean constants */
        sel = new ImaMessageSelector("true");
        assertTrue(sel.dumpSelector().indexOf("boolean true\n")>=0);

        sel = new ImaMessageSelector("TRUE ");
        assertTrue(sel.dumpSelector().indexOf("boolean true\n")>=0);

        sel = new ImaMessageSelector("  false");
        assertTrue(sel.dumpSelector().indexOf("boolean false\n")>=0);
        
        sel = new ImaMessageSelector("  FALSE");
        assertTrue(sel.dumpSelector().indexOf("boolean false\n")>=0);

        sel = new ImaMessageSelector("  faLse");
        assertTrue(sel.dumpSelector().indexOf("boolean false\n")>=0);

        sel = new ImaMessageSelector("tRuE");
        assertTrue(sel.dumpSelector().indexOf("boolean true\n")>=0);
        
        /* Check numeric constants */
        sel = new ImaMessageSelector("\t1 > 0\n");
        assertTrue(sel.dumpSelector().indexOf("const 1\n")>=0);
        
        sel = new ImaMessageSelector("-1 < 0        ");
        assertTrue(sel.dumpSelector().indexOf("const -1\n")>=0);
        
        sel = new ImaMessageSelector("\t\t1.0 > 0x22");
        assertTrue(sel.dumpSelector().indexOf("const 1.0\n")>=0);
        assertTrue(sel.dumpSelector().indexOf("const 34\n")>=0);
        
        sel = new ImaMessageSelector("\r\n1L > 0.0f");
        assertTrue(sel.dumpSelector().indexOf("const 1\n")>=0);
        assertTrue(sel.dumpSelector().indexOf("const 0.0\n")>=0);
        
        sel = new ImaMessageSelector("1234567890123456 > 0L");
        assertTrue(sel.dumpSelector().indexOf("const 1234567890123456\n")>=0);
        
        sel = new ImaMessageSelector("0 < 314.0E-2");
        assertTrue(sel.dumpSelector().indexOf("const 3.14\n")>=0);
        
        sel = new ImaMessageSelector("555E60 > -55E+60");
        assertTrue(sel.dumpSelector().indexOf("const 5.55E62")>=0);
        assertTrue(sel.dumpSelector().indexOf("const -5.5E61")>=0);

        sel = new ImaMessageSelector("   555e60 > 55e+60 or FaLsE");
        assertTrue(sel.dumpSelector().indexOf("const 5.55E62")>=0);
        assertTrue(sel.dumpSelector().indexOf("const 5.5E61")>=0);
        
        /* Check string constants */
        sel = new ImaMessageSelector("'abc' = def");
        assertTrue(sel.dumpSelector().indexOf("string 'abc'\n") >= 0);
        
        sel = new ImaMessageSelector("'abc''def' = def");
        assertTrue(sel.dumpSelector().indexOf("string 'abc'def'\n") >= 0);
        
        sel = new ImaMessageSelector("'abc\ud840\udc0bdef' = def");
        assertTrue(sel.dumpSelector().indexOf("string 'abc\ud840\udc0bdef'\n") >= 0);
    }
    
    /*
     * Test expressions
     * The strategy for testing the compilation is to create the object, and then
     * dump it and see that the expected items exist.  This depends on the syntax
     * of the dump which is not specified.
     */
    public void testExpressions() throws Exception {
        ImaMessageSelector sel;
        
        sel = new ImaMessageSelector("abc");    /* A variable by itself is a valid expression since it might be a boolean */
        assertTrue(sel.dumpSelector().indexOf("var abc\n") >= 0);
        
        sel = new ImaMessageSelector("abc > 5");
        assertTrue(sel.dumpSelector().indexOf("compare GT") >= 0);
        sel = new ImaMessageSelector("abc <>5");
        assertTrue(sel.dumpSelector().indexOf("compare NE") >= 0);
        sel = new ImaMessageSelector("abc=5");
        assertTrue(sel.dumpSelector().indexOf("compare EQ") >= 0);
        sel = new ImaMessageSelector("abc\t< 5");
        assertTrue(sel.dumpSelector().indexOf("compare LT") >= 0);
        sel = new ImaMessageSelector("abc > 5");
        assertTrue(sel.dumpSelector().indexOf("compare GT") >= 0);
        sel = new ImaMessageSelector("abc >=\t5\t");
        assertTrue(sel.dumpSelector().indexOf("compare GE") >= 0);
        sel = new ImaMessageSelector("abc <=5");
        assertTrue(sel.dumpSelector().indexOf("compare LE") >= 0);
        
        sel = new ImaMessageSelector("abc in ('def', 'ghi') Or def LiKE 'a%b'");
        assertTrue(sel.dumpSelector().indexOf("in 'def' 'ghi'\n") >= 0);
        assertTrue(sel.dumpSelector().indexOf("like 'a%b'") >= 0);
        assertTrue(sel.dumpSelector().indexOf("or\n") >= 0);
        
        sel = new ImaMessageSelector("abc not in ('def', 'ghi') Or def LiKE 'a%b'");
        assertTrue(sel.dumpSelector().indexOf("in 'def' 'ghi'\n") >= 0);
        assertTrue(sel.dumpSelector().indexOf("like 'a%b'") >= 0);
        assertTrue(sel.dumpSelector().indexOf("not\n") >= 0);
        
        sel = new ImaMessageSelector("abc Between (3+5.0) and 47E+0");
        assertTrue(sel.dumpSelector().indexOf("between") >= 0);
        assertTrue(sel.dumpSelector().indexOf("calc add") >= 0);
        
        sel = new ImaMessageSelector("abc NOT beTween 1L and (3.0*5f)");
        assertTrue(sel.dumpSelector().indexOf("between") >= 0);
        assertTrue(sel.dumpSelector().indexOf("calc multiply") >= 0);
        assertTrue(sel.dumpSelector().indexOf("not") >= 0);
        
        sel = new ImaMessageSelector("abc + 5 between def*3 and def*4");
        //System.out.println(sel.dumpSelector());
        
        sel = new ImaMessageSelector("abc noT LIKE 'abc' esCape '$'");
        assertTrue(sel.dumpSelector().indexOf("like 'abc'") >= 0);
        
        ImaMessageSelector.debug_flag = false;
        sel = new ImaMessageSelector("not (abc not Like 'abc') aNd abc > ((003-4+005-6)/02.0+(14))");
  //    System.out.println(sel.dumpSelector());
        assertTrue(sel.dumpSelector().indexOf("not\nnot") >= 0);
        assertTrue(sel.dumpSelector().indexOf("const 3\nconst 4\ncalc subtract\nconst 5\n") >= 0);
        assertTrue(sel.dumpSelector().indexOf("calc divide\nconst 14\ncalc add\n") >= 0);
        
        sel = new ImaMessageSelector("(((((true)))))");
        assertTrue(sel.dumpSelector().indexOf("boolean true\n") >= 0);
        
        sel = new ImaMessageSelector("true or (false and false)");
   //   System.out.println(sel.dumpSelector());
        assertTrue(sel.dumpSelector().indexOf("boolean true\n") >= 0);
    }
    
    /*
     * Test compile errors
     */
    public void testBadCompile() throws Exception {
        String err;
        
        err = badSelector("3");
        assertEquals("CWLNC0256", err);

        err = badSelector("3+5");
        assertEquals("CWLNC0256", err);
        
        err = badSelector("(true");
        assertEquals("CWLNC0251", err);

        err = badSelector("true)");
        assertEquals("CWLNC0258", err);
      
        err = badSelector("((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((");
        assertEquals("CWLNC0241", err);
        
        err = badSelector("abc is fred");
        assertEquals("CWLNC0242", err);

        err = badSelector("abc is not fred");
        assertEquals("CWLNC0242", err);
        
        err = badSelector("abc in 'abc'");
        assertEquals("CWLNC0243", err);
        
        err = badSelector("abc in ()");
        assertEquals("CWLNC0244", err);
        
        err = badSelector("abc in ('def' 'ghi')");
        assertEquals("CWLNC0245", err);
        
        err = badSelector("'abc' like 'a*c'");
        assertEquals("CWLNC0255", err);
        
        err = badSelector("abc like def");
        assertEquals("CWLNC0246", err);
        
        err = badSelector("abc like 'a%c' escape '$j$'");
        assertEquals("CWLNC0247", err);
        
        err = badSelector("abc like 'a%c' escape true");
        assertEquals("CWLNC0247", err);
        
        err = badSelector("abc like 'a%c' escape");
        assertEquals("CWLNC0247", err);
        
        err = badSelector("abc +");
        assertEquals("CWLNC0252", err);
        
        err = badSelector("abc or");
        assertEquals("CWLNC0252", err);
        
        err = badSelector("3 + true");
        assertEquals("CWLNC0253", err);

        err = badSelector("true + 3");
        assertEquals("CWLNC0253", err);
        
        err = badSelector("abc = 3ab");
        assertEquals("CWLNC0252", err);
        
        err = badSelector("3 or true");
        assertEquals("CWLNC0254", err);
        
        err = badSelector("true or 3");
        assertEquals("CWLNC0254", err);
        
        /* Only a lot of unary operators can get this error */
        err = badSelector("-----------------------------------------------------------------------------------------3 == 0");
        assertEquals("CWLNC0248", err);
    
        err = badSelector("abc > true");
        assertEquals("CWLNC0259", err);

        err = badSelector("true > abc");
        assertEquals("CWLNC0259", err);

        err = badSelector("'abc' > abc");
        assertEquals("CWLNC0259", err);
        

        err = badSelector("abc > 'abc'");
        assertEquals("CWLNC0259", err);
        
    }
    
    /*
     * Do some simple selections
     */
    public void testSimpleSelect() throws Exception {
        ImaMessage msg = new ImaMessage((ImaSession)null);
        msg.setStringProperty("abc", "ABC");
        msg.setBooleanProperty("btrue", true);
        msg.setIntProperty("ival", 5);
        
        ImaMessageSelector sel;
        
        sel = new ImaMessageSelector("");
        assertEquals(SELECT_TRUE, sel.select(msg));
        
        sel = new ImaMessageSelector("true");
        assertEquals(SELECT_TRUE, sel.select(msg));
        
        sel = new ImaMessageSelector("btrue");
        assertEquals(SELECT_TRUE, sel.select(msg));
        
        sel = new ImaMessageSelector("false");
        assertEquals(SELECT_FALSE, sel.select(msg));

        sel = new ImaMessageSelector("junk");
        assertEquals(SELECT_UNKNOWN, sel.select(msg));
        
        sel = new ImaMessageSelector("abc = 'ABC'");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("abc <> 'ABC'");
        assertEquals(SELECT_FALSE, sel.select(msg));

        sel = new ImaMessageSelector("abc = junk");
        assertEquals(SELECT_UNKNOWN, sel.select(msg));

        sel = new ImaMessageSelector("abc in ('ABC','DEF')");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("abc like 'A%C'");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("abc not like 'A%C'");
        assertEquals(SELECT_FALSE, sel.select(msg));

        sel = new ImaMessageSelector("abc is not null");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("not abc is null");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("junk is null");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("ival between 5 and 9");
        assertEquals(SELECT_TRUE, sel.select(msg));

        sel = new ImaMessageSelector("ival between 9 and 11");
        assertEquals(SELECT_FALSE, sel.select(msg));
        
        sel = new ImaMessageSelector("true or (false and false)");
        assertEquals(SELECT_TRUE, sel.select(msg));
        
    }
   
    
    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaSelectorTest().testConst();
            new ImaSelectorTest().testExpressions();
            new ImaSelectorTest().testBadCompile();
            new ImaSelectorTest().testSimpleSelect();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
}
