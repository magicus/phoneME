/*
 * @(#)Calendar.java	1.6 06/10/10
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

import sun.util.calendar.Gregorian;

/**
 * Gregorian calendar utility methods. The algorithms are from
 * <i>"Calendrical Calculation"</i> by Nachum Dershowitz and Edward
 * M. Reingold (ISBN: 0-521-56474-3).
 *
 * @since 1.4
 */

class Calendar extends Gregorian {
    /**
     * Returns a fixed date of the n-th day-of-week which is after or
     * before the given fixed date.
     * @param nth specifies the n-th one. A positive number specifies
     * <em>after</em> the <code>fixedDate</code>. A negative number
     * or 0 specifies <em>before</em> the <code>fixedDate</code>.
     * @param dayOfWeek the day of week
     * @param fixedDate the fixed date
     * @return the fixed date of the <code>nth dayOfWeek</code> after
     * or before <code>fixedDate</code>
     */
    static int getNthDayOfWeek(int nth, int dayOfWeek, int fixedDate) {
	if (nth > 0) {
	    return (7 * nth + getDayOfWeekDateBefore(fixedDate, dayOfWeek));
	}
	return (7 * nth + getDayOfWeekDateAfter(fixedDate, dayOfWeek));
    }

    /**
     * Returns a date of the given day of week before the given fixed
     * date.
     * @param fixedDate the fixed date
     * @param dayOfWeek the day of week
     * @return the calculated date
     */
    private static int getDayOfWeekDateBefore(int fixedDate, int dayOfWeek) {
	return (getDayOfWeekDateOnOrBefore(fixedDate - 1, dayOfWeek));
    }

    /*
     * Returns a date of the given day of week that is closest to and
     * after the given fixed date.
     * @param fixedDate the fixed date
     * @param dayOfWeek the day of week
     * @return the calculated date
     */
    private static int getDayOfWeekDateAfter(int fixedDate, int dayOfWeek) {
	return (getDayOfWeekDateOnOrBefore(fixedDate + 7, dayOfWeek));
    }

    /*
     * Returns a date of the given day of week on or before the given fixed
     * date.
     * @param fixedDate the fixed date
     * @param dayOfWeek the day of week
     * @return the calculated date
     */
    private static int getDayOfWeekDateOnOrBefore(int fixedDate, int dayOfWeek) {
	--dayOfWeek;
	return (fixedDate - ((fixedDate - dayOfWeek) % 7));
    }
}
