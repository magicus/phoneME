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

package com.sun.j2me.content;

import java.io.IOException;
import java.util.Hashtable;
import java.util.HashMap;

import javax.microedition.content.*;
import javax.microedition.io.Connector;

/**
 * Implementation of Content Handler registry.  It maintains
 * the set of currently registered handlers and updates to
 * the file that holds the permanent set.
 * The RegistryImpl class maintains an array of the current
 * registrations that is initialized on first use.
 */
public final class RegistryImpl {

    /** Cache of created instances. */
    private static final HashMap implCache = new HashMap();

    public static class RegistryFactory {
        /**
         * Internal constructor.
         * @param registry blank (invalid) Registry protecting against
         * illegal access.
         */
        public RegistryFactory(Registry registry) {
            if (registry.getID() != null) {
                throw new RuntimeException("Illegal access");
            }
        }

        /**
         * RegistryImpl access point. If RegistryImpl instance has ever created
         * for given classname then cached value returned.
         * The new RegistryImpl instance is created otherwise.
         *
         * @param classname discovered class name.
         *
         * @return cached or just created RegistryImpl instance
         *
         * @exception IllegalArgumentException if no class or
         *                  class is not valid application.
         */
        public RegistryImpl getRegistryImpl(String classname) {
            RegistryImpl impl = (RegistryImpl) implCache.get(classname);

            if (impl == null) {
                try {
                    impl = new RegistryImpl(AppProxy.forClass(classname));
                } catch (ClassNotFoundException cnfe) {
                    throw new IllegalArgumentException("no application");
                }
                implCache.put(classname, impl);
            } else {
                try {
                    impl.isValid();
                } catch (IllegalArgumentException iae) {
                    implCache.remove(classname);
                    throw iae;
                }
            }

            return impl;
        }

    }

    /** Implementation of the listener. */
    private ResponseListenerImpl listenerImpl;

    /** The ContentHandlerImpl that matches the classname of this Registry. */
    private ContentHandler handler;

    /** The Registry that is delegating to this RegistryImpl. */
    private Registry registry;

    /** The AppProxy for this registry. */
    private final AppProxy application;

    /** Two marginal values for access restrictions. */
    final static String NO_ACCESS_CALLER = "";
    final static String FULL_ACCESS_CALLER = null;

    /** The access restriction resolving value. */
    private String callerId;

    private String appID;

    /**
     * RegistryImpl constructor.
     *
     * @param application the application for this instance
     */
    private RegistryImpl(AppProxy application) {
        this.application = application;
        isValid();
        updateImpl();
    }

    /**
     * Checks if new created or just taken from cache ReistryImpl is
     * either registered handler or registered application.
     *
     * @exception IllegalArgumentException if test failed
     */
    private void isValid() {
        ContentHandler handler = RegistryStore.getHandler(
                                    application.getClassname());
        if (handler == null) {
            if (!application.isRegistered()) {
                throw new IllegalArgumentException("invalid class name");
            }
        }
        if (this.handler == null) {
            this.handler = handler;
        }
    }

    /**
     * Checks if the represented classname is (still) valid application:
     * either registered content handler
     * or properly installed J2ME application.
     *
     * The <code>handler</code> field is set;
     * <code>appID</code> and <code>callerId</code>
     * fields are updated as well during checking.
     *
     * @throws  IllegalArgumentException - if it is not an application.
     */
    private void updateImpl() {
        handler = RegistryStore.getHandler(application.getClassname());

        if (handler == null && (!application.isRegistered())) {
            throw new IllegalArgumentException("not an application");
        }

        appID = handler == null? handler.getID() :
                                 application.getDefaultID();

        callerId = application.isTrusted()? appID: NO_ACCESS_CALLER;
    }

    /**
     * Gets the registered content handler for the
     * application class of this RegistryImpl.
     *
     * @return the content handler for the registered
     * <code>classname</code> if it was registered by this application.
     * Otherwise, it returns <code>null</code>.
     * @exception NullPointerException if <code>classname</code> is
     *       <code>null</code>
     */
    public ContentHandler getServer() {
        return handler;
    }

