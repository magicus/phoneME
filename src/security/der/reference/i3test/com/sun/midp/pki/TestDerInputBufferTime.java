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

package com.sun.midp.pki;
import com.sun.midp.i3test.*;
import java.util.*;
import java.io.IOException;

public class TestDerInputBufferTime extends TestCase {

    void testTime(byte tag) {
        //TimeZone tz = TimeZone.getTimeZone("GMT");
        Calendar c = Calendar.getInstance();
        c.set(Calendar.YEAR, 2009);
        c.set(Calendar.MONTH, 1);
        c.set(Calendar.DATE, 1);
        c.set(Calendar.HOUR_OF_DAY, 15);
        c.set(Calendar.MINUTE, 0);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        long idate = c.getTime().getTime();
        c.set(Calendar.YEAR, 2010);
        long edate = c.getTime().getTime();
        for (; idate <= edate; idate += 600000) {
            Date din = new Date(idate);
            DerOutputStream ost = new DerOutputStream();
            try {
                if (tag == DerValue.tag_UtcTime) {
                    ost.putUTCTime(din);
                    ost.putUTCTime(din);
                } else {
                    ost.putGeneralizedTime(din);
                    ost.putGeneralizedTime(din);
                }
            } catch(IOException e) {
                assertFalse("Failed to write to DerOutputStream", true);
                e.printStackTrace();
                return;
            }
            byte[] ba = ost.toByteArray();
            DerInputStream ist;
            Date dout = null;
            try {
                ist = new DerInputStream(ba);
                if (tag == DerValue.tag_UtcTime) {
                    dout = ist.getUTCTime();
                } else {
                    dout = ist.getGeneralizedTime();
                }
            } catch(IOException e) {
                assertFalse("Failed to read from DerInputStream", true);
                e.printStackTrace();
                return;
            }
            if (!din.equals(dout)) {
                assertFalse("Encoded Date does not match decoded Date ", false);
                System.err.println("din: " + din);
                System.err.println("dout: " + dout);
                return;
            }
        }
        assertTrue(true);
    }


    
    /**
     * Run tests
     */
    public void runTests() {
        declare("testUTCTime");
        testTime(DerValue.tag_UtcTime);
        declare("testGeneralizedTime");
        testTime(DerValue.tag_GeneralizedTime);
    }
}