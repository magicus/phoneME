/*
 * @(#)InputMethodAdapter.java	1.12 06/10/10
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

/**
 * An input method adapter interfaces with the native input methods
 * on a host platform. In general, it looks to the input method
 * framework like a Java input method (that may support a few more
 * locales than a typical Java input method). However, since it
 * often has to work in a slightly hostile environment that's not
 * designed for easy integration into the Java input method
 * framework, it gets some special treatment that's not available
 * to Java input methods.
 *
 * @version 1.8 08/19/02
 * @author Sun Microsystems, Inc.
 */

public abstract class InputMethodAdapter extends InputMethod {
    private Component clientComponent;
    void setClientComponent(Component client) {
        clientComponent = client;
    }

    protected Component getClientComponent() {
        return clientComponent;
    }
}
