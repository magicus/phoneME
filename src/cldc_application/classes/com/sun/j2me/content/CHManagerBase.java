/*
 *
 *
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

package com.sun.j2me.content;

import com.sun.midp.events.Event;
import com.sun.midp.events.EventListener;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventTypes;
import com.sun.midp.main.MIDletProxy;
import com.sun.midp.main.MIDletProxyList;
import com.sun.midp.main.MIDletProxyListListener;

public class CHManagerBase extends com.sun.midp.content.CHManager 
							implements MIDletProxyListListener, EventListener {

    /**
     * Setup to monitor for MIDlets starting and exiting and check
     * for incompletely handled Invocation requests.
     * Cleanup only occurs within the AMS Isolate.
     * This method is only called from MIDletSuiteLoader in the AMS Isolate.
     *
     * @param midletProxyList reference to the MIDlet proxy list
     * @param eventQueue reference to AMS isolate event queue
     */
    public void init(MIDletProxyList midletProxyList, EventQueue eventQueue) {
        midletProxyList.addListener(this);
        eventQueue.registerEventListener(EventTypes.CHAPI_EVENT, this);
    }

    /**
     * Notification that a MIDlet is about to be created.
     * Set the cleanup flag on all invocations for the MIDlet.
     *
     * @param suiteId the storage name of the MIDlet suite
     * @param classname the midlet classname
     */
    public void midletInit(int suiteId, String classname) {
    	InvocationStore.setCleanup(new CLDCAppID(suiteId, classname), true);
    }

    /**
     * The ContentHandler monitor ignores MIDlet added callbacks.
     * The necessary initialization is done in the Isolate and
     * MIDletState that instantiates the MIDlet.
     * Called when a MIDlet is added to the list and only in the AMS
     * Isolate.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
    	AppProxy.midletIsAdded( midlet.getSuiteId(), midlet.getClassName() );
    }

    /**
     * The ContentHandler monitor ignores MIDlet update callbacks.
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
    }

    /**
     * The ContentHandler monitor uses the MIDlet removed callback
     * to cleanup any Invocations in an incorrect state.
     * Called (in the AMS Isolate) when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
    	if( AppProxy.LOGGER != null )
    		AppProxy.LOGGER.println("midletRemoved: " + midlet.getClassName());
	
		// Cleanup unprocessed Invocations
    	CLDCAppID appID = new CLDCAppID(midlet.getSuiteId(), midlet.getClassName());
		RegistryImpl.cleanup(appID);
		AppProxy.midletIsRemoved( appID.suiteID, appID.className );
		// Check for and execute a pending MIDlet suite
		InvocationStoreProxy.invokeNext();
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId, String className,
                          int errorCode, String errorDetails) {
		// Cleanup unprocessed Invocations
    	CLDCAppID appID = new CLDCAppID(suiteId, className);
    	InvocationStore.setCleanup(appID, true);
		RegistryImpl.cleanup(appID);
		AppProxy.midletIsRemoved( suiteId, className );
		// Check for and execute a pending MIDlet suite
		InvocationStoreProxy.invokeNext();
    }

    /**
     * Preprocess an event that is being posted to the event queue.
     * This method will get called in the thread that posted the event.
     * 
     * @param event event being posted
     *
     * @param waitingEvent previous event of this type waiting in the
     *     queue to be processed
     * 
     * @return true to allow the post to continue, false to not post the
     *     event to the queue
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        InvocationStoreProxy.invokeNext();
    }
}
