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

package com.sun.cldc.util.j2me;

import java.util.*;
import java.io.*;

/** This class is an implementation of the subsetted CLDC
 * Calendar class.
 *
 * @see    java.util.TimeZone
 */
public class CalendarImpl extends Calendar {

  public CalendarImpl() {
    super();
  }

  /**
   * Gets this CalendarImpl's current time as a long.
   * @return the current time as UTC milliseconds from the epoch.
   */
  public long getMilliseconds() { return getTimeInMillis(); }

  /**
   * Sets this calendar's current time from the given long value.
   * @param millis the new time in UTC milliseconds from the epoch.
   */
  public void setMilliseconds(long millis) { setTimeInMillis(millis); }

  /** Write the packed time value to a data output object. (The packed time is
   * not the milliseconds, but is the local time (not including timezone
   * packed into a long integer.)
   * <p>
   * It packs the calendar fields into a pair of integers, then uses
   * <code>output.writeInt()</code> to write the integers.
   * The time is packed into the first integer, followed by the date.
   * <br>
   * time: millis (bit 0-9), seconds (bit 10-15), minutes (bit 16-21),
   * hours (bit 22-26)
   * <br>
   * date: day of month (bit 0-4), month (bit 5-8), year (bit 9->)
   *
   * @param output the output object to write to
   *
   * @see #read(DataInput)
   */
  public void write(DataOutput output) throws IOException {
    int packed_time;
    int packed_date;

    packed_time = (get(HOUR_OF_DAY) << 22) | (get(MINUTE) << 16) |
                  (get(SECOND) << 10) | get(MILLISECOND);
    packed_date = (get(YEAR) << 9) | (get(MONTH) << 5) | get(DATE);
    output.writeInt(packed_time);
    output.writeInt(packed_date);
  }

  /** Read the packed time value from a data input object.
   * The packed time is not the milliseconds, but is the local
   * time (not timezone) packed into a pair of integers.
   * The time is packed into the first integer, followed by the date.
   * <br>
   * time: millis (bit 0-9), seconds (bit 10-15), minutes (bit 16-21),
   * hours (bit 22-26)
   * <br>
   * date: day of month (bit 0-4), month (bit 5-8), year (bit 9->)
   * <p>
   * It uses <code>input.readInt()</code> to get an integer
   * packed representation of the date. Then it sets the calendar's
   * fields using this date.
   *
   * @param input the input object to read from
   */
  public void read(DataInput input) throws IOException {
    int packed_time = input.readInt();
    int packed_date = input.readInt();

    set(HOUR_OF_DAY, packed_time >> 22);
    set(MINUTE, packed_time >> 16);
    set(SECOND, packed_time >> 10);
    set(MILLISECOND, packed_time);

    set(YEAR, packed_date >> 9);
    set(MONTH, packed_date >> 5);
    set(DATE, packed_date);
  }

  private static final String[] months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  private final String twoDigitString(int field) {
    if (field == 0)
      return "00";
    else if (field < 10)
      return "0"+Integer.toString(field);
    else
      return Integer.toString(field);
  }

  /**
   * Create a string representing the date and time using the following format:<br>
   * <code>Sun, 06 Nov 1994 08:49:37 GMT</code><br>
   * (The preferred Internet standard, 
   * a fixed-length subset of that defined by RFC 1123, which is an update to RFC 822).
   */
  public String toString() {
    String str;
    switch (get(DAY_OF_WEEK)) {
    case SUNDAY:    str = "Sun, "; break;
    case MONDAY:    str = "Mon, "; break;
    case TUESDAY:   str = "Tue, "; break;
    case WEDNESDAY: str = "Wed, "; break;
    case THURSDAY:  str = "Thu, "; break;
    case FRIDAY:    str = "Fri, "; break;
    case SATURDAY:  str = "Sat, "; break;
    default: str = get(DAY_OF_WEEK) + ", "; break;
    }
    return str +
      twoDigitString(get(DATE)) + " " +
      months[get(MONTH)] + " " +
      get(YEAR) + " " +
      twoDigitString(get(HOUR_OF_DAY)) + ":" +
      twoDigitString(get(MINUTE)) + ":" +
      twoDigitString(get(SECOND)) + " " +
      getTimeZone().getID();
  }
}