    /**
     * Sets the ContentHandlerImpl; update any active RegistryImpl.
     * Replaces the entry in RegisteredTypes list as well.
     *
     * @param server the ContentHandlerImpl for this RegistryImpl
     * @see javax.microedition.content.ContentHandlerServerImpl
     */
    public void setServer(ContentHandlerServer server) {
        handler = server;
        handler.loadAccesses();
    }

    /**
     * Registers the application class using content
     * type(s), suffix(es), and action(s), action name(s),
     * access restrictions and content handler ID.
     * <p>
     * An application can use this method to replace or update
     * its own registrations
     * that have the same classname with new information.
     * The update occurs atomically; the update to the registry
     * either occurs or it does not.
     * <p>
     * The content handler may request to the following
     * items:
     * <ul>
     *    <li>zero or more content types</li>
     *    <li>zero or more suffixes</li>
     *    <li>zero or more actions</li>
     *    <li>zero or more mappings from actions to action names</li>
     *    <li>zero or more access restrictions</li>
     *    <li>a optional application ID</li>
     * </ul>
     *
     * <p>
     * If no exceptions are thrown, then the type(s), suffix(s), action(s),
     * action names, and access restrictions, and ID
     * will be registered for the application class.
     * <p>
     * If an exception is thrown, then the previous registration, if
     * any, will not be removed or modified.
     *
     * @param classname the application class name that implements
     *  this content handler. The value MUST NOT be <code>null</code>
     *        and MUST implement the lifecycle of the Java runtime
     * @param types an array of types to register;
     *   if <code>null</code> it is treated the same as an empty array
     * @param suffixes an array of suffixes to register;
     *   if <code>null</code> it is treated the same as an empty array
     * @param actions an array of actions to register;
     *   if <code>null</code> it is treated the same as an empty array
     * @param actionnames an array of ActionNameMaps to register;
     *   if <code>null</code> it is treated the same as an empty array
     * @param id the content handler ID; if <code>null</code>
     *  a non-null value MUST be provided by the implementation
     * @param accessRestricted the IDs of applications and content
     *  handlers that are
     *  allowed visibility and access to this content handler;
     *  if <code>null</code> then all applications and content
     *  handlers are allowed access; if <code>non-null</code>, then
     *  ONLY applications and content handlers with matching IDs are
     *  allowed access.
     *
     * @return the registered ContentHandler; MUST NOT be <code>null</code>
     * @exception NullPointerException if any of the following items is
     * <code>null</code>:
     * <ul>
     *    <li>classname</li>
     *    <li>any types, suffixes, actions, actionnames, or
     *        accessRestricted array element</li>
     * </ul>
     *
     * @exception IllegalArgumentException can be thrown:
     * <ul>
     *    <li>if any of the <code>types</code>, <code>suffix</code>,
     *        <code>actions</code>, or <code>accessRestricted</code>
     *        strings have a length of zero, or </li>
     *    <li>if the <code>classname</code> does not implement the valid
     *        lifecycle for the Java Runtime,</li>
     *    <li>if the sequence of actions in each ActionNameMap
     *        is not the same as the sequence of <code>actions</code>,
     *        or </li>
     *    <li>if the locales of the ActionNameMaps are not unique, or.</li>
     *    <li>if the length of the <code>accessRestricted</code>
     *        array is zero.</li>.
     * </ul>
     * @exception ClassNotFoundException if the <code>classname</code>
     * is not present
     * @exception ContentHandlerException with a error code of
     *  {@link ContentHandlerException#AMBIGUOUS} if <code>id</code>
     *  is a prefix of any registered handler or if any registered
     *  handler ID is a prefix of this ID
     * @exception SecurityException if registration
     *   is not permitted
     */
    public void register(String classname,
                                       String[] types,
                                       String[] suffixes,
                                       String[] actions,
                                       ActionNameMap[] actionnames,
                                       String id,
                                       String[] accessRestricted)
        throws SecurityException, IllegalArgumentException,
               ClassNotFoundException, ContentHandlerException
    {
        application.checkPermission(AppProxy.REGISTRY_PERMISSION);

        // May throw ClassNotFoundException or IllegalArgumentException
        AppProxy appl = AppProxy.forClass(classname);

        ContentHandlerImpl handler = new ContentHandlerImpl(
                            classname, types, suffixes, actions,
                           actionnames, id, accessRestricted, appl);
        handler.registrationMethod = ContentHandlerImpl.REGISTERED_DYNAMIC;

        ContentHandlerImpl conflict = checkConflicts(handler);
        if (conflict != null) {
             unregister(classname);
        }

        RegistryStore.register(handler);
    }

