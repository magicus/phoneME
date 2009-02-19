/*
 * @(#)InnerClassAttribute.java	1.13 06/10/10
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
import java.util.Enumeration;
import java.util.Vector;

/*
 * A class to represent the innerclass attribute of a class. The innerclass
 * attribute is new in 1.2 and is documented in the 1.2 JVM Spec.
 */
public
class InnerClassAttribute extends Attribute {
//__________ CLASS METHODS
    public static Attribute
    readAttribute(DataInput in, int len,
                  UnicodeConstant name,
                  ConstantObject constants[]) throws IOException {
	return finishReadAttribute(in, len, name, constants);
    }

    public static Attribute
    finishReadAttribute(DataInput in, int len,
                        UnicodeConstant name,
                        ConstantObject constants[]) throws IOException {
	int num        = in.readUnsignedShort();
	Vector entries = new Vector(num);
	int i, next, anonNum;
	ClassConstant innerInfo;
	ClassConstant outerInfo;
	UnicodeConstant innerName;

	for (i=0, anonNum=1; i < num; i++) {
	    innerInfo = (ClassConstant)constants[in.readUnsignedShort()];
	    outerInfo = (ClassConstant)constants[in.readUnsignedShort()];

	    next = in.readUnsignedShort();
	    if (next == 0) { // A zero value means this is an anonymous inner class.
		// Note: We are guessing at the name of the anonymous class.
		// But the guess will only be the correct name if the ordering
		// of the inner class attributes is the same as the ordering
		// of the anonymous inner classes as they appear in the source
		// file. And that is totally dependent on how the javac compiler
		// wants to do it. There is nothing in the JVM spec that says it
		// has to be that way. This won't hurt anything since we are just
		// replacing an unused name field with a guess. The JVM doesn't
		// care what is in this field.
		innerName = new UnicodeConstant((new Integer(anonNum++)).toString());
	    } else {
		innerName = (UnicodeConstant)constants[next];
	    }
	    
	    entries.addElement(new InnerClassEntry(innerInfo,
	                                           outerInfo,
	                                           innerName,
	                                           in.readUnsignedShort()));
//	    entries.add(new InnerClassEntry(innerInfo,
//	                                    outerInfo,
//	                                    innerName,
//	                                    in.readUnsignedShort()));
	}
		
	return new InnerClassAttribute(name, len, entries);
    }

//__________ INSTANCE VARIABLES
    private Vector myInnerClassesVector;
    private InnerClassEntry myInnerClassesArray[];

//__________ CONSTRUCTORS
    public
    InnerClassAttribute(UnicodeConstant name,
                        int len,
                        Vector entries) {
	super(name, len);
	myInnerClassesVector = entries;

	// Save an array copy because some methods use an index value
	// to find individual entries. But don't forget to update this
	// array if the vector changes, like in the removeEntry method.
	myInnerClassesArray = new InnerClassEntry[myInnerClassesVector.size()];
//	myInnerClassesVector.toArray(myInnerClassesArray);
	myInnerClassesVector.copyInto(myInnerClassesArray);
    }

//__________ INSTANCE METHODS
    protected int
    writeData(DataOutput o) throws IOException {
	int numberOfInnerClasses = myInnerClassesVector.size();
	o.writeShort(numberOfInnerClasses);
	
	Enumeration e = myInnerClassesVector.elements();
	while (e.hasMoreElements()) {
	    ((InnerClassEntry)e.nextElement()).write(o);
	}
		
	//return total number of bytes written, which is equal to
	//2 (the number of bytes of the numberOfInnerClasses local
	//variable) + the size of the InnerClassEntry * the number
	//of inner classes.
	return 2 + InnerClassEntry.SIZE * numberOfInnerClasses;
    }


