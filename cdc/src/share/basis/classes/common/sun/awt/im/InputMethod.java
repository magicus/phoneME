/*
 * @(#)InputMethod.java	1.12 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */

package sun.awt.im;

import java.util.Locale;
import java.awt.AWTEvent;

/**
 * InputMethod class is an abstract class for input method.
 * Input method which is implemented as a subclass of this class can be
 * plugged in JDK.
 *
 * @version 1.8 08/19/02
 * @author Sun Microsystems, Inc.
 */

public abstract class InputMethod {
    protected InputMethodContext inputContext;
    /**
     * Constructs a new input method.
     */
    protected InputMethod() {}

    /**
     * Sets the input context which can dispatch input method
     * events to the client component.
     */
    public void setInputContext(InputMethodContext context) {
        inputContext = context;
    }

    /**
     * Dispatch event to input method. InputContext dispatch event with this
     * method. Input method set consume flag if event is consumed in
     * input method.
     *
     * @param e event
     */
    public abstract void dispatchEvent(AWTEvent e);

    /**
     * Activate input method.
     */
    public abstract void activate();

    /**
     * Deactivate input method.
     */
    public abstract void deactivate();

    /**
     * Ends any input composition that may currently be going on in this
     * context. Depending on the platform and possibly user preferences,
     * this may commit or delete uncommitted text. Any changes to the text
     * are communicated to the active component using an input method event.
     *
     * <p>
     * A text editing component may call this in a variety of situations,
     * for example, when the user moves the insertion point within the text
     * (but outside the composed text), or when the component's text is
     * saved to a file or copied to the clipboard.
     *
     */
    public abstract void endComposition();

    /**
     * Disposes of the input method and releases the resources used by it.
     */
    public abstract void dispose();
}
