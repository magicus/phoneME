/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui.virtualkeyboard;

import com.sun.midp.chameleon.layers.VirtualKeyboardLayer;
import com.sun.midp.lcdui.DisplayEventConsumer;
import com.sun.midp.lcdui.DisplayEventProducer;

/**
 * Virtual keyboard public API.
 */
public final class VirtualKeyboard {

    private VirtualKeyboardLayer keyboardLayer;
    private DisplayEventConsumer consumer;
    private static VirtualKeyboard singletone;

    /**
     * Private constructor.
     * @param kbdLayer  class to check virtual keyboard visibility
     *                  status
     * @param evtProducer   class to change virtual keyboard status.
     * @param csm           virtual keyboard status handler.
     */
    private VirtualKeyboard() {
    }

    /**
     * Package private initializer. Used by Display through
     * VirtualKeyboardAPIInitializer.
     * @param kbdLayer  class to check virtual keyboard visibility
     *                  status
     * @param csm           virtual keyboard status handler.
     */
     static void init1(DisplayEventConsumer  csm) {
        getInstance().consumer = csm;

    }

    /**
     * Package private initializer. Used by Display through
     * VirtualKeyboardAPIInitializer. Called when
     * VirtualKeyboardLayer is initialized.
     * @param kbdLayer  class to check virtual keyboard visibility
     *                  status
     */
    static void init2(VirtualKeyboardLayer kbdLayer) {
        getInstance().keyboardLayer = kbdLayer;
    }

    /**
     * Gets virtual keyboard manager.
     *
     * @return singletone.
     */
    public static VirtualKeyboard getInstance() {
        if (null == singletone) {
            singletone = new VirtualKeyboard();
        }
        return singletone;
    }

    /**
     * Shows virtual keyabord. Does nothing if current displayable
     * is not Canvas instance.
     */
    public void show() {
        if (!isShown() && null != consumer) {
            consumer.handleVirtualKeyboardEvent();
        }
    }

    /**
     * Hides virtual keyabord. Does nothing if current displayable
     * is not Canvas instance.
     */
    public void hide() {
        if (isShown() && null != consumer) {
            consumer.handleVirtualKeyboardEvent();
        }
    }

    /**
     * Returns virtual keyabord status.
     *
     * @return virtual keyabord visibility status.
     */
    public boolean isShown() {
        return (null != keyboardLayer &&
                keyboardLayer.isVirtualKeyboardVisible());
    }
}