    /**
     * Removes the content handler registration for the application
     * class and any bindings to the content handler name, content
     * type(s), suffix(es), action(s), and access restrictions.
     *
     * @param classname the name of the content handler class
     * @return if the content handler was
     * successfully removed <code>true</code> is returned,
     * <code>false</code> otherwise
     * @exception NullPointerException if <code>classname</code> is
     * <code>null</code>
     */
    public boolean unregister(String classname) {
        if (classname == null) {
            throw new NullPointerException(
                               "classname argument can not be null");
        }

        ContentHandler curr = null;
        RegistryImpl reg = (RegistryImpl)implCache.get(classname);

        if (reg != null) {
            curr = reg.handler;
        }

        if (curr == null) {
            try {
                curr = RegistryStore.getHandler(classname);
            } catch (IllegalArgumentException iae) {
                // Empty class name falls down without further processing.
            }
        }

        if (curr != null) {
            RegistryStore.unregister(curr.getID());
            if (reg != null) {
                if (reg.application.isRegistered()) {
                    reg.handler = null;
                    reg.updateImpl();
                } else {
                    implCache.remove(classname);
                }
            }
            return true;
        }

        return false;
    }

    /**
     * Check for conflicts between a proposed new handler and the existing
     * handlers. If the handler is being replaced it will be returned.
     * Locate and return any existing handler for the same classname.
     *
     * @param handler the new content handler
     *
     * @return a ContentHandlerImpl within the suite that
     *  need to be removed to register the new ContentHandler
     */
    private static ContentHandlerImpl checkConflicts(ContentHandlerImpl handler)
                throws ContentHandlerException
    {
        ContentHandlerImpl[] handlers = RegistryStore.findConflicted(handler.ID);

        if (handlers != null && handlers.length > 0) {
            if (handlers.length == 1 &&
                handler.storageId == handlers[0].storageId &&
                handler.classname.equals(handlers[0].classname)) {
                        return handlers[0];
                    }

                    throw new ContentHandlerException(
                        "ID would be ambiguous: " + handler.ID,
                        ContentHandlerException.AMBIGUOUS);
            }

        return RegistryStore.getHandler(handler.classname);
    }

    /**
     * Gets all of the content types for which there are registered
     * handlers.
     * After a successful registration, the content handler's type(s),
     * if any, will appear in this list.
     * <P>
     * Only content handlers that this application is
     * allowed to access will be included.</p>
     *
     * @return an array of types; MUST NOT be <code>null</code>
     */
    public String[] getTypes() {
        return RegistryStore.getValues(callerId, RegistryStore.FIELD_TYPES);
    }

    /**
     * Gets all of the IDs of the registered content handlers.
     * <P>
     * Only content handlers that this application is
     * allowed to access will be included.</p>
     * @return an array of content handler IDs;
     *  MUST NOT be <code>null</code>
     */
    public String[] getIDs() {
        return RegistryStore.getValues(callerId, RegistryStore.FIELD_ID);
    }

