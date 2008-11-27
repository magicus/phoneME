/*
 * @(#)NameAndTypeConstant.java	1.12 06/10/10
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
import util.Assert;
import util.DataFormatException;
// This class represents a CONSTANT_NameAndType

public
class NameAndTypeConstant extends ConstantObject {
    // Filled in by Clas.resolveConstants()
    public UnicodeConstant name;
    public UnicodeConstant type;

    // Read in from classfile
    int nameIndex;
    int typeIndex;

    // The unique ID for this name and type (from Std2ID)
    public int ID = 0;

    private NameAndTypeConstant(int ni, int ti) {
	super(Const.CONSTANT_NAMEANDTYPE);
	nameIndex = ni;
	typeIndex = ti;
    }

    public NameAndTypeConstant(UnicodeConstant name, UnicodeConstant type) {
	super(Const.CONSTANT_NAMEANDTYPE);
	this.name = name;
	this.type = type;
	isFlat = true;
    }

    /**
     * Factory method to construct a NameAndTypeConstant instance from the
     * constant pool data stream.  This method is only called from the
     * ConstantObject.readObject() factory.
     */
    static ConstantObject read(DataInput i) throws IOException {
	return new NameAndTypeConstant(i.readUnsignedShort(),
                                       i.readUnsignedShort());
    }

    public void flatten(ConstantPool cp) {
	if (isFlat) return;
        Assert.disallowClassloading();
        name = (UnicodeConstant)cp.elementAt(nameIndex);
	type = (UnicodeConstant)cp.elementAt(typeIndex);
	isFlat = true;
        Assert.allowClassloading();
    }

    public void write(DataOutput o) throws IOException {
	o.writeByte(tag);
	if (isFlat) {
	    o.writeShort(name.index);
	    o.writeShort(type.index);
	} else {
	    throw new DataFormatException("unresolved NameAndTypeConstant");
	    //o.writeShort(nameIndex);
	    //o.writeShort(typeIndex);
	}
    }

    public String toString() {
	if (isFlat) {
	    return "NameAndType: "+name.string+" : "+type.string;
	} else {
	    return "NameAndType[ "+nameIndex+" : "+typeIndex+" ]";
	}
    }

    public void incReference() {
	super.incReference();
	// don't want these to show up in the resulting symbol table.
	//name.incReference();
	//type.incReference();
    }

    public void decReference() {
	super.decReference();
	//name.decReference();
	//type.decReference();
    }
    /*
     * This method can only be called at the very end of processing,
     * just before output writing. Make sure the name and type constants
     * obey the validation constraints as described in ConstantObject.validate.
     * Call ConstantObject.validate to make sure its fields do, too.
     */
    public void
    validate(){
	super.validate();
	name.validate();
	type.validate();
    }


    public int hashCode() {
	return tag + name.string.hashCode() + type.string.hashCode();
    }
    

    public boolean equals(Object o) {
	if (o instanceof NameAndTypeConstant) {
	    NameAndTypeConstant n = (NameAndTypeConstant) o;
	    return name.string.equals(n.name.string) &&
		   type.string.equals(n.type.string);
	} else {
	    return false;
	}
    }

    public boolean isResolved(){ return true; }
}
