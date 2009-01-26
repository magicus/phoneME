/*
 * @(#)ResultVerifier.java	1.5 06/10/10
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
import junit.framework.AssertionFailedError;
import gunit.framework.TestResultVerifier ;
import gunit.framework.TestResultDescription;

/**
 * <code>ResultVerifier</code> presents the textual description of the
 * testcase operation and lets the user the assert the expected outcome
 * with actual one.
 */
public class ResultVerifier implements TestResultVerifier {
    public ResultVerifier() {
    }

    public void verify(Class cls, 
                       String methodName,
                       TestResultDescription desc) {
        String text = desc.getText() ;
        if ( text == null ) {
            // no result description for the test, so the developer
            // has either asserted using Assert.assert() or dont 
            // care for manual verification.
            return ;
        }

        String image_url = desc.getImageURL() ;
        if ( image_url == null ) 
            image_url = "<none>" ;
        System.out.println(cls.getName()+"."+methodName) ;
        System.out.println("Text      : "+text+"\n") ;
        System.out.println("Image URL : "+image_url+"\n") ;

        System.out.print("Enter result [(p)assed/(f)ailed][default=p]:");
        BufferedReader br  
            = new BufferedReader(new InputStreamReader(System.in));
        String result_code = null ;
        try {
            result_code = br.readLine().trim() ;
        }
        catch ( Exception ex ) {
            throw new RuntimeException(ex.getMessage());
        }
        if ( result_code.length() <= 0 || "p".equals(result_code) ) {
            return ;
        }
        else 
        if ( "f".equals(result_code) ) {
            throw new AssertionFailedError("");
        }
        else {
            throw new IllegalArgumentException("Invalid result code");
        }
    }
}
