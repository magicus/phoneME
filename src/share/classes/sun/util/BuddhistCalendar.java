/*
 * @(#)BuddhistCalendar.java	1.10 06/10/10
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

package sun.util;

import java.util.GregorianCalendar;
import java.util.TimeZone;
import java.util.Locale;
import java.util.Calendar;

public class BuddhistCalendar extends GregorianCalendar {

//////////////////
// Class Variables
//////////////////

    private static final int buddhistOffset = 543;

///////////////
// Constructors
///////////////

    /**
     * Constructs a default BuddhistCalendar using the current time
     * in the default time zone with the default locale.
     */
    public BuddhistCalendar() {
        super();
    }

    /**
     * Constructs a BuddhistCalendar based on the current time
     * in the given time zone with the default locale.
     * @param zone the given time zone.
     */
    public BuddhistCalendar(TimeZone zone) {
        super(zone);
    }

    /**
     * Constructs a BuddhistCalendar based on the current time
     * in the default time zone with the given locale.
     * @param aLocale the given locale.
     */
    public BuddhistCalendar(Locale aLocale) {
        super(aLocale);
    }

    /**
     * Constructs a BuddhistCalendar based on the current time
     * in the given time zone with the given locale.
     * @param zone the given time zone.
     * @param aLocale the given locale.
     */
    public BuddhistCalendar(TimeZone zone, Locale aLocale) {
        super(zone, aLocale);
    }

/////////////////
// Public methods
/////////////////
    
    /**
     * Compares this BuddhistCalendar to an object reference.
     * @param obj the object reference with which to compare
     * @return true if this object is equal to <code>obj</code>; false otherwise
     */
    public boolean equals(Object obj) {
        return super.equals(obj) &&
            obj instanceof BuddhistCalendar;
    }
    
    /**
     * Override hashCode.
     * Generates the hash code for the BuddhistCalendar object
     */
    public int hashCode() {
        return super.hashCode() ^ buddhistOffset;
    }

    /**
     * Gets the value for a given time field.
     * @param field the given time field.
     * @return the value for the given time field.
     */
    public int get(int field)
    {
	if (field == YEAR) {
	    return super.get(field) + yearOffset;
	}
	return super.get(field);
    }

    /**
     * Sets the time field with the given value.
     * @param field the given time field.
     * @param value the value to be set for the given time field.
     */
    public void set(int field, int value)
    {
	if (field == YEAR) {
	    super.set(field, value - yearOffset);
	} else {
	    super.set(field, value);
	}
    }

    /**
     * Adds the specified (signed) amount of time to the given time field.
     * @param field the time field.
     * @param amount the amount of date or time to be added to the field.
     */
    public void add(int field, int amount)
    {
        yearOffset = 0;
        super.add(field, amount);
        yearOffset = buddhistOffset;
    }

    /**
     * Add to field a signed amount without changing larger fields.
     * A negative roll amount means to subtract from field without changing
     * larger fields.
     * @param field the time field.
     * @param amount the signed amount to add to <code>field</code>.
     */
    public void roll(int field, int amount)
    {
        yearOffset = 0;
        super.roll(field, amount);
        yearOffset = buddhistOffset;
    }

    private transient int yearOffset = buddhistOffset;

    // Proclaim serialization compatibility with JDK 1.4.0
    static final long serialVersionUID = -8527488697350388578L;
}
