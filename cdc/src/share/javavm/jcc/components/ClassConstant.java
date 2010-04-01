/*
 * @(#)ClassConstant.java	1.15 06/10/21
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
import util.Assert;
import util.DataFormatException;
import util.Localizer;
import consts.Const;

/**
 * Represents a CONSTANT_Class stored in a constant pool.
 */
public
class ClassConstant extends ConstantObject
{
    public UnicodeConstant name;
    
    private boolean	 didLookup;
    private ClassInfo	 theClass;

    public int nameIndex;

    private ClassConstant(int nameIdx) { 
	super(Const.CONSTANT_CLASS);
	this.nameIndex = nameIdx;
    }

    public ClassConstant(UnicodeConstant n) {
        super(Const.CONSTANT_CLASS);
	name = n;

        // NOTE: This class constant is already flat because it has a direct
        // reference to the UTF string that defines it.
        isFlat = true;
    }

    public ClassConstant(ClassInfo ci) {
        super(Const.CONSTANT_CLASS);
	theClass = ci;
	name = new UnicodeConstant(ci.className);

        // NOTE: This class constant is already flat because it has a direct
        // reference to the UTF string that defines it.
        isFlat = true;
	didLookup = true;
    }

    /**
     * Factory method to construct a ClassConstant instance from the
     * constant pool data stream.  This method is only called from the
     * ConstantObject.readObject() factory.
     */
    static ConstantObject read(DataInput in) throws IOException {
	return new ClassConstant(in.readUnsignedShort());
    }

    public void flatten(ConstantPool cp) {
	if (isFlat) return;
        Assert.disallowClassloading();
	name = (UnicodeConstant)cp.elementAt(nameIndex);
	isFlat = true;
        Assert.allowClassloading();
    }

    // Write out reference to the ClassClass data structure
    public void write(DataOutput o) throws IOException {
	o.writeByte(tag);
	if (!isFlat) {
	    throw new DataFormatException("unresolved ClassConstant");
        }
	int x = name.index;
	if (x == 0) {
	    throw new DataFormatException("0 name index for "+name.string);
        }
	o.writeShort(x);
    }

    public String toString() {
	if (isFlat) {
	    return "Class: "+name.toString();
        } else {
	    return "Class: ["+nameIndex+"]";
        }
    }


    public String prettyPrint() {
	return Localizer.getString("classclass.class", name);
    }

    public int hashCode() {
	return tag + name.hashCode();
    }

    public void incReference() {
        super.incReference();
	//name.incReference();
    }

    public void decReference() {
        super.decReference();
	//name.decReference();
    }

    public boolean equals(Object o) {
	if (o instanceof ClassConstant) {
	    ClassConstant c = (ClassConstant) o;
	    return name.string.equals(c.name.string);
	} else {
	    return false;
	}
    }

    public ClassInfo find(){
        Assert.disallowClassloading();
	if ( !didLookup ){
	    ClassLoader loader = null;
	    if (containingClass != null) {
		loader = containingClass.loader;
	    }
	    theClass = ClassTable.lookupClass(name.string, loader);
	    didLookup = true;
	}
        Assert.allowClassloading();
	return theClass; // which may be null
    }

    public void forget(){
	didLookup = false;
	theClass = null;
    }

    /*
     * This method can only be called at the very end of processing,
     * just before output writing. Make sure the name constant
     * obeys the validation constraints as described in ConstantObject.validate.
     * Call ConstantObject.validate to make sure its fields do, too.
     */
    public void
    validate(){
	super.validate();
	name.validate();
    }

    public boolean isResolved(){ return find() != null; }
}
