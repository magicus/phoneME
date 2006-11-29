/*
 * @(#)FMIrefConstant.java	1.21 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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
import util.*;

/*
 * Represents an CONSTANT_Fieldref, CONSTANT_Methodref or
 * CONSTANT_InterfaceMethodref.
 */

public 
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

    FMIrefConstant(){ nSlots = 1;
		     }

    protected FMIrefConstant( int t, ClassConstant c, NameAndTypeConstant s ){
	tag = t;
	clas = c;
	sig = s;
	resolved = true;
	nSlots = 1;
    }
	

    public void read( DataInput in ) throws IOException {
	classIndex = in.readUnsignedShort();
	sigIndex = in.readUnsignedShort();
    }

    public void resolve( ConstantObject table[] ){
	if (resolved) return;
	sig = (NameAndTypeConstant)table[sigIndex];
	clas = (ClassConstant)table[classIndex];
	resolved = true;
    }

    public void write( DataOutput out) throws IOException {
	out.writeByte( tag );
	if ( resolved ){
	    out.writeShort( clas.index );
	    out.writeShort( sig.index );
	} else {
	    throw new DataFormatException(Localizer.getString("fmirefconstant.unresolved_fmirefconstant.dataformatexception"));
	    //out.writeShort( classIndex );
	    //out.writeShort( sigIndex );
	}
    }

    public String toString(){
	String t = (tag==Const.CONSTANT_FIELD)?/*NOI18N*/"FieldRef: ":
		   (tag==Const.CONSTANT_METHOD)?/*NOI18N*/"MethodRef: ":
		   /*NOI18N*/"InterfaceRef: ";
	if (resolved)
	    return t+clas.name.string+/*NOI18N*/" . "+sig.name.string+/*NOI18N*/" : "+sig.type.string;
	else
	    return t+/*NOI18N*/"[ "+classIndex+/*NOI18N*/" . "+sigIndex+/*NOI18N*/" ]";
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
	references++;
	// if this member reference is not resolved,
	// then the sig & class entries must also be in the
	// constant pool.
	if (!isResolved()){
	    sig.incReference();
	    clas.incReference();
	}
    }

    public void decReference() {
	references--;
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

    public ClassComponent find( boolean isMethod ){
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

        if (isMethod) {
	    while ( c != null ){
	        t = (ClassMemberInfo[])c.methods;
                m = findMember(t, thisID);
                if (m != null) {
                    return m;
                }
	        c = c.superClassInfo;
	    }
        } else {
            ClassInfo thisCl = c;

	    /* It's a field. Field resolution follows the lookup order specified
	     * in the VM spec. */

            /* 1. Search the field's class or interface, 'C' first. */
            t = (ClassMemberInfo[])c.fields;
            m = findMember(t, thisID);
            if (m != null) {
                return m;
            }

            /* 2. Otherwise, field lookup is applied recursively to the direct
             *    superinterfaces of 'C'. */
            for (int j = 0; j < thisCl.interfaces.length; j++) {
                c = (thisCl.interfaces[j]).find();
                if (c != null) {
                    t = (ClassMemberInfo[])c.fields;
                    m = findMember(t, thisID);
                    if (m != null) {
                        return m;
                    }
                }
            }

            /* 3. Otherwise, field lookup is applied recursively to the
             *    superclasses of 'C'. */
            c = thisCl.superClassInfo;
            while (c != null) {
                t = (ClassMemberInfo[])c.fields;
                m = findMember(t, thisID);
                if (m != null) {
                    return m;
                }
                c = c.superClassInfo;
            }

            /* 4. Otherwise, field lookup fails. */

        }

	return null;
    }

    /*
     * validate: see ConstantObject.validate. The clas and sig entries may
     * or may not end up in any constant pool. If they do it can be because
     * of a reference from this entry.
     */

    public boolean isResolved(){ return find( tag!=Const.CONSTANT_FIELD ) != null; }
}
