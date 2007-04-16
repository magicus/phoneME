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

import javax.microedition.content.ContentHandler;
import javax.microedition.content.ContentHandlerServer;
import javax.microedition.content.RequestListener;
import javax.microedition.content.ActionNameMap;
import javax.microedition.content.Invocation;

import java.io.DataInputStream;
import java.io.DataOutputStream;

/**
 * The internal structure of a registered content handler.
 */
public class ContentHandlerImpl implements ContentHandler {

    /**
     * The content handler ID.
     * Lengths up to 256 characters MUST be supported.
     * The ID may be <code>null</code>.
     */
    String ID;

    /**
     * The types that are supported by this content handler.
     * If there are no types registered, the this field MUST either be
     * <code>null</code> or refer to an empty array .
     */
    private String[] types;

    /**
     * The suffixes of URLs that are supported by this content handler.
     * If there are no suffixes, then this field MUST be <code>null</code>.
     * The suffixes MUST include any and all punctuation. For example,
     * the string <code>".png"</code>.
     */
    private String[] suffixes;

    /**
     * The actions that are supported by this content handler.
     * If there are no actions, then this field MSUT be <code>null</code>.
     */
    private String[] actions;

    /**
     * The action names that are defined by this content handler.
     */
    ActionNameMap[] actionnames;

    /** The sequence number of this instance; monotonic increasing. */
    final int seqno;

    /** The next sequence number to assign. */
    private static int nextSeqno;

    /**
     * The RequestListenerImpl; if a listener is set.
     */
    RequestListenerImpl listenerImpl;

    /** Empty String array to return when needed. */
    public final static String[] ZERO_STRINGS = {};

    /** Empty ActionNameMap to return when needed. */
    private final static ActionNameMap[] ZERO_ACTIONNAMES =
        new ActionNameMap[0];

    /** Property name for the current locale. */
    private final static String LOCALE_PROP = "microedition.locale";

    /**
     * The MIDlet suite storagename that contains the MIDlet.
     */
    int storageId;

    /**
     * The Name from parsing the Property for the MIDlet
     * with this classname.
     */
    String appname;

    /**
     * The Version parsed from MIDlet-Version attribute.
     */
    String version;

    /**
     * The application class name that implements this content
     * handler.  Note: Only the application that registered the class
     * will see the classname; for other applications the value will be
     * <code>null</code>.
     */
    String classname;

    /**
     * The authority that authenticated this ContentHandler.
     */
    String authority;

    /**
     * The accessRestrictions for this ContentHandler.
     */
    private String[] accessRestricted;

    /**
     * Indicates content handler registration method:
     * dynamic registration from the API, static registration from install
     * or native content handler.
     * Must be similar to enum in jsr211_registry.h
     */
    int registrationMethod;

    /** Content handler statically registered during installation */
    final static int REGISTERED_STATIC = 0;
    /** Dynamically registered content handler via API */
    final static int REGISTERED_DYNAMIC = 1;
    /** Native platform content handler  */
    final static int REGISTERED_NATIVE  = 2;

    /** Count of requests retrieved via {@link #getRequest}. */
    int requestCalls;

    /**
     * Instance is a registration or unregistration.
     * An unregistration needs only storageId and classname.
     */
    boolean removed;

