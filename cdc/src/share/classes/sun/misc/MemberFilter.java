/*
 * @(#)MemberFilter.java	1.6 06/10/10
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
package sun.misc;
/*
 * @(#)MemberFilter.java	1.1 03/06/24
 *
 * A MemberFilter is used in conjunction with a restrictive class loader,
 * such as the MidletClassLoader. It keeps a list of classes and a subset of
 * their fields. Once a class's constant pool is set up,
 * checkMemberAccessValidity will look at it. It will report on any
 * field, method, or interfacemethod references which are
 * (a) to classes named in the restrictions but
 * (b) not on the allowed list.
 * The implementation is very specific to CVM.
 */

// probably want this package-visible only once debugged 

public class
MemberFilter
{
    private int	partialData;
    private String badClass;
    private String badMember;
    private String badSig;

    private native void finalize0();

    protected void finalize(){
	finalize0();
	partialData = 0;
    }

    public MemberFilter(){}

    /*
     * Find ROMized filter data. If exists, set 
     * CVMglobals.dualStackMemberFilter
     * and return true. Otherwise, return false.
     */
    public native boolean findROMFilterData();

    /*
     * the name of a class and arrays of all the fields and methods
     * -- including type signatures -- which are allowed.
     * Separate names from signatures using a colon :
     * classname uses / separators!
     * an element of the methods array could look something like this:
     *	"toString():Ljava/lang/String;"
     */
    public native void
    addRestrictions(String classname, String fields[], String methods[]);

    /*
     * Once addRestrictions has been called a number of times,
     * a call to doneAddingRestrictions() will allow us to form a
     * more compact and easily searched form of the data structure.
     */
    public native void
    doneAddingRestrictions();

    private native boolean
    checkMemberAccessValidity0(Class newclass);

    public synchronized void /* synchronized IS DEBUG: REMOVE IT */
    checkMemberAccessValidity(Class newclass) throws Error
    {
	if (!checkMemberAccessValidity0(newclass)){
	    /* DEBUG */
	    /* the native code will set the badClass, badMember and badSig
	     * fields. But unless you decide that this method should be
	     * synchronized, there is no reason to believe that they won't
	     * get clobbered in another thread!
	     */
            /* Don't throw an exception here.  Let constantpool resolution
             * throw the exception on first use of this illegal member
             * instead. */
	    // throw new Error("Class "+(newclass.getName())+" makes illegal member references to "+badClass+"."+badMember+":"+badSig);
	    /* END DEBUG */
	    // throw new Error("Class "+(newclass.getName())+" makes illegal member references");
	}
    }
}
