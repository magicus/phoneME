/*
 * @(#)ScreenUpdater.java	1.30 06/10/10
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

package sun.awt;

class ScreenUpdaterEntry {
    UpdateClient client;
    long when;
    ScreenUpdaterEntry next;
    Object arg;
    ScreenUpdaterEntry(UpdateClient client, long when, Object arg, ScreenUpdaterEntry next) {
        this.client = client;
        this.when = when;
        this.arg = arg;
        this.next = next;
    }
}

/**
 * A seperate low priority thread that warns clients
 * when they need to update the screen. Clients that
 * need a wakeup call need to call Notify().
 *
 * @version 	1.20, 07/01/98
 * @author 	Arthur van Hoff
 */
public
class ScreenUpdater extends Thread {
    private ScreenUpdaterEntry first;
    /**
     * The screen updater. 
     */
    public final static ScreenUpdater updater = initScreenUpdater();
    private static ScreenUpdater initScreenUpdater() {
        return (ScreenUpdater) java.security.AccessController.doPrivileged(
                new java.security.PrivilegedAction() {
                    public Object run() {
                        ScreenUpdater scr = new ScreenUpdater();
                        scr.setContextClassLoader(null);
                        return scr;
                    }
                }
            );
    }

    private static ThreadGroup getScreenUpdaterThreadGroup() {
        ThreadGroup g = currentThread().getThreadGroup();
        while ((g.getParent() != null) && (g.getParent().getParent() != null)) {
            g = g.getParent();
        }
        return g;
    }

    /**
     * Constructor. Starts the thread.
     */
    private ScreenUpdater() {
        super(getScreenUpdaterThreadGroup(), "Screen Updater");
        start();
    }

    /**
     * Update the next client
     */
    private synchronized ScreenUpdaterEntry nextEntry() throws InterruptedException {
        while (true) {
            if (first == null) {
                wait();
                continue;
            }
            long delay = first.when - System.currentTimeMillis();
            if (delay <= 0) {
                ScreenUpdaterEntry entry = first;
                first = first.next;
                return entry;
            }
            wait(delay);
        }
    }

    /**
     * The main body of the screen updater.
     */
    public void run() {
        try {
            while (true) {
                setPriority(NORM_PRIORITY - 1);
                ScreenUpdaterEntry entry = nextEntry();
                setPriority(NORM_PRIORITY + 1);
                try {
                    entry.client.updateClient(entry.arg);
                } catch (Throwable e) {
                    e.printStackTrace();
                }
                // Because this thread is permanent, we don't want to defeat
                // the garbage collector by having dangling references to
                // objects we no longer care about. Clear out entry so that
                // when we go back to sleep in nextEntry we won't hold a
                // dangling reference to the previous entry we processed.
                entry = null;
            }
        } catch (InterruptedException e) {}
    }

    /**
     * Notify the screen updater that a client needs
     * updating. As soon as the screen updater is
     * scheduled to run it will ask all of clients that
     * need updating to update the screen.
     */
    public void notify(UpdateClient client) {
        notify(client, 100, null);
    }

    public synchronized void notify(UpdateClient client, long delay) {
        notify(client, delay, null);
    }

    public synchronized void notify(UpdateClient client, long delay, Object arg) {
        long when = System.currentTimeMillis() + delay;
        long nextwhen = (first != null) ? first.when : -1L;
        if (first != null) {
            if ((first.client == client) && (first.arg == arg)) {
                if (when >= first.when) {
                    return;
                }
                first = first.next;
            } else {
                for (ScreenUpdaterEntry e = first; e.next != null; e = e.next) {
                    if ((e.next.client == client) && (e.next.arg == arg)) {
                        if (when >= e.next.when) {
                            return;
                        }
                        e.next = e.next.next;
                        break;
                    }
                }
            }
        }
        if ((first == null) || (first.when > when)) {
            first = new ScreenUpdaterEntry(client, when, arg, first);
        } else {
            for (ScreenUpdaterEntry e = first;; e = e.next) {
                if ((e.next == null) || (e.next.when > when)) {
                    e.next = new ScreenUpdaterEntry(client, when, arg, e.next);
                    break;
                }
            }
        }
        if (nextwhen != first.when) {
            super.notify();
        }
    }

    /**
     * Remove any pending entries for the specified client.
     * This method is normally called by the client's dispose() method.
     */
    public synchronized void removeClient(UpdateClient client) {
        ScreenUpdaterEntry entry = first;
        ScreenUpdaterEntry prev = null;
        while (entry != null) {
            if (entry.client.equals(client)) {
                if (prev == null) {
                    first = entry.next;
                } else {
                    prev.next = entry.next;
                }
            } else {
                prev = entry;
            }
            entry = entry.next;
        }
    }
}