    /**
     * Construct a ContentHandlerImpl.
     * Verifies that all strings are non-null
     * @param types an array of types to register; may be
     *  <code>null</code>
     * @param suffixes an array of suffixes to register; may be
     *  <code>null</code>
     * @param actions an array of actions to register; may be
     *  <code>null</code>
     * @param actionnames an array of ActionNameMaps to register; may be
     *  <code>null</code>
     * @param ID the content handler ID; may be <code>null</code>
     * @param accessRestricted the  IDs of applications allowed access
     * @param auth application authority
     *
     * @exception NullPointerException if any types, suffixes,
     *   actions, actionnames array element is null
     *
     * @exception IllegalArgumentException is thrown if any of
     *   the types, suffix, or action strings have a
     *   length of zero or
     *   if the ID has a length of zero or contains any
     *        control character or space (U+0000-U+00020)
     */
    ContentHandlerImpl(String[] types, String[] suffixes,
                       String[] actions, ActionNameMap[] actionnames,
                       String ID, String[] accessRestricted, String auth) {
        this();

        // Verify consistency between actions and ActionNameMaps
        if (actionnames != null && actionnames.length > 0) {
            if (actions == null) {
                throw new IllegalArgumentException("no actions");
            }
            int len = actions.length;
            for (int i = 0; i < actionnames.length; i++) {
                // Verify the actions are the same
                ActionNameMap map = actionnames[i];
                if (len != map.size()) {
                    throw new IllegalArgumentException("actions not identical");
                }

                for (int j = 0; j < len; j++) {
                    if (!actions[j].equals(map.getAction(j))) {
                        throw new
                            IllegalArgumentException("actions not identical");
                    }
                }

                /*
                 * Verify the locale of this ActionNameMap is not the same
                 * as any previous ActionNameMap.
                 */
                for (int j = 0; j < i; j++) {
                    if (map.getLocale().equals(actionnames[j].getLocale())) {
                        throw new IllegalArgumentException("duplicate locale");
                    }
                }
            }
        }

        // Check the ID for invalid characters (controls or space)
        if (ID != null) {
            int len = ID.length();
            if (len == 0) {
                    throw new IllegalArgumentException("invalid ID");
            }
            for (int i = 0; i < ID.length(); i++) {
                if (ID.charAt(i) <= 0x0020) {
                    throw new IllegalArgumentException("invalid ID");
                }
            }
            this.ID = ID;
        }
        this.types = copy(types);
        this.suffixes = copy(suffixes);
        this.actions = copy(actions);
        this.actionnames = copy(actionnames);
        this.accessRestricted = copy(accessRestricted);
        this.authority = auth;
    }

    /**
     * Initialize a new instance with the same information.
     * @param handler another ContentHandlerImpl
     * @see javax.microedition.content.ContentHandlerServerImpl
     */
    protected ContentHandlerImpl(ContentHandlerImpl handler) {
        this();
        types = handler.types;
        suffixes = handler.suffixes;
        ID = handler.ID;
        accessRestricted = handler.accessRestricted;
        actions = handler.actions;
        actionnames = handler.actionnames;
        listenerImpl = handler.listenerImpl;
        storageId = handler.storageId;
        classname = handler.classname;
        version = handler.version;
        registrationMethod = handler.registrationMethod;
        requestCalls = handler.requestCalls;
        authority = handler.authority;
        appname = handler.appname;
    }

    /**
     * Constructor used to read handlers.
     */
    ContentHandlerImpl() {
        seqno = nextSeqno++;
    }

    /**
     * Checks that all of the string references are non-null
     * and not zero length.  If either the argument is null or
     * is an empty array the default ZERO length string array is used.
     *
     * @param strings array to check for null and length == 0
     * @return a non-null array of strings; an empty array replaces null
     * @exception NullPointerException if any string ref is null
     * @exception IllegalArgumentException if any string
     * has length == 0
     */
    public static String[] copy(String[] strings) {
        if (strings != null && strings.length > 0) {
            String[] copy = new String[strings.length];
            for (int i = 0; i < strings.length; i++) {
                if (strings[i].length() == 0) {
                    throw new IllegalArgumentException("string length is 0");
                }
                copy[i] = strings[i];

            }
            return strings;
        } else {
            return ZERO_STRINGS;
        }
    }
    /**
     * Checks that all of the actionname references are non-null.
     *
     * @param actionnames array to check for null and length == 0
     * @return a non-null array of actionnames; an empty array replaces null
     * @exception NullPointerException if any string ref is null
     */
    private static ActionNameMap[] copy(ActionNameMap[] actionnames) {
        if (actionnames != null && actionnames.length > 0) {
            ActionNameMap[] copy = new ActionNameMap[actionnames.length];
            for (int i = 0; i < actionnames.length; i++) {
                // Check for null
                if (actionnames[i] == null) {
                    throw new NullPointerException();
                }
                copy[i] = actionnames[i];
            }
            return copy;
        } else {
            return ZERO_ACTIONNAMES;
        }
    }

    /**
     * Copy an array of ContentHandlers making a new ContentHandler
     * for each ContentHandler.  Make copies of any mutiple object.
     * @param handlers the array of handlers duplicate
     * @return the new array of content handlers
     */
    public static ContentHandler[] copy(ContentHandler[] handlers) {
        ContentHandler[] h = new ContentHandler[handlers.length];
        for (int i = 0; i < handlers.length; i++) {
            h[i] = handlers[i];
        }
        return h;
    }

