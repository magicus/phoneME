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

package com.sun.midp.content;

import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.i3test.TestCase;

import javax.microedition.content.Registry;
import javax.microedition.content.ContentHandler;

/**
 * Test the internal functions of RegistryImpl.
 */
public class TestRegistryImpl extends ExtendedTestCase {
    /** Constant application ID for testing. */
    private static final String SUITE_ID = "GraphicalInstaller";
    /** Constant classname for testing. */
    private static final String CLASSNAME = "classname";

    /** The name and ID of the GraphicalInstaller. */
    private static final String GS_NAME = "GraphicalInstaller";
    /** The suiteId of the GraphicalInstaller. */
    private static final int GS_SUITEID = MIDletSuite.INTERNAL_SUITE_ID;
    /** The class of the GraphicalInstaller. */
    private static final String GS_CLASSNAME =
	"com.sun.midp.installer.GraphicalInstaller";
    /** The types registered for the GraphicalInstaller. */
    private String[] types = {"text/vnd.sun.j2me.app-descriptor",
                            "application/java-archive"};
    /** The suffixes registered for the GraphicalInstaller. */
    private String[] suffixes = {".jad", ".jar"};

    /** The registry to use. */
    RegistryImpl registry;

    /**
     * Run the tests.
     */
    public void runTests() {
	setup();
	test001();
	test002();
    }

    /**
     * Setup the registry.
     */
    void setup() {
	registry = getRegistry();
    }

    /**
     * Test that the built-in suite is registered.
     * The Graphical Installer must be present.
     */
    void test001() {
        declare("Verify built-in registration for GraphicalInstaller");
	try {
	    AppProxy app = AppProxy.getCurrent().forClass(GS_CLASSNAME);
	    ContentHandlerImpl ch = registry.getServer(app);

	    assertNotNull("Verify GraphicalInstaller is present", ch);
	    if (ch != null) {
		assertEquals("Verify getID",
			     GS_NAME, ch.getID());

		assertEquals("Verify classname",
			     GS_CLASSNAME, ch.classname);
		assertEquals("Verify getTypeCount",
			     2,  ch.getTypeCount());
		assertEquals("Verify Type1",
			     types[0], ch.getType(0));
		assertEquals("Verify Type2",
			     types[1], ch.getType(1));
		assertEquals("Verify getSuffixCount",
			     2, ch.getSuffixCount());
		assertEquals("Verify Suffix1",
			     suffixes[0], ch.getSuffix(0));
		assertEquals("Verify Suffix2",
			     suffixes[1], ch.getSuffix(1));
		assertEquals("Verify Action",
			     2, ch.getActionCount());
		assertEquals("Verify Action Name Maps",
			     3, ch.getActionNameMapCount());
	    }
	} catch (ClassNotFoundException cnfe) {
	    fail("Unexpected exception");
	    cnfe.printStackTrace();
	}
    }

    /**
     * Test that the value of the System property "CHAPI-Version"
     * is "1.0".
     */
    void test002() {
	declare("Check system properties");
	String ver = System.getProperty("microedition.chapi.version");
	assertEquals("Verify microedition.chapi.version", "1.0", ver);
    }

}
