/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.module.push;

import com.sun.jump.module.JUMPModuleFactory;
import java.util.Map;
import sun.misc.MIDPConfig;

/** Factory to initialize push module. */
public final class PushFactoryImpl extends JUMPModuleFactory {
    /** Name of MIDP component factory class. */
    private static final String FACTORY_CLASS_NAME =
            "com.sun.midp.jump.push.executive.PushRegistryModuleFactoryImpl";

    /** MIDP component-side push module factory. */
    private JUMPModuleFactory pushModuleFactory = null;

    /** {@inheritDoc} */
    public void load(final Map map) {
        try {
            final Class cls = loadMIDPClass(FACTORY_CLASS_NAME);
            pushModuleFactory = (JUMPModuleFactory) (cls.newInstance());
            pushModuleFactory.load(map);
        } catch (ClassNotFoundException cnfe) {
            logError("failed to load Push module: " + cnfe);
        } catch (InstantiationException ie) {
            logError("failed to load Push module: " + ie);
        } catch (IllegalAccessException iae) {
            logError("failed to load Push module: " + iae);
        }
    }

    /** {@inheritDoc} */
    public void unload() {
        if (pushModuleFactory == null) {
            throw new IllegalStateException("attempt to unload push module"
                    + " before load");
        }
        pushModuleFactory.unload();
        pushModuleFactory = null;
    }

    /**
     * Gets MIDP class loader.
     *
     * @return MIDP class loader
     */
    private static ClassLoader getMIDPClassLoader() {
        /*
         * IMPL_NOTE: technically speaking might return <code>null</code>,
         * but it shouldn't happen if initialization order is correct.
         */
        final ClassLoader cl = MIDPConfig.getMIDPImplementationClassLoader();
        if (cl == null) {
            logError("failed to fetch MIDP class loader, got null instead");
        }
        return cl;
    }

    /**
     * Loads MIDP class.
     *
     * <p>
     * Throws all the exceptions {@link java.lang.Class#forName} throws.
     * </p>
     *
     * @param className name of class to load
     * @return loaded class
     * @throws ClassNotFoundException if unable to find a class
     */
    private static Class loadMIDPClass(final String className)
            throws ClassNotFoundException {
        return Class.forName(className, true, getMIDPClassLoader());
    }

    /**
     * Logs an error.
     *
     * <p>
     * TBD: proper logging
     * </p>
     *
     * @param message message to log
     */
    private static void logError(final String message) {
        System.err.println("[ERROR] PushFactoryImpl: " + message);
    }
}
