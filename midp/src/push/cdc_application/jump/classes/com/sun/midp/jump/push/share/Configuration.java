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

package com.sun.midp.jump.push.share;

import com.sun.midp.jump.push.executive.remote.MIDPContainerInterface;
import com.sun.midp.push.reservation.ReservationDescriptor;
import com.sun.midp.push.reservation.impl.ReservationDescriptorFactory;
import javax.microedition.io.ConnectionNotFoundException;

import com.sun.j2me.security.AccessControlContext;


/**
 * Configuration of Push system.
 *
 * <p>
 * Provides access to common entities to be used in Push (e.g.
 * reservatin descriptor factory)
 * </p>
 */
public final class Configuration {
    /** Hidden constructor. */
    private Configuration() { }

    /**
     * Gets a reference to reservation descriptor factory to use.
     *
     * @return an instance of ReservationDescriptorFactory
     * (cannot be <code>null</code>)
     */
    public static ReservationDescriptorFactory
            getReservationDescriptorFactory() {
        // TBD: implement (dynamic loading?)
        return new ReservationDescriptorFactory() {
            public ReservationDescriptor getDescriptor(
                    final String connectionName, final String filter,
                    final AccessControlContext context)
                        throws  IllegalArgumentException,
                                ConnectionNotFoundException {
                throw new ConnectionNotFoundException("no supported protocols");
            }
        };
    }

    /** MIDP container interface IXC resource URI. */
    public static final String MIDP_CONTAINER_INTERFACE_IXC_URI =
            MIDPContainerInterface.class.getName();
}
