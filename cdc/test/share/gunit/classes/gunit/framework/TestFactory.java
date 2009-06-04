/*
 * @(#)TestFactory.java	1.5 06/10/10
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

import java.io.* ;
import java.util.* ;

import junit.framework.Test ;
import junit.framework.TestSuite ;

/**
 * <code>TestFactory</code> provides methods to create <code>Test</code>
 */
public class TestFactory {
    public static Test createTest(Class[] testClasses) {
        TestSuite suite = new TestSuite() ;
        for ( int i = 0 ; i < testClasses.length ; i ++ ) {
            suite.addTest(new TestSuite(testClasses[i])) ;
        }
        return suite ;
    }

    public static Test createTest(String fileName) {
        try {
            return createTest(new FileInputStream(fileName));
        }
        catch ( Exception ex ) {
            return new TestSuite() ;
        }
    }

    public static Test createTest(InputStream stream) {
        if ( !(stream instanceof BufferedInputStream) ) {
            stream = new BufferedInputStream(stream) ;
        }

        List classes = new ArrayList() ;
        String line = null ;
        BufferedReader reader = new BufferedReader(
                                    new InputStreamReader(stream));
        try {
            while ( (line = reader.readLine()) != null ) {
                try {
                    classes.add(Class.forName(line.trim()));
                }
                catch ( Exception ex ) {
                    System.out.println("Unable to create "+line) ;
                }
            }
        }
        catch ( Exception ex ) {
            System.out.println(ex) ;
        }
        finally {
            try {
                stream.close() ;
            }
            catch ( Exception ex ) {
            }
        }

        return createTest((Class[])classes.toArray(new Class[0]));
    }
}
