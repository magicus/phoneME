/*
 * @(#)Attribute.java	1.14 06/10/10
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
import java.io.DataOutput;
import java.io.DataInput;
import java.io.IOException;
import java.util.Hashtable;
import util.*;

/*
 * A class to represent the general Attribute form.
 * In its most uninterpreted form, "data" is just an
 * array of byte. It can also be other things, even
 * array of ConstantObject. 
 */

public abstract
class Attribute extends ClassComponent
{
    public UnicodeConstant name;
    public int		   length;

    protected
    Attribute( UnicodeConstant n, int l ){
	name = n;
	length = l;
	isFlat = true;
    }

    abstract protected int
    writeData( DataOutput o ) throws IOException;

    public void write( DataOutput o ) throws IOException {
	o.writeShort( name.index );
	o.writeInt( length );
	int trueLength = writeData( o );
	if ( length != trueLength ){
	    throw new DataFormatException(Localizer.getString("attribute.bad_attribute_length_for.dataformatexception", name ));
	}
    }

    public void countConstantReferences(boolean isRelocatable) {
	//
	// if we are producing relocatable output, then
	// we will need our name in the string table.
	// Else not.
	//
	if (isRelocatable) {
	    name.incReference();
        }
    }

    /*
     * This method can only be called at the very end of processing,
     * just before output writing.  We call name.validate to make
     * sure that the UnicodeConstant is not in the constant pool.
     * Subclasses can override to take more specific actions.
     */
    public void validate(){
	name.validate();
    }

    /*
     * Number of bytes we'll write out
     */
    public int length(){
	return 6+length; // believe length unchanged
    }

    public static void
    writeAttributes( Attribute a[], DataOutput o,  boolean verbose ) throws IOException {
	int nattr =  ( a == null ) ? 0 : a.length;
	if ( verbose ){
	    System.out.println(Localizer.getString("attribute.writing_attributes", Integer.toString(nattr)));
	}
	o.writeShort( nattr );
	for ( int j = 0; j < nattr; j++ ){
	    if ( verbose ){
		System.out.println("	"+a[j].name.string );
	    }
	    a[j].write( o );
	}
    }


    public static int
    length( Attribute a[] ){
	int l = 0;
	int arryl = ( a == null ) ? 0 : a.length;
	for ( int i = 0; i < arryl; i++ ){
	    l += a[i].length();
	}
	return l + 2;
    }

    public static Attribute[] 
    readAttributes(
	DataInput i,
	ConstantPool cp,
	Hashtable typetable,
	boolean verbose)
	throws IOException
    {
	int nattr =  i.readUnsignedShort();
	if (verbose) {
	    System.out.println(Localizer.getString(
		"attribute.reading_attributes", Integer.toString(nattr)));
	}
	if (nattr == 0) return null;
	Attribute a[] = new Attribute[nattr];
	ConstantObject constants[] = cp.getConstants();
	for (int j = 0; j < nattr; j++) {
	    UnicodeConstant name =
                (UnicodeConstant)constants[i.readUnsignedShort()];
	    String typename = name.string.intern();
	    AttributeFactory afact = (AttributeFactory)typetable.get(typename);
	    if (afact == null) {
		afact = UninterpretedAttributeFactory.instance;
            }
	    a[j] = afact.finishReadAttribute(i, name, cp);
	}
	return a;
    }

    public static void
    countConstantReferences(Attribute a[], boolean isRelocatable) {
	if (a == null) return;
	for (int i = 0; i < a.length; i++) {
	    a[i].countConstantReferences(isRelocatable);
	}
    }

}
