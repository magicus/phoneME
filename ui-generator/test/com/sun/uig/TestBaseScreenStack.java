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




final class TestBaseScreenStack {
    static final long SMALL_TIMEOUT         = 100;
    static final long REASONABLE_TIMEOUT    = 10000;

    static private void testThrown(Runnable action, Class exClass)
            throws Exception {
        try {
            action.run();
        } catch (Exception e) {
            if (exClass.isInstance(e)) {
                return;
            }

            throw e;
        }
    }

    static private void test(boolean value) {
        if (!value) {
            throw new RuntimeException();
        }
    }

    static private void testAfterDestroy() throws Exception {
        final Screen s = new Screen();
        final ScreenStack ss = new ScreenStack();

        test(!ss.destroyed);
        ss.destroy();
        test(ss.destroyed);

        // all methods should report IllegalStateException
        testThrown(
            new Runnable () { public void run() { ss.show(s); }},
            Class.forName("java.lang.IllegalStateException"));
        test(s.stack == null);

        testThrown(
            new Runnable () {
                public void run() { ss.showAndWait(s, false, 0); }},
            Class.forName("java.lang.IllegalStateException"));
        test(s.stack == null);

        testThrown(
            new Runnable () { public void run() { ss.removeScreen(s); }},
            Class.forName("java.lang.IllegalStateException"));
        test(s.stack == null);

        testThrown(
            new Runnable () { public void run() { ss.destroy(); }},
            Class.forName("java.lang.IllegalStateException"));
    }

    static private void testShow() throws Exception {
        final Screen s = new Screen();
        final Screen s2 = new Screen();
        final ScreenStack ss = new ScreenStack();
        final ScreenStack ss2 = new ScreenStack();

        testThrown(
            new Runnable () { public void run() { ss.show(null); }},
            Class.forName("java.lang.NullPointerException"));

        ss.show(s);
        test(ss.curScreen == s && s.stack == ss);

        testThrown(
            new Runnable () { public void run() { ss2.show(s); }},
            Class.forName("java.lang.IllegalArgumentException"));

        ss.show(s);
        test(ss.curScreen == s && s.stack == ss);

        ss.show(s2);
        test(ss.curScreen == s2 && s.stack == ss && s2.stack == ss);

        ss.show(s);
        test(ss.curScreen == s && s.stack == ss);
    }

    static private void testShowAndWait() throws Exception {
        final Screen s = new Screen();
        final Screen s2 = new Screen();
        final ScreenStack ss = new ScreenStack();
        final ScreenStack ss2 = new ScreenStack();

        testThrown(
            new Runnable () {
                public void run() { ss.showAndWait(null, false, 0); }},
            Class.forName("java.lang.NullPointerException"));

        ss.show(s);
        test(ss.curScreen == s && s.stack == ss);

        testThrown(
            new Runnable () {
                public void run() { ss2.showAndWait(s, false, 0); }},
            Class.forName("java.lang.IllegalArgumentException"));

        ss.curScreen = null;
        new Thread() {
            public void run() {
                ss.showAndWait(s, true, Long.MAX_VALUE);
                test(s.stack == null);
                synchronized(s) {
                    s.notifyAll();
                }
            }
        }.start();

        // wait for showAndWait to block
        synchronized(ss) {
            while(ss.curScreen != s) {
                ss.wait(REASONABLE_TIMEOUT);
            }
        }
        // this should not have any effect as "ss" has only a
        // single screen that can't be removed
        test(!ss.removeScreen(s));
        test(ss.curScreen == s && s.stack == ss);

        // this should remove "s" from the "ss" stack
        ss.show(s2);
        test(ss.curScreen == s2 && s2.stack == ss);
        synchronized(s) {
            while(s.stack != null) {
                s.wait(REASONABLE_TIMEOUT);
            }
        }

        new Thread() {
            public void run() {
                // this should block for some millis and then remove "s"
                // from the "ss" on showAndWait exit
                test(s.stack == null);
                ss.showAndWait(s, true, SMALL_TIMEOUT);
                test(s.stack == null);
                synchronized(s) {
                    s.notifyAll();
                }
            }
        }.start();

        synchronized(ss) {
            while(s.stack != ss) {
                ss.wait(REASONABLE_TIMEOUT);
            }
        }

        synchronized(s) {
            while(s.stack != null) {
                s.wait(REASONABLE_TIMEOUT);
            }
        }

        test(ss.curScreen == s2 && s2.stack == ss);

        new Thread() {
            public void run() {
                // this should block for some millis bit will not remove "s"
                // from the "ss" on showAndWait exit
                ss.showAndWait(s, false, SMALL_TIMEOUT);
                test(ss.curScreen == s && s.stack == ss);
            }
        }.start();
    }

    static private void testRemoveScreen() throws Exception {
        final Screen s = new Screen();
        final Screen s2 = new Screen();
        final ScreenStack ss = new ScreenStack();
        final ScreenStack ss2 = new ScreenStack();

        ss.show(s);

        testThrown(
            new Runnable () {
                public void run() { ss.removeScreen(null); }},
            Class.forName("java.lang.NullPointerException"));

        testThrown(
            new Runnable () {
                public void run() { ss.removeScreen(s2); }},
            Class.forName("java.lang.IllegalArgumentException"));

        testThrown(
            new Runnable () {
                public void run() { ss2.removeScreen(s); }},
            Class.forName("java.lang.IllegalArgumentException"));

        test(!ss.removeScreen(s));
        test(s.stack == ss);

        ss.show(s2);
        test(ss.removeScreen(s));
        test(s.stack == null && ss.curScreen == s2 && s2.stack == ss);

        testThrown(
            new Runnable () {
                public void run() { ss.removeScreen(s); }},
            Class.forName("java.lang.IllegalArgumentException"));

        ss.show(s);
        ss.show(s2);
        test(ss.removeScreen(s2));
        test(s2.stack == null && ss.curScreen == s && s.stack == ss);

        test(!ss.removeScreen(s));
        test(ss.curScreen == s && s.stack == ss);
    }

    static private void testDestroy() throws Exception {
        final Screen s = new Screen();
        final Screen s2 = new Screen();
        final ScreenStack ss = new ScreenStack();
        final ScreenStack ss2 = new ScreenStack();

        ss.show(s);
        ss.show(s2);

        ss.destroy();

        ss2.show(s);
        ss2.show(s2);

        new Thread() {
            public void run() {
                ss2.showAndWait(s, false, Long.MAX_VALUE);
                test(s.stack == null);
                test(ss2.destroyed);
            }
        }.start();

        // wait for showAndWait to block
        synchronized(ss2) {
            while(ss2.curScreen != s) {
                ss2.wait(REASONABLE_TIMEOUT);
            }
        }
        ss2.destroy();
    }

    public static void main(String args[]) throws Exception {
        testAfterDestroy();
        testShow();
        testShowAndWait();
        testRemoveScreen();
        testDestroy();
    }
}


final class Screen {
    BaseScreenStack stack;
}


final class ScreenStack extends BaseScreenStack {
    boolean destroyed;
    Screen  curScreen;

    protected void showScreen(Screen screen) {
        if (screen == null) {
            throw new RuntimeException();
        }
        if (destroyed) {
            throw new RuntimeException();
        }
        curScreen = screen;
        notifyAll();
    }

    protected void destroyImpl() {
        if (destroyed) {
            throw new RuntimeException();
        }
        destroyed = true;
    }
}
