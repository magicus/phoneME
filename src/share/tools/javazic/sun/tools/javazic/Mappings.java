/*
 * @(#)Mappings.java	1.6 06/10/10
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

package sun.tools.javazic;

import	java.util.HashMap;
import	java.util.Iterator;
import	java.util.LinkedList;
import	java.util.Set;
import	java.util.TreeMap;
import	java.util.TreeSet;

/**
 * <code>Mappings</code> generates two Maps and a List which are used by
 * javazic BackEnd.
 *
 * @since 1.4
 */
class Mappings {
   private TreeMap aliases; 
   private LinkedList rawOffsetsIndex; 
   private LinkedList rawOffsetsIndexTable; 

    /**
     * Constructor creates some necessary instances.
     */
    Mappings() {
	aliases = new TreeMap();
	rawOffsetsIndex = new LinkedList();
	rawOffsetsIndexTable = new LinkedList();
    }
 
    /**
     * Generates aliases and rawOffsets tables.
     * @param Zoneinfo
     */
    void add(Zoneinfo zi) {
	int i;
	HashMap zones = zi.getZones();
	Set s = zones.keySet();
	Iterator keys = s.iterator();
	TreeSet perRawOffset;

	while (keys.hasNext()) {
	    String key = (String)keys.next();
	    Zone zone = zi.getZone(key);
	    String zonename = zone.getName();
	    int rawOffset = zone.get(zone.size()-1).getGmtOffset();

	    if (!rawOffsetsIndex.contains(new Integer(rawOffset))) {
		int n = rawOffsetsIndex.size();

		for (i = 0; i < n; i++) {
		    if (((Integer)rawOffsetsIndex.get(i)).intValue()
			> rawOffset) {
			break;
		    }
		}
		rawOffsetsIndex.add(i, new Integer(rawOffset));

		perRawOffset = new TreeSet();
		perRawOffset.add(zonename);
		rawOffsetsIndexTable.add(i, perRawOffset);
	    }
	    else {
		i = rawOffsetsIndex.indexOf(new Integer(rawOffset));
		perRawOffset = (TreeSet)rawOffsetsIndexTable.get(i);
	    }
	    perRawOffset.add(zonename);
	}

	aliases.putAll(zi.getAliases());
    }

    /**
     * Removes invalid aliases from aliases List.
     */
    void resolve() {
	Object o[] = aliases.keySet().toArray();
	int len = o.length;
	int index = rawOffsetsIndexTable.size();

	for (int i = 0; i < len; i++) {
	    String key = o[i].toString();
	    boolean validname = false;

	    for (int j = 0; j < index; j++) {
		TreeSet	perRO = (TreeSet)rawOffsetsIndexTable.get(j);

		if (perRO.contains(aliases.get(key)) && Zone.isTargetZone(key)) {
		    validname = true;
		    perRO.add(key);
		    Main.info("Alias <"+key+"> added to the list.");
		    break;
		}
	    } 

	    if (!validname) {
		Main.info("Alias <"+key+"> removed from the list.");
		aliases.remove(key);
	    }
	}
	return;
    }

    TreeMap getAliases() {
	return(aliases);
    } 

    LinkedList getRawOffsetsIndex() {
	return(rawOffsetsIndex);
    }

    LinkedList getRawOffsetsIndexTable() {
	return(rawOffsetsIndexTable);
    }
}
