/*
 * @(#)ConstantObject.java	1.15 06/10/10
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
import java.io.IOException;
import consts.Const;
import util.DataFormatException;
import util.ValidationException;

/*
 * An abstract class for representing objects in the constant pools
 * All constants are derived from here.
 * Note how this and all its derivatives are Cloneable.
 */

public
abstract class ConstantObject extends ClassComponent implements Cloneable
{
    private int references = 0;
    private int ldcReferences = 0;
    public boolean shared = false;
    public ConstantPool containingPool; // if shared == true, then container.
    public ClassInfo    containingClass; // at least initially. Not good if shared.

    public int index;

    // The type of constant pool entry.
    public int tag;

    // The number of slots this thing "uses" -- usually 1, sometimes 2
    public int nSlots = 1;

    public ConstantObject(int tag) {
        this.tag = tag;
    }
    
    public void incLdcReference() {
        ldcReferences++;
    }

    public void decLdcReference() {
        ldcReferences--;
    }
 
    public void clearLdcReference() {
        ldcReferences = 0;
    }

    public int getLdcReferences() {
        return ldcReferences;
    }

    public void setLdcReferences(int value) {
        ldcReferences = value;
    }

    // Some items are reference counted so that the most frequently
    // accessed ones can be placed in the first 256 entries of the
    // constant table.  Others are just marked as used so we know
    // to include them somewhere in the table.
    public void incReference() {
	references++;
    }

    public void decReference() {
	references--;
    }

    public void clearReference(){
	references = 0;
    }

    public int getReferences() {
        return references;
    }

    public void setReferences(int value) {
        references = value;
    }

    public Object clone(){
	ConstantObject newC;
	try {
	    newC = (ConstantObject)super.clone();
	    newC.references = 0;
	    newC.shared     = false;
	    newC.index      = 0;
	    return newC;
	} catch( CloneNotSupportedException e ){
	    e.printStackTrace();
	    System.exit(1);
	}
	return null;
    }

    /*
     * This method can only be called at the very end of processing,
     * just before output writing. At this stage, all constants
     * must:
     * a) have some references
     * b) have a valid constant-pool index.
     * c) actually be found in a constant pool at the given index.
     * Subtypes of ConstantObject may have additional constraints:
     * overload this method to implement those, then call super.validate
     * to get here.
     * If any constraint is violated, throw a ValidationException.
     * This should stop processing and print a backtrace to the offender.
     */
    public void validate(){
	if ((references == 0) && (ldcReferences == 0)){
	    throw new ValidationException("Unreferenced constant", this);
	}
	if (shared){
	    int upperBound = containingPool.n;
	    if (this.index <= 0 || this.index >= upperBound ){
		throw new ValidationException("Constant index out of range", this);
	    }
	    if (this != containingPool.enumedEntries.elementAt(this.index)){
		throw new ValidationException("Constant index incorrect", this);
	    }
	} else {
	    ConstantPool cp = containingClass.getConstantPool();
	    int upperBound = cp.getLength();
	    if (this.index <= 0 || this.index >= upperBound ){
		throw new ValidationException("Constant index out of range", this);
	    }
	    if (this != cp.elementAt(this.index)){
		throw new ValidationException("Constant index incorrect", this);
	    }
	}
    }

    /*
     * This is to be called by the writer.
     * It returns the index and (when debugging) calls validate().
     */
    public int getIndex(){
	// validate();
	return index;
    }

    public abstract boolean isResolved();


    /* By default, "prettyPrint" is the same as ugly */
    /* You need to override this for reasonable messages */
    public String prettyPrint(){
	return this.toString();
    }

    /**
     * Factory method for creating ConstantObjects from the classfile
     * constant pool entries data stream.
     */
    static public ConstantObject readObject(DataInput in) throws IOException {
	// read the tag and dispatch accordingly
	int tag = in.readUnsignedByte();
	switch(tag) {
	case Const.CONSTANT_UTF8:
	    return UnicodeConstant.read(in);
	case Const.CONSTANT_INTEGER:
	case Const.CONSTANT_FLOAT:
	    return SingleValueConstant.read(tag, in);
	case Const.CONSTANT_DOUBLE:
	case Const.CONSTANT_LONG:
	    return DoubleValueConstant.read(tag, in);
	case Const.CONSTANT_STRING:
	    return StringConstant.read(in);
	case Const.CONSTANT_NAMEANDTYPE:
	    return NameAndTypeConstant.read(in);
	case Const.CONSTANT_CLASS:
	    return ClassConstant.read(in);
	case Const.CONSTANT_FIELD:
	    return FieldConstant.read(in);
	case Const.CONSTANT_METHOD:
	    return MethodConstant.read(in);
	case Const.CONSTANT_INTERFACEMETHOD:
	    return InterfaceMethodConstant.read(in);
	default:
	    throw new DataFormatException("Format error (constant tag "+tag+" )");
	}
    }
}