    /**
     * Gets all of the actions of the registered content handlers.
     * After a successful registration the content handler's action(s),
     * if any, will appear in this list.
     * <P>
     * Only content handlers that this application is
     * allowed to access will be included.</p>
     * @return an array of content handler actions;
     *  MUST NOT be <code>null</code>
     */
    public String[] getActions() {
        return RegistryStore.getValues(callerId, RegistryStore.FIELD_ACTIONS);
    }

    /**
     * Gets all of the suffixes of the registered content handlers.
     * After a successful registration the content handler's suffix(es),
     * if any, will appear in this list.
     * <P>
     * Only content handlers that this application is
     * allowed to access will be included.</p>
     * @return an array of content handler suffixes;
     *  MUST NOT be <code>null</code>
     */
    public String[] getSuffixes() {
        return RegistryStore.getValues(callerId, RegistryStore.FIELD_SUFFIXES);
    }

    /**
     * Checks the Invocation and uses the ID, type, URL, and action,
     * if present, to find a matching ContentHandler and queues this
     * request to it.
     * <p>
     * If the <code>previous</code> Invocation is <code>null</code>, then
     * a new transaction is created; otherwise, this
     * Invocation will use the same transaction as the
     * <code>previous</code> Invocation.
     * <p>
     * The status of this Invocation MUST be <code>INIT</code>.
     * If there is a previous Invocation, that Invocation MUST
     * have a status of <code>ACTIVE</code>.
     * <p>
     * Candidate content handlers are found as described in
     * {@link #findHandler findHandler}. If any handlers are
     * found, one is selected for this Invocation.
     * The choice of content handler is implemention dependent.
     * <p>
     * If there is a non-null <code>previous</code> Invocation,
     * its status is set to <code>HOLD</code>.
     * A copy of the Invocation is made, the status is set to
     * <code>ACTIVE</code> and then queued to the
     * target content handler.
     * If the invoked content handler is not running, it MUST be started
     * as described in <a href="#execution">Invocation Processing</a>.
     *
     * <p>
     * The calling thread blocks while the content handler is being determined.
     * If a network access is needed, there may be an associated delay.
     *
     * @param invocation the Invocation containing the target ID, type,
     *  actions, arguments, and responseRequired parameters;
     *  MUST NOT be <code>null</code>
     * @param previous a previous Invocation for this Invocation;
     *  may be <code>null</code>
     *
     * @return <code>true</code> if the application MUST first
     *  voluntarily exit before the content handler can be started;
     *  <code>false</code> otherwise
     *
     * @exception IllegalArgumentException is thrown if:
     *  <ul>
     *     <li> the ID, type, URL, and action are all
     *          <code>null</code>, or </li>
     *     <li> the argument array contains any <code>null</code>
     *          references</li>
     *  </ul>
     * @exception IOException is thrown if access to the content fails
     * @exception ContentHandlerException is thrown with a reason of:
     *  <ul>
     *      <li><code>TYPE_UNKNOWN</code> if the type
     *          is not set and cannot be determined from the URL, or</li>
     *      <li><code>NO_REGISTERED_HANDLER</code> if
     *          there is no registered content handler that
     *          matches the requested ID, type, url or actions.
     *          </li>
     * </ul>
     * @exception IllegalStateException is thrown if the status of this
     *        Invocation is not <code>INIT</code> or if the status of the
     *        previous Invocation, if any, is not <code>ACTIVE</code>
     * @exception NullPointerException is thrown if the
     *  <code>invocation</code> is <code>null</code>
     * @exception SecurityException if an invoke operation is not permitted
     */
    public boolean invoke(InvocationImpl invocation, InvocationImpl previous)
        throws IllegalArgumentException, IOException,
               ContentHandlerException
    {
        synchronized (registry) {
            // Locate the content handler for this Invocation.
            ContentHandlerImpl handler =
                        (ContentHandlerImpl)findHandler(invocation)[0];

            // Fill in information about the invoking application
            invocation.invokingID = getID();
            invocation.invokingSuiteId = application.getStorageId();
            invocation.invokingClassname = application.getClassname();
            invocation.invokingAuthority = application.getAuthority();
            invocation.invokingAppName = application.getApplicationName();

            boolean shouldExit = invocation.invoke(previous, handler);

            return shouldExit;
        }
    }

