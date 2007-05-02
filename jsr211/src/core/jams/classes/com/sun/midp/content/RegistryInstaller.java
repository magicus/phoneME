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

package com.sun.midp.content;

import java.util.Vector;
import java.util.Enumeration;

import javax.microedition.content.ContentHandlerException;
import javax.microedition.content.ActionNameMap;

import com.sun.midp.installer.InvalidJadException;

/**
 * Support for parsing attributes and installing from the
 * manifest or application descriptors.
 */
final class RegistryInstaller {
    /** Attribute prefix for ContentHandler attributes. */
    private static final String CH_PREFIX = "MicroEdition-Handler-";

    /** Attribute suffix for ContentHandler ID attribute. */
    private static final String CH_ID_SUFFIX = "-ID";

    /** Attribute suffix for ContentHandler visibility attribute. */
    private static final String CH_ACCESS_SUFFIX = "-Access";

    /** Parced handlers to be installed. */
    private Vector instHandlers;

    /** Old handlers to be removed. */
    private Vector remHandlers;

    /**
     * Parse the ContentHandler attributes and check for errors.
     * <ul>
     * <li> Parse attributes into set of ContentHandlers.
     * <li> If none, return
     * <li> Check for permission to install handlers
     * <li> Check each for simple invalid arguments
     * <li> Check each for MIDlet is registered
     * <li> Check each for conflicts with other application registrations
     * <li> Find any current registrations
     * <li> Remove current dynamic registrations from set to be removed
     * <li> Check and resolve any conflicts between static and curr dynamic
     * </ul>
     * @param appl the AppProxy context with one or more applications
     * @return number of handlers prepared for installation.
     * @exception IllegalArgumentException if there is no classname field,
     *   or if there are more than five comma separated fields on the line.
     * @exception NullPointerException if missing components
     * @exception ContentHandlerException if handlers are ambiguous
     * @exception ClassNotFoundException if an application class cannot be found
     * @exception SecurityException if not allowed to register
     */
    int preInstall(AppProxy appl)
        throws ContentHandlerException, ClassNotFoundException
    {
        int i, j, sz;
        int suiteId = appl.getStorageId();
        ContentHandlerImpl[] chs;

        /*
         * Check for any CHAPI attributes;
         * if so, then the MIDlet suite must have permission.
         */
        remHandlers = new Vector();
        instHandlers = parseAttributes(appl);

        /*
         * Remove all static registrations.
         */
        chs = RegistryStore.forSuite(suiteId);
        sz = (chs == null? 0: chs.length);
        for (i = 0; i < sz; i++) {
            if (chs[i] == null)
                continue;
            if (chs[i].registrationMethod != 
                                    ContentHandlerImpl.REGISTERED_STATIC) {
                // Verify dynamic handler.
                try {
                    // is it a valid application?
                    appl.verifyApplication(chs[i].classname);
                    // is there new handler to replace this one?
                    for (j = 0; j < instHandlers.size(); j++) {
                        ContentHandlerImpl handler =
                            (ContentHandlerImpl)instHandlers.elementAt(j);
                        if (handler.classname.equals(chs[i].classname)) {
                            throw new Throwable("Replace dynamic handler");
                        }
                    }
                    // The handler remains.
                    continue;
                } catch(Throwable t) {
                    // Pass down to remove handler
                }
            }

            // Remove handler -- either [static] or [replaced] or [invalid]
            remHandlers.addElement(chs[i]);
            chs[i] = null;
        }

        /* Verify new registrations */
        for (i = 0; i < instHandlers.size(); i++) {
            ContentHandlerImpl handler =
                (ContentHandlerImpl)instHandlers.elementAt(i);

            // Verify ID ...
            // ... look through Registry
            ContentHandlerImpl[] conf = RegistryStore.findConflicted(handler.ID);
            if (conf != null) {
                for (j = 0; j < conf.length; j++) {
                    if (conf[j].storageId != suiteId || !willRemove(conf[j].ID))
                        throw new ContentHandlerException(
                            "Content Handler ID: "+handler.ID,
                          ContentHandlerException.AMBIGUOUS);
                }
            }

            // ... look through newbies
            j = i;
            while (j-- > 0) {
                ContentHandlerImpl other =
                    (ContentHandlerImpl)instHandlers.elementAt(j);
                if (handler.ID.startsWith(other.ID) ||
                    other.ID.startsWith(handler.ID)) {
                        throw new ContentHandlerException(
                            "Content Handler ID: "+handler.ID,
                          ContentHandlerException.AMBIGUOUS);
                }
            }

            // Check permissions for each new handler
            appl.checkRegisterPermission("register");
        }

        return instHandlers.size();
    }

