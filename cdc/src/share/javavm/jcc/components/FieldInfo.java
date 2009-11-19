/*
 * @(#)FieldInfo.java	1.17 06/10/10
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
import java.util.Vector;
import util.Assert;
import util.DataFormatException;
import consts.Const;

/*
 * A Field of a class. Includes statics, static finals, and
 * of course instance fields.
 * In Java, we don't have enumerations, but we do have
 * static final fields, and these have a value attribute.
 */
public
class FieldInfo extends ClassMemberInfo {

    private Attribute 	  fieldAttributes[];
    public ConstantObject value;
    private ConstantValueAttribute  valueAttribute;
    public int		  instanceOffset = -1; // for instance fields only, of course
    public int  	  nSlots = -1;	     // ditto

    // Keep track of any list of fields to be excluded during
    // processing. Initialize once and then remove each entry,
    // if there are any, as it is found.
    private static Vector excludeList = null;

    static void setExcludeList( Vector v ) {
        excludeList = v;
        return;
    }

    public
    FieldInfo( int name, int sig, int access, ClassInfo p ){
	super( name, sig, access, p );
    }

    /*
     * The only field attribute we care about is
     * the ConstantValue attribute
     */
    private static Hashtable fieldAttributeTypes = new Hashtable();
    static{
	fieldAttributeTypes.put("ConstantValue", ConstantValueAttributeFactory.instance );
    }

    // Read attributes from classfile
    private void
    readAttributes( DataInput in ) throws IOException {
	fieldAttributes = Attribute.readAttributes( in, parent.getConstantPool(), fieldAttributeTypes, false );
    }

    // Read field from classfile
    public static FieldInfo 
    readField( DataInput in, ClassInfo p ) throws IOException {
	int acc  = in.readUnsignedShort();
	int name = in.readUnsignedShort();
	int sig =  in.readUnsignedShort();
	FieldInfo fi = new FieldInfo( name, sig, acc, p );
	fi.readAttributes(in );

	ConstantPool cp = p.getConstantPool();
        String cpEntryName = cp.elementAt(name).toString();

        // If this field is on an exclude list, discard it.
        if ( excludeList != null && excludeList.size() > 0 ) {
            // See if this field is to be discarded. The vector holds
            // the signature of fields to be excluded parsed into
            // class and fieldname portions
            for (int i = 0 ; i < excludeList.size() ; i++) {
                MemberNameTriple t =
                    (MemberNameTriple)excludeList.elementAt(i);
                if (t.sameMember(p.className, cpEntryName, null)) {
                    excludeList.remove(i);
                    return (FieldInfo)null;
                } 
            }
        }
	fi.flatten(cp);
	return fi;
    }

    /**
     * Note: resolution here does not involve classloading.
     */
    public void flatten(ConstantPool cp) {
	if (isFlat) return;
        Assert.disallowClassloading();
	super.flatten(cp);
	/*
	 * Parse attributes.
	 * If we find a value attribute, pick it out
	 * for special handling.
	 */
	if (fieldAttributes != null) {
	    for (int i = 0; i < fieldAttributes.length; i++) {
		Attribute a = fieldAttributes[i];
		if (a.name.string.equals("ConstantValue")) {
		    valueAttribute = (ConstantValueAttribute)a;
		    value = valueAttribute.data;
		}
	    }
	}
	switch(type.string.charAt(0)) {
	case 'D':
	case 'J':
	    nSlots = 2; access |= Const.ACC_DOUBLEWORD; break;
	case 'L':
	case '[':
	    nSlots = 1; access |= Const.ACC_REFERENCE; break;
	default:
	    nSlots = 1; break;
	}
	isFlat = true;
        Assert.allowClassloading();
    }

    public void captureValueAttribute(){
	if ( value != null ){
	    value = valueAttribute.data;
	    if (value.tag == Const.CONSTANT_STRING){
		String v = ((StringConstant)value).str.string;
	    }
	}
    }

    public boolean isVolatile() {
	return ( (access & Const.ACC_VOLATILE) != 0 );
    }

    public void
    countConstantReferences( boolean isRelocatable ){
	super.countConstantReferences(isRelocatable);
	Attribute.countConstantReferences( fieldAttributes, isRelocatable );
    }

    public void
    write(DataOutput o) throws IOException {
	o.writeShort(access);
	if (isFlat) {
	    o.writeShort(name.index);
	    o.writeShort(type.index);
	    Attribute.writeAttributes(fieldAttributes, o, false);
	} else {
	    o.writeShort(nameIndex);
	    o.writeShort(typeIndex);
	    o.writeShort(0); // no attribute!
	}
    }

    public String toString() {
	String r = "Field: "+super.toString();
	if (value != null) {
	    r += " Value: "+value.toString();
	}
	return r;
    }


}
