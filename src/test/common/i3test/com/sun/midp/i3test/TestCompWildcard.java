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

package com.sun.midp.i3test;


public class TestCompWildcard extends TestCase {

    /**
     * Compare the filter and source address.
     * @param filter The filter string to be used
     * @param src The incoming address to be tested by the filter
     * @return <tt>true</tt> if the comparison is successful,
     * <tt>false</tt> if it fails
     */
    private native boolean cmpWildCard(String filter, String src);

    /**
     * Runs all the tests.
     */
    public void runTests() {
        declare("Compare * 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("*", "1.2.3.4"));
        declare("Compare 1.* 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("1.*", "1.2.3.4"));
        declare("Compare 1.** 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("1.**", "1.2.3.4"));
        declare("Compare *4 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("*4", "1.2.3.4"));
        declare("Compare 1.?.3.4 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("1.?.3.4", "1.2.3.4"));
        declare("Compare ?*? 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("?*?", "1.2.3.4"));
        declare("Compare ?**?3?*4 1.2.3.4");
        assertTrue("Wrong compare", cmpWildCard("?**?3?*4", "1.2.3.4"));
        declare("Compare *5* 1.2.3.4");
        assertTrue("Wrong compare", !cmpWildCard("*5*", "1.2.3.4"));
        declare("Compare 1.2.3.?? 1.2.3.4");
        assertTrue("Wrong compare", !cmpWildCard("1.2.3.??", "1.2.3.4"));
        declare("Compare *.??.* 1.2.3.4");
        assertTrue("Wrong compare", !cmpWildCard("*.??.*", "1.2.3.4"));
        declare("Compare *ab* axab");
        assertTrue("Wrong compare", cmpWildCard("*ab*", "axab"));
        declare("Compare *aab aaab");
        assertTrue("Wrong compare", cmpWildCard("*aab", "aaab"));
        declare("Compare *abcde*aab uuabcdeuuauuabcdeuuaab");
        assertTrue("Wrong compare", cmpWildCard("*abcde*aab", "uuabcdeuuauuabcdeuuaab"));
    }

}

