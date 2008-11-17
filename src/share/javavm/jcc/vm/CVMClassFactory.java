/*
 * @(#)CVMClassFactory.java	1.22 06/10/22
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
import components.ClassInfo;
import components.ClassTable;
import components.ConstantPool;
import components.UnicodeConstant;
import runtime.CVMWriter;
import util.Localizer;
import util.UnresolvedReferenceList;
import java.util.Comparator;
import java.util.Hashtable;

public class CVMClassFactory implements VMClassFactory, Comparator {
    
    static String primitiveTypeName[] = {
	"void", "boolean", "char", "byte",
	"short", "int", "long", "float",
	"double"
    };

    /*
     * STATIC INITIALIZER:
     * MAKE SURE THE PRIMITIVE TYPES GET SMALL TYPEID NUMBERS
     * DO THIS BEFORE CONSTRUCTION OF ANY CVMClass!
     */
    static{
	for ( int i = 0 ; i < primitiveTypeName.length; i++ ){
	    CVMDataType.lookupClassname( primitiveTypeName[i] );
	}
    }
    
    public ClassClass newVMClass( ClassInfo c ){
	return new CVMClass( c );
    }

    private static void
    setType( String name, int value ){
	ClassInfo ci = ClassTable.lookupClass(name);
	ClassClass cc;
	if ( (ci == null) || ( ( cc = ci.vmClass ) == null) ){
	    throw new Error("Lookup failed for primitive class "+name);
	}
	((CVMClass)cc).typeCode = value;
    }

    /** 
     * This method is responsible for assigning the classIDs (i.e. class
     * typeID) for all the classes that have been "loaded" in the system.
     * The value of the typeid to be assigned is attained from CVMDataType.
     */
    private static void
    setAllClassID(){
	// null had better work here!
	// we'll find out soon enough.
	ClassClass allClasses[] = ClassClass.getClassVector( null );
	int n = allClasses.length;
	for ( int i = 0; i < n; i++ ){
	    CVMClass cvmc = (CVMClass) allClasses[i];
            UnicodeConstant className = cvmc.classInfo.thisClass.name;
            String utfName = CVMWriter.getUTF(className);
	    int classId = CVMDataType.lookupClassname(utfName);
            cvmc.setClassId(classId);
	}
    }

    public void
    setTypes(){
	setType( "void", Const.T_VOID );
	setType( "boolean", Const.T_BOOLEAN );
	setType( "char", Const.T_CHAR );
	setType( "byte", Const.T_BYTE );
	setType( "short", Const.T_SHORT );
	setType( "int", Const.T_INT );
	setType( "long", Const.T_LONG );
	setType( "float", Const.T_FLOAT );
	setType( "double", Const.T_DOUBLE );
	setAllClassID();
    }

    public ConstantPool 
    makeResolvable(
	ConstantPool cp,
	UnresolvedReferenceList missingRefs,
	String source)
    {
	//
	// For the CVM, this is actually pretty trivial stuff...
        //
	CVMClass.makeResolvable(cp, missingRefs, source);
	return cp;
    }

    /*
     * Compare two CVMClass objects by classid
     */
    public int
    compare( Object o1, Object o2 ){
	// just do the casts.
	// unacceptable comparison results in
	// classcast exception.
	CVMClass c1 = (CVMClass)o1;
	CVMClass c2 = (CVMClass)o2;
        int classId1 = c1.getClassId();
        int classId2 = c2.getClassId();

        // NOTE: This comparison method is used in the sorting of CVM Class
        // instances in CVM_ROMClasses[].  The Class instances of the Class
        // and Array types are sorted in incremental order of typeid values.
        // The only exception is that deep array types (i.e. array types with
        // depth >= 7) will be sorted based only on the basetype field of
        // their typeids (i.e. the depth field is ignored ... hence the
        // masking operations below).  This makes it possible to access Class
        // instances for Class and Deep array types in CVM_ROMClasses[] by
        // indexing.
        if ((classId1 & CVMTypeCode.CVMtypeArrayMask) == CVMTypeCode.CVMtypeBigArray) {
            classId1 &= CVMTypeCode.CVMtypeBasetypeMask;
        }
        if ((classId2 & CVMTypeCode.CVMtypeArrayMask) == CVMTypeCode.CVMtypeBigArray) {
            classId2 &= CVMTypeCode.CVMtypeBasetypeMask;
        }
        return classId1-classId2; // should turn into -1, 0, 1. oops.
    }
}