    private boolean willRemove(String ID) {
        Enumeration en = remHandlers.elements();
        while (en.hasMoreElements()) {
            ContentHandlerImpl handler = (ContentHandlerImpl) en.nextElement();
            if (handler.ID.equals(ID))
                return true;
        }
        return false;
    }

    /**
     * Parse the ContentHandler attributes and check for errors.
     *
     * @param appl the AppProxy context with one or more applications
     *
     * @return a Vector of the ContentHandlers parsed from the attributes
     *
     * @exception IllegalArgumentException if there is no classname field,
     *   or if there are more than five comma separated fields on the line.
     * @exception NullPointerException if missing components
     * @exception ContentHandlerException if there are conflicts between
     *  content handlers
     * @exception ClassNotFoundException if an application class cannot be found
     */
    private static Vector parseAttributes(AppProxy appl)
        throws ContentHandlerException, ClassNotFoundException
    {
        Vector handlers = new Vector();
        for (int index = 1; ; index++) {
            String sindex = Integer.toString(index);
            String handler_n = CH_PREFIX.concat(sindex);
            String value = appl.getProperty(handler_n);
            if (value == null) {
                break;
            }
            String[] types = null;
            String[] suffixes = null;
            String[] actions = null;
            String[] locales = null;
            String classname;
            String[] fields = split(value, ',');

            switch (fields.length) {
            case 5: // Has locales
                locales = split(fields[4], ' ');
                // Fall through
            case 4: // Has actions
                actions = split(fields[3], ' ');
                // Fall through
            case 3: // Has suffixes
                suffixes = split(fields[2], ' ');
                // Fall through
            case 2: // Has types
                    // Parse out the types (if any)
                types = split(fields[1], ' ');
                    // Fall through
            case 1: // Has classname
                classname = fields[0];
                if (classname != null && classname.length() > 0) {
                    // Has non-empty classname
                    break;
                }
                // No classname, fall through to throw exception
            case 0: // no nothing; error
            default: // too many fields, error
                throw
                    new IllegalArgumentException("Too many or too few fields");
            }

            // Get the application info for this new class;
            // Throws ClassNotFoundException or IllegalArgumentException
            AppProxy newAppl = appl.forClass(classname);

            ActionNameMap[] actionnames =
                parseActionNames(actions, locales, handler_n, newAppl);

            // Parse the ID if any and the Access attribute
            String idAttr = handler_n.concat(CH_ID_SUFFIX);
            String id = newAppl.getProperty(idAttr);
            String visAttr = handler_n.concat(CH_ACCESS_SUFFIX);
            String visValue = newAppl.getProperty(visAttr);
            String[] accessRestricted = split(visValue, ' ');

            // Default the ID if not supplied
            if (id == null) {
                // Generate a unique ID based on the MIDlet suite
                id = newAppl.getApplicationID();
            }

            // Now create the handler
            ContentHandlerImpl handler =
                new ContentHandlerImpl(types, suffixes, actions,
                                       actionnames, id, accessRestricted,
                                       newAppl.getAuthority());

            // Fill in the non-public fields
            handler.classname = classname;
            handler.storageId = newAppl.getStorageId();
            handler.appname = newAppl.getApplicationName();
            handler.version = newAppl.getVersion();

            /* Check new registration does not conflict with others. */
            for (int i = 0; i < handlers.size(); i++) {
                ContentHandlerImpl curr =
                            (ContentHandlerImpl)handlers.elementAt(i);
                if (curr.classname.equals(handler.classname)) {
                    handlers.insertElementAt(handler, i);
                    handler = null;
                    break;
                }
            }
            if (handler != null) { // not yet inserted
                handlers.addElement(handler);
            }
        }
        return handlers;
    }

