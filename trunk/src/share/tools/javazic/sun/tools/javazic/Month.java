/*
 * @(#)Month.java	1.6 06/10/10
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

import java.util.HashMap;

/**
 * Month class handles month related manipulation.
 *
 * @since 1.4
 */
class Month {
    private static HashMap months = new HashMap();
    static {
	months.put("Jan", new Integer(0));
	months.put("Feb", new Integer(1));
	months.put("Mar", new Integer(2));
	months.put("Apr", new Integer(3));
	months.put("May", new Integer(4));
	months.put("Jun", new Integer(5));
	months.put("Jul", new Integer(6));
	months.put("Aug", new Integer(7));
	months.put("Sep", new Integer(8));
	months.put("Oct", new Integer(9));
	months.put("Nov", new Integer(10));
	months.put("Dec", new Integer(11));
    }

    /**
     * Parses the specified string as a month abbreviation.
     * @param name the month abbreviation
     * @return the month value
     */
    static int parse(String name) {
	Integer m = (Integer) months.get(name);
	if (m != null) {
	    return m.intValue();
	}
	throw new IllegalArgumentException("wrong month name: " + name);
    }

    private static String upper_months[] = {
	"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
	"JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };

    /**
     * @param month the numth number
     * @return the month name in uppercase of the specified month
     */
    static String toString(int month) {
	if (month >= 0 && month <= 11) {
	    return "Calendar."+ upper_months[month];
	}
	throw new IllegalArgumentException("wrong month number: " + month);
    }
}
