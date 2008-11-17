/*
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

public class VMInspector
{
    public static native void exit(int status);

    public static native boolean enableGC();
    public static native boolean disableGC();
    public static native boolean gcIsDisabled();
    public static native void keepAllObjectsAlive(boolean keepAlive);

    // Misc debug utility functions:

    /**
     * Returns an object reference for the specified object address.
     *
     * @return	an object reference which refers to the specified object
     * 		address.
     * @exception IllegalStateException if the GC is not disabled before this
     *		  method is called.
     * @exception IllegalArgumentException if the specified object address
     *		  does not point to a valid object.
     */
    public static native Object addrToObject(long objAddr)
        throws IllegalStateException,IllegalArgumentException;

    public static native long objectToAddr(Object obj)
        throws IllegalStateException;

    // Object inspection utilities:
    public static native void dumpObject(long objAddr);
    public static native void dumpClassBlock(long cbAddr);
    public static native void dumpObjectReferences(long objAddr);
    public static native void dumpClassReferences(String classname);
    public static native void dumpClassBlocks(String classname);

    // System info utilities:
    public static native void dumpSysInfo();
    public static native void dumpHeapSimple();
    public static native void dumpHeapVerbose();
    public static native void dumpHeapStats();

    // GC utilities:
    public static native void dumpObjectGCRoots(long objAddr);

    // Heap state capture utility:
    public static final int SORT_NONE = 0;
    public static final int SORT_BY_OBJ = 1;
    public static final int SORT_BY_OBJCLASS = 2;

    public static native void captureHeapState(String name);
    public static native void releaseHeapState(int id);
    public static native void releaseAllHeapState();
    public static native void listHeapStates();
    public static native void dumpHeapState(int id, int sortKey);
    public static native void compareHeapState(int id1, int id2);

    // Class utilities:
    public static native void listAllClasses();

    // Thread utilities:
    public static native void listAllThreads();
    public static native void dumpAllThreads();
    public static native void dumpStack(long eeAddr);
}