    /**
     * Scan the available properties for the locale specific
     * attribute names and parse and The actionname maps for
     * each.
     * @param actions the actions parsed for the handler
     * @param locales the list of locales to check for action names
     * @param prefix the prefix of the current handler attribute name
     * @param appl the AppProxy context with one or more applications
     * @return an array of ActionNameMap's
     * @exception IllegalArgumentException if locale is missing
     */
    private static ActionNameMap[] parseActionNames(String[] actions,
                                             String[] locales,
                                             String prefix,
                                             AppProxy appl)
    {
        if (locales == null || locales.length == 0) {
            return null;
        }
        prefix = prefix.concat("-");
        Vector maps = new Vector();
        for (int i = 0; i < locales.length; i++) {
            String localeAttr = prefix.concat(locales[i]);
            String localeValue = appl.getProperty(localeAttr);
            if (localeValue == null) {
                throw new IllegalArgumentException("missing locale");
            }
            String[] actionnames = split(localeValue, ',');
            ActionNameMap map =
                new ActionNameMap(actions, actionnames, locales[i]);
            maps.addElement(map);
        }
        if (maps.size() > 0) {
            ActionNameMap[] result = new ActionNameMap[maps.size()];
            maps.copyInto(result);
            return result;
        } else {
            return null;
        }
    }

    /**
     * Split the values in a field by delimiter and return a string array.
     * @param string the string containing the values
     * @param delim the delimiter that separates the values
     * @return a String array of the values; must be null
     */
    static String[] split(String string, char delim) {
        String[] ret = ContentHandlerImpl.ZERO_STRINGS;
        if (string != null) {
            Vector values = getDelimSeparatedValues(string, delim);
            ret = new String[values.size()];
            values.copyInto(ret);
        }
        return ret;
    }

    /**
     * Create a vector of values from a string containing delimiter separated
     * values. The values cannot contain the delimiter. The output values will
     * be trimmed of whitespace. The vector may contain zero length strings
     * where there are 2 delimiters in a row or a comma at the end of the input
     * string.
     *
     * @param input input string of delimiter separated values
     * @param delim the delimiter separating values
     * @return vector of string values.
     */
    private static Vector getDelimSeparatedValues(String input, char delim) {
        Vector output = new Vector(5, 5);
        int len;
        int start;
        int end;

        input = input.trim();
        len = input.length();
        if (len == 0) {
            return output;
        }

        for (start = end = 0; end < len; ) {
            // Skip leading spaces and control chars
            while (start < len && (input.charAt(start) <= ' ')) {
                start += 1;
            }

            // Scan for end delimiter (tab also if delim is space)
            for (end = start; end < len; end++) {
                char c = input.charAt(end);
                if (c == delim || (c == '\t' && delim == ' ')) {
                    output.addElement(input.substring(start, end).trim());
                    start = end + 1;
                    break;
                }
            }
        }

        end = len;
        output.addElement(input.substring(start, end).trim());

        return output;
    }

    /**
     * Performs static installation (registration) the application
     * to handle the specified type and to provide a set of actions.
     *
     * @exception InvalidJadException if there is a content handlers
     * IDs conflict
     */
    void install() {
        int i, sz;

        // Remove static and conflicted handlers.
        sz = (remHandlers == null? 0: remHandlers.size());
        for (i = 0; i < sz; i++) {
            ContentHandlerImpl handler =
                                 (ContentHandlerImpl)remHandlers.elementAt(i);
            RegistryStore.unregister(handler.getID());
        }

        // Install new handlers.
        sz = (instHandlers == null? 0: instHandlers.size());
        for (i = 0; i < sz; i++) {
            ContentHandlerImpl handler =
                                 (ContentHandlerImpl)instHandlers.elementAt(i);
            RegistryStore.register(handler);
            if (AppProxy.LOG_INFO) {
                AppProxy.getCurrent().logInfo("Register: " +
                            handler.classname +
                            ", id: " + handler.getID());
            }
        }
    }

    /**
     * Performs static uninstallation (unregistration) of the application.
     *
     * @param suiteId suite ID to be unregistered
     * @param update flag indicated whether the given suite is about remove or
     * update
     */
    static void uninstallAll(int suiteId, boolean update) {
        ContentHandlerImpl[] chs = RegistryStore.forSuite(suiteId);
        for (int i = 0; i < chs.length; i++) {
            if (!update || chs[i].registrationMethod == 
                                    ContentHandlerImpl.REGISTERED_STATIC) {
                RegistryStore.unregister(chs[i].getID());
            }
        }
    }

}
