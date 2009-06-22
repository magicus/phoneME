/*
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
package com.sun.midp.lcdui;


import javax.microedition.lcdui.TextField;

import com.sun.midp.lcdui.DisplayContainer;
import com.sun.midp.lcdui.ForegroundEventConsumer;
import com.sun.midp.main.NativeForegroundState;

import com.sun.midp.events.Event;
import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventListener;

import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.security.ImplicitlyTrustedClass;


/**
 * Native virtual keyboard.
 */
public class NativeVirtualKeyboard {

    public static final int CONSTRAINT_ANY          = TextField.ANY;
    public static final int CONSTRAINT_NUMERIC      = TextField.NUMERIC;
    public static final int CONSTRAINT_DECIMAL      = TextField.DECIMAL;
    public static final int CONSTRAINT_EMAILADDR    = TextField.EMAILADDR;
    public static final int CONSTRAINT_PHONENUMBER  = TextField.PHONENUMBER;
    public static final int CONSTRAINT_URL          = TextField.URL;

    // see javacall_input.h
    private static final int JAVACALL_TEXTFIELD_MODIFIER_DELETE_INITIAL_TEXT = 0x400000;

    public static final int MODE_EDIT_INITIAL_TEXT      = 0x0;
    public static final int MODE_DELETE_INITIAL_TEXT    = 0x1;
    public static final int MODE_PASSWORD               = 0x10;

    private final static Object LOCK = new Object();

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    private static class SecurityTrusted implements ImplicitlyTrustedClass {}

    private static EventQueue eventQueue;

    private static DisplayContainer displayContainer;


    private static class NvkEvent extends Event {
        String initialText;
        int maxChars;
        int modes;
        int constraint;
        String result;
        boolean processed;

        NvkEvent() {
            super(EventTypes.NATIVE_VIRTUAL_KEYBOARD_EVENT);
        }
    }

    private static class NvkEventListener implements EventListener {
        public boolean preprocess(Event event, Event waitingEvent) {
            return true;
        }

        public void process(Event event) {
            NvkEvent nvkEvent = (NvkEvent)event;

            synchronized (LOCK) {
                try {
                    nvkEvent.result =
                        editTextImpl(nvkEvent.initialText, nvkEvent.maxChars,
                                nvkEvent.modes, nvkEvent.constraint);
                } catch (InterruptedException e) {
                    nvkEvent.result = null;
                } finally {
                    // Notify blocked 'editText()'
                    nvkEvent.processed = true;
                    LOCK.notifyAll();
                }
            }
        }
    }

    static {
        // should initialize 'displayContainer' from
        // 'javax.microedition.lcdui.Display' class
        init0();

        if (displayContainer == null) {
            throw new IllegalStateException();
        }

        eventQueue =
            EventQueue.getEventQueue(
                SecurityInitializer.requestToken(new SecurityTrusted()));

        eventQueue.registerEventListener(
            EventTypes.NATIVE_VIRTUAL_KEYBOARD_EVENT, new NvkEventListener());
    }

    /**
     * Runs native virtual keyboard to edit the given text.
     * Blocks until edit session is not complete.
     * Returns changed text.
     * @param text
     *  initial text or <code>null</code>;
     * @param maxChars
     *  maximum number of characters in the return string;
     * @param modes
     *  text editing mode ids;
     * @param constraint
     *  text editing constraint id;
     * @throws IllegalArgumentException
     *  if number of characters in the initial text is larger then <code>maxChars</code> value;
     *  if <code>maxChars</code> is zero;
     *  if <code>modes</code> or <code>constraint</code> are not recognized;
     * @throws RuntimeException
     *  if editing session was interrupted; e.g. if vm is suspended editing session is interrupted.
     */
    public static String editText(
            String text, int maxChars,
            int modes, int constraint) throws InterruptedException {

        if (maxChars <= 0) {
            throw new IllegalArgumentException();
        }

        if (text != null && maxChars < text.length()
        && 0 == (modes & MODE_DELETE_INITIAL_TEXT)) {
            throw new IllegalArgumentException();
        }

        switch (modes) {
        case MODE_EDIT_INITIAL_TEXT:
            modes = 0;
            break;

        case MODE_DELETE_INITIAL_TEXT:
            modes = JAVACALL_TEXTFIELD_MODIFIER_DELETE_INITIAL_TEXT;
            break;

        case MODE_EDIT_INITIAL_TEXT | MODE_PASSWORD:
            modes = 0 | TextField.PASSWORD;
            break;

        case MODE_DELETE_INITIAL_TEXT | MODE_PASSWORD:
            modes = JAVACALL_TEXTFIELD_MODIFIER_DELETE_INITIAL_TEXT | TextField.PASSWORD;
            break;

        default:
            throw new IllegalArgumentException();
        }

        switch (constraint) {
        case CONSTRAINT_ANY:
        case CONSTRAINT_NUMERIC:
        case CONSTRAINT_DECIMAL:
        case CONSTRAINT_EMAILADDR:
        case CONSTRAINT_PHONENUMBER:
        case CONSTRAINT_URL:
            break;
        default:
            throw new IllegalArgumentException();
        }

        String resultText;
        if (EventQueue.isDispatchThread()) {
            // In MIDP event handling thread context
            // launch native virtual keyboard immediatly
            resultText = editTextImpl(text, maxChars, modes, constraint);
        } else {
            NvkEvent event = new NvkEvent();
            event.initialText = text;
            event.maxChars = maxChars;
            event.modes = modes;
            event.constraint = constraint;

            synchronized (LOCK) {
                eventQueue.post(event);
                while (!event.processed) {
                    LOCK.wait();
                }
            }

            resultText = event.result;
        }

        if (null == resultText) {
            // some internal error
            throw new InterruptedException();
        }

        return resultText;
    }

    private static String editTextImpl(
            String text, int maxChars,
            int modes, int constraint) throws InterruptedException {

        if (!EventQueue.isDispatchThread()) {
            throw new IllegalStateException();
        }

        ForegroundEventConsumer fc =
                displayContainer.findForegroundEventConsumer(
                    NativeForegroundState.getState());
        if (fc == null) {
            // no current display
            throw new InterruptedException();
        }

        // Disable screen updates
        // IMPORTANT: should be called from MIDP event handler thread only!
        fc.handleDisplayBackgroundNotifyEvent();

        try {
            return editText0(text, maxChars, modes, constraint);
        } finally {
            // Enable screen updates and redraw screen
            // IMPORTANT: should be called from MIDP event handler thread only!
            fc.handleDisplayForegroundNotifyEvent();
        }
    }

    private static native void init0();

    private static native String editText0(
            String text, int maxChars, int modes, int constraint);
}
