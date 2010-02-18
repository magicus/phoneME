/*
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

package com.sun.midp.push.reservation.impl;

import javax.microedition.io.ConnectionNotFoundException;

import com.sun.j2me.security.AccessControlContext;
import com.sun.j2me.security.AccessControlContextAdapter;

import com.sun.midp.push.reservation.ProtocolFactory;
import com.sun.midp.push.reservation.ReservationDescriptor;

import junit.framework.TestCase;

public final class RDFactoryTest extends TestCase {
    private static final AccessControlContext NOOP_ACCESS_CONTROL_CONTEXT =
        new AccessControlContextAdapter() {
            /** {@inheritDoc} */
            public void checkPermissionImpl(
                String name, String resource, String extraValue) {}
    };

    private void _testInvalidConnectionName(final String connectionName) {

        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                // Shouldn't ever get here
                throw new RuntimeException("Should fail earlier");
            }
        });
        try {
            factory.getDescriptor(connectionName, "",
				  NOOP_ACCESS_CONTROL_CONTEXT);
            fail("IAE should be thrown");
        } catch (IllegalArgumentException iae) {
            // Ignored
        } catch (ConnectionNotFoundException e) {
            fail("Unexpected CNFE");
        }
    }

    private void _testValidConnectionName(final String connectionName) {
        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                return Common.STUB_PROTOCOL_FACTORY;
            }
        });
        try {
            final ReservationDescriptor rd = factory
                .getDescriptor(connectionName, "", NOOP_ACCESS_CONTROL_CONTEXT);
            assertNotNull(rd);
        } catch (ConnectionNotFoundException e) {
            fail("Unexpected CNFE");
        }
    }

    public void testEmptyConnectionName() {
        _testInvalidConnectionName("");
    }

    public void testNoColonConnectionName() {
        _testInvalidConnectionName("foo");
    }

    public void testLeadingDigitProtocolName() {
        _testInvalidConnectionName("1:bar");
    }

    public void testLeadingPlusProtocolName() {
        _testInvalidConnectionName("+:bar");
    }

    public void testLeadingMinusProtocolName() {
        _testInvalidConnectionName("-:bar");
    }

    public void testLeadingDotProtocolName() {
        _testInvalidConnectionName(".:bar");
    }

    public void testLeadingBadCharProtocolName() {
        _testInvalidConnectionName("@:bar");
    }

    public void testInvalidInternalCharProtocolName1() {
        _testInvalidConnectionName("f@A-+.:bar");
    }

    public void testInvalidInternalCharProtocolName2() {
        _testInvalidConnectionName("fA@-+.:bar");
    }

    public void testInvalidInternalCharProtocolName3() {
        _testInvalidConnectionName("fA-@+.:bar");
    }

    public void testInvalidInternalCharProtocolName4() {
        _testInvalidConnectionName("fA-+@.:bar");
    }

    public void testInvalidInternalCharProtocolName5() {
        _testInvalidConnectionName("fA-+.@:bar");
    }
    
    private void _testValidProtocol(final String protocol) {
        _testValidConnectionName(protocol + ":bar");
    }

    public void testValidLeadingCharProtocolName() {
        _testValidProtocol(Common.VALID_ONE_CHAR_PROTOCOL);
    }

    public void testValidSecondAlphaCharProtocolName() {
        _testValidProtocol(Common.VALID_SECOND_ALPHA_CHAR_PROTOCOL);
    }

    public void testValidSecondDigitCharProtocolName() {
        _testValidProtocol(Common.VALID_SECOND_DIGIT_CHAR_PROTOCOL);
    }

    public void testValidSecondPlusCharProtocolName() {
        _testValidProtocol(Common.VALID_SECOND_PLUS_CHAR_PROTOCOL);
    }

    public void testValidSecondMinusCharProtocolName() {
        _testValidProtocol(Common.VALID_SECOND_MINUS_CHAR_PROTOCOL);
    }

    public void testValidSecondDotCharProtocolName() {
        _testValidProtocol(Common.VALID_SECOND_DOT_CHAR_PROTOCOL);
    }

    public void testValidProtocolName() {
        _testValidProtocol(Common.VALID_PROTOCOL);
    }

    public void testValidEmptyTargetAndParms() {
        _testValidConnectionName("foo:");
    }

    public void testValidWhateverTargetAndParms() {
        _testValidConnectionName("foo:1@_  ");
    }

    public void testLowercaseProtocol() {
        final String PROTOCOL = "fOo-B.34Ar+12.3";
        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                assertEquals(PROTOCOL.toLowerCase(), protocol);
                return Common.STUB_PROTOCOL_FACTORY;
            }
        });
        try {
            final ReservationDescriptor rd = factory.getDescriptor(
                    PROTOCOL + ":oops", "",
                    NOOP_ACCESS_CONTROL_CONTEXT);
            assertNotNull(rd);
        } catch (ConnectionNotFoundException cnfe) {
            fail("Unexpected CNFE");
        }
    }

    public void testCreateDescriptorInvokedCorrectly() {
        final boolean [] hasBeenInvoked = { false };
        final String PROTOCOL = "fO.o+B-aR";
        final String TARGET_AND_PARAMS = ":) target-and-guess-what :)";
        final String FILTER = "and : eve#n a @filter- with  + specials";

        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                return new ProtocolFactory() {
                    public ReservationDescriptor createDescriptor(
                            final String protocol,
                            final String targetAndParams,
                            final String filter,
                            final AccessControlContext context)
                                throws IllegalArgumentException,
                                    SecurityException {
                        hasBeenInvoked[0] = true;
                        assertEquals(PROTOCOL.toLowerCase(), protocol);
                        assertEquals(TARGET_AND_PARAMS, targetAndParams);
                        assertEquals(FILTER, filter);
                        assertNotNull(context);
                        return Common.STUB_RESERVATION_DESCR;
                    }
                };
            }
        });
        try {
            final ReservationDescriptor rd = factory.getDescriptor(
                    PROTOCOL + ":" + TARGET_AND_PARAMS,
                    FILTER, NOOP_ACCESS_CONTROL_CONTEXT);
            assertNotNull(rd);
        } catch (ConnectionNotFoundException cnfe) {
            fail("Unexpected CNFR");
        }
        assertTrue(hasBeenInvoked[0]);
    }

    public void testCreateDescriptorPropogatesIAE() {
        final String PROTOCOL = "fO.o+B-aR";

        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                return new ProtocolFactory() {
                    public ReservationDescriptor createDescriptor(
                            final String protocol,
                            final String targetAndParams,
                            final String filter,
                            final AccessControlContext context)
                                throws IllegalArgumentException,
                                    SecurityException {
                        throw new IllegalArgumentException("unittesting");
                    }
                };
            }
        });
        try {
            factory.getDescriptor(PROTOCOL + ":", "",
				  NOOP_ACCESS_CONTROL_CONTEXT);
            fail("IAE should be thrown");
        } catch (IllegalArgumentException iae) {
            // Ignored
        } catch (ConnectionNotFoundException cnfe) {
            fail("Unexpected CNFE");
        }
    }

    public void testCreateDescriptorPropogatesSE() {
        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                return new ProtocolFactory() {
                    public ReservationDescriptor createDescriptor(
                            final String protocol,
                            final String targetAndParams,
                            final String filter,
                            final AccessControlContext context)
                                throws IllegalArgumentException,
                                    SecurityException {
                        context.checkPermission("", "", "");
                        throw new RuntimeException("shouldn't get here");
                    }
                };
            }
        });
        try {
            factory.getDescriptor("foo:", "",
                    new AccessControlContextAdapter() {
                        public void checkPermissionImpl(
                                final String permissionName,
                                final String resource, final String extraValue)
                                    throws SecurityException {
                            throw new SecurityException("unittesting");
                        }
            });
            fail("SE should be thrown");
        } catch (SecurityException se) {
            // Ignored
        } catch (ConnectionNotFoundException cnfe) {
            fail("Unexpected CNFE");
        }
    }

    public void testConnectionNotFoundException() {
        final RDFactory factory = new RDFactory(new ProtocolRegistry() {
            public ProtocolFactory get(final String protocol) {
                // Emulate factory which supports no protocols
                return null;
            }
        });
        try {
            factory.getDescriptor("foo:", "", NOOP_ACCESS_CONTROL_CONTEXT);
            fail("CNFE should be thrown");
        } catch (ConnectionNotFoundException cnfe) {
            // Ignored
        }
    }
}
