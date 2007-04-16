/*
 * 	
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.lcdui;

import com.sun.midp.i3test.*;

public class TestTwo extends TestCase {

    public void testCreate() {
        Gauge g = new Gauge("GaugeTest", true, 100, 50);
        assertEquals(100, g.maxValue);
        assertEquals(50, g.value);
        assertNotNull(g.gaugeLF);
        assertTrue(g.interactive);
    }

    public void testCheckValueIndefinite() {
        Gauge g = new Gauge("GaugeTest", false, 100, 50);

        // value out of range should get set to CONTINUOUS_IDLE
        g.setMaxValue(Gauge.INDEFINITE);
        assertEquals(Gauge.CONTINUOUS_IDLE, g.value);
    }

    public void testCheckValueAbove() {
        Gauge g = new Gauge("GaugeTest", true, 47, 29);

        // value above top of range should get set to MAX
        g.setMaxValue(22);
        assertEquals(22, g.value);
    }

    public void testCheckValueBelow() {
        Gauge g = new Gauge("GaugeTest", true, 100, 50);

        // value less than zero should get set to zero
        g.setValue(-17);
        assertEquals(0, g.value);
    }

    // test driver

    public void runTests() {
        declare("testCreate");
        testCreate();
        declare("testCheckValueIndefinite");
        testCheckValueIndefinite();
        declare("testCheckValueAbove");
        testCheckValueAbove();
        declare("testCheckValueBelow");
        testCheckValueBelow();
    }

}
