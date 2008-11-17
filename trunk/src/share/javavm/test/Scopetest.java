/*
 * @(#)Scopetest.java	1.5 06/10/10
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
 *
 */
/*
 * Regression test for ROMizer bug #4817979.
 * You must build with CVM_PRELOAD_TEST=true to reproduce.
 */
public class Scopetest {

static void test( scopetest.a.C obj, String objname, String expect ){
    String result = obj.callF();
    if ( result.equals(expect)){
	System.out.println(objname+".f() OK");
    } else {
	System.out.println("BAD: calling "+objname+".f() => "+result+" expected "+expect);
    }
}

public static void main( String ignore[] ){

    test( new scopetest.a.C(), "a.C", "a.C" );
    test( new scopetest.b.C1(), "b.C1", "a.C" );
    test( new scopetest.a.C2(), "a.C2", "a.C2");

}
}
