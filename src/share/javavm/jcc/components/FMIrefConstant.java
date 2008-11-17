/*
 * @(#)FMIrefConstant.java	1.21 06/10/10
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

package components;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import consts.Const;
import jcc.Str2ID;
import util.Assert;
import util.*;

/**
 * An abstract class to represent CP constants which define class members.
 * This class will serve as a common super class for the classes which will
 * contain the CP constants: CONSTANT_Fieldref, CONSTANT_Methodref, and
 * CONSTANT_InterfaceMethodref.
 */
public abstract
class FMIrefConstant extends ConstantObject
{
    // These fields are filled in by Clas.resolveConstant().
    public NameAndTypeConstant sig;
    public ClassConstant clas;

    boolean	computedID;
    int		ID;

    // These fields are read from the class file
    public int classIndex;
    public int sigIndex;

    FMIrefConstant(int tag) { super(tag); }

    protected FMIrefConstant(int tag, ClassConstant c, NameAndTypeConstant s) {
	super(tag);
	clas = c;
	sig = s;
	isFlat = true;
    }
	
    /**
     * Read in the constant pool data for this constant member object.
     * This method should only be called by the read() factory methods of
     * its concrete subclasses.
     */
    protected void readIndexes(DataInput in) throws IOException {
	classIndex = in.readUnsignedShort();
	sigIndex = in.readUnsignedShort();
    }

    public void flatten(ConstantPool cp) {
	if (isFlat) return;
        Assert.disallowClassloading();
	sig = (NameAndTypeConstant)cp.elementAt(sigIndex);
	clas = (ClassConstant)cp.elementAt(classIndex);
	isFlat = true;
        Assert.allowClassloading();
    }

    public void write(DataOutput out) throws IOException {
	out.writeByte(tag);
	if (isFlat) {
	    out.writeShort(clas.index);
	    out.writeShort(sig.index);
	} else {
	    throw new DataFormatException(Localizer.getString(
                "fmirefconstant.unresolved_fmirefconstant.dataformatexception"));
	    //out.writeShort( classIndex );
	    //out.writeShort( sigIndex );
	}
    }

    public String toString(){
	String t = (tag==Const.CONSTANT_FIELD)?/*NOI18N*/"FieldRef: ":
		   (tag==Const.CONSTANT_METHOD)?/*NOI18N*/"MethodRef: ":
		   /*NOI18N*/"InterfaceRef: ";
	if (isFlat)
	    return t+clas.name.string+/*NOI18N*/" . "+
                   sig.name.string+/*NOI18N*/" : "+sig.type.string;
	else
	    return t+/*NOI18N*/"[ "+classIndex+/*NOI18N*/" . "+
                   sigIndex+/*NOI18N*/" ]";
    }

    public String prettyPrint(){
	switch (tag){
	case Const.CONSTANT_FIELD:
	    return Localizer.getString("classclass.field_with_class",
		    clas.name, sig.name);
	case Const.CONSTANT_METHOD:
	case Const.CONSTANT_INTERFACEMETHOD:
	    return Localizer.getString("classclass.method_with_class",
		    clas.name, sig.name, sig.type);
	default:
	    return toString();
	}
    }

    public void incReference() {
	super.incReference();
	// if this member reference is not resolved,
	// then the sig & class entries must also be in the
	// constant pool.
	if (!isResolved()){
	    sig.incReference();
	    clas.incReference();
	}
    }

    public void decReference() {
	super.decReference();
	sig.decReference();
	clas.decReference();
    }

    public int hashCode() {
	return tag + sig.hashCode() + clas.hashCode();
    }

    public boolean equals(Object o) {
	if (o instanceof FMIrefConstant) {
	    FMIrefConstant f = (FMIrefConstant) o;
	    return tag == f.tag && clas.name.equals(f.clas.name) &&
		sig.name.equals(f.sig.name) && sig.type.equals(f.sig.type);
	} else {
	    return false;
	}
    }

    public int getID(){
	if ( ! computedID ){
	    ID = Str2ID.sigHash.getID( sig.name, sig.type );
	    computedID = true;
	}
	return ID;
    }

    private ClassComponent findMember(ClassMemberInfo[] t, int thisID) {
        for (int i = 0; i < t.length; i++) {
	    if (thisID == t[i].getID()) {
                return t[i];
            }
        }
        return null;
    }

    // Recursively looks up a method (specified by thisID) in the interfaces
    // that class c implements and their superinterfaces.
    private ClassComponent findMethodInSuperInterfaces(ClassInfo c, int thisID) {
        ClassComponent m;
        ClassMemberInfo t[];

	// Fail if no interfaces to check:
	if (c.interfaces == null) {
	    return null;
	}

	// Search direct superinterfaces:
	for (int j = 0; j < c.interfaces.length; j++) {
	    ClassInfo itf = (c.interfaces[j]).find();
	    if (itf != null) {
		t = (ClassMemberInfo[])itf.methods;
		m = findMember(t, thisID);
		if (m != null) {
		    return m;
		} else {
		    // Search superinterface's superface:
		    m = findMethodInSuperInterfaces(itf, thisID);
		    if (m != null) {
			return m;
		    }
		}
	    }
	}
	return null;
    }

