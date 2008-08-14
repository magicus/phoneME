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

import com.sun.midp.push.reservation.ProtocolFactory;
import junit.framework.TestCase;

public final class MapProtocolRegistryTest extends TestCase {
    private void testSameInstanceInternal(final String protocol,
            final ProtocolFactory protocolFactory) {
        final MapProtocolRegistry registry = new MapProtocolRegistry();
        registry.bind(protocol, protocolFactory);
        assertSame(protocolFactory, registry.get(protocol.toLowerCase()));
    }

    private void testSameInstanceInternal(final String protocol) {
        testSameInstanceInternal(protocol, Common.STUB_PROTOCOL_FACTORY);
    }

    public void testLowerCaseProtocol() {
        testSameInstanceInternal("FOO");
    }

    public void testValidLeadingCharProtocolName() {
        testSameInstanceInternal(Common.VALID_ONE_CHAR_PROTOCOL);
    }

    public void testValidSecondAlphaCharProtocolName() {
        testSameInstanceInternal(Common.VALID_SECOND_ALPHA_CHAR_PROTOCOL);
    }

    public void testValidSecondDigitCharProtocolName() {
        testSameInstanceInternal(Common.VALID_SECOND_DIGIT_CHAR_PROTOCOL);
    }

    public void testValidSecondPlusCharProtocolName() {
        testSameInstanceInternal(Common.VALID_SECOND_PLUS_CHAR_PROTOCOL);
    }

    public void testValidSecondMinusCharProtocolName() {
        testSameInstanceInternal(Common.VALID_SECOND_MINUS_CHAR_PROTOCOL);
    }

    public void testValidSecondDotCharProtocolName() {
        testSameInstanceInternal(Common.VALID_SECOND_DOT_CHAR_PROTOCOL);
    }

    public void testValidProtocolName() {
        testSameInstanceInternal(Common.VALID_PROTOCOL);
    }

    // TBD: more tests, esp. negative ones
}
