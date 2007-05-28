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

package com.sun.midp.links;

import java.util.Hashtable;
import java.util.Enumeration;
import java.io.IOException;
import java.io.InterruptedIOException;

/**
 * Passes set of Links from one Isolate to another. The main drawback
 * of LinkPortal class, which serves the same purpose, is that it passes
 * set of Links in form of plain Java array. Therefore, if there are several
 * different entities which want to pass some Links via LinkPortal, they 
 * need to agree on two things first:
 * - Who creates this array
 * - What entries in this array will be used by each entity
 *
 * This may be cumbersome and prone to errors. 
 *
 * NamedLinkPortal was designed to solve this. As the name implies, each Link
 * has the name (string, basically) associated with it. Those names are used 
 * to retrieve Links after they have been passed to Isolate. To phrase it 
 * differently, NamedLinkPortal uses Hashtable of Links indexed by strings,
 * instead of LinkPortal's array. It also owns this hashtable, so there is 
 * no need to worry about ownership issues, as in case with LinkPortal's array.
 *
 * The usage scenario for NamedLinkPortal is following:
 * - Sender Isolate puts Links into portal via putLink method. 
 * - After all Links have been put into portal, sender Isolate invokes
 *   sendLinks method, which sends Links stored in portal to receiver 
 *   Isolate via some other Link. sendLinks method blocks until Links
 *   are received.
 * - Receiver Isolate invokes receiveLinks method to receive sent Links.
 *   If sender hasn't sent Links yet, receiveLinks blocks.
 * - Upon returning from receiveLinks method, receiver Isolate can get sent
 *   Links via getLink method.
 */
public final class NamedLinkPortal {

    /** Strings exchanged while sending/receiving Links */
    final static String START_SESSION_STR = "Starting sending named links";
    final static String END_SESSION_STR = "Finished sending named links";

    /** Hashtable of Links */
    static Hashtable links = new Hashtable();

    /** True if Links have been sent to receiver */
    static boolean linksSent = false;

    /** True if Links have been received from sender */
    static boolean linksReceived = false;


    /**
     * Puts Link into portal.
     *
     * @param name Link's name
     * @param link Link to put
     */
    public static void putLink(String name, Link link) {
        if (linksSent) {
            throw new IllegalStateException();
        }

        if (name == null || link == null) {
            throw new IllegalArgumentException();
        }

        links.put(name, link);
    }   

    /**
     * Gets Link by name after they have been received by receiver.
     *
     * @param name Link's name
     */
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

    /**
     * Sends Links to receiver Isolate.
     *
     * @param sendLink Link to use for sending
     */
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

    /** 
     * Receives Links from sender Isolate.
     *
     * @param receiveLink Link to use for receiving
     */
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

    /**
     * Private constructor to prevent creating class instance.
     */
    private NamedLinkPortal() {
    }
}
