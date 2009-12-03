/*
 * @(#)ConstantPool.java	1.20 06/10/10
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
import java.util.Arrays;
import java.util.Comparator;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;
import java.io.PrintStream;
import util.ValidationException;

/*
 * A ConstantPool represented a shared collection of ConstantObjects.
 * They behave just as they do when part of individual Classes, including
 * all the usual rules about first entry being null, and (for purposes of
 * our internal bookkeeping) always being a null entry following any two-word
 * constant: this makes simple indexing much easier.
 *
 * Originally, this class was used to form the shared part of a multi-class
 * output file. But that idea was deprecated long ago. It is now used to form
 * the shared constant pool used internally in our VM for all ROMized classes
 * when the -sharedCP command-line flag is given. This is seen as a space saving
 * device (much less duplication), as well as a time saver (less resolution of
 * those duplicate entries). Unfortunately, it means that some entries will
 * be out of range of the usual instructions such as ldc, so the bytecodes have
 * to be rewritten to accomodate.
 * In order to minimize the rewriting necessary, constants are sorted by
 * number of references from the code (see field ConstantObject.ldcReferences)
 * so that the statically most popular come first and their indices will fit
 * into the normal, short-form instructions.
 */
public 
class ConstantPool implements Comparator {

    protected Hashtable	h;	// for "quick" lookup
    protected Vector	enumedEntries;	// for enumeration in order
    protected int		n;
    protected ConstantObject constants[]= null;
    protected boolean	locked = false;
    private boolean	impureConstants = false;
    private boolean	needsTypeTable = false;

    public ConstantPool(){
	h = new Hashtable( 500, 0.7f );
	enumedEntries = new Vector();
	enumedEntries.addElement(null ); // 0th element is string of length 0.
	n = 1;
    }

    public ConstantPool(ConstantObject[] c) {
	constants = c;
    }
    