    public void
    mergeConstantsIntoSharedPool(ConstantPool sharedCP)
    {
	InnerClassEntry ice;
	Enumeration e = myInnerClassesVector.elements();
	while (e.hasMoreElements()) {
	    ice = (InnerClassEntry)e.nextElement();

	    if (ice.innerInfo != null) {
		ice.innerInfo = (ClassConstant)sharedCP.add(ice.innerInfo);
	    }
	    if (ice.outerInfo != null) {
		ice.outerInfo = (ClassConstant)sharedCP.add(ice.outerInfo);
	    }
	}

	// Don't forget to update the array copy now that we've changed
	// the vector.
	myInnerClassesVector.copyInto(myInnerClassesArray);
    }

    /*
     * I am unsure exactly how this should operate. If innerInfo is null,
     * should I still check the values of outerInfo and innerName? I would
     * think that it wouldn't matter because if innerInfo is null, the other
     * two should also be null.
     */
    public void
    countConstantReferences(boolean isRelocatable) {
	super.countConstantReferences(isRelocatable);

	InnerClassEntry ice;
	Enumeration e = myInnerClassesVector.elements();
	while (e.hasMoreElements()) {
	    ice = (InnerClassEntry)e.nextElement();

	    if (ice.innerInfo != null) {
		ice.innerInfo.incReference();
	    }
	    if (ice.outerInfo != null) {
		ice.outerInfo.incReference();
	    }
	    /* 
	     * innerNameIndex not supported because utf8 cp entries
	     * are not kept around.
	     * 
	    if (ice.innerName != null) {
		ice.innerName.incReference();
	    }
	    */
	}

	// Don't forget to update the array copy now that we've changed
	// the vector. 
	myInnerClassesVector.copyInto(myInnerClassesArray);
    }

    /*
     * This method only called just before writing output.
     * Make sure that all our ClassConstant entries end up in a constant
     * pool. Call super.validate so it can check its fields, too.
     */
    public void
    validate(){
	super.validate();
	InnerClassEntry ice;
	Enumeration e = myInnerClassesVector.elements();
	while (e.hasMoreElements()) {
	    ice = (InnerClassEntry)e.nextElement();

	    if (ice.innerInfo != null) {
		ice.innerInfo.validate();
	    }
	    if (ice.outerInfo != null) {
		ice.outerInfo.validate();
	    }
	}
    }

    public void
    removeEntry(int i) {
System.out.println("*** Removing inner class "+ myInnerClassesArray[i].getFullName());
//	myInnerClassesVector.remove(myInnerClassesArray[i]);
	myInnerClassesVector.removeElement(myInnerClassesArray[i]);
	
	// Don't forget to update the array copy now that we've changed
	// the vector.
	myInnerClassesArray = new InnerClassEntry[myInnerClassesVector.size()];
//	myInnerClassesVector.toArray(myInnerClassesArray);
	myInnerClassesVector.copyInto(myInnerClassesArray);
    }	
    
    public int
    getInnerClassCount() {
	return myInnerClassesVector.size();
    }
    
    /*
     * The following four methods return information about individual
     * inner class entries contained in this attribute. These methods
     * call corresponding methods in the InnerClassEntry class. They
     * are self-explanatory.
     */

    public int
    getInnerInfoIndex(int i) {	
	return myInnerClassesArray[i].getInnerInfoIndex();
    }

    public int
    getOuterInfoIndex(int i) {
	return myInnerClassesArray[i].getOuterInfoIndex();
    }

    public int
    getInnerNameIndex(int i) {	
	return myInnerClassesArray[i].getInnerNameIndex();
    }
    
    public int
    getAccessFlags(int i) {	
	return myInnerClassesArray[i].getAccessFlags();
    }
    
    /*
     * Like the above methods, these methods return information about
     * an individual inner class entry. The string returned will be
     * the name of the inner class, if it has one. If the inner class
     * is anonymous, it will not have a name and the string "anonymous"
     * will be returned.
     */
    public String
    getName(int i) {
	return myInnerClassesArray[i].getName();
    }
    
    /*
     * Like the getName method, but return the outer class name as well.
     */
    public String
    getFullName(int i) {
	return myInnerClassesArray[i].getFullName();
    }
}
