/*
 * @(#)CVMpkg.java	1.10 06/10/10
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
package vm;
import java.util.Vector;
/*
 * Package name in CVM type system.
 */

public class CVMpkg {
    public String	pkgName;
    public CVMpkg	next;
    public CVMDataType typeData[];
    public int	        entryNo;

    public final static int NCLASSHASH = 11;
    public final static int NPACKAGEHASH = 17;

    public static Vector pkgVector = new Vector(); // must come before nullPackage!

    public static CVMpkg packages[] = new CVMpkg[NPACKAGEHASH];
    static int    nextPkgNo = 0;

    synchronized static void enlist( CVMpkg x ){
	x.entryNo = nextPkgNo++;
	pkgVector.addElement( x );
    }
    
    CVMpkg( String pname ){
	pkgName = pname;
	next = null;
	typeData = new CVMDataType[NCLASSHASH];
	enlist( this );
    }

    /*
     * order matters. In order to invoke the usual constructor,
     * this seems to have to follow it!!
     */

    public static CVMpkg nullPackage = new CVMpkg( "" );


    public static CVMpkg
    lookup( String pkgname ){
	if ( pkgname.length() == 0 ) return nullPackage;
	pkgname = pkgname.intern();
	long nameHash = CVMDataType.computeHash( pkgname );
	/* make sure % is unsigned! */
	int bucket = (int)((nameHash&0xffffffffL) % (long)NPACKAGEHASH);
	/*DEBUG System.out.println("hash package "+pkgname+" to "+bucket); */

	for ( CVMpkg thisPkg = packages[bucket];
	    thisPkg != null;
	    thisPkg = thisPkg.next )
	{
	    if ( pkgname == thisPkg.pkgName )
		return thisPkg;
	}
	// not there, must add.
	CVMpkg p = new CVMpkg( pkgname );
	p.next = packages[bucket];
	packages[bucket] = p;
	return p;
    }
}

