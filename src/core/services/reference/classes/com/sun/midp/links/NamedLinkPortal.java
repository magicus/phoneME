/*
 *
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
 */

package com.sun.midp.links;

import java.util.Hashtable;
import java.util.Enumeration;
import java.io.IOException;
import java.io.InterruptedIOException;

public final class NamedLinkPortal {
    final static String START_SESSION_STR = "Starting sending named links";
    final static String END_SESSION_STR = "Finished sending named links";

    static Hashtable links = new Hashtable();
    static boolean linksSent = false;
    static boolean linksReceived = false;

    public static Link getLink(String name) {
        if (!linksReceived) {
            throw new IllegalStateException();
        }

        Link link = (Link)links.get(name);
        if (link == null) {
            throw new IllegalArgumentException();
        }

        return link;
    }

    public static void putLink(String linkName, Link link) {
        if (linksSent) {
            throw new IllegalStateException();
        }

        if (linkName == null || link == null) {
            throw new IllegalArgumentException();
        }

        links.put(linkName, link);
    }

    public static void sendLinks(Link sendLink) 
        throws ClosedLinkException, 
               InterruptedIOException, 
               IOException {

        /**
         * Arguments sanity checks
         */

        if (sendLink == null) {
            throw new IllegalArgumentException();
        }

        if (linksSent) {
            throw new IllegalStateException();
        }

        Enumeration linksEnum = links.elements();
        while (linksEnum.hasMoreElements()) {
            Link link = null;
            try {
                link = (Link)linksEnum.nextElement();
            } catch (ClassCastException e) {
                throw new IllegalStateException();
            }

            if (!link.isOpen()) {
                throw new IllegalStateException();
            }
        }

        // start session
        LinkMessage startSessionMsg = LinkMessage.newStringMessage(
                START_SESSION_STR);
        sendLink.send(startSessionMsg);

        // send named links
        Enumeration linkNamesEnum = links.keys();
        while (linkNamesEnum.hasMoreElements()) {
            String linkName = (String)linkNamesEnum.nextElement();
            Link link = (Link)links.get(linkName);
            if (link == null) {
                throw new IllegalStateException();
            }

            LinkMessage linkNameMsg = LinkMessage.newStringMessage(linkName); 
            sendLink.send(linkNameMsg);

            LinkMessage linkMsg = LinkMessage.newLinkMessage(link); 
            sendLink.send(linkMsg);
        }

        // end session
        LinkMessage endSessionMsg = LinkMessage.newStringMessage(
                END_SESSION_STR);
        sendLink.send(endSessionMsg);

        linksSent = true;
    }

    public static void receiveLinks(Link receiveLink) 
        throws ClosedLinkException, 
               InterruptedIOException, 
               IOException {

        /**
         * Arguments sanity checks
         */

        if (receiveLink == null) {
            throw new IllegalArgumentException();
        }

        if (linksReceived) {
            throw new IllegalStateException();
        }

        // start session
        LinkMessage startSessionMsg = receiveLink.receive();
        String startSessionStr = startSessionMsg.extractString();
        if (!startSessionStr.equals(START_SESSION_STR)) {
            throw new IllegalStateException();
        }

        Hashtable l = new Hashtable();

        while (true) {
            LinkMessage strMsg = receiveLink.receive();
            String str = strMsg.extractString();
            if (str.equals(END_SESSION_STR)) {
                break;
            }

            String linkName = str;
            LinkMessage linkMsg = receiveLink.receive();
            Link link = linkMsg.extractLink();
            if (!link.isOpen()) {
                throw new IllegalStateException();
            }

            l.put(linkName, link);
            
        }

        links = l;
        linksReceived = true;
    }

    private NamedLinkPortal() {
    }
}
