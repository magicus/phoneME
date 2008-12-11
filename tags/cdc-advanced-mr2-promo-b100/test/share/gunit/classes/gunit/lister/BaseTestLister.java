/*
 * @(#)BaseTestLister.java	1.5 06/10/10
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */


package gunit.lister ;

import java.util.* ;
import java.io.* ;
import java.lang.reflect.* ;
import junit.framework.Test ;
import junit.framework.TestCase ;
import junit.framework.TestSuite ;
import junit.textui.TestRunner ;
import gunit.framework.TestFactory ;

/**
 * <code>BaseTestLister</code> is a base class for listing testcases
 */
public abstract class BaseTestLister {
    List testcases ;
    protected PrintStream out ;

    protected BaseTestLister() {
        this.testcases = new ArrayList() ;
        setStream(System.out) ;
    }

    protected final void addTestcases(Test test) {
        if ( test instanceof TestSuite ) {
            addTestcases((TestSuite)test) ;
        }
        else
        if ( test instanceof TestCase ) {
            addTestcase((TestCase)test);
        }
        else {
            addTestcases(test.getClass()) ;
        }
    }

    protected final void setStream(PrintStream stream) {
        this.out = stream ;
    }

    void addTestcases(TestSuite suite) {
        Enumeration e = suite.tests() ;
        while ( e.hasMoreElements() ) {
            Test t = (Test) e.nextElement() ;
            addTestcases(t) ;
        }
    }

    void addTestcase(TestCase testcase) {
        try {
            Method method = testcase.getClass().getMethod(testcase.getName(),
                                                          null) ;
            if ( isPublicTestMethod(method)) {
                this.testcases.add(method) ;
            }
        }
        catch ( Exception ex ) {
        }
    }

    void addTestcases(Class cls) {
        try {
            Method[] methods = cls.getMethods() ;
            for ( int i = 0 ; i < methods.length ; i ++ ) {
                if ( isPublicTestMethod(methods[i])) {
                    this.testcases.add(methods[i]) ;
                }
            }
        }
        catch (Exception ex) {
        }
    }

    private boolean isPublicTestMethod(Method m) {
        return isTestMethod(m) && Modifier.isPublic(m.getModifiers());
    }
    
    private boolean isTestMethod(Method m) {
        String name= m.getName();
        Class[] parameters= m.getParameterTypes();
        Class returnType= m.getReturnType();
        return parameters.length == 0 && 
            name.startsWith("test") && 
            returnType.equals(Void.TYPE);
     }

    /**
     * List the testcase methods contained within the lister
     */
    public void list() {
        Iterator iter = this.testcases.iterator() ;
        while ( iter.hasNext() ) {
            listTestCase((Method)iter.next());
        }
    }

    // --tb | --testbundle         <filename>
    protected void usage() {
        System.err.println("Usage : java "+
                           getClass().getName() 
                           + " --tb <filename>|<testcaseclass>") ;
    }

    /**
     * Start the lister to list tests specified in args
     */
    protected void start(String args[]) {
        if ( args.length == 0) {
            usage() ;
            return ;
        }

        String test_bundle = null ;
        if ( "--tb".equals(args[0]) || "--testbundle".equals(args[0]) ) {
            if ( args.length > 1 ) {
                test_bundle = args[1] ;
            }
            else {
                usage() ;
                return ;
            }
        }
       
        Test test = null ;
        if ( test_bundle != null ) {
            test = TestFactory.createTest(test_bundle) ;
        }
        else {
            TestRunner runner = new TestRunner() ;
            test = runner.getTest(args[0]) ;
        }
        addTestcases(test) ;
        list() ;
    }

    /**
     * List the testcase method specified
     */
    public abstract void listTestCase(Method method) ;
}
