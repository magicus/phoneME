/*
 * @(#)TestContext.java	1.7 06/10/10
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

package gunit.framework ;

import java.util.* ;
import java.io.* ;

/**
 * <code>TestContext</code> provides the context for the test. An 
 * instance is got from the factory method 
 * <code>TestContext.getInstance()</code>
 */
public class TestContext {
    /**
     * Value of this property is a <code>Class</code> object that
     * specifies test container class
     */
    public static final String TEST_CONTAINER_CLASS = 
        "gunit.test.container.class" ;

    /**
     * Value of this property is a <code>Class</code> object that
     * specifies result printer class, which should be a subclass
     * of <code>junit.textui.ResultPrinter</code>
     */
    public static final String RESULT_PRINTER_CLASS = 
        "gunit.result.printer.class" ;

    /**
     * Value of this property is a <code>Class</code> object that
     * specifies result container class
     */
    public static final String TEST_RESULT_VERIFIER_CLASS = 
        "gunit.test.result.verifier.class" ;

    /**
     * Value of this property is a <code>String</code> object that
     * specifies a file that contains the arguments for the testcases
     */
    public static final String TEST_ARGS_FILENAME = 
        "gunit.test.args.filename" ;

    /**
     * Value of this property is a <code>String</code> object that
     * specifies the directories (seperated using :) 
     * where the reference images are kept
     */
    public static final String REF_IMAGE_PATH = 
        "gunit.refimage.path" ;

    /**
     * Value of this property is a <code>TestFilter</code> object.
     */
    public static final String TEST_FILTER = 
        "gunit.test.filter.object" ;

    private static TestContext instance = null ;

    public synchronized static TestContext getInstance() {
        if ( instance == null ) {
            instance = new TestContext() ;
        }
        return instance ;
    }

    Map map ;
    Map modifiedMap ;
    private TestContext() {
        // load the defaults from the stream
        init() ;
    }

    private void init() {
        InputStream stream = 
            getClass().getResourceAsStream("defaults.properties") ;
        if ( stream == null ) {
            return ;
        }

        Properties prop = new Properties() ;
        try {
            prop.load(stream) ;
        }
        catch ( Exception ex ) {
        }

        this.map = new HashMap() ;
        this.modifiedMap = new HashMap() ;
        this.map.putAll(prop) ;
    }

    public String getStringValue(String name) {
        //System.out.println("get("+name+")="+this.map.get(name));
        return (String)this.map.get(name) ;
    }

    public Class getClassValue(String name) {
        String value = getStringValue(name);
        if ( value == null ) 
            return null ;
        try {
            return Class.forName(value);
        }
        catch (Exception ex) {
            return null ;
        }
    }

    public int getIntValue(String name) {
        String value = getStringValue(name);
        if ( value == null ) 
            return 0 ;
        try {
            return Integer.parseInt(value);
        }
        catch (Exception ex) {
            return 0 ;
        }
    }

    public boolean getBooleanValue(String name) {
        String value = getStringValue(name);
        if ( value == null ) 
            return false ;
        try {
            return Boolean.valueOf(value).booleanValue();
        }
        catch (Exception ex) {
            return false ;
        }
    }

    public Object getObjectValue(String name) {
        Class value = getClassValue(name) ;
        if ( value == null ) 
            return null ;
        try {
            return value.newInstance() ;
        }
        catch (Exception ex) {
            return null ;
        }
    }

    /**
     * Returns the object assigned to the <i>name</i> without doing any
     * processing as the other <code>getXXXValue()</code> methods
     */
    public Object getValue(String name) {
        Object value = this.map.get(name) ;
        return value ;
    }

    public synchronized void setValue(String name, Object value) {
        if ( this.modifiedMap.get(name) == null ) {
            this.modifiedMap.put(name, Boolean.TRUE);
            this.map.put(name, value) ;
        }
    }

    private void dumpMap(String mesg, Map m) {
        Iterator iter = m.keySet().iterator() ;
        System.out.println(mesg) ;
        while (iter.hasNext() ) {
            Object key = iter.next();
            System.out.println(key+"="+m.get(key));
        }
    }
}
