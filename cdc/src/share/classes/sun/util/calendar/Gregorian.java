/*
 * @(#)Gregorian.java	1.7 06/10/10
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

package sun.util.calendar;

/**
 * Gregorian calendar calculations. The algorithms are from "Calendrical
 * Calculation" by Nachum Dershowitz and Edward M. Reingold (ISBN:
 * 0-521-56474-3).
 */

public class Gregorian implements CalendarSystem {

    /**
     * The number of days between Gregorian January 1, 1 and January
     * 1, 1970.
     */
    public static final int EPOCH_DATE = 719163;

    /**
     * Calculates Gregorian calendar fields from the given UTC value
     * which is the difference from Gregorian January 1, 1970 00:00:00
     * GMT.
     * @param utc the UTC value in milliseconds
     * @return the CaledarDate object that contains the calculated Gregorian calendar field values.
     */
    public static CalendarDate getCalendarDate(long utc) {
	long days;
	int millis;

	days = utc / ONE_DAY;
	millis = (int)(utc % ONE_DAY);
	if (millis < 0) {
	    millis += ONE_DAY;
	    days--;
	}
	days += EPOCH_DATE;
	CalendarDate cdate = getCalendarDateFromFixedDate(days);
	cdate.setTimeOfDay(millis);
	return cdate;
    }

    /**
     * Calculates milliseconds of given time from EPOCH, 1970-01-01 00:00AM.
     */
    public static long dateToMillis(CalendarDate date) {
	long gd = getFixedDate(date.getYear(),
			       date.getMonth(),
			       date.getDate());
	return ((gd - EPOCH_DATE) * ONE_DAY + date.getTimeOfDay());
    }

    /**
     * Calculates milliseconds of given time from EPOCH, 1970-01-01 00:00AM.
     */
    public static long dateToMillis(int year, int month, int day, 
				    int milliseconds) {
	long gd = getFixedDate(year, month, day);
	return ((gd - EPOCH_DATE) * ONE_DAY + milliseconds);
    }

    public static boolean validate(CalendarDate date) {
	int month = date.getMonth();
	if (month < JANUARY || month > DECEMBER) {
	    return false;
	}
	int days = getMonthLength(date.getYear(), month);
	if (date.getDate() <= 0 || date.getDate() > days) {
	    return false;
	}
	return true;
    }

    private static final int[] days_in_month
	= { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    /**
     * @param month 0-based
     */
    public static int getMonthLength(int year, int month) {
	int days = days_in_month[month];
	if (month == FEBRUARY && isLeapYear(year)) {
	    days++;
	}
	return days;
    }

    /**
     * Returns number of days from 0001-01-01. Counting leap correction.
     */
    public static final long getFixedDate(int year, int month, int day) {
	int prevyear = year - 1;
	month++;	// 1-based month numbering
	long days ;

	if (prevyear >= 0) {
	    days = (365 * (long)prevyear)
		   + (prevyear / 4)
		   - (prevyear / 100)
		   + (prevyear / 400)
		   + ((367 * month - 362) / 12)
		   + day;
	} else {
	    days = (365 * (long)prevyear)
		   + floorDivide(prevyear, 4)
		   - floorDivide(prevyear, 100)
		   + floorDivide(prevyear, 400)
		   + floorDivide((367 * month - 362), 12)
		   + day;
	}
	if (month > 2) {
	    days -= (isLeapYear(year)) ? 1 : 2;
	}
	return days;
    }

    /**
     * Calculates year/month/day from given date. The date is from 0001-01-01.
     */
    public static CalendarDate getCalendarDateFromFixedDate(long fixedDate) {
	int year = getYear(fixedDate);
	int pday = (int)(fixedDate - getFixedDate(year, JANUARY, 1));
	int corr = 2;
	long mar1 = getFixedDate(year, MARCH, 1);
	if (fixedDate < mar1) {
	    corr = 0;
	} else if (fixedDate >= mar1 && isLeapYear(year)) {
	    corr = 1;
	}
	int month = floorDivide((12 * (pday + corr) + 373), 367) - 1;
	int day = (int)(fixedDate - getFixedDate(year, month, 1) + 1);
	int dow = getDayOfWeekFromFixedDate(fixedDate);
	CalendarDate cdate =  new CalendarDate(year, month, day);
	cdate.setDayOfWeek(dow);
	return cdate;
    }

    /**
     * Returns day of week of given day
     */
    public static int getDayOfWeek(CalendarDate date) {
	long fixedDate = getFixedDate(date.getYear(),
				      date.getMonth(),
				      date.getDate());
	return getDayOfWeekFromFixedDate(fixedDate);
    }

    private static final int getDayOfWeekFromFixedDate(long fixedDate) {
	if (fixedDate >= 0) {
	    return (int)(fixedDate % 7) + SUNDAY;
	}
	return (int)mod(fixedDate, 7) + SUNDAY;
    }
	
    /**
     * Returns Gregorian year number of given date
     */
    private static final int getYear(long fixedDate) {
	long d0;
	int  d1, d2, d3;
	int  n400, n100, n4, n1;
	int  year;

	if (fixedDate >= 0) {
	    d0 = fixedDate - 1;
	    n400 = (int)(d0 / 146097);
	    d1 = (int)(d0 % 146097);
	    n100 = d1 / 36524;
	    d2 = d1 % 36524;
	    n4 = d2 / 1461;
	    d3 = d2 % 1461;
	    n1 = d3 / 365;
	} else {
	    d0 = fixedDate - 1;
	    n400 = (int)floorDivide(d0, 146097L);
	    d1 = (int)mod(d0, 146097L);
	    n100 = floorDivide(d1, 36524);
	    d2 = mod(d1, 36524);
	    n4 = floorDivide(d2, 1461);
	    d3 = mod(d2, 1461);
	    n1 = floorDivide(d3, 365);
	}
	year = 400 * n400 + 100 * n100 + 4 * n4 + n1;
	if (!(n100 == 4 || n1 == 4)) {
	    ++year;
	}
	return year;
    }

    /**
     * @return true if the given year is a Gregorian leap year.
     */
    public static final boolean isLeapYear(int year) {
	if (year >= 0) {
	    return (((year % 4) == 0)
		    && (((year % 100) != 0) || ((year % 400) == 0)));
	}
	return (((mod(year, 4) == 0) 
		 && ((mod(year, 100) != 0) || (mod(year, 400) == 0))));
    }

    /**
     * Floor function working with negative number. 
     * floor(3.14) = 3 and floor(-3.14) = -4.
     */
    private static final long floorDivide(long n, long d) {
	return ((n >= 0) ? 
		(n / d) : (((n + 1L) / d) - 1L));
    }

    private static final int floorDivide(int n, int d) {
	return ((n >= 0) ? 
		(n / d) : (((n + 1) / d) - 1));
    }

    private static final long mod(long x, long y) {
	return (x - y * floorDivide(x, y));
    }
    
    private static final int mod(int x, int y) {
	return (x - y * floorDivide(x, y));
    }
}
