/*
 * @(#)CVMDataType.java	1.9 06/10/10
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
 * CVM has a complex scheme for representing types as 16-bit
 * cookies. This code mimics the run-time code and thus builds the
 * corresponding data structures, to be ROMed.
 */

package vm;
import java.util.Vector;
import java.util.Enumeration;

public abstract class CVMDataType implements CVMTypeCode, util.ClassFileConst {

/* instance data */
    /* Instances of this only appear in the
     * table. The only run-time external representation is 
     * the cookie that references a table entry. Here are
     * the fields of the table.
     */
    public int		 entryNo;
    public int           typeNo;
    public CVMDataType   next;	// hash chain

    /* see also the sub-types which contain all the interesting instance data */

/* static data */
    public
    static Vector dataTypes   = new Vector();
    static int    nextEntryNo = 0;
    static int    nextTypeNo  = CVMtypeLastScalar+1;

/* constructor &c */

    static synchronized void enlist( CVMDataType x ){
	x.entryNo = nextEntryNo++;
	x.typeNo  = nextTypeNo++;
	dataTypes.addElement( x );
    }

    CVMDataType(){
	enlist( this );
    }

/* instance methods */


/* static methods */

    /*
     * These really belong in CVMTypeCode...
     */
    public static boolean
    isArray( int t ){
	return (((t)&CVMtypeArrayMask) != 0 );
    }

    public static boolean typeEquals( int t1, int t2 ){
	return ((t1)==(t2));
    }

    public static boolean
    isBigArray( int t ){
	return (((t)&CVMtypeArrayMask)==CVMtypeBigArray);
    }

    /*
     * Return count of table entries.
     */
    public static int nTableEntries(){
	return dataTypes.size();
    }

    /*
     * We use a very simple minded name hashing scheme.
     * Based on the LAST few letters of the name, which are
     * the most likely to differ, I believe.
     */
    public static int
    computeHash( String name ){
	int v = 0;
	int t = name.length();
	int n = Math.min( t, 7 );
	t -= n;
	while ( n--> 0 ){
	    v = (v*19) + name.charAt(t+n) - '/';
	}
	return v;
    }

    /*
     * Class name is just a name, stripped of any envelopping
     * L...;
     */
    static CVMClassDataType
    referenceClass( String className ){
	// parse into class and package
	// package may be null, of course.
	// lookup package, use as primary key to finding
	// the class.
	int endPkg = className.lastIndexOf( SIGC_PACKAGE );
	String pkgName;
	String classInPkg;
	if ( endPkg < 0 ){
	    pkgName = "";
	    classInPkg = className;
	} else {
	    pkgName = className.substring(0,endPkg);
	    classInPkg = className.substring(endPkg+1);
	}
	classInPkg = classInPkg.intern();
	CVMpkg thisPkg = CVMpkg.lookup( pkgName );
	//
	// have a package, have a class-name-in-package.
	// do simple lookup.
	int nameHash = computeHash( classInPkg );
	/* make sure % is unsigned! */
	int bucket = (int)((((long)nameHash)&0xffffffffL) % CVMpkg.NCLASSHASH);
	CVMDataType prev = null;
	for ( CVMDataType d = thisPkg.typeData[bucket];
	    d != null ;
	    d = d.next )
	{
	    if ( d instanceof CVMClassDataType ){
		CVMClassDataType c = (CVMClassDataType)d;
		if (c.classInPackage == classInPkg) {
		    return c;
                }
	    }
	    prev = d;
	}
	// not there. add it.
	CVMClassDataType c = new CVMClassDataType( thisPkg, classInPkg );
	// at end of the chain, NOT AT HEAD.
	if (prev == null){
	    thisPkg.typeData[bucket] = c;
	    c.next = null;
	} else {
	    prev.next = c;
	    c.next  = null;
	}
	return c;
    }

    static CVMArrayDataType
    referenceArray( int depth, int baseType ){
	return referenceArray( depth, baseType, CVMpkg.nullPackage, 0 );
    }

