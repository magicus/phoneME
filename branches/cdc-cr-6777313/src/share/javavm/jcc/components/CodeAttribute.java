/*
 * @(#)CodeAttribute.java	1.13 06/10/10
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
import util.DataFormatException;
import java.util.Hashtable;

/*
 * A class to represent the Code Attribute
 * of a method
 */

public
class CodeAttribute extends Attribute
{
    public int		  stack;
    public int		  locals;
    public byte		  code[];
    public ExceptionEntry exceptionTable[];
    public Attribute	  codeAttributes[];

    public
    CodeAttribute( UnicodeConstant name, int l, int ns, int nl, byte c[],
	ExceptionEntry et[], Attribute ca[] ){
	super( name, l );
	stack  = ns;
	locals = nl;
	code   = c;
	exceptionTable = et;
	codeAttributes = ca;
    }

    public void
    countConstantReferences(boolean isRelocatable) {
	super.countConstantReferences(isRelocatable);
	if (exceptionTable != null) {
	    for(int i = 0; i < exceptionTable.length; i++) {
		exceptionTable[i].countConstantReferences();
	    }
	}
	Attribute.countConstantReferences(codeAttributes, isRelocatable);
    }

    protected int
    writeData( DataOutput o ) throws IOException{
	int trueLength = 8 + code.length;
	o.writeShort( stack );
	o.writeShort( locals );
	o.writeInt( code.length );
	o.write( code, 0, code.length );
	if ( exceptionTable == null ){
	    o.writeShort( 0 );
	    trueLength += 2;
	} else {
	    o.writeShort( exceptionTable.length );
	    for ( int i = 0; i < exceptionTable.length ; i ++ ){
		exceptionTable[i].write( o );
	    }
	    trueLength += 2 + exceptionTable.length*ExceptionEntry.size;
	}
	if ( codeAttributes == null ){
	    o.writeShort(0);
	    trueLength += 2;
	} else {
	    Attribute.writeAttributes( codeAttributes, o, false );
	    trueLength += Attribute.length( codeAttributes );
	}
	return trueLength;
    }

    /*
     * This hashtable is for use when reading code attributes.
     * The only code attribute we're interested in is
     * the LineNumberTableAttribute.
     * Other stuff we ignore.
     */
    static private Hashtable codeAttributeTypes = new Hashtable();
    static{
	codeAttributeTypes.put( "LineNumberTable", LineNumberTableAttributeFactory.instance );
	codeAttributeTypes.put( "LocalVariableTable", LocalVariableTableAttributeFactory.instance );
    }

    public static Attribute
    readAttribute(DataInput i, ConstantPool cp)
	throws IOException
    {
	UnicodeConstant name =
	    (UnicodeConstant)cp.elementAt(i.readUnsignedShort());
	return finishReadAttribute(i, name, cp);
    }

    //
    // for those cases where we alread read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute(
	DataInput in,
	UnicodeConstant name,
	ConstantPool cp)
	throws IOException
    {
	int l;
	int nstack;
	int nlocals;
	ConstantObject d;
	ExceptionEntry exceptionTable[];

	l       = in.readInt();
	nstack  = in.readUnsignedShort();
	nlocals = in.readUnsignedShort();

	int codesize = in.readInt();
	byte code[] = new byte[codesize];
	in.readFully(code);

	int tableSize = in.readUnsignedShort();
	exceptionTable = new ExceptionEntry[tableSize];
	ConstantObject constants[] = cp.getConstants();
	for (int j = 0; j < tableSize; j++) {
	    int startPC = in.readUnsignedShort();
	    int endPC = in.readUnsignedShort();
	    int handlerPC = in.readUnsignedShort();
	    int catchTypeIndex = in.readUnsignedShort();
	    ClassConstant catchType;
	    if (catchTypeIndex == 0){
		catchType = null;
	    } else {
		catchType = (ClassConstant)constants[catchTypeIndex];
	    }
	    exceptionTable[j] =
                new ExceptionEntry(startPC, endPC, handlerPC, catchType);
	}

	Attribute a[] = Attribute.readAttributes(in, cp,
						 codeAttributeTypes, false);
	return new CodeAttribute(name, l, nstack, nlocals,
				 code, exceptionTable, a);
    }

}
