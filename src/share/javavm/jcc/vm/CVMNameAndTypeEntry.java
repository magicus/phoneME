/*
 * Copyright  2008 Sun Microsystems, Inc. All Rights Reserved.  
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
package vm;
import java.util.Vector;

/*
 * Collect composite name and type pairs. Turn them into little integers.
 * Build a hash table of them for writing out to the romjava image.
 */
public class CVMNameAndTypeEntry
{
    /* instance members */
    public int nameID;
    public int typeID;
    public CVMNameAndTypeEntry next;
    public int entryNo;
    public String stringValue;

    /* static members */
    public final static int HASH_SIZE = 41*13;
    public static CVMNameAndTypeEntry hash[] =
                       new CVMNameAndTypeEntry[HASH_SIZE];

    static int nextEntryNo = 0;
    public static Vector table = new Vector();

    /* constructor &c */

    static synchronized void enlist(CVMNameAndTypeEntry x, int xhash) {
	x.entryNo = nextEntryNo++;
	table.addElement(x);
	x.next = hash[xhash];
	hash[xhash] = x;
    }

    CVMNameAndTypeEntry(int nameID, int typeID, int myhash) {
	this.nameID = nameID;
        this.typeID = typeID;
	enlist(this, myhash);
    }

    /* static methods */
    /*
     * We use a very simple minded name hashing scheme.
     * This scheme needs to be consistent with the hashing scheme used in
     * typeid.c's referenceNameAndType().
     */
    public static int computeHash(int nameID, int typeID) {
        int v = nameID ^ typeID;
	return v;
    }

    public static String lookupEnter(int nameID, int typeID) {
        // If we can encode the composite nameAndTypeID within 31 bits,
        // then just do that:
        int typeIDBase = typeID & 0xffffff;
        int typeIDDepth = typeID >>> 24;

        // Encode a small memberID if possible:

        /* The encoding for small memberIDs is as follows:

                     name   arrayDepth   baseType   marker
              bits    16         2           13        1

           marker is the bit that, if set, indicates that the memberID is
           of the small encoding (above) as opposed to the CVMMemberTypeID
           pointer format below.

           baseType maps to the lower 13 bits of the sigID component of the
           memberID.

           arrayDepth maps to bit 24 and 25 of the sigID component of the
           memberID.

           name maps to the lower 16 bits of the nameID component of the
           memberID.
        */
        if (((nameID & 0xffff) == nameID) &&
            ((typeIDDepth & 0x3) == typeIDDepth) &&
            ((typeIDBase & 0x1fff) == typeIDBase)) {

            int value = ((nameID      << 16) |
                         (typeIDDepth << 14) |
                         (typeIDBase  << 1) | 0x1);

            String stringValue = "0x" + Integer.toHexString(value);
            return stringValue;
        }

        // Else, we look it up in the hash table:
	int hval = (int)(((long)computeHash(nameID, typeID) & 0xffffffffL)
                             % HASH_SIZE);

	CVMNameAndTypeEntry entry;
	for (entry = hash[hval]; entry != null; entry = entry.next) {
	    if (entry.nameID == nameID && entry.typeID == typeID) {
                // Return the encoded big memberID of the existing entry:
                return entry.stringValue;
            }
	}

        // If we don't find it in the hash table, we create a new entry:
	entry = new CVMNameAndTypeEntry(nameID, typeID, hval);
        // Remember to return the negative value of the entryNo because
        // positive values are reserved for the small value encoding above:

        // Encode a big memberID based on the new entry:
        entry.stringValue = "&CVMtypeidNameAndTypeEntries.data[" +
                            entry.entryNo + "].compID";
        return entry.stringValue;
    }

    public static int tableSize(){
	return nextEntryNo;
    }
}
