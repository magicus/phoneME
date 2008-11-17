/*
 * @(#)CVMClass.java	1.34 06/10/22
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
/*
 * CVM-specific internal representation of
 * a class. Target-machine independent.
 * There is a references from each instance of components.ClassInfo to 
 * one of these, and a reference back as well.
 * Derived from the JDK-specific ClassClass
 *
 * !!See also CVMVM for VM-specific info not associated directly with a class.
 */
import components.*;
import util.*;
import consts.Const;
import consts.CVMConst;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;
import java.util.StringTokenizer;

public class
CVMClass extends ClassClass implements Const, CVMConst, CVMTypeCode {
    public CVMMethodInfo	methods[];
    private String		myNativeName;
    protected int		typeCode = 0;
    private int                 classId  = CVM_T_ERROR;

    public int			nStaticWords;
    public int			nStaticRef;
    public FieldInfo		statics[];

    //
    // Most methods are compressible, which makes their enclosing class
    // compressible
    //
    private boolean             isCompressible = true;
    
    public CVMClass( ClassInfo c ){
	classInfo = c;
	c.vmClass = this;

	if ( c.methods != null ){
	    // per-method, CVM-specific info.
	    methods = new CVMMethodInfo[ c.methods.length ];
	    for( int i = 0; i < methods.length; i++ ){
		components.MethodInfo m = c.methods[i];
		methods[i] = new CVMMethodInfo( m );
		if (!hasStaticInitializer &&
		    m.isStaticMember() &&
		    m.name.string.equals(/*NOI18N*/"<clinit>")) {
		    hasStaticInitializer = true;
		}
	    }
	}
    }

    public void setCompressible(boolean isCompressible) {
	this.isCompressible = isCompressible;
    }
    
    public boolean isCompressible() {
	return this.isCompressible;
    }
    
    public String getNativeName(){
	if ( myNativeName == null ){
	    if (classInfo instanceof ArrayClassInfo) {
		ArrayClassInfo aci = (ArrayClassInfo)classInfo;
		if (aci.depth == 1) {
		    /*
		     * There are some special arrays of well-known basic
		     * types here.
		     */
		    if (aci.baseType != Const.T_CLASS) {
			myNativeName = "manufacturedArrayOf"+
			    aci.getNativeName();
		    } else if (aci.baseClass.find().superClassInfo == null) {
			myNativeName = "manufacturedArrayOfObject";
		    } else {
		        myNativeName = "manufacturedArrayClass"+
			    aci.arrayClassNumber;
		    }
		} else {
		    myNativeName = "manufacturedArrayClass"+
			aci.arrayClassNumber;
		}
	    } else {
		myNativeName = classInfo.getGenericNativeName();
	    }
	}
	return myNativeName;
    }

    public int
    CVMflags(){
	int flagval = 0;
	int acc = classInfo.access;
	if ((acc & ACC_PUBLIC)    != 0) flagval |= CVM_CLASS_ACC_PUBLIC;
	if ((acc & ACC_FINAL)     != 0) flagval |= CVM_CLASS_ACC_FINAL;
	if ((acc & ACC_SUPER)     != 0) flagval |= CVM_CLASS_ACC_SUPER;
	if ((acc & ACC_INTERFACE) != 0) flagval |= CVM_CLASS_ACC_INTERFACE;
	if ((acc & ACC_ABSTRACT)  != 0) flagval |= CVM_CLASS_ACC_ABSTRACT;
	if (isPrimitiveClass()) {
	    flagval |= CVM_CLASS_ACC_PRIMITIVE;
        }
	if (classInfo.hasNonTrivialFinalizer) {
            flagval |= CVM_CLASS_ACC_FINALIZABLE;
        }
	if (classInfo.isReference()) {
            flagval |= CVM_CLASS_ACC_REFERENCE;
        }
	return flagval;

    }

    public int
    typecode(){
	return typeCode;
    }

    // Accessor methods for the classId field:
    public int getClassId() {
	return classId;
    }
    protected void setClassId(int id) {
	classId = id;
    }

    public void orderStatics(){
	// count statics.
	// arranged them ref-first
	// do not assign offsets.
	FieldInfo f[] = classInfo.fields;
	nStaticWords = 0;
	if ( (f == null) || (f.length == 0 ) ) return; // nothing to do.
	int nFields = f.length;
	int nStatic = 0;
	int nRef = 0;
	for ( int i = 0; i < nFields; i++ ){
	    FieldInfo fld = f[i];
	    if ( fld.isStaticMember() ){
		nStatic += 1;
		char toptype =  fld.type.string.charAt(0);
		if ( (toptype == 'L') || (toptype=='['))
		    nRef += 1;
	    }
	}
	int refOff     = 0;
	int scalarOff = nRef;
	int totalStaticWords = nRef;
	statics = new FieldInfo[ nStatic ];
	for ( int i = 0; i < nFields; i++ ){
	    FieldInfo fld = f[i];
	    if ( fld.isStaticMember() ){
		char toptype =  fld.type.string.charAt(0);
		if ( (toptype == 'L') || (toptype=='[')){
		    statics[refOff] = fld;
		    refOff += 1;
		} else {
		    statics[scalarOff] = fld;
		    scalarOff += 1;
		    totalStaticWords += fld.nSlots;
		}
	    }
	}
	nStaticWords = totalStaticWords;
	nStaticRef   = nRef;
    }

    /* Inline all the methods in this code object */
    public void getInlining() { 
        for( int i = 0; i < methods.length; i++ ){
	    methods[i].getInlining();
	}
    }

    public boolean
    adjustSymbolicConstants(UnresolvedReferenceList missingReferences){
	ConstantPool cp = classInfo.getConstantPool();

	if (!isPartiallyResolved(cp)) {
	    return true;
	}
	makeResolvable(cp, missingReferences, classInfo.className);
	impureConstants = true;
	return false;
    }

    public static void
    makeResolvable(
	ConstantPool cp,
	UnresolvedReferenceList missingReferences,
	String source)
    {
	//
	// This is easy.
	// There is no change to the structure of the constant pool.
	// Mark the pool as impure and emit a bunch of warning
	// messages. We don't need to add utf8's to the pool!
	//
	ConstantObject consts[] = cp.getConstants();
	int nconst = cp.getLength();
        for( int i = 1; i < nconst; i += consts[i].nSlots ){
            ConstantObject o;
	    String className;
            o = consts[i];
	    if ( o.isResolved() ){
		continue;
	    }
            switch( o.tag ){
            case Const.CONSTANT_CLASS:
                ClassConstant co = (ClassConstant)o;
                className = co.name.string;
		missingReferences.add(o, source);
                continue;
            case Const.CONSTANT_FIELD:
            case Const.CONSTANT_METHOD:
            case Const.CONSTANT_INTERFACEMETHOD:
                FMIrefConstant fo = (FMIrefConstant)o;
                if ( fo.clas.isResolved() ){
                    // This is a missing member of a resolved class.
                    // Print this out
                    // To print all the references to a totally missing
                    // Add it to the list.
		    missingReferences.add(o, source);

		}
		/* else ...
		 * we will also stumble across the unresolved ClassConstant
		 * object and will add it to the list in the above case.
		 */

		/*
		 * If this memberRef constant is in the shared pool,
		 * the the constants it references had better be as well.
		 * If it is not, then they had better not be either.
		 */
		if (fo.shared){
		    ClassConstant cc = fo.clas;
		    NameAndTypeConstant sig = fo.sig;
		    if (cc.shared){
			// This is not likely at all.
			if (cc.containingPool != fo.containingPool){
			    throw new ValidationException(
				"MemberRef and Class in different pools "+
				fo);
			}
		    } else {
			// oops. Need to make sure I can reference a shared
			// class constant.
			cc = (ClassConstant)fo.containingPool.lookup(cc);
			if (cc == null){
			    throw new ValidationException(
				"Shared MemberRef referenced Class not in shared pool "+fo);
			}
			fo.clas = cc;
		    }
		    if (sig.shared){
			// This is not likely at all.
			if (sig.containingPool != fo.containingPool){
			    throw new ValidationException(
				"MemberRef and NameAndType in different pools "+
				fo);
			}
		    } else {
			// oops. Need to make sure I can reference a shared
			// NameAndType constant.
			sig = (NameAndTypeConstant)fo.containingPool.lookup(sig);
			if (sig == null){
			    throw new ValidationException(
				"Shared MemberRef referenced NameAndType not in shared pool "+fo);
			}
			fo.sig = sig;
		    }
		}else if (fo.clas.shared || fo.sig.shared){
		    throw new ValidationException(
			"Shared clas/sig referenced from unshared memberRef "+
			fo);
		}
                continue;
            }
        }
    }

    /* 
     * Moved globals to CVMROMGlobals.
     * This requires that we can identify classes that contain writeable
     * methods.
     * The code to decide about writeability is copied from 
     * CVMWriter.writeMethods().
     */
    public boolean hasWritableMethods() {
	for (int i = 0; i < methods.length; i++) {
	    if (methods[i].codeHasJsr())
		return true;
	}
    	
	return false;
    }
}
