/*
 * @(#)CVMInitInfo.java	1.10 06/10/10
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
package runtime;

class CVMInitInfo {
    /*
     * This class puts together a list of "initialization records".
     *
     * This takes two possible forms:
     * 
     * A triple of:
     *     from-address
     *     to-address
     *     byte count
     *
     * Or a list of:
     *     pointers to ROMizer generated data structs.
     *
     * This data is interpreted by the startup code.
     */
    String fromAddress;
    String toAddress;
    String byteCount;
    CVMInitInfo next;

    CVMInitInfo( String f, String t, String c, CVMInitInfo n ){
	fromAddress = f;
	toAddress = t;
	byteCount = c;
	next = n;
    }

    //
    // The single address version
    //
    CVMInitInfo( String f, CVMInitInfo n ){
	fromAddress = f;
	next = n;
    }

    CVMInitInfo() {
    }

    CVMInitInfo initList = null;

    public void
    addInfo( String f, String t, String c ){
	initList = new CVMInitInfo( f, t, c, initList );
    }

    public void
    addInfo( String f ){
	initList = new CVMInitInfo( f, initList );
    }

    public void
    write( CCodeWriter out, String typename, String dataname){
	out.println("const "+typename+" "+dataname+"[] = {");
	if (initList.toAddress == null && initList.byteCount == null) {
	    // The single address variant
	    for ( CVMInitInfo p = initList; p != null; p = p.next ){
		out.println("    "+p.fromAddress+",");
	    }
	    out.println("    NULL"); /* terminator */
	} else {
	    // The triple variant
	    for ( CVMInitInfo p = initList; p != null; p = p.next ){
		out.println("    { "+p.fromAddress+", "+p.toAddress+", "+p.byteCount+" },");
	    }
	    out.println("    {NULL, NULL, 0}"); /* terminator */
	}
	
	out.println("};");
    }
}