    /**
     * Reinvokes the Invocation and uses the ID, type, URL, and action
     * to find a matching ContentHandler and re-queues this request to
     * it. Reinvocation is used to delegate the handling of an active
     * Invocation to another content handler.
     * The processing of the Invocation instance is complete and the
     * status is set to <code>OK</code>. Responses to the
     * reinvocation will be queued to the original invoking
     * application, if a response is required.
     *
     * <p>
     * Candidate content handlers are found as described in
     * {@link #findHandler findHandler}. If any handlers are
     * found, one is selected for this Invocation.
     * The choice of content handler is implementation dependent.
     * <p>
     * The status of this Invocation is set to <code>OK</code>.
     * A copy of the Invocation is made, the status is set to
     * <code>ACTIVE</code>, and then queued to the
     * target content handler.
     * If the invoked content handler application is not running,
     * it MUST be started
     * as described in <a href="#execution">Invocation Processing</a>.
     *
     * <p>
     * The calling thread blocks while the content handler is being determined.
     * If a network access is needed there may be an associated delay.
     *
     * @param invocation an Invocation containing the target ID, type,
     *  action, arguments, and responseRequired parameters;
     *  MUST NOT be <code>null</code>
     *
     * @return <code>true</code> if the application MUST first
     *  voluntarily exit before the content handler can be started;
     *  <code>false</code> otherwise
     *
     * @exception IllegalArgumentException is thrown if:
     *  <ul>
     *     <li> the ID, type, and URL are all <code>null</code>, or </li>
     *     <li> the argument array contains any <code>null</code>
     *          references</li>
     *  </ul>
     * @exception IOException is thrown if access to the content fails
     * @exception ContentHandlerException is thrown with a reason of:
     *      <code>NO_REGISTERED_HANDLER</code> if
     *          there is no registered content handler that
     *          matches the requested ID, type, URL, and action
     *
     * @exception NullPointerException is thrown if the
     *  <code>invocation</code> is <code>null</code>
     * @exception SecurityException if an invoke operation is not
     *  permitted or if access to the content is not permitted
     */
    public boolean reinvoke(InvocationImpl invocation)
        throws IllegalArgumentException, IOException,
               ContentHandlerException, SecurityException
    {
        synchronized (registry) {
            // Locate the content handler for this Invocation.
            ContentHandlerImpl handler =
                                (ContentHandlerImpl)findHandler(invocation)[0];

            // Save the TID in case the invoke fails
            int tid = invocation.tid;

            // The information about the invoking application is already set
            boolean shouldExit = invocation.invoke(null, handler);

            /*
             * Only if the invoke succeeds can the original Invocation be
             * discarded.
             * Restore the tid so the correct native invoc is disposed.
             */
            invocation.tid = tid;
            invocation.setStatus(InvocationImpl.DISPOSE);
            invocation.setStatus(Invocation.OK);

            return shouldExit;
        }
    }