    /**
     * compare
     *   
     * @param  o1 first object to compare.
     * @param  o2 second object to compare.
     * @return -1 if obj1 > obj2, 0 if obj1 == obj2, 1 if obj1 < obj2.
     */  
    public int compare(java.lang.Object o1, java.lang.Object o2) {
        ConstantObject obj1 = (ConstantObject) o1;
        ConstantObject obj2 = (ConstantObject) o2;

        if (obj1.getLdcReferences() < obj2.getLdcReferences()) {
	   return 1;
        } else if (obj1.getLdcReferences() == obj2.getLdcReferences())
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
        Arrays.sort(arr, this);

	enumedEntries.removeAllElements();
	enumedEntries.addElement(null);
	for (int i = 0; i < arr.length; i++) {
	    arr[i].index = nNew;
	    nNew += arr[i].nSlots;
	    enumedEntries.addElement(arr[i]);
	    for (int j =arr[i].nSlots; j > 1; j-- )
                enumedEntries.addElement( null ); // place holder
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
     *
     * The constant's shared flag is set and its containingPool field
     * points back to this. This is only interesting for validation
     * at this point.
     *
     * This should be the ONLY way to add constants into the pool.
     * appendElement went away.
     */
    public ConstantObject
    add( ConstantObject s ) {
	if ( s.containingPool != null) {
	    if (s.containingPool==this ){
		return s; // this very object already ok in this pool
	    } else {
		throw new InternalError("Shared constant "+s);
	    }
	}
	ConstantObject r = (ConstantObject)h.get( s );
	if ( r == null ){
	    if ( locked ){
		throw new Error("add on locked ConstantPool");
	    }
	    r = s;
	    r.index = n;
	    n += r.nSlots;
	    r.containingPool = this;
	    r.shared = true;
	    h.put( r, r );
	    enumedEntries.addElement( r );
	    for ( int i =r.nSlots; i > 1; i-- )
		enumedEntries.addElement( null ); // place holder.
	    constants = null; // mark any "constants" as obsolete!
	} else { 
	    r.setLdcReferences(r.getLdcReferences() + s.getLdcReferences());
	    r.setReferences(r.getReferences() + s.getReferences());
	}
	return r; // a similar object in the pool.
    }

    public ConstantObject
    lookup( ConstantObject s ) {
	if ( s.containingPool == this) {
	    return s; // this very object already ok in this pool
	}
	return (ConstantObject)h.get( s ); // may be null
    }

    public boolean needsTypeTable() {
	// unquickened bytecodes
	return needsTypeTable;
    }

    public void setNeedsTypeTable() {
	needsTypeTable = true;
    }

    public ConstantObject elementAt(int i) {
	return getConstants()[i];
    }

    public ConstantObject[] getConstants(){
	if (constants != null) {
            return constants;
        }
	constants = new ConstantObject[enumedEntries.size()];
	enumedEntries.copyInto(constants);
	return constants;
    }

    public int getLength() {
	return getConstants().length;
    }

    public Enumeration
    getEnumeration(){
	return enumedEntries.elements();
    }

    public void
    lock(){ locked = true; }

    public void
    unlock(){ locked = false; }

    private int
    read(DataInput in) throws IOException {
	int n = in.readUnsignedShort();
	ConstantObject c[] = new ConstantObject[n];
	for (int i = 1; i < n; i+=c[i].nSlots) {
	    c[i] = ConstantObject.readObject(in);
	}
	//System.err.println("DEBUG CONSTANTPOOL DUMP" );
	//for (int i = 1; i < n; i+=c[i].nSlots ){
	//    System.err.println("\enumedEntries#"+i+"\enumedEntries"+c[i].toString());
	//}
	for (int i = 1; i < n; i+=c[i].nSlots) {
	    c[i].flatten(this);
	}
	for (int i = 1; i < n; i+=c[i].nSlots) {
	    add(c[i]);
	}
	constants = c;
	return n;
    }

    public void
    clearAllReferences(){
	ConstantObject c;
	for( int i=1; i< n; i+=c.nSlots){
	    c = (ConstantObject)enumedEntries.elementAt(i);
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
    private void smashConstantPool(){
	int nNew = 1;
	ConstantObject o;
	// first, count and index.
	for ( int i = 1; i < n; i += o.nSlots ){
	    o = (ConstantObject)enumedEntries.elementAt(i);
	    if (o.getReferences() == 0) {
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
	    o = (ConstantObject)enumedEntries.elementAt(i);
	    if (o.getReferences() != 0) {
		// we're keeping it.
		newConstants.addElement(o);
		for ( int j =o.nSlots; j > 1; j-- )
		    newConstants.addElement( null ); // place holder.
	    }
	}
	enumedEntries = newConstants;
	n = nNew;
	constants = null; // mark as obsolete
    }

    /* 
     * sweep over the constant pool. For each constant in it,
     * ensure that 
     * (a) the index is correct.
     * (b) two-word constants are followed by null entries,
     *     one-word entries are not.
     * (c) there are no UnicodeConstants.
     * (d) all constants are flagged as 'shared' and belong in this pool.
     * (e) no constant is unreferenced
     */
    public void
    validate(){
	ConstantObject[] constantPool = getConstants();
	if (constantPool == null){
	    return; // it probably won't
	}
	int nConsts = constantPool.length;
	if (constantPool[0] != null){
	    throw new ValidationException(
		    "Shared constant pool entry 0 must be zero");
	}
	for (int i = 1; i < nConsts; i++ ){
	    ConstantObject c = constantPool[i];
	    if (c == null){
		throw new ValidationException(
			"Unexpected null constant pool entry in shared pool");
	    }
	    if (c.index != i){
		throw new ValidationException(
			"Bad constant self-index in shared pool", c);
	    }
	    if (!c.shared){
		throw new ValidationException(
			"Unshared constant in shared pool", c);
	    }
	    if (c.containingPool != this){
		throw new ValidationException(
			"Constant contained in wrong shared pool?", c);
	    }
	    if (c instanceof DoubleValueConstant){
		// following element must be zero.
		i += 1;
		if ((i >= nConsts) || (constantPool[i] != null)){
		    throw new ValidationException(
			"Bad double value constant (no null) in shared pool", c);
		}
	    }else if (c instanceof UnicodeConstant){
		throw new ValidationException(
		    "Shared constant pool contains Unicode constant", c);
	    }
	    if (c.getReferences() + c.getLdcReferences() == 0) {
		throw new ValidationException(
		    "Shared constant pool contains unreferenced constant", c);
	    }
	}
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
	    ob = (ConstantObject)enumedEntries.elementAt(i);
	    if ( ob != null )
		ob.write(o);
	}
	return n;
    }

    public void
    dump( PrintStream o ){
	ConstantObject c;
	for( int i=1; i< n; i+=c.nSlots){
	    c = (ConstantObject)enumedEntries.elementAt(i);
	    o.println("\t["+c.index+"]\t"+
                      c.getReferences()+"\t"+c.toString() );
	}
    }
}