    /**
     * Get the nth type supported by the content handler.
     * @param index the index into the types
     * @return the nth type
     * @exception IndexOutOfBounds if index is less than zero or
     *     greater than or equal to the value of the
     *     {@link #getTypeCount getTypeCount} method.
     */
    public String getType(int index) {
        return get(index, getTypes());
    }


    /**
     * Get the number of types supported by the content handler.
     *
     * @return the number of types
     */
    public int getTypeCount() {
        return getTypes().length;
    }

    /**
     * Get types supported by the content handler.
     *
     * @return array of types supported
     */
    String [] getTypes() {
        if (types == null) {
            types = RegistryStore.getArrayField(ID, RegistryStore.FIELD_TYPES);
        }
        return types;
    }

    /**
     * Determine if a type is supported by the content handler.
     *
     * @param type the type to check for
     * @return <code>true</code> if the type is supported;
     *  <code>false</code> otherwise
     * @exception NullPointerException if <code>type</code>
     * is <code>null</code>
     */
    public boolean hasType(String type) {
        return has(type, getTypes(), true);
    }

    /**
     * Get the nth suffix supported by the content handler.
     * @param index the index into the suffixes
     * @return the nth suffix
     * @exception IndexOutOfBounds if index is less than zero or
     *     greater than or equal to the value of the
     *     {@link #getSuffixCount getSuffixCount} method.
     */
    public String getSuffix(int index) {
        return get(index, getSuffixes());
    }

    /**
     * Get the number of suffixes supported by the content handler.
     *
     * @return the number of suffixes
     */
    public int getSuffixCount() {
        return getSuffixes().length;
    }

    /**
     * Determine if a suffix is supported by the content handler.
     *
     * @param suffix the suffix to check for
     * @return <code>true</code> if the suffix is supported;
     *  <code>false</code> otherwise
     * @exception NullPointerException if <code>suffix</code>
     * is <code>null</code>
     */
    public boolean hasSuffix(String suffix) {
        return has(suffix, getSuffixes(), true);
    }

    /**
     * Get suffixes supported by the content handler.
     *
     * @return array of suffixes supported
     */
    String [] getSuffixes() {
        if (suffixes == null) {
            suffixes =
                RegistryStore.getArrayField(ID, RegistryStore.FIELD_SUFFIXES);
        }
        return suffixes;
    }

    /**
     * Get the nth action supported by the content handler.
     * @param index the index into the actions
     * @return the nth action
     * @exception IndexOutOfBounds if index is less than zero or
     *     greater than or equal to the value of the
     *     {@link #getActionCount getActionCount} method.
     */
    public String getAction(int index) {
        return get(index, getActions());
    }

    /**
     * Get the number of actions supported by the content handler.
     *
     * @return the number of actions
     */
    public int getActionCount() {
        return getActions().length;
    }

    /**
     * Determine if a action is supported by the content handler.
     *
     * @param action the action to check for
     * @return <code>true</code> if the action is supported;
     *  <code>false</code> otherwise
     * @exception NullPointerException if <code>action</code>
     * is <code>null</code>
     */
    public boolean hasAction(String action) {
        return has(action, getActions(), false);
    }

    /**
     * Get actions supported by the content handler.
     *
     * @return array of actions supported
     */
    String [] getActions() {
        if (actions == null) {
            actions =
                RegistryStore.getArrayField(ID, RegistryStore.FIELD_ACTIONS);
        }
        return actions;
    }

    /**
     * Gets the value at index in the string array.
     * @param index of the value
     * @param strings array of strings to get from
     * @return string at index.
     * @exception IndexOutOfBounds if index is less than zero or
     *     greater than or equal length of the array.
     */
    private String get(int index, String[] strings) {
        if (index < 0 || index >= strings.length) {
            throw new IndexOutOfBoundsException();
        }
        return strings[index];
    }

