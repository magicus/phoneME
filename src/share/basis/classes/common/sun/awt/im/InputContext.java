/*
 * @(#)InputContext.java	1.15 06/10/10
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

import java.awt.Component;
import java.util.Locale;
import java.awt.AWTEvent;
import java.awt.Toolkit;
import java.awt.event.FocusEvent;
import java.awt.event.ComponentEvent;
//import sun.awt.SunToolkit;

/**
 * An InputContext object manages the communication between text editing
 * components and input methods. It dispatches events between them, and
 * forwards requests for information from the input method to the text
 * editing component.
 *
 * <p>
 * By default, one InputContext instance is created per Window instance,
 * and this input context is shared by all components within the window's
 * container hierarchy. However, this means that only one text input
 * operation is possible at any one time within a window, and that the
 * text needs to be committed when moving the focus from one text component
 * to another. If this is not desired, text components can create their
 * own input context instances.
 *
 * @see java.awt.Component#getInputContext
 * @version 1.10, 08/19/02
 * @author Sun Microsystems, Inc.
 */

public class InputContext extends java.awt.im.InputContext {
    private InputMethod inputMethod;
    private boolean inputMethodCreationFailed;
    private Component currentClientComponent;
    /**
     * Constructs an InputContext.
     */
    protected InputContext() {}

    /**
     * Dispatches an event to the active input method. Called by AWT.
     *
     * @param event The event
     */
    public synchronized void dispatchEvent(AWTEvent event) {
        InputMethod inputMethod = getInputMethod();
        int id = event.getID();
        switch (id) {
        case ComponentEvent.COMPONENT_HIDDEN:
            dispose();
            currentClientComponent = null;
            break;

        case FocusEvent.FOCUS_GAINED:
            activate((Component) event.getSource());
            if (inputMethod != null)
                inputMethod.dispatchEvent(event);
            break;

        case FocusEvent.FOCUS_LOST:
            if (inputMethod != null)
                inputMethod.dispatchEvent(event);
            deactivate((Component) event.getSource());
            break;

        default:
            if (inputMethod != null)
                inputMethod.dispatchEvent(event);
        }
    }

    /**
     * Activates input context for the given component. If the input
     * context previously was active for a different component and there
     * is uncommitted text for that component, the text is committed.
     * Called by AWT when the keyboard focus moves to the component.
     *
     * @param client The client component into which text is entered.
     */
    synchronized void activate(Component client) {
        currentClientComponent = client;
        if (inputMethod != null) {
            if (inputMethod instanceof InputMethodAdapter) {
                ((InputMethodAdapter) inputMethod).setClientComponent(client);
            }
            inputMethod.activate();
        }
    }
    
    /**
     * Deactivates input context for the given component. Called by AWT
     * when the keyboard focus moves away from the component.
     *
     * @param target The component into which text was entered.
     */
    synchronized void deactivate(Component client) {
        if (inputMethod != null) {
            inputMethod.deactivate();
        }
        currentClientComponent = null;
    }

    /**
     * Returns the client component.
     */
    Component getClientComponent() {
        Component client = currentClientComponent;
        return client;
    }

    /**
     * Disposes of the input context and release the resources used by it.
     * Called by AWT.
     */
    public void dispose() {
        if (inputMethod != null) {
            inputMethod.dispose();
            inputMethod = null;
        }
    }

    InputMethod getInputMethod() {
        if (inputMethod != null) {
            return inputMethod;
        }
        if (inputMethodCreationFailed) {
            return null;
        }
        inputMethodCreationFailed = true;
/***
**        try {
**            Toolkit toolkit = Toolkit.getDefaultToolkit();
**            inputMethod = ((SunToolkit) toolkit).getInputMethodAdapter();
**        } catch (Exception e) {
**            // there are a number of bad things that can happen while creating
**            // the input method. In any case, we just continue without an
**            // input method.
**            inputMethodCreationFailed = true;
**        }
**        if (inputMethod != null) {
**            inputMethod.setInputContext((InputMethodContext)this);
**        }
***/
        return inputMethod;
    }

    public boolean selectInputMethod(Locale locale) {
	if (locale == null) 
 	   throw new NullPointerException("Locale is null");
        return false;
    }

    boolean compositionEnabled = false;
    public void setCompositionEnabled(boolean enable) {
        compositionEnabled = enable;
    }

    public boolean isCompositionEnabled() {
        return compositionEnabled;
    }

    public void removeNotify(Component client) {
	if (client == null) 
 	   throw new NullPointerException("Component is null");
    }

}