    /**
     * Gets the next Invocation response pending for this application.
     * The method blocks until an Invocation response is available, but
     * not for longer than the timeout period.
     * The method can be unblocked with a call to
     * {@link #cancelGetResponse}.
     * The application can process the Invocation based on
     * its status. The status is one of
     * <code>OK</code>, <code>CANCELLED</code>, or <code>ERROR</code>.
     * <p>
     * If the Invocation  was invoked with
     * {@link #invoke(InvocationImpl invocation, InvocationImpl
     * previous)},
     * the <code>getPrevious</code> method MUST return the
     * previous Invocation.
     * If the status of the previous Invocation is <code>HOLD</code>
     * then its status is restored to <code>ACTIVE</code>.
     *
     * <p>
     * If the original Invocation instance is reachable, then it
     * MUST be updated with the values from the response
     * and be returned to the application. If it is not
     * reachable, then a new instance is returned from getResponse
     * with the response values.
     *
     * @param wait <code>true</code> if the method
     *  MUST wait for an Invocation if one is not currently available;
     *  otherwise <code>false</code>
     *
     * @exception IllegalArgumentException if the context is not valid
     *
     * @return the next pending response Invocation or <code>null</code>
     *  if the timeout expires and no Invocation is available or
     *  if cancelled with {@link #cancelGetResponse}
     * @see #invoke
     * @see #cancelGetResponse
     */
    public InvocationImpl getResponse(boolean wait) {
        // Application has tried to get a response; reset cleanup flags on all
        if (responseCalls == 0) {
            InvocationStore.setCleanup(application.getStorageId(),
                                       application.getClassname(), false);
        }
        responseCalls++;

        // Find a response for this application and context
        InvocationImpl invoc =
            InvocationStore.getResponse(resp, application.getStorageId(),
                                        application.getClassname(), wait);
        if (invoc != null) {
                // If there is a previousTid then restore the previous
                if (invoc.previousTid != 0) {
                    /*
                     * There will be a previous Invocation unless the app has
                     * already finished it. It will have a HOLD status.
                     */
                    invoc.previous =
                        InvocationStore.getByTid(invoc.previousTid, 0);
                }

            if (invoc.previous != null &&
                invoc.previous.status == Invocation.HOLD) {
                // Restore ACTIVE status to a previously HELD Invocation
                invoc.previous.setStatus(Invocation.ACTIVE);
            }

            // Make an attempt to gain the foreground
            if (invoc.invokingSuiteId != MIDletSuite.UNUSED_SUITE_ID &&
                invoc.invokingClassname != null) {

                // Strong FG transition requested
                application.requestForeground(invoc.invokingSuiteId,
                                              invoc.invokingClassname);
            }

            return invoc.invocation;
        }
        return null;
    }


    /**
     * Cancels a pending <code>getResponse</code>.
     * This method will force a Thread blocked in a call to the
     * <code>getResponse</code> method for the same application
     * context to return early.
     * If no Thread is blocked; this call has no effect.
     */
    public void cancelGetResponse() {
        InvocationStore.cancel();
    }

