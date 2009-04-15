/*
 * @(#)ArrayClassInfo.java	1.30 06/10/22
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
import consts.Const;
import components.*;
import util.*;

/*
 * An array is a class.
 * It is a subclass of java.lang.Object.
 * It has all the class-related runtime data structures, or at least
 * may of them.
 * It is not read in from class files, but is made up on-the-fly
 * when the classresolver sees a reference to a classname starting with
 * "[".
 *
 * In order to resolve such references early, we must do likewise here.
 */
public
class ArrayClassInfo extends ClassInfo {

    private static int nFake = 0;
    public int    	 arrayClassNumber;
    public int    	 depth;
    public int    	 baseType;
    public ClassConstant baseClass;
    public int           elemClassAccess;
    public ClassConstant subarrayClass;
    
    /*
     * Given signature s,
     * fill in
     * depth ( i.e. number of opening [ )
     * basetype ( i.e. thing after the last [ )
     * and baseClass ( if basetype is a classtype )
     */
    private void fillInTypeInfo( String s ) throws DataFormatException {
	int index = 0;
	char c;
	while( ( c = s.charAt( index ) ) == Const.SIGC_ARRAY )
	    index++;
	depth = index;
	switch( c ){
	case Const.SIGC_INT:
	    baseType = Const.T_INT;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_LONG:
	    baseType = Const.T_LONG;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_FLOAT:
	    baseType = Const.T_FLOAT;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_DOUBLE:
	    baseType = Const.T_DOUBLE;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_BOOLEAN:
	    baseType = Const.T_BOOLEAN;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_BYTE:
	    baseType = Const.T_BYTE;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_CHAR:
	    baseType = Const.T_CHAR;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_SHORT:
	    baseType = Const.T_SHORT;
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_CLASS:
	    baseType = Const.T_CLASS;	    
	    ClassInfo baseClassInfo = baseClass.find();
	    
	    // The base class' access should propagate to all arrays with
	    // that base class, so elemClassAccess really is equivalent
	    // to the the access flags of the base class
	    elemClassAccess = baseClassInfo.access;
	    break;

	default:
	    throw new DataFormatException(Localizer.getString("arrayclassinfo.malformed_array_type_string.dataformatexception",s));
	}
	// For CVM, we want to know the sub-class, which is different from
	// the base class.
	if ( depth > 1 ){
	    // sub-classes have already been entered
	    ClassInfo ci = ClassTable.lookupClass(s.substring(1));
	    subarrayClass = new ClassConstant(ci);
	} else if ( depth == 1 ) {
	    subarrayClass = baseClass;
	}
    }

    public ArrayClassInfo( boolean v, String s, ClassConstant base )
	throws DataFormatException
    {
	super(v);
	arrayClassNumber = nFake++;
	baseClass = base;
	className = s;
	thisClass = new ClassConstant( new UnicodeConstant( s ) );
	superClassInfo = ClassTable.lookupClass("java/lang/Object");
	superClass = superClassInfo.thisClass;
	access = Const.ACC_FINAL|Const.ACC_ABSTRACT;
	methods = new MethodInfo[0];
	fields  = new FieldInfo[0];
	fillInTypeInfo( s );
	access |= (elemClassAccess & Const.ACC_PUBLIC);
    }

    protected String createGenericNativeName() { 
        return /*NOI18N*/"fakeArray" + arrayClassNumber;
    }

    public void buildFieldtable() {}

    public void buildMethodtable() {}

    public String getNativeName() {
	switch ( baseType ) {
	case Const.T_INT:
	    return "Int";
	case Const.T_LONG:
	    return "Long";
	case Const.T_FLOAT:
	    return "Float";
	case Const.T_DOUBLE:
	    return "Double";
	case Const.T_BOOLEAN:
	    return "Boolean";
	case Const.T_BYTE:
	    return "Byte";
	case Const.T_CHAR:
	    return "Char";
	case Const.T_SHORT:
	    return "Short";
	default:
	    return "ERROR";
	}
    }

    public static boolean
	collectArrayClass(String cname, components.ClassLoader classloader, boolean verbose)
    {
	// cname is the name of an array class
	// make sure it doesn't exist ( it won't if it came from a 
	// class constant ), and instantiate it. For CVM, do the same with
	// any sub-array types.
	//
	// make sure the base type (if a class) already exists:
	// under CVM, it cannot correctly represent an array
	// of an unresolved base class.

	boolean good = true;
	int lastBracket = cname.lastIndexOf('[');
	ClassInfo baseClassInfo = null;
	if ( lastBracket < cname.length()-2 ){
	    // it is a [[[[[Lsomething;
	    // isolate the something and see if its defined.
	    String baseClass = cname.substring(lastBracket+2, cname.length()-1);
	    baseClassInfo = ClassTable.lookupClass(baseClass, classloader);
	    if (baseClassInfo == null) {
		// base class not defined. punt this.
		if (verbose) {
		    System.out.println(Localizer.getString(
                        "javacodecompact.array_class_not_instantiated",
                        cname, baseClass));
		}
		return good;
	    }
	} else {
	    String baseClass = cname.substring(lastBracket+1, cname.length());
	    baseClassInfo =
                ClassTable.lookupPrimitiveClass(baseClass.charAt(0));
	    if (baseClassInfo == null) {
		// base class not defined. punt this.
                System.out.println(Localizer.getString(
                    "javacodecompact.array_class_not_instantiated",
                    cname, baseClass));
		if (verbose) {
		    System.out.println(Localizer.getString(
                        "javacodecompact.array_class_not_instantiated",
                        cname, baseClass));
		}
//		return false;
	    }
	}
	components.ClassLoader loader = baseClassInfo.loader;
	ClassConstant baseClassConstant = new ClassConstant(baseClassInfo);
	// enter sub-classes first
	for (int i = lastBracket; i >= 0; --i) {
	    String aname = cname.substring(i);
	    ClassInfo cinfo = ClassTable.lookupClass(aname, loader);
	    if (cinfo != null) {
                // this one exists. But subclasses may not, so keep going.
		continue;
	    }
	    try {
		ClassInfo newArray =
                    new ArrayClassInfo(verbose, aname, baseClassConstant);
		ClassTable.enterClass(newArray, loader);
	    } catch ( DataFormatException e ){
		e.printStackTrace();
		good = false;
		break; // out of do...while
	    }
	}
	return good;
    }

}
