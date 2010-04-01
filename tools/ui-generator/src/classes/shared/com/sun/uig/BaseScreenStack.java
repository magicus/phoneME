/*
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
 */

package com.sun.uig;

import java.util.Stack;


public abstract class BaseScreenStack {
    private Stack       screens;
    private boolean     destroyed;

    protected abstract void showScreen(Screen screen);
    protected abstract void destroyImpl();

    /**
     * Shows a <code>screen</code> on top of all screens of this stack.
     * <p>If the screen is already on this stack it is set topmost visible
     * screen.
     *
     * @param screen
     *  a screen to show
     * @exception NullPointerException
     *  if a <code>screen</code> is <code>null</code>
     * @exception IllegalArgumentException
     *  if a <code>screen</code> is already on some stack other then this one
     * @exception IllegalStateException
     *  if this stack is destroyed
     *  (i.e. {@link #destroy()} has been called already)
     */
    public synchronized void show(Screen screen) {
        if (destroyed) {
            throw new IllegalStateException("Stack has beed destroyed");
        }

        synchronized (screen) {
            if (screen.isParentStack(this)) {
                screens.removeElement(screen);
            } else if (!screen.isParentStack(null)) {
                throw new IllegalArgumentException(
                    "Screen is already on a stack");
            }

            if (screens == null) {
                screens = new Stack();
            } else if (screens.contains(screen)) {
                // should never get here, but just in case...
                throw new IllegalArgumentException(
                    "Screen is already on a stack");
            }

            screen.setParentStack(this);
            screens.push(screen);
        }

        // This should be called for a screen even if it is already
        // the topmost. It is derived class responsibility to add
        // extra optimizations whenever it can ignore this call if the screen
        // is already the topmost visible one.
        showScreen(screen);
    }

    /**
     * Shows a <code>screen</code> on top of all screens of this stack.
     * Blocks until sequential successful
     * {@link #show}, {@link #showAndWait}, {@link #removeScreen} or
     * {@link #destroy} is called or when <code>timeout</code> expires.
     * <p>If the screen is already on this stack it is set topmost visible
     * screen.
     *
     * @param screen
     *  a screen to show
     * @param autoHide
     *  if <code>true</code> removes the <code>screen</code> from this
     *  stack on function exit
     * @param timeout
     *  timeout in milliseconds to wait for sequential {@link #show},
     *  {@link #showAndWait}, {@link #removeScreen} or {@link #destroy()}
     *  is called
     * @exception NullPointerException
     *  if a <code>screen</code> is <code>null</code>
     * @exception IllegalArgumentException
     *  if a <code>screen</code> is already on some stack other then this one
     * @exception IllegalStateException
     *  if this stack is destroyed
     *  (i.e. {@link #destroy()} has been called already)
     */
    public synchronized void showAndWait(
        Screen screen, boolean autoHide, long timeout) {

        show(screen);
        try {
            long time = System.currentTimeMillis();
            while (!destroyed && screens.peek() == screen) {
                wait(timeout);
                if (timeout <= System.currentTimeMillis() - time) {
                    break;
                }
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        if (autoHide && screens.size() > 1) {
            synchronized (screen) {
                screens.removeElement(screen);
                screen.setParentStack(null);
            }

            showScreen((Screen)screens.peek());
        }
    }

    private void pop() {
        Screen screen = (Screen)screens.peek();
        synchronized (screen) {
            screens.pop();
            screen.setParentStack(null);
        }
    }

    /**
     * Should be called to notify this stack that a <code>screen</code>
     * is not needed any more. If the <code>screen</code> is the last
     * screen on this stack it stays visible and associated with this stack
     * until the next successful {@link #show}, {@link #showAndWait} or
     * {@link #destroy} call.
     * <p>It is recommended that caller always push a "base" screen first on
     * the stack to ensure there is always a screen other then the "base" one
     * to remove immediately from the stack.
     *
     * @return <code>true</code> if the given screen is a part of this stack
     *  and it was successfully removed from it;
     * <br> <code>false</code> if the given screen is the last screen of
     *  this stack and it was not removed from it;
     *
     * @exception NullPointerException
     *  if a <code>screen</code> is <code>null</code>
     * @exception IllegalArgumentException
     *  if a <code>screen</code> is not on this stack
     * @exception IllegalStateException
     *  if this stack is destroyed
     *  (i.e. {@link #destroy()} has been called already)
     */
    public synchronized boolean removeScreen(Screen screen) {
        if (destroyed) {
            throw new IllegalStateException("Stack has beed destroyed");
        }

        synchronized (screen) {
            if (!screen.isParentStack(this)) {
                throw new IllegalArgumentException(
                    "Screen not on this stack");
            }

            if (screens.size() > 1) {
                if (screens.peek() == screen) {
                    pop();
                } else {
                    screens.removeElement(screen);
                    screen.setParentStack(null);
                }

                showScreen((Screen)screens.peek());
                notifyAll();
            }

            return (screen.isParentStack(null));
        }
    }

    /**
     * Returns topmost screen.
     *
     * @exception IllegalStateException
     *  {@link #destroy()} has been called already on this stack
     */
    public synchronized Screen top() {
        if (destroyed) {
            throw new IllegalStateException("Stack has beed destroyed");
        }

        if (screens != null) {
            return (Screen)screens.peek();
        }

        return null;
    }

    /**
     * Destroys this stack. After stack is destroyed every method will throw
     * <code>IllegalStateException</code> exception. When stack is destroyed
     * all screens are hidden.
     * <p>If there is pending {@link #showAndWait} call it is unblocked.
     * <p>Screens from the destroyed stack can be pushed on other stacks.
     *
     * @exception IllegalStateException
     *  {@link #destroy()} has been called already on this stack
     */
    public synchronized void destroy() {
        if (destroyed) {
            throw new IllegalStateException("Stack has beed destroyed");
        }

        destroyed = true;
        if (screens != null) {
            for (int i = 0, count = screens.size(); i < count; ++i) {
                pop();
            }
            notifyAll();
        }

        destroyImpl();
    }

    public BaseScreenStack() {
    }
}
