/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
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
    public String        baseName;
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
	    baseName = "Int";
	    baseClass = new ClassConstant(new UnicodeConstant("int"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_LONG:
	    baseType = Const.T_LONG;
	    baseName = "Long";
	    baseClass = new ClassConstant(new UnicodeConstant("long"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_FLOAT:
	    baseType = Const.T_FLOAT;
	    baseName = "Float";
	    baseClass = new ClassConstant(new UnicodeConstant("float"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_DOUBLE:
	    baseType = Const.T_DOUBLE;
	    baseName = "Double";
	    baseClass = new ClassConstant(new UnicodeConstant("double"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_BOOLEAN:
	    baseType = Const.T_BOOLEAN;
	    baseName = "Boolean";
	    baseClass = new ClassConstant(new UnicodeConstant("boolean"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_BYTE:
	    baseType = Const.T_BYTE;
	    baseName = "Byte";
	    baseClass = new ClassConstant(new UnicodeConstant("byte"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_CHAR:
	    baseType = Const.T_CHAR;
	    baseName = "Char";
	    baseClass = new ClassConstant(new UnicodeConstant("char"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_SHORT:
	    baseType = Const.T_SHORT;
	    baseName = "Short";
	    baseClass = new ClassConstant(new UnicodeConstant("short"));
	    elemClassAccess = Const.ACC_PUBLIC;
	    break;
	case Const.SIGC_CLASS:
	    baseType = Const.T_CLASS;	    
	    int start = ++index;
	    while (s.charAt(index) != Const.SIGC_ENDCLASS) {
		index++;
	    }

	    baseClass = new ClassConstant(
		new UnicodeConstant( s.substring( start, index ) ) 
	    );
	    ClassInfo baseClassInfo = baseClass.find();
	    if (baseClassInfo != null) {
		// The base class' access should propagate to all arrays with
		// that base class, so elemClassAccess really is equivalent
		// to the the access flags of the base class
		elemClassAccess = baseClassInfo.access;
		altNametable = baseClassInfo.altNametable;
	    } else {
		elemClassAccess = Const.ACC_PUBLIC;
	    }
	    break;

	default:
	    throw new DataFormatException(Localizer.getString("arrayclassinfo.malformed_array_type_string.dataformatexception",s));
	}
	// For CVM, we want to know the sub-class, which is different from
	// the base class.
	if ( depth > 1 ){
	    subarrayClass = new ClassConstant( new UnicodeConstant( s.substring(1) ) );
	} else if ( depth == 1 ) {
	    subarrayClass = baseClass;
	}
    }


    public ArrayClassInfo( boolean v, String s ) throws DataFormatException {
	super(v);
	constants = new ConstantObject[0];
	arrayClassNumber = nFake++;
	fillInTypeInfo( s );
	className = s;
	thisClass = new ClassConstant( new UnicodeConstant( s ) );
	superClassInfo = ClassTable.lookupClass("java/lang/Object", false);
	superClass = superClassInfo.thisClass;
	access = Const.ACC_FINAL|Const.ACC_ABSTRACT;
	access |= (elemClassAccess & Const.ACC_PUBLIC);
	methods = new MethodInfo[0];
	fields  = new FieldInfo[0];
    }

    protected String createGenericNativeName() { 
        return /*NOI18N*/"fakeArray" + arrayClassNumber;
    }
}