    // Recursively looks up a field (specified by thisID) in the interfaces
    // that class c implements and their superinterfaces.
    private ClassComponent findFieldInSuperInterfaces(ClassInfo c, int thisID) {
        ClassComponent m;
        ClassMemberInfo t[];

	// Fail if no interfaces to check:
	if (c.interfaces == null) {
	    return null;
	}

	// Search direct superinterfaces:
	for (int j = 0; j < c.interfaces.length; j++) {
	    ClassInfo itf = (c.interfaces[j]).find();
	    if (itf != null) {
		t = (ClassMemberInfo[])itf.fields;
		m = findMember(t, thisID);
		if (m != null) {
		    return m;
		} else {
		    // Search superinterface's superface:
		    m = findFieldInSuperInterfaces(itf, thisID);
		    if (m != null) {
			return m;
		    }
		}
	    }
	}
	return null;
    }

    public ClassComponent find (int tag) {
	if ( ! computedID ){
	    ID = Str2ID.sigHash.getID( sig.name, sig.type );
	    computedID = true;
	}

        ClassComponent m;
	ClassInfo c = clas.find();
        int thisID = this.getID();
        ClassMemberInfo t[];

	/* If the class itself is unresolved, all members are too */
	if (c == null){
	    return null;
	}

        if (tag == Const.CONSTANT_METHOD) {
	    ClassInfo origClass = c;

	    // Verify some assertions:
	    if (c.isInterface()) {
		System.err.println(Localizer.getString(
		    "fmirefconstant.class_should_be_a_class", c));
		return null;
	    }

	    // Method resolution follows the lookup order specified in the
	    // VM spec ($5.4.3.3).
	    //
	    // 1. Look up method in the current class.
	    // 2. Look up method in the superclasses recursively.
	    // 3. If not found, look up method in superinterfaces
	    //    recursively.

	    // 1. Lookup method in the current class, and
	    // 2. Lookup method in the superclasses:
	    while (c != null) {
		// First, search for the method in the current class:
		t = (ClassMemberInfo[])c.methods;
		m = findMember(t, thisID);
		if (m != null) {
		    return m;
		}

		// If not found, try again with the superclass:
		c = c.superClassInfo;
	    }

	    // 3. Lookup method in the superinterfaces:
	    c = origClass;
	    while (c != null) {
		// First, search for the method in the implemented interfaces
		// and their superinterfaces:
		m = findMethodInSuperInterfaces(c, thisID);
		if (m != null) {
		    return m;
		}

		// If not found, repeat for interface methods in superclass:
	        c = c.superClassInfo;
	    }

            // 4. If still not found, method/interfacemethod lookup fails.

        } else if (tag == Const.CONSTANT_INTERFACEMETHOD) {

	    ClassInfo origClass = c;

	    // Verify some assertions:
	    if (!c.isInterface()) {
		System.err.println(Localizer.getString(
		    "fmirefconstant.class_should_be_an_interface", c));
		return null;
	    }

	    // NOTE: In the case of Const.CONSTANT_INTERFACEMETHOD, then
	    // there can only be one superclass i.e. java.lang.Object.
	    ClassInfo cSuper = c.superClassInfo;
	    if (!cSuper.isJavaLangObject()) {
		System.err.println(Localizer.getString(
                    "fmirefconstant.interface_super_should_be_java_lang_object",
		    c, cSuper));
		return null;
	    }	

	    // InterfaceMethod resolution follows the lookup order specifed
	    // in the VM spec ($5.4.3.4).
	    //
	    // 1. Look up method in the current interface.
	    // 2. If not found, look up method in superclass
	    //    java.lang.Object (including its interfaces).

	    // 1. Lookup method in the current class:
	    t = (ClassMemberInfo[])c.methods;
	    m = findMember(t, thisID);
	    if (m != null) {
		return m;
	    }

	    //    Lookup method in the implemented interfaces and their
	    //    superinterfaces:
	    m = findMethodInSuperInterfaces(c, thisID);
	    if (m != null) {
		return m;
	    }

	    // 2. Lookup method in the super class:
	    t = (ClassMemberInfo[])cSuper.methods;
	    m = findMember(t, thisID);
	    if (m != null) {
		return m;
	    }

	    //    Lookup method in the implemented interfaces and their
	    //    superinterfaces:
	    m = findMethodInSuperInterfaces(cSuper, thisID);
	    if (m != null) {
		return m;
	    }

            // 3. If still not found, method/interfacemethod lookup fails.

        } else {
	    // tag should be Const.CONSTANT_FIELD.

	    // Field resolution follows the lookup order specified in the
	    // VM spec ($5.4.3.2).
	    //
	    // 1. Look up field in current class.
	    // 2. If not found, look up field in implemented interfaces
	    //    and their super-interfaces.
	    // 3. If not found, recurse into the superclass of the current
	    //    class and repeat steps 1, 2, and 3 as necessary.

	    while (c != null) {

		// 1. Lookup field in the current class:
		t = (ClassMemberInfo[])c.fields;
		m = findMember(t, thisID);
		if (m != null) {
		    return m;
		}

		// 2. Lookup field in implemented interfaces and recursively
		//    in their implemented superinterfaces:
		m = findFieldInSuperInterfaces(c, thisID);
		if (m != null) {
		    return m;
		}

		// 3. Otherwise, repeat with superclass: */
		c = c.superClassInfo;
	    }

            // 4. If still not found, field lookup fails.
        }

	return null;
    }

    /*
     * validate: see ConstantObject.validate. The clas and sig entries may
     * or may not end up in any constant pool. If they do it can be because
     * of a reference from this entry.
     */

    public boolean isResolved(){ return find(tag) != null; }
}
