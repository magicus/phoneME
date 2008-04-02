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

package java.util;

/**
 * The class Date represents a specific instant in time, with millisecond
 * precision.
 * <p>
 * This Class has been subset for the MID Profile based on JDK 1.3.
 * In the full API, the class Date had two additional functions.
 * It allowed the interpretation of dates as year, month, day, hour,
 * minute, and second values. It also allowed the formatting and
 * parsing of date strings. Unfortunately, the API for these functions
 * was not amenable to internationalization. As of JDK 1.1, the Calendar
 * class should be used to convert between dates and time fields and
 * the DateFormat class should be used to format and parse date strings.
 * The corresponding methods in Date are deprecated.
 * <p>
 * Although the Date class is intended to reflect coordinated universal
 * time (UTC), it may not do so exactly, depending on the host environment
 * of the Java Virtual Machine. Nearly all modern operating systems assume
 * that 1 day = 24x60x60 = 86400 seconds in all cases. In UTC, however,
 * about once every year or two there is an extra second, called a "leap
 * second." The leap second is always added as the last second of the
 * day, and always on December 31 or June 30. For example, the last minute
 * of the year 1995 was 61 seconds long, thanks to an added leap second.
 * Most computer clocks are not accurate enough to be able to reflect the
 * leap-second distinction.
 *
 * @see		TimeZone
 * @see		Calendar
 * @version	1.0 (J2ME MIDP)
 */
public class Date {
  private long millis;

  /**
   * Allocates a <code>Date</code> object and initializes it to 
   * represent the current time specified number of milliseconds since the 
   * standard base time known as "the epoch", namely January 1, 
   * 1970, 00:00:00 GMT. 
   * @see     java.lang.System#currentTimeMillis()
   */
  public Date() {
    this(System.currentTimeMillis());
  }

  /**
   * Allocates a <code>Date</code> object and initializes it to 
   * represent the specified number of milliseconds since the 
   * standard base time known as "the epoch", namely January 1, 
   * 1970, 00:00:00 GMT. 
   *
   * @param   date   the milliseconds since January 1, 1970, 00:00:00 GMT.
   * @see     java.lang.System#currentTimeMillis()
   */
  public Date(long date) {
    millis = date;
  }

  /**
   * Returns the number of milliseconds since January 1, 1970, 00:00:00 GMT
   * represented by this <tt>Date</tt> object.
   *
   * @return  the number of milliseconds since January 1, 1970, 00:00:00 GMT
   *          represented by this date.
   *
   * @see #setTime 
   */
  public long getTime() {
    return millis;
  }

  /**
   * Sets this <tt>Date</tt> object to represent a point in time that is 
   * <tt>time</tt> milliseconds after January 1, 1970 00:00:00 GMT.
   *
   * @param   time   the number of milliseconds.
   *
   * @see #getTime
   */
  public void setTime(long time) {
    millis = time;
  }

  /**
   * Compares two dates for equality.
   * The result is <code>true</code> if and only if the argument is 
   * not <code>null</code> and is a <code>Date</code> object that 
   * represents the same point in time, to the millisecond, as this object.
   * <p>
   * Thus, two <code>Date</code> objects are equal if and only if the 
   * <code>getTime</code> method returns the same <code>long</code> 
   * value for both. 
   *
   * @param   obj   the object to compare with.
   * @return  <code>true</code> if the objects are the same;
   *          <code>false</code> otherwise.
   * @see     java.util.Date#getTime()
   */
  public boolean equals(Object obj) {
    return obj != null && obj instanceof Date && getTime() == ((Date) obj).getTime();
  }

  /**
   * Returns a hash code value for this object. The result is the 
   * exclusive OR of the two halves of the primitive <tt>long</tt> 
   * value returned by the {@link Date#getTime} 
   * method. That is, the hash code is the value of the expression:
   * <blockquote><pre>
   * (int)(this.getTime()^(this.getTime() >>> 32))</pre></blockquote>
   *
   * @return  a hash code value for this object. 
   */
  public int hashCode() {
    long ht = getTime();
    return (int) ht ^ (int) (ht >> 32);
  }
}
