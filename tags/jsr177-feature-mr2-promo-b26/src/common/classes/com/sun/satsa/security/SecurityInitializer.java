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
package com.sun.satsa.security;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.midp.security.SecurityInitializerImpl;

/**
 * A utility class that initializes internal security token for 
 * JSR 177 implemenation classes. Modify this class instead of
 * com.sun.midp.security.SecurityInitializer each time another 
 * JSR 177 implementation class requires initializing security token.
 */
public final class SecurityInitializer {

    /** List of the trusted subsystem classes that can request for token */
    final static private String[] trustedClasses = new String[] {
        "com.sun.midp.io.j2me.apdu.Protocol$SecurityTrusted",
        "com.sun.satsa.acl.ACSlot$SecurityTrusted",
        "com.sun.midp.io.j2me.jcrmi.Protocol$SecurityTrusted",
        "com.sun.satsa.pki.PKIManager$SecurityTrusted"
    };

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {};

    /**
     * Internal implementation of the JSR security initializer
     * redispatching security token requested from the core
     * security initializer
     */
    private static SecurityInitializerImpl impl =
        new SecurityInitializerImpl(
            com.sun.midp.security.SecurityInitializer.requestToken(
                new SecurityTrusted()),
            trustedClasses);

    /**
     * Hand out internal security token to trusted requesters
     *
     * @param trusted object to check whether token can be given to caller
     * @return if the object is really trusted to requested
     */
    final public static SecurityToken requestToken(
            ImplicitlyTrustedClass trusted) {
        return impl.requestToken(trusted);
    }
}
