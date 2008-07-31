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


/** Class loading implementation of <code>ProtocolRegistry</code>. */
public final class ClassLoadingProtocolRegistry implements ProtocolRegistry {
    /** Resolver of protocols. */
    private final ProtocolClassResolver resolver;

    /** Class loader to use. */
    private final ClassLoader classLoader;

    /**
     * Ctor.
     *
     * @param resolver resolver to use (cannot be <code>null</code>)
     * @param classLoader class loader to use (cannot be <code>null</code>)
     */
    public ClassLoadingProtocolRegistry(
            final ProtocolClassResolver resolver,
            final ClassLoader classLoader) {
        this.resolver = resolver;
        this.classLoader = classLoader;
    }

    /** {@inheritDoc} */
    public ProtocolFactory get(final String protocol) {
        /*
         * IMPL_NOTE: I don't think we need to cache protocol in HasMap:
         * assuption is that getClassName should be fast enough.  Still
         * it's an option
         */
        final String name = resolver.getClassName(protocol);
        if (name == null) {
            return null;
        }
        try {
            final Class cls = Class.forName(name, true, classLoader);
            final Object instance = cls.newInstance();
            return (ProtocolFactory) instance;
        } catch (ClassNotFoundException cnfe) {
            logWarning("Failed to instantiate protocol factory", cnfe);
            return null;
        } catch (ExceptionInInitializerError iiie) {
            logWarning("Failed to instantiate protocol factory", iiie);
            return null;
        } catch (LinkageError le) {
            logWarning("Failed to instantiate protocol factory", le);
            return null;
        } catch (InstantiationException ie) {
            logWarning("Failed to instantiate protocol factory", ie);
            return null;
        } catch (IllegalAccessException iae) {
            logWarning("Failed to instantiate protocol factory", iae);
            return null;
        } catch (ClassCastException cce) {
            logWarning("Failed to instantiate protocol factory", cce);
            return null;
        }
    }

    /**
     * Logs warning message.
     *
     * @param msg message to log
     * @param t exception to log
     */
    private static void logWarning(final String msg, final Throwable t) {
        // TBD: use system-wide logging
        System.err.println("[WARNING] " + msg + ": " + t);
    }
}
