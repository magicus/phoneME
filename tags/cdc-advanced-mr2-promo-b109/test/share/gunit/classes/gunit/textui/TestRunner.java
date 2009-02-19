/*
 * @(#)TestRunner.java	1.7 06/10/10
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


package gunit.textui ;

import java.io.* ;
import java.util.* ;
import junit.framework.Test ;
import junit.framework.TestResult ;
import junit.textui.ResultPrinter ;
import gunit.framework.TestContext ;
import gunit.framework.TestFilter ;
import gunit.framework.TestFactory ;

/**
 * <Code>TestRunner</code> is a runner that can run a suite of tests.
 * It supports all junit.textui.TestRunner options and 
 * inaddition to that it supports gunit specific options.
 * <b>java gunit.textui.TestRunner --h</b> would print the usage of 
 * the runner
 */
public class TestRunner extends junit.textui.TestRunner {
    /**
     * Constructs a TestRunner.
     */
    public TestRunner() {
        this(System.out);
    }

    /**
     * Constructs a TestRunner using the given stream for all the output
     */
    public TestRunner(PrintStream writer) {
        this(new ResultPrinter(writer));
    }
   
    /**
     * Constructs a TestRunner using the given ResultPrinter all the output
     */
    public TestRunner(ResultPrinter printer) {
        super(printer) ;
    }

    private static final String USAGE_FILE = "TestRunner_usage.txt" ;

    protected void usage() {
        usage(USAGE_FILE) ;
    }

    private static void usage(String usageFile) {
        InputStream stream =
            TestRunner.class.getResourceAsStream(usageFile) ;
        if ( stream == null ) {
            System.out.println("Unable to get resource for "+usageFile) ;
            return ;
        }
        if ( !(stream instanceof BufferedInputStream) ) {
            stream = new BufferedInputStream(stream) ;
        }
        BufferedReader reader =
            new BufferedReader(new InputStreamReader(stream)) ;
        try {
            String line = null ;
            while ((line = reader.readLine()) != null ) {
                System.out.println(line) ;
            }
        }
        catch ( Exception ex ) {
            System.out.println("Unable to read the usage:"+ex) ;
        }
    }

    // All gunit.textui.TestRunner options starts with "--", while that of
    // junit.textui.TestRunner starts with "-"
    //
    // gunit.textui.TestRunner options
    // -------------------------------
    // --cc | --containerclass     <classname>
    // --pc | --printerclass       <classname>
    // --ta | --testargs           <filename>.xml
    // --tb | --testbundle         <filename>
    // --rp | --refimagepath       <directory>[:<directory>]
    protected TestResult start(String args[]) throws Exception {
        String test_bundle = null ;
        List junit_args = new ArrayList() ;
        boolean wait = false ; // junit.textui.TestRunner option

        TestContext context = TestContext.getInstance() ;
        try {
            for (int i= 0; i < args.length; i++) {
                if ( args[i].startsWith("--") ) {
                    if ( "--h".equals(args[i]) || 
                         "--help".equals(args[i])) {
                        usage() ;
                        System.exit(SUCCESS_EXIT);
                    }
                    if ( "--cc".equals(args[i]) || 
                         "--containerclass".equals(args[i])) {
                        context.setValue(TestContext.TEST_CONTAINER_CLASS,
                                         args[++i]);
                    }
                    else
                    if ( "--pc".equals(args[i]) || 
                         "--printerclass".equals(args[i])) {
                        context.setValue(TestContext.RESULT_PRINTER_CLASS,
                                         args[++i]);
                    }
                    else
                    if ( "--ta".equals(args[i]) || 
                         "--testargs".equals(args[i])) {
                        String arg = args[++i] ;
                        if ( new File(arg).exists() ) {
                            context.setValue(TestContext.TEST_ARGS_FILENAME, 
                                             arg) ;
                        }
                        else {
                            System.out.println(arg+" file does not exist") ;
                            usage() ;
                            System.exit(EXCEPTION_EXIT);
                        }
                    }
                    else
                    if ( "--tb".equals(args[i]) || 
                         "--testbundle".equals(args[i])) {
                        test_bundle = args[++i] ;
                        if ( !(new File(test_bundle).exists()) ){
                            System.out.println(test_bundle+
                                               " file does not exist") ;
                            usage() ;
                            System.exit(EXCEPTION_EXIT);
                        }
                    }
                    else
                    if ( "--rp".equals(args[i]) || 
                         "--refimagepath".equals(args[i])) {
                        context.setValue(TestContext.REF_IMAGE_PATH,
                                         args[++i]);
                    }
                }
                else {
                    junit_args.add(args[i]) ;
                    if ( "-wait".equals(args[i]) )
                        wait = true ;
                }
            }
        }
        catch ( ArrayIndexOutOfBoundsException bex ) {
            usage() ;
            System.exit(EXCEPTION_EXIT);
        }
            
        // the last junit argument is the <testcase-class> or
        // <testcase-method> (supported only by gunit), we examine the
        // last argument and if it is a <testcase-method>, create a 
        // test filter with the method name and pass the class to 

        // if test_bundle is present then the <testcase-class> or 
        // <testcase-method> is ignored.
        if ( test_bundle == null && junit_args.size() > 0 ) {
            String arg = (String)junit_args.get(junit_args.size()-1) ;
            // check if the "arg" is a class
            try {
                Class.forName(arg) ;
            }
            catch ( Exception ex ) {
                // arg is probably <classname>.<methodname>
                int index = arg.lastIndexOf('.') ;
                if ( index > 0 ) {
                    String class_name = arg.substring(0,index);
                    String method_name = arg.substring(index+1) ;
                    context.setValue(TestContext.TEST_FILTER, 
                                new SingleMethodTestFilter(class_name,
                                                           method_name)) ;

                    // replace the last argument with the class name
                    junit_args.set(junit_args.size()-1, class_name) ;
                }
            }
            
        }

        ResultPrinter printer = (ResultPrinter)
            context.getObjectValue(TestContext.RESULT_PRINTER_CLASS);
        if ( printer != null ) 
            setPrinter(printer) ;
        
        if ( test_bundle != null ) {
            Test suite = TestFactory.createTest(test_bundle) ;
            return super.doRun(suite, wait);
        }
        else {
            return super.start((String[])junit_args.toArray(new String[0]));
        }
    }

    public static void main(String args[]) {
        TestRunner aTestRunner= createRunner() ;
        try {
            TestResult r= aTestRunner.start(args);
            if (!r.wasSuccessful()) 
                System.exit(FAILURE_EXIT);
            System.exit(SUCCESS_EXIT);
        } catch(Exception e) {
            System.err.println(e.getMessage());
            System.exit(EXCEPTION_EXIT);
        }
    }

    private static TestRunner createRunner() {
        String class_name = System.getProperty("gunit.textui.runner.class",
                                               TestRunner.class.getName());
        return createRunner(class_name) ;
    }

    private static TestRunner createRunner(String runnerClass) {
        try {
            return (TestRunner)Class.forName(runnerClass).newInstance() ;
        }
        catch ( Exception ex ) {
            System.out.println("WARNING:Unable to create runner "+runnerClass+
                               ",so creating "+TestRunner.class) ;
            return new TestRunner() ;
        }
    }

    static class SingleMethodTestFilter implements TestFilter {
        String className ;
        String methodName ;

        SingleMethodTestFilter(String className, String methodName) {
            this.className = className ;
            this.methodName = methodName ;
        }

        public boolean allowExecution(Class cls, String method) {
            return ( cls.getName().equals(this.className) && 
                     method.equals(this.methodName) );
        }
    }
}