    /**
     * Determines if the string is in the array.
     * @param string to locate
     * @param strings array of strings to get from
     * @param ignoreCase true to ignore case in matching
     * @return <code>true</code> if the value is found
     * @exception NullPointerException if <code>string</code>
     * is <code>null</code>
     */
    private boolean has(String string, String[] strings, boolean ignoreCase) {
        int len = string.length(); // Throw NPE if null
        for (int i = 0; i < strings.length; i++) {
            if (strings[i].length() == len &&
                string.regionMatches(ignoreCase, 0, strings[i], 0, len)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get the mapping of actions to action names for the current
     * locale supported by this content handler. The behavior is
     * the same as invoking {@link #getActionNameMap} with the current
     * locale.
     *
     * @return an ActionNameMap; if there is no map available for the
     *  current locale, then it MUST be <code>null</code>
     */
    public ActionNameMap getActionNameMap() {
        String locale = System.getProperty(LOCALE_PROP);
        return (locale == null) ? null : getActionNameMap(locale);
    }

    /**
     * Get the mapping of actions to action names for the requested
     * locale supported by this content handler.
     * The locale is matched against the available locales.
     * If a match is found it is used.  If an exact match is
     * not found, then the locale string is shortened and retried
     * if either of the "_" or "-" delimiters is present.
     * The locale is shortened by retaining only the characters up to
     * but not including the last occurence of the delimiter
     * (either "_" or "-").
     * The shortening and matching is repeated as long as the string
     * contains one of the delimiters.
     * Effectively, this will try the full locale and then try
     * without the variant or country code, if they were present.
     *
     * @param locale for which to find an ActionNameMap;
     *   MUST NOT be <code>null</code>
     * @return an ActionNameMap; if there is no map available for the
     *  locale, then it MUST be <code>null</code>
     * @exception NullPointerException if the locale is <code>null</code>
     */
    public ActionNameMap getActionNameMap(String locale) {
        while (locale.length() > 0) {
            for (int i = 0; i < getActionNames().length; i++) {
                if (locale.equals(getActionNames()[i].getLocale())) {
                    return getActionNames()[i];
                }
            }
            int lastdash = locale.lastIndexOf('-');
            if (lastdash < 0) {
                break;
            }
            locale = locale.substring(0, lastdash);
        }
        return null;
    }

    /**
     * Gets the number of action name maps supported by the content handler.
     *
     * @return the number of action name maps
     */
    public int getActionNameMapCount() {
        return getActionNames().length;
    }

    /**
     * Gets the n<sup>th</sup> ActionNameMap supported by the
     * content handler.
     * @param index the index of the locale
     * @return the n<sup>th</sup> ActionNameMap
     *
     * @exception IndexOutOfBoundsException if index is less than zero or
     *     greater than or equal to the value of the
     *     {@link #getActionNameMapCount getActionNameMapCount} method.
     */
    public ActionNameMap getActionNameMap(int index) {
        if (index < 0 || index >= getActionNames().length) {
            throw new IndexOutOfBoundsException();
        }
        return getActionNames()[index];
    }

    /**
     * Get actions names for the content handler.
     *
     * @return array of actions names
     */
    private ActionNameMap[] getActionNames() {
        if (actionnames == null) {
            String [] locales =
                RegistryStore.getArrayField(ID, RegistryStore.FIELD_LOCALES);
            String [] names   =
                RegistryStore.getArrayField(ID, RegistryStore.FIELD_ACTION_MAP);

            actionnames = new ActionNameMap[locales.length];
            for (int index = 0; index < locales.length; index++) {
                String [] temp = new String[getActions().length];

                System.arraycopy(names,
                                 index * getActions().length,
                                 temp,
                                 0,
                                 getActions().length);

                actionnames[index] = new ActionNameMap(getActions(),
                                                       temp,
                                                       locales[index]);
            }
        }
        return actionnames;
    }

    /**
     * Returns the name used to present this content handler to a user.
     * The value is extracted from the normal installation information
     * about the content handler application.
     *
     * @return the user-friendly name of the application;
     * it MUST NOT be <code>null</code>
     */
    public String getAppName() {
        loadAppData();
        return appname;
    }

    /**
     * Gets the version number of this content handler.
     * The value is extracted from the normal installation information
     * about the content handler application.
     * @return the version number of the application;
     * MAY be <code>null</code>
     */
    public String getVersion() {
        loadAppData();
        return version;
    }

    /**
     * Get the content handler ID.  The ID uniquely identifies the
     * application which contains the content handler.
     * After registration and for every registered handler,
     * the ID MUST NOT be <code>null</code>.
     * @return the ID; MUST NOT be <code>null</code> unless the
     *  ContentHandler is not registered.
     */
    public String getID() {
        return ID;
    }

    /**
     * Gets the name of the authority that authorized this application.
     * This value MUST be <code>null</code> unless the device has been
     * able to authenticate this application.
     * If <code>non-null</code>, it is the string identifiying the
     * authority.  For example,
     * if the application was a signed MIDlet, then this is the
     * "subject" of the certificate used to sign the application.
     * <p>The format of the authority for X.509 certificates is defined
     * by the MIDP Printable Representation of X.509 Distinguished
     * Names as defined in class
     * <code>javax.microedition.pki.Certificate</code>. </p>
     *
     * @return the authority; may be <code>null</code>
     */
    public String getAuthority() {
        loadAppData();
        return authority;
    }

    /**
     * Initializes fields retrieved from AppProxy 'by-demand'.
     */
    private void loadAppData() {
        if (appname == null) {
            try {
                AppProxy app = 
                        AppProxy.getCurrent().forApp(storageId, classname);
                appname = app.getApplicationName();
                version = app.getVersion();
                authority = app.getAuthority();
            } catch (Throwable t) {
            }
            if (appname == null) {
                appname = "";
            }
        }
    }

    /**
     * Gets the n<sup>th</sup> ID of an application or content handler
     * allowed access to this content handler.
     * The ID returned for each index must be the equal to the ID
     * at the same index in the <tt>accessAllowed</tt> array passed to
     * {@link javax.microedition.content.Registry#register Registry.register}.
     *
     * @param index the index of the ID
     * @return the n<sup>th</sup> ID
     * @exception IndexOutOfBoundsException if index is less than zero or
     *     greater than or equal to the value of the
     *     {@link #accessAllowedCount accessAllowedCount} method.
     */
    public String getAccessAllowed(int index) {
        return get(index, getAccessRestricted());
    }

    /**
     * Gets the number of IDs allowed access by the content handler.
     * The number of IDs must be equal to the length of the array
     * of accessRestricted passed to
     * {@link javax.microedition.content.Registry#register Registry.register}.
     * If the number of IDs is zero then all applications and
     * content handlers are allowed access.
     *
     * @return the number of accessRestricteds
     */
    public int accessAllowedCount() {
        return getAccessRestricted().length;
    }

    /**
     * Determines if an ID MUST be allowed access by the content handler.
     * Access MUST be allowed if the ID has a prefix that exactly matches
     * any of the IDs returned by {@link #getAccessAllowed}.
     * The prefix comparison is equivalent to
     * <code>java.lang.String.startsWith</code>.
     *
     * @param ID the ID for which to check access
     * @return <code>true</code> if access MUST be allowed by the
     *  content handler;
     *  <code>false</code> otherwise
     * @exception NullPointerException if <code>accessRestricted</code>
     * is <code>null</code>
     */
    public boolean isAccessAllowed(String ID) {
        ID.length();                // check for null
        if (getAccessRestricted().length == 0) {
            return true;
        }
        for (int i = 0; i < getAccessRestricted().length; i++) {
            if (ID.startsWith(getAccessRestricted()[i])) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get accesses for the content handler.
     *
     * @return array of allowed class names
     */
    private String [] getAccessRestricted() {
        if (accessRestricted == null) {
            accessRestricted =
                RegistryStore.getArrayField(ID, RegistryStore.FIELD_ACCESSES);
        }
        return accessRestricted;
    }

    /**
     * Gets the next Invocation request pending for this
     * ContentHandlerServer.
     * The method can be unblocked with a call to
     * {@link #cancelGetRequest cancelGetRequest}.
     * The application should process the Invocation as
     * a request to perform the <code>action</code> on the content.
     *
     * @param wait <code>true</code> if the method must wait for
     * for an Invocation if one is not available;
     * <code>false</code> if the method MUST NOT wait.
     * @param invocation an Invocation instance that will delegate to
     * the result; if any
     * @return the next pending Invocation or <code>null</code>
     *  if no Invocation is available; <code>null</code>
     *  if cancelled with {@link #cancelGetRequest cancelGetRequest}
     * @see javax.microedition.content.Registry#invoke
     * @see javax.microedition.content.ContentHandlerServer#finish
     */
    public InvocationImpl getRequest(boolean wait, Invocation invocation) {
        // Application has tried to get a request; reset cleanup flags on all
        if (requestCalls == 0) {
            InvocationStore.setCleanup(storageId, classname, false);
        }
        requestCalls++;

        InvocationImpl invoc =
            InvocationStore.getRequest(storageId, classname, wait);
        if (invoc != null) {
            // Keep track of number of requests delivered to the application
            AppProxy.requestForeground(invoc.invokingSuiteId,
                                       invoc.invokingClassname,
                                       invoc.suiteId,
                                       invoc.classname);
            invoc.invocation = invocation;
        }
        return invoc;
    }

    /**
     * Cancel a pending <code>getRequest</code>.
     * This method will force a Thread blocked in a call to the
     * <code>getRequest</code> method for the same application
     * context to return early.
     * If no Thread is blocked; this call has no effect.
     */
    public void cancelGetRequest() {
        InvocationStore.cancel();
    }

    /**
     * Finish this Invocation and set the status for the response.
     * The <code>finish</code> method may only be called when this
     * Invocation
     * has a status of <code>ACTIVE</code> or <code>HOLD</code>.
     * <p>
     * The content handler may modify the URL, type, action, or
     * arguments before invoking <code>finish</code>.
     * If the method {@link Invocation#getResponseRequired} returns
     * <code>true</code> then the modified
     * values MUST be returned to the invoking application.
     *
     * @param invoc the Invocation to finish
     * @param status the new status of the Invocation. This MUST be either
     *         <code>OK</code> or <code>CANCELLED</code>.
     *
     * @return <code>true</code> if the MIDlet suite MUST
     *   voluntarily exit before the response can be returned to the
     *   invoking application
     *
     * @exception IllegalArgumentException if the new
     *   <code>status</code> of the Invocation
     *    is not <code>OK</code> or <code>CANCELLED</code>
     * @exception IllegalStateException if the current
     *   <code>status</code> of the
     *   Invocation is not <code>ACTIVE</code> or <code>HOLD</code>
     * @exception NullPointerException if the invocation is <code>null</code>
     */
    protected boolean finish(InvocationImpl invoc, int status) {
        int currst = invoc.getStatus();
          if (currst != Invocation.ACTIVE && currst != Invocation.HOLD) {
             throw new IllegalStateException("Status already set");
         }
        // If ACTIVE or HOLD it must be an InvocationImpl
        return invoc.finish(status);
    }

    /**
     * Set the listener to be notified when a new request is
     * available for this content handler.  The request MUST
     * be retrieved using {@link #getRequest}.
     *
     * @param listener the listener to register;
     *   <code>null</code> to remove the listener.
     */
    public void setListener(RequestListener listener) {
        synchronized (this) {
            if (listener != null || listenerImpl != null) {
                // Create or update the active listener thread
                if (listenerImpl == null) {
                    listenerImpl =
                        new RequestListenerImpl(this, listener);
                } else {
                    listenerImpl.setListener(listener);
                }

                // If the listener thread no longer needed; clear it
                if (listener == null) {
                    listenerImpl = null;
                }
            }
        }
    }

    /**
     * Notify the request listener of a pending request.
     * Overridden by subclass.
     */
    protected void requestNotify() {
    }

    /**
     * Compare two ContentHandlerImpl's for equality.
     * Classname, storageID, and seqno must match.
     * @param other another ContentHandlerImpl
     * @return true if the other handler is for the same class,
     * storageID, and seqno.
     */
    boolean equals(ContentHandlerImpl other) {
        return seqno == other.seqno &&
            storageId == other.storageId &&
            classname.equals(other.classname);
    }

    /**
     * Debug routine to print the values.
     * @return a string with the details
     */
    public String toString() {
        if (AppProxy.LOG_INFO) {
            StringBuffer sb = new StringBuffer(80);
            sb.append("CH:");
            sb.append(" classname: ");
            sb.append(classname);
            sb.append(", removed: ");
            sb.append(removed);
            sb.append(", flag: ");
            sb.append(registrationMethod);
            sb.append(", types: ");
            toString(sb, types);
            sb.append(", ID: ");
            sb.append(ID);
            sb.append(", suffixes: ");
            toString(sb, suffixes);
            sb.append(", actions: ");
            toString(sb, actions);
            sb.append(", access: ");
            toString(sb, accessRestricted);
            sb.append(", suiteID: ");
            sb.append(storageId);
            sb.append(", authority: ");
            sb.append(authority);
            sb.append(", appname: ");
            sb.append(appname);
            return sb.toString();
        } else {
            return super.toString();
        }
    }

    /**
     * Append all of the strings inthe array to the string buffer.
     * @param sb a StringBuffer to append to
     * @param strings an array of strings.
     */
    private void toString(StringBuffer sb, String[] strings) {
        if (strings == null) {
            sb.append("null");
            return;
        }
        for (int i = 0; i < strings.length; i++) {
            if (i > 0) {
                sb.append(':');
            }
            sb.append(strings[i]);
        }
    }
}
