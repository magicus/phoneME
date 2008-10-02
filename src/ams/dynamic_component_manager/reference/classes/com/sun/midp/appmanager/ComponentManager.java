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

package com.sun.midp.appmanager;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

/**
 * Component Manager
 *
 * Implements a built-in MIDlet that manages the dynamically installed components.
 */
public class ComponentManager extends MIDlet {
    /**
     * Signals the <code>MIDlet</code> that it has entered the
     * <em>Active</em> state.
     * In the <em>Active</EM> state the <code>MIDlet</code> may
     * hold resources.
     * The method will only be called when
     * the <code>MIDlet</code> is in the <em>Paused</em> state.
     * <p/>
     * Two kinds of failures can prevent the service from starting,
     * transient and non-transient.  For transient failures the
     * <code>MIDletStateChangeException</code> exception should be thrown.
     * For non-transient failures the <code>notifyDestroyed</code>
     * method should be called.
     * <p/>
     * If a Runtime exception occurs during <code>startApp</code> the
     * MIDlet will be
     * destroyed immediately.  Its <code>destroyApp</code> will be
     * called allowing
     * the MIDlet to cleanup.
     *
     * @throws javax.microedition.midlet.MIDletStateChangeException
     *          is thrown
     *          if the <code>MIDlet</code>
     *          cannot start now but might be able to start at a
     *          later time.
     */
    protected void startApp() throws MIDletStateChangeException {
    }

    /**
     * Signals the <code>MIDlet</code> to enter
     * the <em>Paused</em> state.
     * In the <em>Paused</em> state the <code>MIDlet</code> must
     * release shared resources
     * and become quiescent. This method will only be called
     * called when the <code>MIDlet</code> is in the <em>Active</em> state. <p>
     * <p/>
     * If a Runtime exception occurs during <code>pauseApp</code> the
     * MIDlet will be destroyed immediately.  Its
     * <code>destroyApp</code> will be called allowing
     * the MIDlet to cleanup.
     */
    protected void pauseApp() {
    }

    /**
     * Signals the <code>MIDlet</code> to terminate and enter the
     * <em>Destroyed</em> state.
     * In the destroyed state the <code>MIDlet</code> must release
     * all resources and save any persistent state. This method may
     * be called from the <em>Paused</em> or
     * <em>Active</em> states. <p>
     * <code>MIDlet</code>s should
     * perform any operations required before being terminated, such as
     * releasing resources or saving preferences or
     * state. <p>
     * <p/>
     * <strong>Note:</strong> The <code>MIDlet</code> can request that
     * it not enter the <em>Destroyed</em>
     * state by throwing an <code>MIDletStateChangeException</code>. This
     * is only a valid response if the <code>unconditional</code>
     * flag is set to <code>false</code>. If it is <code>true</code>
     * the <code>MIDlet</code> is assumed to be in the <em>Destroyed</em> state
     * regardless of how this method terminates. If it is not an
     * unconditional request, the <code>MIDlet</code> can signify that it
     * wishes to stay in its current state by throwing the
     * <code>MIDletStateChangeException</code>.
     * This request may be honored and the <code>destroy()</code>
     * method called again at a later time.
     * <p/>
     * <p>If a Runtime exception occurs during <code>destroyApp</code> then
     * they are ignored and the MIDlet is put into the <em>Destroyed</em>
     * state.
     *
     * @param unconditional If true when this method is called, the
     *                      <code>MIDlet</code> must cleanup and release all resources.  If
     *                      false the <code>MIDlet</code> may throw
     *                      <CODE>MIDletStateChangeException</CODE> to indicate it does not
     *                      want to be destroyed at this time.
     * @throws javax.microedition.midlet.MIDletStateChangeException
     *          is thrown
     *          if the <code>MIDlet</code> wishes to continue to
     *          execute (Not enter the <em>Destroyed</em> state).
     *          This exception is ignored if <code>unconditional</code>
     *          is equal to <code>true</code>.
     */
    protected void destroyApp(boolean unconditional) throws MIDletStateChangeException {
    }
}