    /**
     * Sets the listener to be notified when a new response is
     * available for the application context.  The request must
     * be retrieved using {@link #getResponse getResponse}.
     *
     * @param listener the listener to register;
     *   <code>null</code> to remove the listener.
     */
    public void setListener(ResponseListener listener) {

        // Create or update the listener implementation
        synchronized (this) {
            if (listener != null || listenerImpl != null) {
                // Create or update the active listener thread
                if (listenerImpl == null) {
                    listenerImpl =
                        new ResponseListenerImpl(this, listener);
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
     * Gets the registered content handlers that could be used for
     * this Invocation.  Only handlers accessible to the application
     * are considered. The values for ID, type, URL, and
     * action are used in the following order:
     * <ul>
     *    <li>If the ID is non-null, then the set of candidate
     *        handlers is determined from the {@link #forID forID}
     *        method with the  parameter <tt>exact</tt> set to false.
     *        If there is an exact match it MUST be returned as
     *        the first handler.
     *        The type and URL are ignored. If there are no handlers that match
     *        the requested ID then a <tt>ContentHandlerException</tt>
     *        is thrown.</li>
     *
     *    <li>If the ID and type are <code>null</code> and
     *        the URL is <code>non-null</code> and
     *        If the protocol supports typing of content, then
     *        the type is determined
     *        as described in {@link Invocation#findType}.
     *        If the type cannot be determined from the content,
     *        the type is set to <code>null</code>.</li>
     *
     *    <li>If the ID is null and type is non-null,
     *        then the set of candidate handlers is determined from the
     *        {@link #forType forType} method.
     *        If there are no handlers that match the requested type
     *        then a <tt>ContentHandlerException</tt> is thrown. </li>
     *
     *    <li>If both the ID and type are <code>null</code> and
     *        the URL is <code>non-null</code> and
     *        if the protocol does not support typing of content
     *        or the type was not available from the content,
     *        then the set of candidate handlers
     *        includes any handler with a suffix that matches the
     *        end of the path component of the URL.
     *        If there are no handlers that match a registered
     *        suffix then a <tt>ContentHandlerException</tt> is thrown.</li>
     *
     *    <li>If the ID, type, and URL are all null, the set of candidate
     *        handlers includes all of the accessible handlers.</li>
     *
     *    <li>If the action is non-null, the set of candidate handlers
     *        is reduced to contain only handlers that support the
     *        action.</li>
     *
     *    <li>If the set of candidate handlers is empty
     *        then a <tt>ContentHandlerException</tt> is thrown.</li>
     * </ul>
     * <p>
     * The calling thread blocks while the ID and type are being determined.
     * If a network access is needed there may be an associated delay.
     *
     * @param invoc the ID, type, action, and URL that
     *  are needed to identify one or more content handlers;
     *  must not be <code>null</code>
     * @return an array of the <code>ContentHandler</code>(s)
     *  that could be used for this Invocation; MUST NOT be <code>null</code>;
     *
     * @exception IOException is thrown if access to the content fails
     * @exception ContentHandlerException is thrown with a reason of
     *      <code>NO_REGISTERED_HANDLER</code> if
     *          there is no registered content handler that
     *          matches the requested ID, type, URL, and action
     *
     * @exception IllegalArgumentException is thrown if ID, type, URL,
     *  and action are all <code>null</code> or
     *  if the content is accessed via the URL and the URL is invalid
     * @exception NullPointerException is thrown if the
     *  <code>invocation</code> is <code>null</code>
     * @exception SecurityException is thrown if access to the content
     *  is not permitted
     */
    public ContentHandler[] findHandler(InvocationImpl invoc)
        throws IOException, ContentHandlerException
    {
        ContentHandler[] handlers = null;
        if (invoc.getID() != null) {
            ContentHandler handler = forID(invoc.getID(), false);
            if (handler != null) {
                handlers = new ContentHandler[1];
                handlers[0] = handler;
            }
        } else {
            String action = invoc.getAction();

            // ID is null
            synchronized (mutex) {
                // Inhibit types change while doing lookups
                if (invoc.getType() == null &&
                    invoc.getURL() != null) {
                    try {
                        invoc.findType();
                    } catch (ContentHandlerException che) {
                        // Type is null
                    }
                }
                if (invoc.getType() != null) {
                    // The type is known; lookup the handlers
                    handlers = forType(invoc.getType());
                } else if (invoc.getURL() != null) {
                    /**
                     * Call platform specific function for
                     * getting handler by URL
                     */
                    ContentHandler suitable =
                        RegistryStore.getByURL(getID(), invoc.getURL(), action);

                    if (suitable != null) {
                        handlers = new ContentHandler[1];
                        handlers[0] = suitable;
                    }
                } else if (action != null) {
                    handlers = forAction(action);
                    action = null;
                } else {
                    throw new IllegalArgumentException(
                                "not ID, type, URL, or action");
                }

                // Set of candidate handlers; check for matching action
                if (handlers != null && action != null) {
                    int rem = 0;    // number of handlers to remove
                    for (int i = 0; i < handlers.length; i++) {
                        if (!handlers[i].hasAction(action)) {
                            handlers[i] = null;
                            rem++;
                        }
                    }
                    if (rem > 0) {
                        int newsz = handlers.length - rem;
                        if (newsz > 0) {
                            ContentHandler[] newhand =
                                                new ContentHandler[newsz];
                            int j;
                            int k;
                            for (j = k = 0; j < newsz; j++) {
                                while (handlers[k] == null) {
                                    k++;
                                }
                                newhand[j] = handlers[k++];
                            }
                            handlers = newhand;
                        } else {
                            handlers = null;
                        }
                    }
                }
            }
        }


        if (handlers == null || handlers.length == 0) {
            throw new ContentHandlerException("no registered handler",
                            ContentHandlerException.NO_REGISTERED_HANDLER);
        }
        return handlers;
    }

    /**
     * Gets the registered content handlers for the content type.
     * <P>
     * Only content handlers that are visible and accessible to this
     * application are returned.
     *
     * @param type the type of the requested content handlers
     * @return an array of the <code>ContentHandler</code>s registered
     *  for the type; MUST NOT be <code>null</code>.
     *  An empty array is returned if there are no
     * <code>ContentHandler</code>s accessible to
     *  this application.
     * @exception NullPointerException if <code>type</code> is
     *       <code>null</code>
     */
    public ContentHandler[] forType(String type) {
        return RegistryStore.findHandler(callerId, RegistryStore.FIELD_TYPES,
                                                                        type);
    }

    /**
     * Gets the registered content handlers that support the action.
     * <P>
     * Only content handlers that are visible and accessible to this
     * application are returned.
     *
     * @param action content handlers for which the action is supported
     * @return an array of the <code>ContentHandler</code>s registered
     *  for the action; MUST NOT be <code>null</code>;
     *  an empty array is returned if no <code>ContentHandler</code>s
     *  are accessible to this application
     * @exception NullPointerException if <code>action</code> is
     *       <code>null</code>
     */
    public ContentHandler[] forAction(String action) {
        return RegistryStore.findHandler(callerId, RegistryStore.FIELD_ACTIONS,
                                                                    action);
    }

    /**
     * Gets all of the content handlers for the suffix.
     * Only content handlers that are visible and accessible to this
     * application are returned.
     *
     * @param suffix the suffix to be used to get the associated
     * content handlers
     *
     * @return an array of the <code>ContentHandler</code>s registered
     *  for the suffix; MUST NOT be <code>null</code>.
     *  An empty array is returned if there are none accessible to
     *  this application
     *
     * @exception NullPointerException if <code>suffix</code> is
     *       <code>null</code>
     */
    public ContentHandler[] forSuffix(String suffix) {
        return RegistryStore.findHandler(callerId, RegistryStore.FIELD_SUFFIXES,
                                                                    suffix);
    }

    /**
     * Gets the registered content handler for the ID.
     * The query can be for an exact match or for the handler
     * matching the prefix of the requested ID.
     * <P>
     * Only a content handler which is visible to and accessible to this
     * application will be returned.
     *
     * @param ID the content handler application ID of the content
     *       handler requested
     * @param exact <code>true</code> to require an exact match;
     * <code>false</code> to allow a registered content handler ID
     *                 to match a prefix of the requested ID
     *
     * @return the content handler that matches the ID,
     *       otherwise <code>null</code>
     *
     * @exception NullPointerException if <code>ID</code> is
     *       <code>null</code>
     */
    public ContentHandler forID(String ID, boolean exact) {
        return RegistryStore.getHandler(callerId, ID,
                exact? RegistryStore.SEARCH_EXACT: RegistryStore.SEARCH_PREFIX);
    }

    /**
     * Stores Registry instance as front-end representative.
     * @param registry new-created Registry instance.
     */
    public void setRegistry(Registry registry) {
        if (this.registry != null ||
                registry.getID() != null) {
            throw new RuntimeException("Illegal access");
        }
        this.registry = registry;
    }

    public InvocationImpl getRequest(boolean wait) {
        /** TODO: implement method */
        return null;  //To change body of created methods use File | Settings | File Templates.
    }

    public void cancelGetRequest() {
        /** TODO: implement method */
    }

    public boolean finish(InvocationImpl invocImpl, int status) {
        /** TODO: implement method. */
        return false;  //To change body of created methods use File | Settings | File Templates.
    }

    public void setListener(RequestListener listener) {
        /** TODO: implement method. */
    }

    public Registry getRegistry() {
        return registry;
    }

    public String getID() {
        return appID;
    }
}
