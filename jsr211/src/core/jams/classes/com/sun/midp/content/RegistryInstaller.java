/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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


    /**
     * Private constructor to prevent any instances.
     */
    private RegistryInstaller() {
    }

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
     * <li> Return set of handlers to be added and removed
     *      for registration step.
     * </ul>
     * @param appl the AppProxy context with one or more applications
     * @return a vector of ContentHandlerImpl instances parsed from the props.
     * @exception IllegalArgumentException if there is no classname field,
     *   or if there are more than five comma separated fields on the line.
     * @exception NullPointerException if missing components
     * @exception ContentHandlerException if handlers are ambiguous
     * @exception ClassNotFoundException if an application class cannot be found
     * @exception SecurityException if not allowed to register
     */
    static Vector preInstall(AppProxy appl)
        throws ContentHandlerException, ClassNotFoundException
    {
        // Uninstall any handler with the same suiteId
        int suiteId = appl.getStorageId();
        uninstallAll(suiteId, true);

        /*
         * Check for any CHAPI attributes;
         * if so, then the MIDlet suite must have permission.
         */
        Vector newhandlers = parseAttributes(appl);
        /* For each new registration */
        for (int i = 0; i < newhandlers.size(); i++) {
            ContentHandlerImpl handler =
                (ContentHandlerImpl)newhandlers.elementAt(i);

            // test if ID is valid
            if (!RegistryStore.testId(handler.getID())) {
                throw new ContentHandlerException("ID would be ambiguous",
                          ContentHandlerException.AMBIGUOUS);
            }

            // Check permissions for each new handler
            appl.checkRegisterPermission("register");
        }

        return newhandlers;
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
     * @param handlers a Vector of ContentHandlerImpl objects
     * to be added to Registry database.
     * @exception InvalidJadException if there is a content handlers
     * IDs conflict
     */
    static void install(Vector handlers) throws InvalidJadException {
        for (int i = 0; i < handlers.size(); i++) {
            ContentHandlerImpl handler =
                                 (ContentHandlerImpl)handlers.elementAt(i);
            ContentHandlerImpl test = RegistryStore.getHandler(null, 
                                handler.getID(), RegistryStore.SEARCH_TEST);
            if (test != null) {
                // if the same handler was not removed - just reinstall it
                if (!handler.getID().equals(test.getID())
                    || test.registrationMethod != 
                                           ContentHandlerImpl.REGISTERED_STATIC
                    || !test.classname.equals(handler.classname)
                    || test.storageId != handler.storageId) {
                    while (i-- > 0) {
                      ContentHandlerImpl tmpHandler =
                                     (ContentHandlerImpl)handlers.elementAt(i);
                      RegistryStore.unregister(tmpHandler.getID());
                    }
                    throw new InvalidJadException(
                                InvalidJadException.CONTENT_HANDLER_CONFLICT);
                }
            }
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
