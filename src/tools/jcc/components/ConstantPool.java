/*
 *   
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
 */

package components;
import java.io.DataOutput;
import java.io.DataInput;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;
import java.io.PrintStream;
import sun.misc.Compare;

public 
class ConstantPool implements Compare {

    protected Hashtable	h;	// for "quick" lookup
    protected Vector	t;	// for enumeration in order
    protected int		n;
    protected ConstantObject constants[]= null;
    protected boolean	locked = false;
    public boolean   impureConstants = false;

    public ConstantPool(){
	h = new Hashtable( 500, 0.7f );
	t = new Vector();
	t.addElement(null ); // 0th element is string of length 0.
	n = 1;
    }
    
    /**
     * doCompare
     *   
     * @param  obj1 first object to compare.
     * @param  obj2 second object to compare.
     * @return -1 if obj1 > obj2, 0 if obj1 == obj2, 1 if obj1 < obj2.
     */  
    public int doCompare( java.lang.Object o1, java.lang.Object o2) {
        ConstantObject obj1 = (ConstantObject) o1;
        ConstantObject obj2 = (ConstantObject) o2;

        if (obj1.ldcReferences < obj2.ldcReferences) {
	   return 1;
        } else if (obj1.ldcReferences == obj2.ldcReferences)
           return 0;
        return -1;
    }

    public void doSort() {
	int count = 0;
	int nNew = 1;

	// Retrieve the ConstantObject array from the hashtable.
	Enumeration e = h.elements();
	ConstantObject arr[] = new ConstantObject[h.size()];
	while (e.hasMoreElements()) { 
	    arr[count]= (ConstantObject) e.nextElement();
	    count++;
	}

	// Sorting the ConstantObject with descending reference
	// count.	
	sun.misc.Sort.quicksort(arr, this);

	t.removeAllElements();
	t.addElement(null);
	for (int i = 0; i < arr.length; i++) {
	    arr[i].index = nNew;
	    nNew += arr[i].nSlots;
	    t.addElement(arr[i]);
	    for (int j =arr[i].nSlots; j > 1; j-- )
                t.addElement( null ); // place holder
	}
	constants = null;
    }

    /**
     * Return the ConstantObject in constant table corresponding to
     * the given ConstantObject s.
     * Inserts s if it is not already there.
     * The index member of the returned value (which
     * may be the object s!) will be set to its index in our
     * table. There will be no element of index 0.
     */
    public ConstantObject
    add( ConstantObject s ) {
	if ( s.shared && s.containingPool==this ) return s; // this is already it!
	ConstantObject r = (ConstantObject)h.get( s );
	if ( r == null ){
	    if ( locked ){
		throw new Error("add on locked ConstantPool");
	    }
	    if ( s.shared ){
		// already in another constant pool!
		// this copy is not deep enough!!!
		r = (ConstantObject) s.clone();
	    } else {
		r = s;
	    }
	    r.externalize( this ); // Deeper copy, as required.
	    r.index = n;
	    n += r.nSlots;
	    r.shared = true;
	    r.containingPool = this;
	    h.put( r, r );
	    t.addElement( r );
	    for ( int i =r.nSlots; i > 1; i-- )
		t.addElement( null ); // place holder.
	    constants = null; // mark any "constants" as obsolete!
	}
	return r;
    }

    /*
     * Like add() above, but with less management: still
     * to ensure that there's only one entry like this in the pool,
     * but doesn't externalize/share.
     */
   
    public ConstantObject
    appendElement( ConstantObject s ){
	ConstantObject r = (ConstantObject)h.get( s );
	if ( r == null ){
	    if ( locked ){
		throw new Error("add on locked ConstantPool");
	    }
	    r = s;
	    r.index = n;
	    n += r.nSlots;
	    h.put( r, r );
	    t.addElement( r );
	    for ( int i =r.nSlots; i > 1; i-- )
		t.addElement( null ); // place holder.
	    constants = null; // mark any "constants" as obsolete!
	} else { 
	    r.ldcReferences += s.ldcReferences;
	    r.references += s.references;
	}
	return r;
    }

    public ConstantObject
    dup( ConstantObject s ) {
	if ( s.shared && s.containingPool==this ) return s;
	return this.add( (ConstantObject) s.clone() );
    }

    public ConstantObject[]
    getConstants(){
	if ( constants != null )
	    return constants;
	constants = new ConstantObject[ t.size() ];
	t.copyInto( constants );
	return constants;
    }

    public Enumeration
    getEnumeration(){
	return t.elements();
    }

    public void
    lock(){ locked = true; }

    public void
    unlock(){ locked = false; }

    public int
    read( DataInput in ) throws IOException {
	int n = in.readUnsignedShort();
	ConstantObject c[] = new ConstantObject[n];
	for (int i = 1; i < n; i+=c[i].nSlots ){
	    c[i] = ConstantObject.readObject(in);
	}
	//System.err.println("DEBUG CONSTANTPOOL DUMP" );
	//for (int i = 1; i < n; i+=c[i].nSlots ){
	//    System.err.println("\t#"+i+"\t"+c[i].toString() );
	//}
	for (int i = 1; i < n; i+=c[i].nSlots ){
	    c[i].resolve( c );
	}
	for (int i = 1; i < n; i+=c[i].nSlots ){
	    add( c[i] );
	}
	constants = c; // IMPL_NOTE: consider whether this should be fixed 
	return n;
    }

    public void
    clearAllReferences(){
	ConstantObject c;
	for( int i=1; i< n; i+=c.nSlots){
	    c = (ConstantObject)t.elementAt(i);
	    c.clearReference();
	}
    }

    /*
     * If we are not loading entire classes, then there is
     * some chance that unreferenced constants have sneaked into
     * this pool. They can be deleted and the table made smaller.
     * This is a waste of time when partial class loading is not done.
     *
     * Naturally, we preserve the null entries.
     *
     */
    public void smashConstantPool(){
	int nNew = 1;
	ConstantObject o;
	// first, count and index.
	for ( int i = 1; i < n; i += o.nSlots ){
	    o = (ConstantObject)t.elementAt(i);
	    if ( o.references == 0 ){
		o.index = -1;
		h.remove( o );
	    } else {
		// we're keeping it.
		o.index = nNew;
		nNew += o.nSlots;
	    }
	}
	if ( nNew == n ) return; // all done!

	// copy live ones from old vector to new.
	Vector newConstants = new Vector( nNew );
	newConstants.addElement( null );
	for ( int i = 1; i < n; i += o.nSlots ){
	    o = (ConstantObject)t.elementAt(i);
	    if ( o.references != 0 ){
		// we're keeping it.
		newConstants.addElement(o);
		for ( int j =o.nSlots; j > 1; j-- )
		    newConstants.addElement( null ); // place holder.
	    }
	}
	t = newConstants;
	n = nNew;
	constants = null; // mark as obsolete
    }

    /**
     * Write out the number of constants to write.
     * Then write out the constants, in order.
     * Returns total number of constants written.
     */
    public int
    write( DataOutput o ) throws IOException {
	o.writeShort(n);
	ConstantObject ob;
	for (int i = 1; i < n; i+=ob.nSlots ){
	    ob = (ConstantObject)t.elementAt(i);
	    if ( ob != null )
		ob.write(o);
	}
	return n;
    }

    public void
    dump( PrintStream o ){
	ConstantObject c;
	for( int i=1; i< n; i+=c.nSlots){
	    c = (ConstantObject)t.elementAt(i);
	    o.println("\t["+c.index+"]\t"+c.references+"\t"+c.toString() );
	}
    }
}