    static CVMArrayDataType
    referenceArray( int depth, CVMClassDataType d ){
	return referenceArray( depth, d.typeNo, d.pkg, computeHash( d.classInPackage ) );
    }

    static CVMArrayDataType
    referenceArray( int depth, int baseType, CVMpkg thisPkg, int nameHash ){
	//
	// have a package,
	// do simple lookup.
	// make sure % is unsigned!
	int bucket = (int)((((long)nameHash)&0xffffffffL) % CVMpkg.NCLASSHASH);
	for ( CVMDataType d = thisPkg.typeData[bucket];
	    d != null ;
	    d = d.next )
	{
	    if ( d instanceof CVMArrayDataType ){
		CVMArrayDataType a = (CVMArrayDataType)d;
		if ( a.depth == depth && a.baseType == baseType )
		    return a;
	    }
	}
	// not there. add it.
	CVMArrayDataType a = new CVMArrayDataType( depth, baseType );
	// at end...
	a.next = thisPkg.typeData[bucket];
	thisPkg.typeData[bucket] = a;
	return a;
    }

    public static int
    CVMtypeArrayDepth( int t ){
	if (!isBigArray(t)){
	    return t>>CVMtypeArrayShift;
	}
	CVMArrayDataType d = (CVMArrayDataType)(dataTypes.elementAt( t&CVMtypeBasetypeMask));
	return d.depth;
    }

    public static int CVMtypeArrayBasetype( int t ){
	if (!isBigArray(t) ){
	    return t&CVMtypeBasetypeMask;
	}
	CVMArrayDataType d = (CVMArrayDataType)(dataTypes.elementAt( t&CVMtypeBasetypeMask));
	return d.baseType;
    }

    public static int
    parseSignature( String sig ){
	int sigLength = sig.length();
	int sigIndex  = 0;

	int arrayDepth = 0;
	int baseType   = 0;

	CVMClassDataType d = null;

	while ( (sigIndex<sigLength) && (sig.charAt(sigIndex)==SIGC_ARRAY) ){
	    sigIndex += 1;
	    arrayDepth += 1;
	}
	if ( sigIndex >= sigLength ) return CVM_T_ERROR;
	switch ( sig.charAt( sigIndex++ ) ){
	case SIGC_VOID:
	    baseType = CVM_T_VOID;
	    break;
	case SIGC_INT:
	    baseType = CVM_T_INT;
	    break;
	case SIGC_SHORT:
	    baseType = CVM_T_SHORT;
	    break;
	case SIGC_LONG:
	    baseType = CVM_T_LONG;
	    break;
	case SIGC_BYTE:
	    baseType = CVM_T_BYTE;
	    break;
	case SIGC_CHAR:
	    baseType = CVM_T_CHAR;
	    break;
	case SIGC_FLOAT:
	    baseType = CVM_T_FLOAT;
	    break;
	case SIGC_DOUBLE:
	    baseType = CVM_T_DOUBLE;
	    break;
	case SIGC_BOOLEAN:
	    baseType = CVM_T_BOOLEAN;
	    break;
	case SIGC_CLASS:
	    d = referenceClass( sig.substring( sigIndex, sigLength-1) );
	    baseType = d.typeNo;
	    break;
	default:
	    return CVM_T_ERROR;
	}
	if ( arrayDepth <= CVMtypeMaxSmallArray )
	    return (arrayDepth<<CVMtypeArrayShift)+baseType;
	/*
	 * Here for a deep array.
	 */
	if ( d==null ){
	    baseType = referenceArray( arrayDepth, baseType ).typeNo;
	} else {
	    baseType = referenceArray( arrayDepth, d ).typeNo;
	}
	return CVMtypeBigArray|baseType;
    }

    /*
     * Take either a stripped classname (no L...;)
     * or an array pseudo-classname (starts with [)
     * Return a type cookie.
     */
    public static int
    lookupClassname( String name ){
	if ( name.charAt(0) == SIGC_ARRAY )
	    return parseSignature( name );
	else
	    return referenceClass( name ).typeNo;
    }

}
